#include <fmt/printf.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

#include <nlohmann/json.hpp>

#include <array>
#include <filesystem>

using nlohmann::json;

namespace fs = std::filesystem;

struct Options {
    std::string input;
    bool external = false;

    Options(int count, const char **args) {
        CLI::App app("Hue analyzer for images.");

        app.add_option("-i", input, "Input CSV database file for MET.")->required();
        app.add_flag("-e", external, "Output subsample file for future processing.");

        app.parse(count, args);
    }
};

struct ImageData {
    int32_t width = 0;
    int32_t height = 0;
    uint8_t *data = nullptr;

    explicit ImageData(const std::string &path) {
        data = stbi_load(path.c_str(), &width, &height, nullptr, 3);

        if (!data)
            throw std::runtime_error("Something went horribly wrong...");
    }

    ~ImageData() {
        stbi_image_free(data);
    }
};

struct RGB {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

struct HSL {
    double hue = 0;
    double saturation = 0;
    double lightness = 0;

    explicit HSL(const RGB &rgb) {
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
};

constexpr std::array samples = {
    "RED",
    "YELLOW",
    "GREEN",
    "CYAN",
    "BLUE",
    "MAGENTA",
    "WHITE",
    "BLACK",
    "GRAY"
};

std::string join(const std::array<uint64_t, samples.size()> &arr) {
    std::array<std::string, samples.size()> texts;

    for (size_t a = 0; a < samples.size(); a++) {
        texts[a] = fmt::format("{:>10}: {}", samples[a], arr[a]);
    }

    return fmt::format("{}", fmt::join(texts, "\n"));
}

std::string join(const std::array<double, samples.size()> &arr) {
    std::array<std::string, samples.size()> texts;

    for (size_t a = 0; a < samples.size(); a++) {
        texts[a] = fmt::format("{:>10}: %{:.1f}", samples[a], arr[a] * 100);
    }

    return fmt::format("{}", fmt::join(texts, "\n"));
}

template <typename T>
json create(const std::array<T, samples.size()> &arr) {
    json result;

    for (size_t a = 0; a < samples.size(); a++) {
        result[samples[a]] = std::to_string(arr[a]);
    }

    return result;
}

struct AnalysisResult {
    uint64_t numPixels = 0;
    std::array<uint64_t, samples.size()> sampleFrequency = { };

    std::array<double, samples.size()> normalize() const {
        std::array<double, samples.size()> result = { };

        auto normalize = [this](uint64_t i) {
            return static_cast<double>(i) / static_cast<double>(numPixels);
        };

        std::transform(sampleFrequency.begin(), sampleFrequency.end(), result.begin(), normalize);

        return result;
    }

    std::string toString() const {
        return fmt::format(
            "Pixel Count: {}\n"
            "Sample Frequency:\n{}\n"
            "Sample Normalized:\n{}\n",
            numPixels,
            join(sampleFrequency),
            join(normalize()));
    }

    json toJson() const {
        return {
            { "pixelCount", std::to_string(numPixels) },
            { "sampleFrequencies", create(sampleFrequency) },
            { "sampleNormalized", create(normalize()) }
        };
    }

    AnalysisResult() = default;
    explicit AnalysisResult(const ImageData &image) {
        numPixels = image.width * image.height;

        const RGB *colors = reinterpret_cast<RGB *>(image.data);

        for (uint64_t a = 0; a < numPixels; a++) {
            const RGB &color = colors[a];
            HSL hsl(color);

            if (hsl.lightness < 0.03) {
                sampleFrequency[6]++; // black
            } else if (hsl.lightness > 0.9) {
                sampleFrequency[7]++; // white
            } else if (hsl.saturation < 0.15) {
                sampleFrequency[8]++; // gray
            } else {
                size_t index = std::fmod(hsl.hue + 30.0, 360.0) / 60.0;

                sampleFrequency[index]++;
            }
        }
    }
};

struct AnalysisPool {
    uint64_t totalPictures = 0;

    uint64_t totalPixels = 0;
    std::array<uint64_t, samples.size()> rawFrequency = { };

    std::array<double, samples.size()> avgNormal = { };
    std::array<double, samples.size()> minNormal = { };
    std::array<double, samples.size()> maxNormal = { };
    std::array<double, samples.size()> standardDeviation = { };

    std::array<double, samples.size()> rawNormalize() const {
        std::array<double, samples.size()> result = { };

        auto normalize = [this](uint64_t i) {
            return static_cast<double>(i) / static_cast<double>(totalPixels);
        };

        std::transform(rawFrequency.begin(), rawFrequency.end(), result.begin(), normalize);

        return result;
    }

    std::string toString() const {
        return fmt::format(
            "Total Pictures: {}\n"
            "Total Pixels: {}\n"
            "Raw Frequencies:\n{}\n"
            "Raw Normal:\n{}\n"
            "Average Normal:\n{}\n"
            "Min Normal:\n{}\n"
            "Max Normal:\n{}\n"
            "Standard Deviation:\n{}\n",
            totalPictures,
            totalPixels,
            join(rawFrequency),
            join(rawNormalize()),
            join(avgNormal),
            join(minNormal),
            join(maxNormal),
            join(standardDeviation));
    }

    json toJson() const {
        return {
            { "totalPictures", std::to_string(totalPictures) },
            { "totalPixels", std::to_string(totalPixels) },
            { "rawFrequencies", create(rawFrequency) },
            { "rawNormalized", create(rawNormalize()) },
            { "avgNormalized", create(avgNormal) },
            { "minNormalized", create(minNormal) },
            { "maxNormalized", create(maxNormal) },
            { "standardDeviation", create(standardDeviation) }
        };
    }

    explicit AnalysisPool(const std::vector<AnalysisResult> &results) {
        if (results.empty())
            return;

        totalPictures = results.size();

        for (const AnalysisResult &result : results) {
            totalPixels += result.numPixels;

            auto normal = result.normalize();

            for (size_t a = 0; a < samples.size(); a++) {
                rawFrequency[a] += result.sampleFrequency[a];
                avgNormal[a] += normal[a];
            }
        }

        for (double &normal : avgNormal) {
            normal /= static_cast<double>(totalPictures);
        }

        minNormal = results.front().normalize();
        maxNormal = minNormal;

        // Second pass for min/max normals and SD
        for (const AnalysisResult &result : results) {
            auto normals = result.normalize();

            for (size_t a = 0; a < samples.size(); a++) {
                if (minNormal[a] > normals[a])
                    minNormal[a] = normals[a];

                if (maxNormal[a] < normals[a])
                    maxNormal[a] = normals[a];

                double diffMean = normals[a] - avgNormal[a];
                standardDeviation[a] += diffMean * diffMean;
            }
        }

        if (results.size() >= 2) {
            for (double &sd : standardDeviation) {
                sd = std::sqrt(sd / (totalPictures - 1.0));
            }
        }
    }
};

int main(int count, const char **args) {
    Options options(count, args);

    std::string path = options.input;
    if (!fs::exists(path)) {
        fmt::print("Could not find file or directory at \"{}\".\n", path);
        return 1;
    }

    if (fs::is_directory(path)) {
        std::vector<AnalysisResult> results;

        for (const auto &file : fs::recursive_directory_iterator(path)) {
            if (fs::is_directory(file))
                continue;

            fs::path p = file.path();
            fs::path extension = p.extension();

            if (!(extension == ".jpg" || extension == ".jpeg" || extension == ".png"))
                continue;

            if (!options.external) {
                fmt::print("Processing {}...\n", p);
            }

            ImageData data(p);
            results.emplace_back(data);
        }

        AnalysisPool pool(results);

        if (options.external) {
            fmt::print("{}\n", pool.toJson().dump(4));
        } else {
            fmt::print("\n{}\n", pool.toString());
        }
    } else {
        ImageData data(path);
        AnalysisResult result(data);

        if (options.external) {
            fmt::print("{}\n", result.toJson().dump(4));
        } else {
            fmt::print("{}\n", result.toString());
        }
    }

    return 0;
}
