#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)
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

#include "pac_network.h"
#include "config.h"
std::string get_exec_directory(char *current_path)
{
    std::string current_path_string(current_path);
#ifdef _WIN32
    auto last_slash_index = current_path_string.find_last_of("\\");
#else
    auto last_slash_index = current_path_string.find_last_of("/");
#endif
    return current_path_string.substr(0, last_slash_index > 1 ? last_slash_index : 0);
} 


// was needed because SDL was redeclaring main on something like that
#undef main

// IYUV, RGB888, RGBx, NV12
const int pixel_format = SDL_PIXELFORMAT_IYUV;
SDL_Renderer *sdlRenderer;
int screen_buffer_w = 0;
int screen_buffer_h = 0;
SDL_Texture *sdlTexture;
unsigned char *screen_buffer;

// !threadsafe
GstElement *pipeline;
GstBus *bus;

//Refresh Event
#define REFRESH_EVENT (SDL_USEREVENT + 1)
//Break
#define BREAK_EVENT (SDL_USEREVENT + 2)

// Multithread problem ?
int thread_exit = 0;

typedef struct
{
    int *argc;
    char ***argv;
} GStreamerThreadArgs;

GstFlowReturn frame_recorded_callback(GstAppSink *appsink, void *customData)
{
    // Pulling the frame from gstreamer
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo video_frame_info;
    GstVideoInfo video_info;
    gst_buffer_map(buffer, &video_frame_info, GST_MAP_READ);
    gst_video_info_from_caps(&video_info, gst_sample_get_caps(sample));
    
    // printf("Video size: [%d, %d] Video format: %s\n", video_info.width, video_info.height, video_info.finfo->name);
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

    gst_buffer_unmap(buffer, &video_frame_info);
    gst_sample_unref(sample);

    SDL_Event event;
    event.type = REFRESH_EVENT;
    if (!SDL_PushEvent(&event))
    {
        printf("Error adding refresh event: %s\n", SDL_GetError());
    }

    return GST_FLOW_OK;
}

int gstreamer_thread_fn(void *opaque)
{
    GStreamerThreadArgs *args = (GStreamerThreadArgs *)opaque;
    gst_init(args->argc, args->argv);
    auto pipeline_args = "udpsrc port=9996 caps=\"application/x-rtp, media=video, clock-rate=90000, encoding-name=H264, payload=96\" ! rtph264depay ! queue ! h264parse ! nvh264dec ! videoconvert ! video/x-raw,format=I420 ! appsink name=sink";
    pipeline = gst_parse_launch(pipeline_args, NULL);

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

    printf("Closing Gstreamer thread\n");

    return 0;
}

int main(int argc, char *argv[])
{
    std::string executable_directory = get_exec_directory(argv[0]);
    Config configuration = Config(executable_directory + "/" + "client.conf");
    
    int window_width = stoi(configuration["window_width"]);
    int window_heigth = stoi(configuration["window_heigth"]);
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window *screen;
    //SDL 2.0 Support for multiple windows
    const char *window_name = "Pac - Client";
    screen = SDL_CreateWindow(window_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              window_width, window_heigth, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!screen)
    {
        printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
        return -1;
    }
    sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

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
    std::string path = configuration["audio_file_path"];
    mmusic = Mix_LoadWAV(path.c_str());
    if (mmusic == NULL)
    {
        fprintf(stderr, "Unable to load wave file: %s\n", path.c_str());
    }

    sdlTexture = SDL_CreateTexture(sdlRenderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, screen_buffer_w, screen_buffer_h);
    SDL_Rect sdlRect;

    GStreamerThreadArgs gstreamer_args{&argc, &argv};
    SDL_Thread *gstreamer_thread = SDL_CreateThread(gstreamer_thread_fn, NULL, &gstreamer_args);
    SDL_Event event;
    bool isPlaying = true;
    //Lance la musique depuis le début du fichier
    Mix_PlayChannel(stoi(configuration["channel"]), mmusic, stoi(configuration["loop"]));

    while (true)
    {
        //Wait
        SDL_WaitEvent(&event);
        if (event.type == REFRESH_EVENT && isPlaying)
        {
            if (SDL_UpdateTexture(sdlTexture, NULL, screen_buffer, screen_buffer_w))
            {
                printf("Error updating texture: %s\n", SDL_GetError());
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
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                SDL_Event event;
                event.type = SDL_QUIT;
                SDL_PushEvent(&event);
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
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_element_send_event(pipeline, gst_event_new_eos());
            break;
        }
    }
    SDL_Quit();
    return 0;
}
