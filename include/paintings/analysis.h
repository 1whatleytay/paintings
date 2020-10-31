#pragma once

#include <paintings/image.h>

#include <array>

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

std::string join(const std::array<uint64_t, samples.size()> &arr);
std::string join(const std::array<double, samples.size()> &arr);

struct AnalysisResult {
    uint64_t numPixels = 0;
    std::array<uint64_t, samples.size()> sampleFrequency = { };
    std::array<double, samples.size()> normalized = { };

    std::string toString() const;

    AnalysisResult() = default;
    explicit AnalysisResult(const ImageData &image);
};
