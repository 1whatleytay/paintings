#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>

#include <fmt/printf.h>

#include <paintings/image.h>
#include <paintings/colors.h>

#include <array>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

struct Options {
    std::string input;
    std::string output;

    Options(int count, const char **args) {
        CLI::App app("Visualizer for the hue color classifier.");

        app.add_option("-i,--input", input, "Input picture file.")->required();
        app.add_option("-o,--output", output, "Output picture file.")->required();

        try {
            app.parse(count, args);
        } catch (const CLI::ParseError &e) {
            throw std::runtime_error(e.what());
        }
    }
};

constexpr std::array colorValues = {
    RGB(0xFF0000), // "RED",
    RGB(0xFFFF00), // "YELLOW",
    RGB(0x00FF00), // "GREEN",
    RGB(0x00FFFF), // "CYAN",
    RGB(0x0000FF), // "BLUE",
    RGB(0xFF00FF), // "MAGENTA",
    RGB(0xFFFFFF), // "WHITE",
    RGB(0xFFFFFF), // "BLACK",
    RGB(0x808080), // "GRAY"
};

int main(int count, const char **args) {
    try {
        Options options(count, args);

        ImageData image(options.input);
        RGB *input = reinterpret_cast<RGB *>(image.data);
        std::vector<RGB> output(image.width * image.height);

        for (size_t a = 0; a < image.width * image.height; a++)
            output[a] = colorValues[HSL(input[a]).classify()];

        stbi_write_png(
            options.output.c_str(), image.width, image.height,
            3, output.data(), static_cast<int>(image.width * sizeof(RGB)));
    } catch (const std::runtime_error &e) {
        fmt::print("{}\n", e.what());
    }
}