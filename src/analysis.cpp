#include <paintings/analysis.h>

#include <paintings/colors.h>

#include <fmt/format.h>

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

std::string AnalysisResult::toString() const {
    return fmt::format(
        "Pixel Count: {}\n"
        "Sample Frequency:\n{}\n"
        "Sample Normalized:\n{}\n",
        numPixels,
        join(sampleFrequency),
        join(normalized));
}

AnalysisResult::AnalysisResult(const ImageData &image) {
    numPixels = image.width * image.height;

    const RGB *colors = reinterpret_cast<RGB *>(image.data);

    for (uint64_t a = 0; a < numPixels; a++) {
        const RGB &color = colors[a];
        HSL hsl(color);

        sampleFrequency[hsl.classify()]++;

//        if (hsl.lightness < 0.03) {
//            sampleFrequency[6]++; // black
//        } else if (hsl.lightness > 0.9) {
//            sampleFrequency[7]++; // white
//        } else if (hsl.saturation < 0.15) {
//            sampleFrequency[8]++; // gray
//        } else {
//            size_t index = std::fmod(hsl.hue + 30.0, 360.0) / 60.0;
//
//            sampleFrequency[index]++;
//        }
    }

    auto normalize = [this](uint64_t i) {
        return static_cast<double>(i) / static_cast<double>(numPixels);
    };

    std::transform(sampleFrequency.begin(), sampleFrequency.end(), normalized.begin(), normalize);
}
