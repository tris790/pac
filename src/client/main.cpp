#include <stdio.h>
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)
#include <SDL.h>
#include <SDL_mixer.h>
#include <lib.hh>
#include <thread>
#include <chrono>
#include <inttypes.h>
#include <assert.h>

#include "pac_network.h"
#include "config.h"

Config configuration = Config("client.conf");

// was needed because SDL was redeclaring main on something like that
#undef main

int window_width = stoi(configuration["window_width"]), window_heigth = stoi(configuration["window_heigth"]);
const int pixel_w = stoi(configuration["pixel_w"]), pixel_h = stoi(configuration["pixel_h"]);
const size_t screen_buffer_size = pixel_w * pixel_h * 4;
unsigned char *screen_buffer = (unsigned char *)malloc(screen_buffer_size);

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
    std::string server_hostname(configuration["hostname"]);
    auto receive_port = stoi(configuration["receive_port"]);
    auto send_port = stoi(configuration["send_port"]);
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
        // The server will tell us when a whole video frame is sent so we can refresh
        bool building_a_frame = true;
        while (building_a_frame && !thread_exit)
        {
            uvgrtp::frame::rtp_frame *video_network_frame = rtp_stream->pull_frame(5);
            if (video_network_frame)
            {
                NetworkPacket *video_network_packet = (NetworkPacket *)video_network_frame->payload;
                if (video_network_packet->packet_type == NETWORK_PACKET_TYPE::VIDEO)
                {
                    // We received a video frame, assuming the network frame is full size
                    int video_bytes_received = sizeof(video_network_packet->data);
                    if (video_network_packet->buffer_offset + video_bytes_received > screen_buffer_size)
                    {
                        // the network frame was too huge for our buffer, only take the remaining
                        video_bytes_received = screen_buffer_size - video_network_packet->buffer_offset;
                    }

                    // Copy the network frame content into our screen buffer
                    memcpy(screen_buffer + video_network_packet->buffer_offset, video_network_packet->data, video_bytes_received);
                }
                else if (video_network_packet->packet_type == NETWORK_PACKET_TYPE::FLUSH_SCREEN)
                {
                    // We received enough network frame to create a video frame so we refresh the screen
                    building_a_frame = false;
                }
            }
            uvgrtp::frame::dealloc_frame(video_network_frame);
        }

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

int main()
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
    std::string path = configuration["audio_file_path"];
    mmusic = Mix_LoadWAV(path.c_str());
    if (mmusic == NULL)
    {
        fprintf(stderr, "Unable to load wave file: %s\n", path.c_str());
    }

    Uint32 pixformat = 0;
    //IYUV: Y + U + V  (3 planes)
    //YV12: Y + V + U  (3 planes)
    pixformat = SDL_PIXELFORMAT_RGB888;

    SDL_Texture *sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat, SDL_TEXTUREACCESS_STREAMING, pixel_w, pixel_h);
    SDL_Rect sdlRect;

    SDL_Thread *network_thread = SDL_CreateThread(network_thread_fn, NULL, NULL);
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
            SDL_UpdateTexture(sdlTexture, NULL, screen_buffer, pixel_w * 4);

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
    free(screen_buffer);
    return 0;
}
