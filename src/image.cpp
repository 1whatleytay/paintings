#include <paintings/image.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

ImageData::ImageData(const std::string &path) {
    data = stbi_load(path.c_str(), &width, &height, nullptr, 3);

    if (!data)
        throw std::runtime_error("Something went horribly wrong...");
}

ImageData::ImageData(const uint8_t *input, size_t size) {
    data = stbi_load_from_memory(input, size, &width, &height, nullptr, 3);

    if (!data)
        throw std::runtime_error("Something went horribly wrong...");
}

ImageData::~ImageData() {
    stbi_image_free(data);
}
