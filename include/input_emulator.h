#ifndef _INPUT_EMULATOR__
#define _INPUT_EMULATOR__

#include "stdio.h"
#include <SDL_events.h>
#include <SDL_keycode.h>

class InputEmulator
{
public:
    static void handle_sdl_event(SDL_Event &event);

private:
    static void emulate_mouse_movement(int mouse_x, int mouse_y);
    static void emulate_keyboard_key(char key);
    static void emulate_mouse_click(Uint8 button, bool pressed);
};
#endif
