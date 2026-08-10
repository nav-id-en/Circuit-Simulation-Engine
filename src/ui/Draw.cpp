#include "proteus/ui/Draw.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <vector>

namespace proteus::ui {
namespace {

using Glyph = std::array<Uint8, 7>;

Glyph glyphFor(char character) {
    const auto value = static_cast<char>(
        std::toupper(static_cast<unsigned char>(character)));
    switch (value) {
    case 'A': return {14, 17, 17, 31, 17, 17, 17};
    case 'B': return {30, 17, 17, 30, 17, 17, 30};
    case 'C': return {14, 17, 16, 16, 16, 17, 14};
    case 'D': return {30, 17, 17, 17, 17, 17, 30};
    case 'E': return {31, 16, 16, 30, 16, 16, 31};
    case 'F': return {31, 16, 16, 30, 16, 16, 16};
    case 'G': return {14, 17, 16, 23, 17, 17, 15};
    case 'H': return {17, 17, 17, 31, 17, 17, 17};
    case 'I': return {31, 4, 4, 4, 4, 4, 31};
    case 'J': return {7, 2, 2, 2, 2, 18, 12};
    case 'K': return {17, 18, 20, 24, 20, 18, 17};
    case 'L': return {16, 16, 16, 16, 16, 16, 31};
    case 'M': return {17, 27, 21, 21, 17, 17, 17};
    case 'N': return {17, 25, 21, 19, 17, 17, 17};
    case 'O': return {14, 17, 17, 17, 17, 17, 14};
    case 'P': return {30, 17, 17, 30, 16, 16, 16};
    case 'Q': return {14, 17, 17, 17, 21, 18, 13};
    case 'R': return {30, 17, 17, 30, 20, 18, 17};
    case 'S': return {15, 16, 16, 14, 1, 1, 30};
    case 'T': return {31, 4, 4, 4, 4, 4, 4};
    case 'U': return {17, 17, 17, 17, 17, 17, 14};
    case 'V': return {17, 17, 17, 17, 17, 10, 4};
    case 'W': return {17, 17, 17, 21, 21, 21, 10};
    case 'X': return {17, 17, 10, 4, 10, 17, 17};
    case 'Y': return {17, 17, 10, 4, 4, 4, 4};
    case 'Z': return {31, 1, 2, 4, 8, 16, 31};
    case '0': return {14, 17, 19, 21, 25, 17, 14};
    case '1': return {4, 12, 4, 4, 4, 4, 14};
    case '2': return {14, 17, 1, 2, 4, 8, 31};
    case '3': return {30, 1, 1, 14, 1, 1, 30};
    case '4': return {2, 6, 10, 18, 31, 2, 2};
    case '5': return {31, 16, 16, 30, 1, 1, 30};
    case '6': return {14, 16, 16, 30, 17, 17, 14};
    case '7': return {31, 1, 2, 4, 8, 8, 8};
    case '8': return {14, 17, 17, 14, 17, 17, 14};
    case '9': return {14, 17, 17, 15, 1, 1, 14};
    case '.': return {0, 0, 0, 0, 0, 6, 6};
    case ',': return {0, 0, 0, 0, 6, 6, 4};
    case ':': return {0, 6, 6, 0, 6, 6, 0};
    case ';': return {0, 6, 6, 0, 6, 6, 4};
    case '!': return {4, 4, 4, 4, 4, 0, 4};
    case '?': return {14, 17, 1, 2, 4, 0, 4};
    case '+': return {0, 4, 4, 31, 4, 4, 0};
    case '-': return {0, 0, 0, 31, 0, 0, 0};
    case '=': return {0, 31, 0, 31, 0, 0, 0};
    case '_': return {0, 0, 0, 0, 0, 0, 31};
    case '/': return {1, 2, 2, 4, 8, 8, 16};
    case '\\': return {16, 8, 8, 4, 2, 2, 1};
    case '(': return {2, 4, 8, 8, 8, 4, 2};
    case ')': return {8, 4, 2, 2, 2, 4, 8};
    case '[': return {14, 8, 8, 8, 8, 8, 14};
    case ']': return {14, 2, 2, 2, 2, 2, 14};
    case '<': return {2, 4, 8, 16, 8, 4, 2};
    case '>': return {8, 4, 2, 1, 2, 4, 8};
    case '#': return {10, 31, 10, 10, 31, 10, 0};
    case '%': return {17, 2, 4, 8, 16, 17, 0};
    case '&': return {12, 18, 20, 8, 21, 18, 13};
    case '@': return {14, 17, 23, 21, 23, 16, 14};
    case '*': return {0, 21, 14, 31, 14, 21, 0};
    case '\'': return {6, 4, 8, 0, 0, 0, 0};
    case '"': return {10, 10, 20, 0, 0, 0, 0};
    case '|': return {4, 4, 4, 4, 4, 4, 4};
    case '~': return {0, 0, 9, 22, 0, 0, 0};
    case ' ': return {0, 0, 0, 0, 0, 0, 0};
    default: return {31, 17, 21, 21, 21, 17, 31};
    }
}

void writeLittle16(std::ofstream& output, std::uint16_t value) {
    output.put(static_cast<char>(value & 0xFFU));
    output.put(static_cast<char>((value >> 8U) & 0xFFU));
}

void writeLittle32(std::ofstream& output, std::uint32_t value) {
    for (int byte = 0; byte < 4; ++byte) {
        output.put(static_cast<char>((value >> (byte * 8U)) & 0xFFU));
    }
}

void appendBig32(std::vector<Uint8>& output, std::uint32_t value) {
    output.push_back(static_cast<Uint8>((value >> 24U) & 0xFFU));
    output.push_back(static_cast<Uint8>((value >> 16U) & 0xFFU));
    output.push_back(static_cast<Uint8>((value >> 8U) & 0xFFU));
    output.push_back(static_cast<Uint8>(value & 0xFFU));
}

std::uint32_t crc32(const Uint8* data, std::size_t size) {
    std::uint32_t result = 0xFFFFFFFFU;
    for (std::size_t index = 0; index < size; ++index) {
        result ^= data[index];
        for (int bit = 0; bit < 8; ++bit) {
            const auto mask =
                static_cast<std::uint32_t>(
                    -static_cast<std::int32_t>(result & 1U));
            result = (result >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return result ^ 0xFFFFFFFFU;
}

void appendChunk(std::vector<Uint8>& png, const char type[4],
                 const std::vector<Uint8>& data) {
    appendBig32(png, static_cast<std::uint32_t>(data.size()));
    const auto crcStart = png.size();
    for (int index = 0; index < 4; ++index) {
        png.push_back(static_cast<Uint8>(type[index]));
    }
    png.insert(png.end(), data.begin(), data.end());
    appendBig32(png, crc32(png.data() + crcStart,
                           4U + data.size()));
}

} // namespace

bool contains(const SDL_Rect& rect, int x, int y) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w
        && y < rect.y + rect.h;
}

bool intersects(const SDL_Rect& first, const SDL_Rect& second) {
    return first.x < second.x + second.w && first.x + first.w > second.x
        && first.y < second.y + second.h
        && first.y + first.h > second.y;
}

Color withAlpha(Color color, Uint8 alpha) {
    color.a = alpha;
    return color;
}

Color mix(Color first, Color second, float amount) {
    amount = std::clamp(amount, 0.0F, 1.0F);
    const auto channel = [amount](Uint8 a, Uint8 b) {
        return static_cast<Uint8>(
            static_cast<float>(a)
            + (static_cast<float>(b) - static_cast<float>(a)) * amount);
    };
    return {channel(first.r, second.r), channel(first.g, second.g),
            channel(first.b, second.b), channel(first.a, second.a)};
}

void setColor(SDL_Renderer* renderer, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void fillRect(SDL_Renderer* renderer, const SDL_Rect& rect, Color color) {
    setColor(renderer, color);
    SDL_RenderFillRect(renderer, &rect);
}

void strokeRect(SDL_Renderer* renderer, const SDL_Rect& rect, Color color,
                int thickness) {
    setColor(renderer, color);
    for (int offset = 0; offset < std::max(1, thickness); ++offset) {
        SDL_Rect current{rect.x + offset, rect.y + offset,
                         std::max(0, rect.w - offset * 2),
                         std::max(0, rect.h - offset * 2)};
        SDL_RenderDrawRect(renderer, &current);
    }
}

void line(SDL_Renderer* renderer, int x1, int y1, int x2, int y2,
          Color color, int thickness) {
    setColor(renderer, color);
    const auto width = std::max(1, thickness);
    const auto half = width / 2;
    if (std::abs(x2 - x1) >= std::abs(y2 - y1)) {
        for (int offset = -half; offset <= half; ++offset) {
            SDL_RenderDrawLine(renderer, x1, y1 + offset, x2, y2 + offset);
        }
    } else {
        for (int offset = -half; offset <= half; ++offset) {
            SDL_RenderDrawLine(renderer, x1 + offset, y1, x2 + offset, y2);
        }
    }
}

void circle(SDL_Renderer* renderer, int centerX, int centerY, int radius,
            Color color, bool filled) {
    setColor(renderer, color);
    if (filled) {
        for (int y = -radius; y <= radius; ++y) {
            const auto x = static_cast<int>(
                std::sqrt(static_cast<double>(radius * radius - y * y)));
            SDL_RenderDrawLine(renderer, centerX - x, centerY + y,
                               centerX + x, centerY + y);
        }
        return;
    }
    int x = radius;
    int y = 0;
    int error = 1 - radius;
    while (x >= y) {
        const SDL_Point points[] = {
            {centerX + x, centerY + y}, {centerX + y, centerY + x},
            {centerX - y, centerY + x}, {centerX - x, centerY + y},
            {centerX - x, centerY - y}, {centerX - y, centerY - x},
            {centerX + y, centerY - x}, {centerX + x, centerY - y}};
        for (const auto& point : points) {
            SDL_RenderDrawPoint(renderer, point.x, point.y);
        }
        ++y;
        if (error < 0) {
            error += 2 * y + 1;
        } else {
            --x;
            error += 2 * (y - x) + 1;
        }
    }
}

void roundedPanel(SDL_Renderer* renderer, const SDL_Rect& rect, Color fill,
                  Color border, int radius) {
    radius = std::clamp(radius, 0, std::min(rect.w, rect.h) / 2);
    fillRect(renderer,
             {rect.x + radius, rect.y, rect.w - radius * 2, rect.h}, fill);
    fillRect(renderer,
             {rect.x, rect.y + radius, rect.w, rect.h - radius * 2}, fill);
    for (int y = 0; y < radius; ++y) {
        const auto delta = radius - y;
        const auto x = static_cast<int>(std::sqrt(
            static_cast<double>(radius * radius - delta * delta)));
        SDL_Rect row{rect.x + radius - x, rect.y + y,
                     rect.w - 2 * (radius - x), 1};
        fillRect(renderer, row, fill);
        row.y = rect.y + rect.h - 1 - y;
        fillRect(renderer, row, fill);
    }
    strokeRect(renderer, rect, border);
}

int textWidth(std::string_view value, int scale) {
    return static_cast<int>(value.size()) * 6 * std::max(1, scale);
}

int textHeight(int scale) {
    return 7 * std::max(1, scale);
}

std::string clippedText(std::string value, int maxWidth, int scale) {
    const auto cell = 6 * std::max(1, scale);
    if (static_cast<int>(value.size()) * cell <= maxWidth) return value;
    const auto count = std::max(0, maxWidth / cell - 3);
    if (count <= 0) return {};
    value.resize(static_cast<std::size_t>(count));
    return value + "...";
}

void text(SDL_Renderer* renderer, int x, int y, std::string_view value,
          Color color, int scale) {
    scale = std::max(1, scale);
    setColor(renderer, color);
    int cursor = x;
    for (const auto character : value) {
        const auto glyph = glyphFor(character);
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((glyph[static_cast<std::size_t>(row)]
                     & (1U << (4 - column)))
                    == 0U) {
                    continue;
                }
                SDL_Rect pixel{cursor + column * scale, y + row * scale,
                               scale, scale};
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
        cursor += 6 * scale;
    }
}

void textCentered(SDL_Renderer* renderer, const SDL_Rect& rect,
                  std::string_view value, Color color, int scale) {
    text(renderer, rect.x + (rect.w - textWidth(value, scale)) / 2,
         rect.y + (rect.h - textHeight(scale)) / 2, value, color, scale);
}

bool saveRendererBmp(SDL_Renderer* renderer, const std::string& path,
                     int width, int height) {
    return saveRendererBmpRegion(renderer, path, {0, 0, width, height});
}

bool saveRendererBmpRegion(SDL_Renderer* renderer, const std::string& path,
                           const SDL_Rect& region) {
    const auto width = region.w;
    const auto height = region.h;
    if (!renderer || width <= 0 || height <= 0) return false;
    std::vector<std::uint32_t> pixels(
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
    if (SDL_RenderReadPixels(renderer, &region, SDL_PIXELFORMAT_ARGB8888,
                             pixels.data(), width * 4)
        != 0) {
        return false;
    }
    const std::filesystem::path outputPath(path);
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }
    std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
    if (!output) return false;

    const auto pixelBytes =
        static_cast<std::uint32_t>(width * height * 4);
    const auto fileSize = 14U + 40U + pixelBytes;
    output.put('B');
    output.put('M');
    writeLittle32(output, fileSize);
    writeLittle16(output, 0);
    writeLittle16(output, 0);
    writeLittle32(output, 54);
    writeLittle32(output, 40);
    writeLittle32(output, static_cast<std::uint32_t>(width));
    writeLittle32(output, static_cast<std::uint32_t>(height));
    writeLittle16(output, 1);
    writeLittle16(output, 32);
    writeLittle32(output, 0);
    writeLittle32(output, pixelBytes);
    writeLittle32(output, 2835);
    writeLittle32(output, 2835);
    writeLittle32(output, 0);
    writeLittle32(output, 0);

    for (int y = height - 1; y >= 0; --y) {
        const auto* row =
            reinterpret_cast<const char*>(
                pixels.data() + static_cast<std::size_t>(y * width));
        output.write(row, width * 4);
    }
    return static_cast<bool>(output);
}

bool saveRendererPng(SDL_Renderer* renderer, const std::string& path,
                     int width, int height) {
    return saveRendererPngRegion(renderer, path, {0, 0, width, height});
}

bool saveRendererPngRegion(SDL_Renderer* renderer, const std::string& path,
                           const SDL_Rect& region) {
    if (!renderer || region.w <= 0 || region.h <= 0) return false;
    std::vector<std::uint32_t> pixels(
        static_cast<std::size_t>(region.w)
        * static_cast<std::size_t>(region.h));
    if (SDL_RenderReadPixels(renderer, &region, SDL_PIXELFORMAT_ARGB8888,
                             pixels.data(), region.w * 4)
        != 0) {
        return false;
    }

    std::vector<Uint8> raw;
    raw.reserve(static_cast<std::size_t>(region.h)
                * (static_cast<std::size_t>(region.w) * 4U + 1U));
    for (int y = 0; y < region.h; ++y) {
        raw.push_back(0);
        for (int x = 0; x < region.w; ++x) {
            const auto pixel =
                pixels[static_cast<std::size_t>(y * region.w + x)];
            raw.push_back(static_cast<Uint8>((pixel >> 16U) & 0xFFU));
            raw.push_back(static_cast<Uint8>((pixel >> 8U) & 0xFFU));
            raw.push_back(static_cast<Uint8>(pixel & 0xFFU));
            raw.push_back(static_cast<Uint8>((pixel >> 24U) & 0xFFU));
        }
    }

    std::vector<Uint8> compressed = {0x78U, 0x01U};
    std::size_t position = 0;
    while (position < raw.size()) {
        const auto blockSize =
            std::min<std::size_t>(65535U, raw.size() - position);
        compressed.push_back(
            position + blockSize == raw.size() ? 0x01U : 0x00U);
        const auto length = static_cast<std::uint16_t>(blockSize);
        const auto inverted = static_cast<std::uint16_t>(~length);
        compressed.push_back(static_cast<Uint8>(length & 0xFFU));
        compressed.push_back(static_cast<Uint8>(length >> 8U));
        compressed.push_back(static_cast<Uint8>(inverted & 0xFFU));
        compressed.push_back(static_cast<Uint8>(inverted >> 8U));
        compressed.insert(compressed.end(),
                          raw.begin()
                              + static_cast<std::ptrdiff_t>(position),
                          raw.begin()
                              + static_cast<std::ptrdiff_t>(
                                    position + blockSize));
        position += blockSize;
    }
    std::uint32_t s1 = 1;
    std::uint32_t s2 = 0;
    for (const auto value : raw) {
        s1 = (s1 + value) % 65521U;
        s2 = (s2 + s1) % 65521U;
    }
    appendBig32(compressed, (s2 << 16U) | s1);

    std::vector<Uint8> png = {
        0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU};
    std::vector<Uint8> header;
    appendBig32(header, static_cast<std::uint32_t>(region.w));
    appendBig32(header, static_cast<std::uint32_t>(region.h));
    header.insert(header.end(), {8U, 6U, 0U, 0U, 0U});
    appendChunk(png, "IHDR", header);
    appendChunk(png, "IDAT", compressed);
    appendChunk(png, "IEND", {});

    const std::filesystem::path outputPath(path);
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }
    std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
    if (!output) return false;
    output.write(reinterpret_cast<const char*>(png.data()),
                 static_cast<std::streamsize>(png.size()));
    return static_cast<bool>(output);
}

} // namespace proteus::ui
