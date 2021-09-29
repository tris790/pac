#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)
#include "SDL.h"
#include <lib.hh>

#undef main
const int bpp = 12;

int screen_w = 1920, screen_h = 1000;
const int pixel_w = 1920, pixel_h = 1080;
unsigned char buffer[pixel_w * pixel_h * bpp / 8];

//Refresh Event
#define REFRESH_EVENT (SDL_USEREVENT + 1)
//Break
#define BREAK_EVENT (SDL_USEREVENT + 2)

int thread_exit = 0;

int refresh_video(void *opaque)
{
    thread_exit = 0;
    while (thread_exit == 0)
    {
        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(40);
    }
    thread_exit = 0;
    //Break
    SDL_Event event;
    event.type = BREAK_EVENT;
    SDL_PushEvent(&event);
    return 0;
}

int network(void *opaque)
{
    uvgrtp::context ctx;
    printf("Client1\n");
    /* See sending.cc for more details */
    uvgrtp::session *sess = ctx.create_session("127.0.0.1");
    printf("Client2\n");

    /* See sending.cc for more details */
    uvgrtp::media_stream *hevc = sess->create_stream(8889, 8888, RTP_FORMAT_GENERIC, RTP_NO_FLAGS);
    printf("Client3\n");

    /* pull_frame() will block until a frame is received.
     *
     * If that is not acceptable, a separate thread for the reader should be created */
    uvgrtp::frame::rtp_frame *frame = nullptr;
    printf("Client4\n");

    for (;;)
    {
        printf("Client4.5\n");
        frame = hevc->pull_frame();
        if (frame)
        {
            printf("Client5 '%s'\n", frame->payload);
            (void)uvgrtp::frame::dealloc_frame(frame);
            break;
        }
    }

    /* You can also specify for a timeout for the operation and if the a frame is not received 
     * within that time limit, pull_frame() returns a nullptr 
     *
     * The parameter tells how long time a frame is waited in milliseconds */
    printf("Client6\n");
    frame = hevc->pull_frame(200);

    /* Frame must be freed manually */
    uvgrtp::frame::dealloc_frame(frame);

    ctx.destroy_session(sess);
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

    FILE *fp = NULL;
    fp = fopen("C:/Storage/Coding/c++/SdlVideo/x64/Release/output.yuv", "rb+");

    if (fp == NULL)
    {
        printf("cannot open this file\n");
        return -1;
    }

    SDL_Rect sdlRect;

    SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video, NULL, NULL);
    SDL_Thread *network_thread = SDL_CreateThread(network, NULL, NULL);
    SDL_Event event;
    bool isPlaying = true;
    while (1)
    {
        //Wait
        SDL_WaitEvent(&event);
        if (event.type == REFRESH_EVENT && isPlaying)
        {
            if (fread(buffer, 1, pixel_w * pixel_h * bpp / 8, fp) != pixel_w * pixel_h * bpp / 8)
            {
                // Loop
                fseek(fp, 0, SEEK_SET);
                fread(buffer, 1, pixel_w * pixel_h * bpp / 8, fp);
            }
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