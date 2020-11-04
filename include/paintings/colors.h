#pragma once

#include <cstdint>
#include <cstdlib>

struct RGB {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;

    constexpr RGB() = default;
    explicit constexpr RGB(uint32_t value) {
        red = (value & 0xFF0000u) >> 16u;
        green = (value & 0x00FF00u) >> 8u;
        blue = (value & 0x0000FFu);
    }
};

struct HSL {
    double hue = 0;
    double saturation = 0;
    double lightness = 0;

    size_t classify() const;

    explicit HSL(const RGB &rgb);
};
