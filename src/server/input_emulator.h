#pragma once

#include <SDL_events.h>
#include <SDL_keycode.h>

#include <winuser.h>

void emulate_mouse_movement(int mouse_x, int mouse_y)
{
    SetCursorPos(mouse_x, mouse_y);
}

void handle_sdl_event(SDL_Event event)
{
    SDL_EventType event_type = (SDL_EventType)event.type;
    if (event_type == SDL_MOUSEMOTION)
    {
        int mouse_x = event.motion.x;
        int mouse_y = event.motion.y;
        emulate_mouse_movement(mouse_x, mouse_y);
    }
    else if (event_type == SDL_MOUSEBUTTONDOWN)
    {
    }
    else if (event_type == SDL_MOUSEBUTTONUP)
    {
    }
    else if (event_type == SDL_MOUSEWHEEL)
    {
    }
    else if (event_type == SDL_KEYDOWN)
    {
    }
    else if (event_type == SDL_KEYUP)
    {
    }
    else if (event_type == SDL_MOUSEBUTTONUP)
    {
    }
}
