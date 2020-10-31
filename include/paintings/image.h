#pragma once

#include <string>

struct ImageData {
    int32_t width = 0;
    int32_t height = 0;
    uint8_t *data = nullptr;

    explicit ImageData(const std::string &path);
    ImageData(const uint8_t *input, size_t size);

    ~ImageData();
};
