#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)
#include "SDL.h"
#include <lib.hh>
#include <thread>
#include <chrono>
#include <inttypes.h>

#undef main
const int bpp = 12;

int screen_w = 1920, screen_h = 1000;
const int pixel_w = 1920, pixel_h = 1080;
unsigned char buffer[pixel_w * pixel_h * bpp / 8] = {0};

//Refresh Event
#define REFRESH_EVENT (SDL_USEREVENT + 1)
//Break
#define BREAK_EVENT (SDL_USEREVENT + 2)

// Multithread problem ?
int thread_exit = 0;

int refresh_video_thread_fn(void *opaque)
{
    thread_exit = 0;
    while (thread_exit == 0)
    {
        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(40);
    }

    //Break
    SDL_Event event;
    event.type = BREAK_EVENT;
    SDL_PushEvent(&event);
    return 0;
}

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
        printf("Failed to connect to %s\n", server_hostname.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    printf("Succesfully connected to %s\n", server_hostname.c_str());

    uvgrtp::frame::rtp_frame *start_frame = nullptr;
    while (!thread_exit)
    {
        start_frame = rtp_stream->pull_frame();
        if (start_frame->payload_len == 1)
        {
            printf("Received payload %" PRIu8 "\n", *start_frame->payload);
        }
        if (start_frame && start_frame->payload_len == 1 && *start_frame->payload == 0)
        {
            printf("Received payload %" PRIu8 "\n", *start_frame->payload);
            uvgrtp::frame::dealloc_frame(start_frame);
            bool receivingFrame = true;
            int offset = 0;
            while (receivingFrame)
            {
                uvgrtp::frame::rtp_frame *data_frame = rtp_stream->pull_frame();
                if (data_frame && data_frame->payload_len == 1)
                {
                    receivingFrame = false;
                    printf("Frame ended\n");
                }
                else
                {
                    memcpy(buffer + offset, data_frame->payload, data_frame->payload_len);
                    offset += data_frame->payload_len;
                }
                uvgrtp::frame::dealloc_frame(data_frame);
            }
        }
        else
        {
            uvgrtp::frame::dealloc_frame(start_frame);
        }

        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
        // memcpy(buffer, frame->payload, 1024);
        // SDL_Delay(40);
    }

    ctx.destroy_session(session);

    //Break
    SDL_Event event;
    event.type = BREAK_EVENT;
    SDL_PushEvent(&event);
    return 0;
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window *screen;
    //SDL 2.0 Support for multiple windows
    const char *window_name = "StadiaLike - Client";
    screen = SDL_CreateWindow(window_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!screen)
    {
        printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
        return -1;
    }
    SDL_Renderer *sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

    Uint32 pixformat = 0;
    //IYUV: Y + U + V  (3 planes)
    //YV12: Y + V + U  (3 planes)
    pixformat = SDL_PIXELFORMAT_IYUV;

    SDL_Texture *sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat, SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);
    SDL_Rect sdlRect;

    SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video_thread_fn, NULL, NULL);
    SDL_Thread *network_thread = SDL_CreateThread(network_thread_fn, NULL, NULL);
    SDL_Event event;
    bool isPlaying = true;
    while (1)
    {
        //Wait
        SDL_WaitEvent(&event);
        if (event.type == REFRESH_EVENT && isPlaying)
        {
            SDL_UpdateTexture(sdlTexture, NULL, buffer, pixel_w);

            //FIX: If window is resize
            sdlRect.x = 0;
            sdlRect.y = 0;
            sdlRect.w = screen_w;
            sdlRect.h = screen_h;

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
            SDL_GetWindowSize(screen, &screen_w, &screen_h);
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