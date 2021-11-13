#ifndef _INPUT_EMULATOR__
#define _INPUT_EMULATOR__

#include "stdio.h"
#include <SDL_events.h>
#include <SDL_keycode.h>

#ifdef _WINN32
#include <winuser.h>

void emulate_mouse_movement(int mouse_x, int mouse_y)
{
    SetCursorPos(mouse_x, mouse_y);
}

void emulate_keyboard_key(Uint8 key)
{
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;
    input.ki.wScan = (WORD)key;
    input.ki.dwFlags = KEYEVENTF_UNICODE;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    if ((key & 0xFF00) == 0xE000)
    {
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    SendInput(1, &input, sizeof(INPUT));
}

void emulate_mouse_click(Uint8 button)
{
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    if (button == SDL_BUTTON_LEFT)
    {
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    }
    else if (button == SDL_BUTTON_RIGHT)
    {
        input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    }

    SendInput(1, &input, sizeof(INPUT));
}
#else
void emulate_mouse_movement(int mouse_x, int mouse_y)
{
    printf("Not Implemented\n");
}

void emulate_keyboard_key(Uint8 key)
{
    printf("Not Implemented\n");
}

void emulate_mouse_click(Uint8 button)
{
    printf("Not Implemented\n");
}
#endif

void handle_sdl_event(SDL_Event event)
{
    SDL_EventType event_type = (SDL_EventType)event.type;
    if (event_type == SDL_MOUSEMOTION)
    {
        int mouse_x = event.motion.x;
        int mouse_y = event.motion.y;
        printf("Mouse move evt: %d %d\n", mouse_x, mouse_y);
        emulate_mouse_movement(mouse_x, mouse_y);
    }
    else if (event_type == SDL_MOUSEBUTTONDOWN)
    {
        printf("Mouse left click evt\n");
        emulate_mouse_click(event.button.button);
    }
    else if (event_type == SDL_MOUSEBUTTONUP)
    {
        printf("Mouse right click evt\n");
        emulate_mouse_click(event.button.button);
    }
    else if (event_type == SDL_MOUSEWHEEL)
    {
    }
    else if (event_type == SDL_KEYDOWN)
    {
        printf("Keyboard evt %d\n", event.button.button);
        emulate_keyboard_key(event.button.button);
    }
    else if (event_type == SDL_KEYUP)
    {
        printf("Keyboard evt %d\n", event.button.button);
        emulate_keyboard_key(event.button.button);
    }
    else if (event_type == SDL_MOUSEBUTTONUP)
    {
    }
}
#endif