#include <fmt/printf.h>

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

#include <array>
#include <filesystem>

namespace fs = std::filesystem;

struct ImageData {
    int32_t width = 0;
    int32_t height = 0;
    uint8_t *data = nullptr;

    explicit ImageData(const std::string &path) {
        int32_t channels;
        data = stbi_load(path.c_str(), &width, &height, &channels, 3);

        if (!data || channels != 3)
            throw std::runtime_error("Something went horribly wrong...");
    }

    ~ImageData() {
        stbi_image_free(data);
    }
};

struct Color {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;

    std::tuple<uint64_t, uint64_t> difference(const Color &other) const {
        uint64_t diffRed = std::max(red, other.red) - std::min(red, other.red);
        uint64_t diffGreen = std::max(green, other.green) - std::min(green, other.green);
        uint64_t diffBlue = std::max(blue, other.blue) - std::min(blue, other.blue);

        return std::make_tuple(
            diffRed + diffGreen + diffBlue,
            diffRed * diffRed + diffGreen * diffGreen + diffBlue * diffBlue);
    }

    constexpr explicit Color(uint32_t color) noexcept {
        red = (color & 0xFF0000u) >> 16u;
        green = (color & 0x00FF00u) >> 8u;
        blue = (color & 0x0000FFu);
    }
};

constexpr std::array samples = {
    std::make_tuple(Color(0xFFFFFF), "WHITE"), // white
    std::make_tuple(Color(0x000000), "BLACK"), // black
    std::make_tuple(Color(0xFF0000), "RED"), // red
    std::make_tuple(Color(0x00FF00), "GREEN"), // green
    std::make_tuple(Color(0x0000FF), "BLUE"), // blue
    std::make_tuple(Color(0xFFFF00), "YELLOW"), // yellow
    std::make_tuple(Color(0x00FFFF), "CYAN"), // cyan
    std::make_tuple(Color(0xFF00FF), "MAGENTA"), // magenta
    std::make_tuple(Color(0x808080), "GRAY"), // gray
    std::make_tuple(Color(0xFFA500), "ORANGE"), // orange
    std::make_tuple(Color(0x800080), "PURPLE"), // purple
};

struct AnalysisResults {
    uint64_t numPixels = 0;
    std::array<uint64_t, samples.size()> sampleFrequency = { };
    std::array<uint64_t, samples.size()> squaredFrequency = { };
    std::array<uint64_t, samples.size()> sampleScore = { };
    std::array<uint64_t, samples.size()> squaredScore = { };

    static std::string join(const std::array<uint64_t, samples.size()> &arr) {
        std::array<std::string, samples.size()> texts;

        for (size_t a = 0; a < samples.size(); a++) {
            texts[a] = fmt::format("{:>10}: {}", std::get<1>(samples[a]), arr[a]);
        }

        return fmt::format("{}", fmt::join(texts, "\n"));
    }

    static std::string join(const std::array<double, samples.size()> &arr) {
        std::array<std::string, samples.size()> texts;

        for (size_t a = 0; a < samples.size(); a++) {
            texts[a] = fmt::format("{:>10}: {:.3f}", std::get<1>(samples[a]), arr[a]);
        }

        return fmt::format("{}", fmt::join(texts, "\n"));
    }

    std::string toString() const {
        auto normalize = [this](uint64_t i) {
            return static_cast<double>(i) / static_cast<double>(numPixels);
        };

        std::array<double, samples.size()> sampleNormalized = { };
        std::transform(sampleFrequency.begin(), sampleFrequency.end(), sampleNormalized.begin(), normalize);

        std::array<double, samples.size()> squaredNormalized = { };
        std::transform(squaredFrequency.begin(), squaredFrequency.end(), squaredNormalized.begin(), normalize);

        return fmt::format(
            "Pixel Count: {}\n"
            "Sample Frequency:\n{}\n"
            "Squared Frequency:\n{}\n"
            "Sample Score:\n{}\n"
            "Squared Score:\n{}\n"
            "Sample Normalized:\n{}\n"
            "Squared Normalized:\n{}\n",
            numPixels,
            join(sampleFrequency),
            join(squaredFrequency),
            join(sampleScore),
            join(squaredScore),
            join(sampleNormalized),
            join(squaredNormalized));
    }

    explicit AnalysisResults(const ImageData &image) {
        numPixels = image.width * image.height;

        const Color *colors = reinterpret_cast<Color *>(image.data);

        for (uint64_t a = 0; a < numPixels; a++) {
            std::array<uint64_t, samples.size()> sumDifference = { };
            std::array<uint64_t, samples.size()> squareDifference = { };

            const Color &color = colors[a];

            for (size_t b = 0; b < samples.size(); b++) {
                const Color &sample = std::get<0>(samples[b]);

                const auto [diff, squared] = sample.difference(color);

                sumDifference[b] = diff;
                squareDifference[b] = squared;
            }

            auto find = [](const std::array<uint64_t, samples.size()> &arr) {
                size_t index = 0;
                uint64_t value = ~0ull;

                for (size_t a = 0; a < arr.size(); a++) {
                    if (arr[a] < value) {
                        value = arr[a];
                        index = a;
                    }
                }

                return std::make_tuple(index, value);
            };

            auto [diffIndex, diffValue] = find(sumDifference);
            auto [squareIndex, squareValue] = find(squareDifference);

            sampleFrequency[diffIndex]++;
            squaredFrequency[squareIndex]++;
            sampleScore[diffIndex] += diffValue;
            squaredScore[squareIndex] += squareValue;
        }
    }
};

int main(int count, const char **args) {
    if (count != 2) {
        fmt::print("Usage: analyze-true path/to/file");
        return 1;
    }

    std::string path = args[1];
    if (!fs::exists(path)) {
        fmt::print("Could not find file or directory at \'{}\'.", path);
        return 1;
    }

    if (fs::is_directory(path)) {
        throw std::runtime_error("oops");
    } else {
        ImageData data(path);
        AnalysisResults results(data);

        fmt::print("{}\n", results.toString());
    }

    return 0;
}
