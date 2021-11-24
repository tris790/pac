#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)
#include <stdio.h>
#include <lib.hh>
#include <thread>
#include <chrono>
#include <inttypes.h>
#include <assert.h>
#include <atomic>

#include <SDL.h>
#include <SDL_mixer.h>
#include <gst/gst.h>
#include <glib.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video-info.h>
#include <gst/audio/audio-info.h>

#include "pac_network.h"
#include "config.h"
#include "logger.h"
#include "io_utils.h"

// was needed because SDL was redeclaring main on something like that
#undef main

auto stream_audio_enable = std::strcmp(configuration["stream_audio"].c_str(), "true") == 0;

// IYUV, RGB888, RGBx, NV12
const int pixel_format = SDL_PIXELFORMAT_IYUV;
Config configuration;
SDL_Renderer *sdlRenderer;
int screen_buffer_w = 0;
int screen_buffer_h = 0;
SDL_Texture *sdlTexture;
Mix_Chunk *mmusic;
unsigned char *screen_buffer;
unsigned char *audio_buffer;
int audio_buffer_length;

// !threadsafe
GstElement *pipeline;
GstElement *pipeline_audio;
GstBus *bus;

//Refresh Event
#define REFRESH_EVENT (SDL_USEREVENT + 1)
//Break
#define BREAK_EVENT (SDL_USEREVENT + 2)

// Multithread problem ?
int thread_exit = 0;

int network_thread_fn(void *opaque)
{
    uvgrtp::context ctx;
    std::string server_hostname(configuration["hostname"]);
    auto receive_port = stoi(configuration["receive_port"]);
    auto send_port = stoi(configuration["send_port"]);
    uvgrtp::session *session = ctx.create_session(server_hostname);
    uvgrtp::media_stream *rtp_stream = nullptr;

    logger.debug("Connecting to %s:%d", server_hostname.c_str(), receive_port);

    // Retry to connect every second if connection failed
    while ((rtp_stream = session->create_stream(receive_port, send_port, RTP_FORMAT_GENERIC, RCE_FRAGMENT_GENERIC)) == nullptr)
    {
        logger.error("Failed to created a socket %s", server_hostname.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    logger.debug("Succesfully created a socket %s", server_hostname.c_str());

    while (!thread_exit)
    {
        uvgrtp::frame::rtp_frame *video_network_frame = rtp_stream->pull_frame(5);
        if (video_network_frame)
        {
            auto packet = video_network_frame->payload;
            auto packet_size = video_network_frame->payload_len;
            logger.debug("Client size: %lld", packet_size);
        }
        uvgrtp::frame::dealloc_frame(video_network_frame);

        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
    }

    ctx.destroy_session(session);

    SDL_Event event;
    event.type = BREAK_EVENT;
    SDL_PushEvent(&event);
    return 0;
}

typedef struct
{
    int *argc;
    char ***argv;
} GStreamerThreadArgs;

GstFlowReturn frame_recorded_callback(GstAppSink *appsink, void *customData)
{
    static guint frame_count = 0;

    // Pulling the frame from gstreamer
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo video_frame_info;
    GstVideoInfo video_info;
    gst_buffer_map(buffer, &video_frame_info, GST_MAP_READ);
    gst_video_info_from_caps(&video_info, gst_sample_get_caps(sample));

    if (frame_count % 120 == 0)
        logger.debug("[Transcode] Video Stream: width=%d height=%d fps=%d.%d format=%s size=%d (frame: %d)", video_info.width, video_info.height, video_info.fps_n, video_info.fps_d, video_info.finfo->name, video_info.size, frame_count);

    if (screen_buffer_w != video_info.width || screen_buffer_h != video_info.height)
    {
        screen_buffer_w = video_info.width;
        screen_buffer_h = video_info.height;
        if (screen_buffer)
        {
            free(screen_buffer);
        }
        if (sdlTexture)
        {
            SDL_DestroyTexture(sdlTexture);
        }
        screen_buffer = (unsigned char *)malloc(video_frame_info.size);
        sdlTexture = SDL_CreateTexture(sdlRenderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, video_info.width, video_info.height);
    }
    memcpy(screen_buffer, video_frame_info.data, video_frame_info.size);

    frame_count++;
    gst_buffer_unmap(buffer, &video_frame_info);
    gst_sample_unref(sample);

    SDL_Event event;
    event.type = REFRESH_EVENT;

    if (!SDL_PushEvent(&event))
    {
        logger.error("Error adding refresh event: %s", SDL_GetError());
    }

    return GST_FLOW_OK;
}

int gstreamer_thread_audio_fn(void *opaque)
{
    GStreamerThreadArgs *args = (GStreamerThreadArgs *)opaque;
    gst_init(args->argc, args->argv);
    auto pipeline_args = configuration["gst_audio_receiver"];
    logger.debug("pipeline is : %s", pipeline_args.c_str());
    pipeline_audio = gst_parse_launch(pipeline_args.c_str(), NULL);

    gst_element_set_state(pipeline_audio, GST_STATE_PLAYING);

    bus = gst_element_get_bus(pipeline_audio);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                                 (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ERROR:
    {
        GError *err;
        gchar *debug;

        gst_message_parse_error(msg, &err, &debug);
        g_print("Error: %s\n", err->message);
        g_error_free(err);
        g_free(debug);

        break;
    }
    case GST_MESSAGE_EOS:
        /* end-of-stream */
        break;
    default:
        /* unhandled message */
        break;
    }

    if (msg != NULL)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline_audio, GST_STATE_NULL);
    gst_object_unref(pipeline_audio);

    printf("Closing Gstreamer audio thread\n");

    return 0;
}

int gstreamer_thread_fn(void *opaque)
{
    GStreamerThreadArgs *args = (GStreamerThreadArgs *)opaque;
    gst_init(args->argc, args->argv);
    pipeline = gst_parse_launch(configuration["gstreamer_pipeline_args"].c_str(), NULL);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    gst_app_sink_set_emit_signals((GstAppSink *)sink, true);
    gst_app_sink_set_drop((GstAppSink *)sink, true);
    gst_app_sink_set_max_buffers((GstAppSink *)sink, 1);
    GstAppSinkCallbacks callbacks = {NULL, NULL, (GstFlowReturn(*)(GstAppSink *, gpointer))frame_recorded_callback};
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &callbacks, NULL, NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                                 (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    switch (GST_MESSAGE_TYPE(msg))
    {
    case GST_MESSAGE_ERROR:
    {
        GError *err;
        gchar *debug;

        gst_message_parse_error(msg, &err, &debug);

        logger.error("GST_MESSAGE_ERROR - %s", err->message);

        g_error_free(err);
        g_free(debug);

        break;
    }
    case GST_MESSAGE_EOS:
        /* end-of-stream */
        break;
    default:
        /* unhandled message */
        break;
    }

    if (msg != NULL)
        gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    logger.debug("Closing Gstreamer thread");

    return 0;
}

int main(int argc, char *argv[])
{
    logger.info("Initializing the client");

    configuration = Config(get_exec_directory(argv[0]) + "/client.conf");
    int window_width = stoi(configuration["window_width"]);
    int window_heigth = stoi(configuration["window_heigth"]);

    auto steam_audio_enable = std::strcmp(configuration["stream_audio"].c_str(), "true") == 0;

    if (steam_audio_enable
            ? SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)
            : SDL_Init(SDL_INIT_VIDEO))
    {
        logger.error("Could not initialize SDL - %s", SDL_GetError());
        return -1;
    }

    logger.debug("[Now] SDL initialized");

    SDL_Window *screen;
    //SDL 2.0 Support for multiple windows
    const char *window_name = "Pac - Client";
    screen = SDL_CreateWindow(window_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              window_width, window_heigth, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!screen)
    {
        logger.error("SDL: could not create window - exiting:%s", SDL_GetError());
        return -1;
    }

    logger.debug("[Now] Window created");

    sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

    sdlTexture = SDL_CreateTexture(sdlRenderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, screen_buffer_w, screen_buffer_h);
    SDL_Rect sdlRect;

    GStreamerThreadArgs gstreamer_args{&argc, &argv};
    SDL_Thread *gstreamer_thread = SDL_CreateThread(gstreamer_thread_fn, NULL, &gstreamer_args);
    if (stream_audio_enable)
        SDL_Thread *gstreamer_thread_audio = SDL_CreateThread(gstreamer_thread_audio_fn, NULL, &gstreamer_args);
    SDL_Event event;
    bool isPlaying = true;

    while (true)
    {
        //Wait
        SDL_WaitEvent(&event);
        if (event.type == REFRESH_EVENT && isPlaying)
        {
            if (SDL_UpdateTexture(sdlTexture, NULL, screen_buffer, screen_buffer_w))
            {
                logger.error("Error updating texture: %s", SDL_GetError());
            }

            //FIX: If window is resize
            sdlRect.x = 0;
            sdlRect.y = 0;
            sdlRect.w = window_width;
            sdlRect.h = window_heigth;

            SDL_RenderClear(sdlRenderer);
            SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
            SDL_RenderPresent(sdlRenderer);
        }
        else if (event.type == SDL_KEYDOWN)
        {
            logger.debug("SDL_Event: We got a key down event (SDL_KEYDOWN)");

            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                SDL_Event event;
                event.type = SDL_QUIT;
                SDL_PushEvent(&event);
            }
            else if (event.key.keysym.sym == SDLK_SPACE)
            {
                isPlaying = !isPlaying;

                logger.debug("Reporting playback state %s", (isPlaying) ? "played" : "paused");
            }
        }
        else if (event.type == SDL_WINDOWEVENT)
        {
            //If Resize
            SDL_GetWindowSize(screen, &window_width, &window_heigth);
        }
        else if (event.type == SDL_QUIT)
        {
            logger.debug("SDL_Event: We got a quit event (SDL_QUIT)");

            thread_exit = 1;
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_element_send_event(pipeline, gst_event_new_eos());
            if (stream_audio_enable)
            {
                gst_element_set_state(pipeline_audio, GST_STATE_NULL);
                gst_element_send_event(pipeline_audio, gst_event_new_eos());
            }
            break;
        }
    }
    SDL_Quit();
    return 0;
}
