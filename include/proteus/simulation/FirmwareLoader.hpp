#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace proteus {

struct FirmwareImage {
    std::vector<std::uint8_t> bytes;
    std::uint32_t startAddress = 0;
};

class FirmwareLoader {
public:
    [[nodiscard]] static FirmwareImage loadIntelHexFile(const std::string& path);
    [[nodiscard]] static FirmwareImage parseIntelHex(const std::string& text);
};

} // namespace proteus
