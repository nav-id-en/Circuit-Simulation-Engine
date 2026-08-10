#pragma once

#include "proteus/ui/Sdl.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace proteus::ui {

struct Color {
    Uint8 r = 255;
    Uint8 g = 255;
    Uint8 b = 255;
    Uint8 a = 255;
};

namespace colors {
inline constexpr Color background{10, 16, 27, 255};
inline constexpr Color panel{18, 29, 45, 255};
inline constexpr Color panelLight{27, 42, 62, 255};
inline constexpr Color border{58, 80, 104, 255};
inline constexpr Color text{222, 233, 242, 255};
inline constexpr Color muted{139, 160, 181, 255};
inline constexpr Color accent{49, 203, 159, 255};
inline constexpr Color accentBlue{62, 150, 255, 255};
inline constexpr Color warning{255, 190, 65, 255};
inline constexpr Color error{255, 91, 105, 255};
inline constexpr Color highSignal{255, 83, 98, 255};
inline constexpr Color lowSignal{55, 139, 255, 255};
inline constexpr Color analogSignal{255, 205, 82, 255};
inline constexpr Color undefinedSignal{130, 145, 160, 255};
} // namespace colors

[[nodiscard]] bool contains(const SDL_Rect& rect, int x, int y);
[[nodiscard]] bool intersects(const SDL_Rect& first, const SDL_Rect& second);
[[nodiscard]] Color withAlpha(Color color, Uint8 alpha);
[[nodiscard]] Color mix(Color first, Color second, float amount);

void setColor(SDL_Renderer* renderer, Color color);
void fillRect(SDL_Renderer* renderer, const SDL_Rect& rect, Color color);
void strokeRect(SDL_Renderer* renderer, const SDL_Rect& rect, Color color,
                int thickness = 1);
void line(SDL_Renderer* renderer, int x1, int y1, int x2, int y2,
          Color color, int thickness = 1);
void circle(SDL_Renderer* renderer, int centerX, int centerY, int radius,
            Color color, bool filled = false);
void roundedPanel(SDL_Renderer* renderer, const SDL_Rect& rect, Color fill,
                  Color border, int radius = 7);

[[nodiscard]] int textWidth(std::string_view text, int scale = 2);
[[nodiscard]] int textHeight(int scale = 2);
[[nodiscard]] std::string clippedText(std::string text, int maxWidth,
                                      int scale = 2);
void text(SDL_Renderer* renderer, int x, int y, std::string_view value,
          Color color = colors::text, int scale = 2);
void textCentered(SDL_Renderer* renderer, const SDL_Rect& rect,
                  std::string_view value, Color color = colors::text,
                  int scale = 2);

[[nodiscard]] bool saveRendererBmp(SDL_Renderer* renderer,
                                   const std::string& path, int width,
                                   int height);
[[nodiscard]] bool saveRendererBmpRegion(SDL_Renderer* renderer,
                                         const std::string& path,
                                         const SDL_Rect& region);
[[nodiscard]] bool saveRendererPng(SDL_Renderer* renderer,
                                   const std::string& path, int width,
                                   int height);
[[nodiscard]] bool saveRendererPngRegion(SDL_Renderer* renderer,
                                         const std::string& path,
                                         const SDL_Rect& region);

} // namespace proteus::ui
