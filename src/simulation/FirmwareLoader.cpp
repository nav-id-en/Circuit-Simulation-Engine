#include "proteus/simulation/FirmwareLoader.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace proteus {
namespace {

std::uint8_t parseByte(const std::string& text, std::size_t offset) {
    if (offset + 2 > text.size()) {
        throw std::runtime_error("Unexpected end of Intel HEX record.");
    }
    const auto token = text.substr(offset, 2);
    std::size_t consumed = 0;
    const auto value = std::stoul(token, &consumed, 16);
    if (consumed != 2 || value > 0xFFU) {
        throw std::runtime_error("Invalid hexadecimal byte: " + token);
    }
    return static_cast<std::uint8_t>(value);
}

std::string trim(std::string value) {
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(), [](unsigned char c) {
                    return !std::isspace(c);
                }));
    value.erase(
        std::find_if(value.rbegin(), value.rend(), [](unsigned char c) {
            return !std::isspace(c);
        }).base(),
        value.end());
    return value;
}

} // namespace

FirmwareImage FirmwareLoader::loadIntelHexFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Cannot open firmware file: " + path);
    }
    std::ostringstream content;
    content << input.rdbuf();
    return parseIntelHex(content.str());
}

FirmwareImage FirmwareLoader::parseIntelHex(const std::string& text) {
    FirmwareImage image;
    std::istringstream stream(text);
    std::string line;
    std::uint32_t extendedAddress = 0;
    bool endOfFileSeen = false;
    std::size_t lineNumber = 0;

    while (std::getline(stream, line)) {
        ++lineNumber;
        line = trim(std::move(line));
        if (line.empty()) {
            continue;
        }
        if (line.front() != ':' || line.size() < 11
            || (line.size() - 1) % 2 != 0) {
            throw std::runtime_error(
                "Invalid Intel HEX syntax on line "
                + std::to_string(lineNumber) + ".");
        }

        const auto byteCount = parseByte(line, 1);
        const auto address =
            static_cast<std::uint16_t>((parseByte(line, 3) << 8U)
                                       | parseByte(line, 5));
        const auto recordType = parseByte(line, 7);
        const auto expectedLength =
            static_cast<std::size_t>(11 + byteCount * 2);
        if (line.size() != expectedLength) {
            throw std::runtime_error(
                "Intel HEX byte count mismatch on line "
                + std::to_string(lineNumber) + ".");
        }

        std::uint8_t checksum = byteCount;
        checksum = static_cast<std::uint8_t>(
            checksum + static_cast<std::uint8_t>(address >> 8U));
        checksum =
            static_cast<std::uint8_t>(checksum
                                      + static_cast<std::uint8_t>(address));
        checksum = static_cast<std::uint8_t>(checksum + recordType);

        std::vector<std::uint8_t> data;
        data.reserve(byteCount);
        for (std::size_t index = 0; index < byteCount; ++index) {
            const auto value = parseByte(line, 9 + index * 2);
            checksum = static_cast<std::uint8_t>(checksum + value);
            data.push_back(value);
        }
        checksum = static_cast<std::uint8_t>(
            checksum + parseByte(line, 9 + byteCount * 2));
        if (checksum != 0) {
            throw std::runtime_error(
                "Intel HEX checksum failed on line "
                + std::to_string(lineNumber) + ".");
        }

        if (recordType == 0x00) {
            const auto absoluteAddress =
                extendedAddress + static_cast<std::uint32_t>(address);
            const auto requiredSize =
                static_cast<std::size_t>(absoluteAddress) + data.size();
            if (requiredSize > 16U * 1024U * 1024U) {
                throw std::runtime_error(
                    "Firmware image exceeds the 16 MiB safety limit.");
            }
            if (image.bytes.size() < requiredSize) {
                image.bytes.resize(requiredSize, 0xFF);
            }
            std::copy(data.begin(), data.end(),
                      image.bytes.begin()
                          + static_cast<std::ptrdiff_t>(absoluteAddress));
        } else if (recordType == 0x01) {
            endOfFileSeen = true;
            break;
        } else if (recordType == 0x04) {
            if (data.size() != 2) {
                throw std::runtime_error(
                    "Invalid extended linear address record.");
            }
            extendedAddress =
                static_cast<std::uint32_t>((data[0] << 8U) | data[1])
                << 16U;
        } else if (recordType == 0x05) {
            if (data.size() == 4) {
                image.startAddress =
                    (static_cast<std::uint32_t>(data[0]) << 24U)
                    | (static_cast<std::uint32_t>(data[1]) << 16U)
                    | (static_cast<std::uint32_t>(data[2]) << 8U)
                    | data[3];
            }
        }
    }

    if (!endOfFileSeen) {
        throw std::runtime_error(
            "Intel HEX end-of-file record is missing.");
    }
    return image;
}

} // namespace proteus
