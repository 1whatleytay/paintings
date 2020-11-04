#include <paintings/colors.h>

#include <cmath>
#include <algorithm>

size_t HSL::classify() const {
    if (lightness < 0.03) {
        return 6; // black
    } else if (lightness > 0.9) {
        return 7; // white
    } else if (saturation < 0.15) {
        return 8; // gray
    } else {
        return static_cast<size_t>(std::fmod(hue + 30.0, 360.0) / 60.0);
    }
}

HSL::HSL(const RGB &rgb) {
    double r = static_cast<double>(rgb.red) / 255.0;
    double g = static_cast<double>(rgb.green) / 255.0;
    double b = static_cast<double>(rgb.blue) / 255.0;

    double max = std::max(std::max(r, g), b);
    double min = std::min(std::min(r, g), b);
    double diff = max - min;

    lightness = (max + min) / 2;

    if (diff == 0) {
        hue = 0;
        saturation = 0;
    } else {
        if (r == max) {
            hue = 60 * (g - b) / diff;
            if (hue < 0)
                hue += 360;
        } else if (g == max) {
            hue = 60 * ((b - r) / diff + 2.0);
        } else if (b == max) {
            hue = 60 * ((r - g) / diff + 4.0);
        }

        saturation = diff / (1 - std::abs(2 * lightness - 1));
    }
}
