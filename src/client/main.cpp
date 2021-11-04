#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)
#include "SDL.h"
#include "SDL_mixer.h"
#include <lib.hh>
#include <thread>
#include <chrono>
#include <inttypes.h>
#include <assert.h>
#include "pac_network.h"

#include <gst/gst.h>
#include <glib.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

// was needed because SDL was redeclaring main on something like that
#undef main

int window_width = 800, window_heigth = 600;
const int pixel_w = 1920, pixel_h = 1080;

const int screen_buffer_size = pixel_w * pixel_h * 3;
unsigned char screen_buffer[screen_buffer_size];

//Refresh Event
#define REFRESH_EVENT (SDL_USEREVENT + 1)
//Break
#define BREAK_EVENT (SDL_USEREVENT + 2)

// Multithread problem ?
int thread_exit = 0;

int network_thread_fn(void *opaque)
{
    thread_exit = 0;
    uvgrtp::context ctx;
    std::string server_hostname("127.0.0.1");
    auto receive_port = 8889;
    auto send_port = 8888;
    uvgrtp::session *session = ctx.create_session(server_hostname);
    printf("Connecting to %s:%d\n", server_hostname.c_str(), receive_port);
    uvgrtp::media_stream *rtp_stream = nullptr;

    // Retry to connect every second if connection failed
    while ((rtp_stream = session->create_stream(receive_port, send_port, RTP_FORMAT_GENERIC, RCE_FRAGMENT_GENERIC)) == nullptr)
    {
        printf("Failed to created a socket %s\n", server_hostname.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    printf("Succesfully created a socket %s\n", server_hostname.c_str());

    while (!thread_exit)
    {
        uvgrtp::frame::rtp_frame *video_network_frame = rtp_stream->pull_frame(5);
        if (video_network_frame)
        {
            auto packet = video_network_frame->payload;
            auto packet_size = video_network_frame->payload_len;
            printf("Client size: %lld\n", packet_size);
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

int gstreamer_thread_fn(void *opaque)
{
    GStreamerThreadArgs *args = (GStreamerThreadArgs *)opaque;
    gst_init(args->argc, args->argv);
    auto pipeline_args = "udpsrc port=9996 caps = \"application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96\" ! rtph264depay ! queue ! decodebin ! videoconvert ! autovideosink";
    GstElement *pipeline = gst_parse_launch(pipeline_args, NULL);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    gst_app_sink_set_emit_signals((GstAppSink *)sink, true);
    gst_app_sink_set_drop((GstAppSink *)sink, true);
    gst_app_sink_set_max_buffers((GstAppSink *)sink, 1);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    GstBus *bus = gst_element_get_bus(pipeline);
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
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window *screen;
    //SDL 2.0 Support for multiple windows
    const char *window_name = "StadiaLike - Client";
    screen = SDL_CreateWindow(window_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              window_width, window_heigth, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!screen)
    {
        printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
        return -1;
    }
    SDL_Renderer *sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

    // Initialisation mixer
    int audiomixer = Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 512);
    if (audiomixer < 0)
    {
        fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
        exit(-1);
    }

    audiomixer = Mix_AllocateChannels(4);
    if (audiomixer < 0)
    {
        fprintf(stderr, "Unable to allocate mixing channels: %s\n", SDL_GetError());
        exit(-1);
    }

    Mix_Chunk *mmusic;
    std::string path = "D:/Data_mick/Universite/projet/pac/assets/Beyond.wav";
    mmusic = Mix_LoadWAV(path.c_str());
    if (mmusic == NULL)
    {
        fprintf(stderr, "Unable to load wave file: %s\n", path.c_str());
    }

    Uint32 pixformat = 0;
    //IYUV: Y + U + V  (3 planes)
    //YV12: Y + V + U  (3 planes)
    // pixformat = SDL_PIXELFORMAT_RGB888;
    pixformat = SDL_PIXELFORMAT_IYUV;

    SDL_Texture *sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat, SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);
    SDL_Rect sdlRect;

    SDL_Thread *network_thread = SDL_CreateThread(network_thread_fn, NULL, NULL);
    GStreamerThreadArgs gstreamer_args{&argc, &argv};
    SDL_Thread *gstreamer_thread = SDL_CreateThread(gstreamer_thread_fn, NULL, &gstreamer_args);
    SDL_Event event;
    bool isPlaying = true;
    //Lance la musique depuis le début du fichier
    Mix_PlayChannel(1, mmusic, -1);

    while (true)
    {
        //Wait
        SDL_WaitEvent(&event);
        if (event.type == REFRESH_EVENT && isPlaying)
        {
            SDL_UpdateTexture(sdlTexture, NULL, screen_buffer, pixel_w);

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
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                thread_exit = 1;
            }
            else if (event.key.keysym.sym == SDLK_SPACE)
            {
                isPlaying = !isPlaying;
            }
        }
        else if (event.type == SDL_WINDOWEVENT)
        {
            //If Resize
            SDL_GetWindowSize(screen, &window_width, &window_heigth);
        }
        else if (event.type == SDL_QUIT)
        {
            thread_exit = 1;
        }
        else if (event.type == BREAK_EVENT)
        {
            break;
        }
    }
    SDL_Quit();
    return 0;
}
