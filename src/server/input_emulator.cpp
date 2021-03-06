#include <input_emulator.h>
#include <iostream>
#include <logger.h>

#ifdef _WIN32
#include <windows.h>

void InputEmulator::emulate_mouse_movement(int mouse_x, int mouse_y)
{
    SetCursorPos(mouse_x, mouse_y);
}

void InputEmulator::emulate_keyboard_key(char sdl_key, bool isUp)
{
    auto vk_key = VkKeyScanExA(sdl_key, NULL);
    logger.debug("Windows key: %d", vk_key);

    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk_key;
    input.ki.dwFlags = isUp ? KEYEVENTF_KEYUP : 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    SendInput(1, &input, sizeof(INPUT));
}

void InputEmulator::emulate_mouse_click(Uint8 button, bool pressed)
{
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    if (button == SDL_BUTTON_LEFT)
    {
        input.mi.dwFlags = pressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    }
    else if (button == SDL_BUTTON_RIGHT)
    {
        input.mi.dwFlags = pressed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    }

    SendInput(1, &input, sizeof(INPUT));
}
#else
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

void InputEmulator::emulate_mouse_movement(int mouse_x, int mouse_y)
{
    Display *display = XOpenDisplay(0);

    Window root = DefaultRootWindow(display);
    XWarpPointer(display, None, root, 0, 0, 0, 0, mouse_x, mouse_y);
    XFlush(display);
    XCloseDisplay(display);
}

void InputEmulator::emulate_keyboard_key(Uint8 key)
{
    logger.warning("Not Implemented");
}

void InputEmulator::emulate_mouse_click(Uint8 button, bool pressed)
{
    //Button 1: The 'Left-Click' mouse button
    //Button 2: The 'Middle-Click' mouse button, or pressing down on the scroll wheel
    //Button 3: The 'Right-Click' mouse button
    //Button 4: Scrolling up one increment on the mouse wheel
    //Button 5: Scrolling down one increment on the mouse wheel
    //Button 6: Usually the 'back' button or scrolling one increment left on your mouse wheel
    //Button 7: Usually the 'forward' button or scrolling one increment right on your mouse wheel
    //Buttons 6-9 can differ depending on the type of mouse you have. Some mice may not even have these buttons at all!

    int x11_button = 0;
    if (button == SDL_BUTTON_LEFT)
    {
        x11_button = 1;
    }
    else if (button == SDL_BUTTON_RIGHT)
    {
        x11_button = 3;
    }

    Display *display = XOpenDisplay(NULL);
    XTestFakeButtonEvent(display, x11_button, pressed, CurrentTime);
    XFlush(display);
    XCloseDisplay(display);
}
#endif

void InputEmulator::handle_sdl_event(SDL_Event &event, int screen_width, int screen_height)
{
    SDL_EventType event_type = (SDL_EventType)event.type;
    if (event_type == SDL_MOUSEMOTION)
    {
        float ratiox = event.motion.xrel / static_cast<float>(screen_width);
        float ratioy = event.motion.yrel / static_cast<float>(screen_height);
        int mouse_x = (int)(event.motion.x / ratiox);
        int mouse_y = (int)(event.motion.y / ratioy);
        // logger.debug("Mouse move evt: %d %d", mouse_x, mouse_y);
        emulate_mouse_movement(mouse_x, mouse_y);
    }
    else if (event_type == SDL_MOUSEBUTTONDOWN)
    {
        logger.debug("Mouse pressed evt");
        emulate_mouse_click(event.button.button, true);
    }
    else if (event_type == SDL_MOUSEBUTTONUP)
    {
        logger.debug("Mouse release click evt");
        emulate_mouse_click(event.button.button, false);
    }
    else if (event_type == SDL_MOUSEWHEEL)
    {
        logger.warning("Not Implemented");
    }
    else if (event_type == SDL_KEYDOWN)
    {
        logger.debug("Kbd down evt %d", event.key.keysym.sym);
        emulate_keyboard_key(event.key.keysym.sym, false);
    }
    else if (event_type == SDL_KEYUP)
    {
        logger.debug("Kbd up evt %d", event.key.keysym.sym);
        emulate_keyboard_key(event.key.keysym.sym, true);
    }
}
