#pragma once

#include <paintings/analysis.h>

#include <array>
#include <vector>

struct AnalysisPool {
    uint64_t totalPictures = 0;

    uint64_t totalPixels = 0;
    std::array<uint64_t, samples.size()> rawFrequency = { };
    std::array<double, samples.size()> rawNormalized = { };

    std::array<double, samples.size()> avgNormal = { };
    std::array<double, samples.size()> minNormal = { };
    std::array<double, samples.size()> maxNormal = { };
    std::array<double, samples.size()> standardDeviation = { };

    std::string toString() const;

    explicit AnalysisPool(const std::vector<AnalysisResult> &results);
};
