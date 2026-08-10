#pragma once

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else

// Small SDL2 declaration subset used only when development headers are absent.
// Normal Windows/Linux builds use the official SDL2 headers.

#include <cstdint>

extern "C" {

using Uint8 = std::uint8_t;
using Sint8 = std::int8_t;
using Uint16 = std::uint16_t;
using Sint16 = std::int16_t;
using Uint32 = std::uint32_t;
using Sint32 = std::int32_t;
using Uint64 = std::uint64_t;
using Sint64 = std::int64_t;

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Surface;

struct SDL_Point {
    int x;
    int y;
};

struct SDL_Rect {
    int x;
    int y;
    int w;
    int h;
};

struct SDL_Color {
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
};

using SDL_Keycode = Sint32;
using SDL_Scancode = int;
using SDL_Keymod = int;

struct SDL_Keysym {
    SDL_Scancode scancode;
    SDL_Keycode sym;
    Uint16 mod;
    Uint32 unused;
};

struct SDL_CommonEvent {
    Uint32 type;
    Uint32 timestamp;
};

struct SDL_WindowEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint8 event;
    Uint8 padding1;
    Uint8 padding2;
    Uint8 padding3;
    Sint32 data1;
    Sint32 data2;
};

struct SDL_KeyboardEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint8 state;
    Uint8 repeat;
    Uint8 padding2;
    Uint8 padding3;
    SDL_Keysym keysym;
};

struct SDL_TextInputEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    char text[32];
};

struct SDL_MouseMotionEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint32 which;
    Uint32 state;
    Sint32 x;
    Sint32 y;
    Sint32 xrel;
    Sint32 yrel;
};

struct SDL_MouseButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint32 which;
    Uint8 button;
    Uint8 state;
    Uint8 clicks;
    Uint8 padding1;
    Sint32 x;
    Sint32 y;
};

struct SDL_MouseWheelEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint32 which;
    Sint32 x;
    Sint32 y;
    Uint32 direction;
    float preciseX;
    float preciseY;
    Sint32 mouseX;
    Sint32 mouseY;
};

union SDL_Event {
    Uint32 type;
    SDL_CommonEvent common;
    SDL_WindowEvent window;
    SDL_KeyboardEvent key;
    SDL_TextInputEvent text;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_MouseWheelEvent wheel;
    Uint8 padding[56];
};

int SDL_Init(Uint32 flags);
void SDL_Quit();
const char* SDL_GetError();
int SDL_SetHint(const char* name, const char* value);

SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h,
                             Uint32 flags);
void SDL_DestroyWindow(SDL_Window* window);
void SDL_SetWindowTitle(SDL_Window* window, const char* title);
void SDL_SetWindowMinimumSize(SDL_Window* window, int minW, int minH);
void SDL_GetWindowSize(SDL_Window* window, int* w, int* h);

SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, Uint32 flags);
void SDL_DestroyRenderer(SDL_Renderer* renderer);
int SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, int blendMode);
int SDL_SetRenderDrawColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b,
                           Uint8 a);
int SDL_RenderClear(SDL_Renderer* renderer);
void SDL_RenderPresent(SDL_Renderer* renderer);
int SDL_RenderDrawPoint(SDL_Renderer* renderer, int x, int y);
int SDL_RenderDrawLine(SDL_Renderer* renderer, int x1, int y1, int x2, int y2);
int SDL_RenderDrawLines(SDL_Renderer* renderer, const SDL_Point* points,
                        int count);
int SDL_RenderDrawRect(SDL_Renderer* renderer, const SDL_Rect* rect);
int SDL_RenderFillRect(SDL_Renderer* renderer, const SDL_Rect* rect);
int SDL_RenderSetClipRect(SDL_Renderer* renderer, const SDL_Rect* rect);
int SDL_RenderReadPixels(SDL_Renderer* renderer, const SDL_Rect* rect,
                         Uint32 format, void* pixels, int pitch);

int SDL_PollEvent(SDL_Event* event);
Uint64 SDL_GetTicks64();
void SDL_Delay(Uint32 milliseconds);
Uint32 SDL_GetMouseState(int* x, int* y);
SDL_Keymod SDL_GetModState();
void SDL_StartTextInput();
void SDL_StopTextInput();
int SDL_HasClipboardText();
char* SDL_GetClipboardText();
int SDL_SetClipboardText(const char* text);
void SDL_free(void* memory);
int SDL_ShowSimpleMessageBox(Uint32 flags, const char* title,
                             const char* message, SDL_Window* window);

} // extern "C"

#define SDL_INIT_VIDEO 0x00000020u
#define SDL_WINDOWPOS_CENTERED 0x2FFF0000u
#define SDL_WINDOW_SHOWN 0x00000004u
#define SDL_WINDOW_HIDDEN 0x00000008u
#define SDL_WINDOW_RESIZABLE 0x00000020u
#define SDL_WINDOW_ALLOW_HIGHDPI 0x00002000u
#define SDL_RENDERER_SOFTWARE 0x00000001u
#define SDL_RENDERER_ACCELERATED 0x00000002u
#define SDL_RENDERER_PRESENTVSYNC 0x00000004u
#define SDL_BLENDMODE_NONE 0x00000000
#define SDL_BLENDMODE_BLEND 0x00000001
#define SDL_PIXELFORMAT_ARGB8888 372645892u

#define SDL_QUIT 0x100u
#define SDL_WINDOWEVENT 0x200u
#define SDL_KEYDOWN 0x300u
#define SDL_KEYUP 0x301u
#define SDL_TEXTINPUT 0x303u
#define SDL_MOUSEMOTION 0x400u
#define SDL_MOUSEBUTTONDOWN 0x401u
#define SDL_MOUSEBUTTONUP 0x402u
#define SDL_MOUSEWHEEL 0x403u
#define SDL_WINDOWEVENT_RESIZED 0x05u
#define SDL_WINDOWEVENT_SIZE_CHANGED 0x06u

#define SDL_BUTTON_LEFT 1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT 3
#define SDL_BUTTON(X) (1u << ((X)-1))
#define SDL_BUTTON_LMASK SDL_BUTTON(SDL_BUTTON_LEFT)

#define KMOD_NONE 0x0000
#define KMOD_LSHIFT 0x0001
#define KMOD_RSHIFT 0x0002
#define KMOD_SHIFT (KMOD_LSHIFT | KMOD_RSHIFT)
#define KMOD_LCTRL 0x0040
#define KMOD_RCTRL 0x0080
#define KMOD_CTRL (KMOD_LCTRL | KMOD_RCTRL)
#define KMOD_LALT 0x0100
#define KMOD_RALT 0x0200
#define KMOD_ALT (KMOD_LALT | KMOD_RALT)

#define SDLK_UNKNOWN 0
#define SDLK_RETURN '\r'
#define SDLK_ESCAPE '\x1B'
#define SDLK_BACKSPACE '\b'
#define SDLK_TAB '\t'
#define SDLK_SPACE ' '
#define SDLK_DELETE 127
#define SDLK_a 'a'
#define SDLK_c 'c'
#define SDLK_e 'e'
#define SDLK_h 'h'
#define SDLK_j 'j'
#define SDLK_l 'l'
#define SDLK_m 'm'
#define SDLK_n 'n'
#define SDLK_o 'o'
#define SDLK_p 'p'
#define SDLK_q 'q'
#define SDLK_r 'r'
#define SDLK_s 's'
#define SDLK_v 'v'
#define SDLK_w 'w'
#define SDLK_x 'x'
#define SDLK_y 'y'
#define SDLK_z 'z'
#define SDLK_PLUS '+'
#define SDLK_MINUS '-'
#define SDLK_EQUALS '='
#define SDLK_KP_PLUS 1073741911
#define SDLK_KP_MINUS 1073741910
#define SDLK_LEFT 1073741904
#define SDLK_RIGHT 1073741903
#define SDLK_UP 1073741906
#define SDLK_DOWN 1073741905

#define SDL_MESSAGEBOX_ERROR 0x00000010u
#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"

#endif
