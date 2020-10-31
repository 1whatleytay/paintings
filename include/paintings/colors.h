#pragma once

#include <cstdint>

struct RGB {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

struct HSL {
    double hue = 0;
    double saturation = 0;
    double lightness = 0;

    explicit HSL(const RGB &rgb);
};
