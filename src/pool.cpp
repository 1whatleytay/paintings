#include <paintings/pool.h>

#include <fmt/format.h>

std::string AnalysisPool::toString() const {
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
        join(rawNormalized),
        join(avgNormal),
        join(minNormal),
        join(maxNormal),
        join(standardDeviation));
}

AnalysisPool::AnalysisPool(const std::vector<AnalysisResult> &results) {
    if (results.empty())
        return;

    totalPictures = results.size();

    for (const AnalysisResult &result : results) {
        totalPixels += result.numPixels;

        auto normal = result.normalized;

        for (size_t a = 0; a < samples.size(); a++) {
            rawFrequency[a] += result.sampleFrequency[a];
            avgNormal[a] += normal[a];
        }
    }

    auto normalize = [this](uint64_t i) {
        return static_cast<double>(i) / static_cast<double>(totalPixels);
    };

    std::transform(rawFrequency.begin(), rawFrequency.end(), rawNormalized.begin(), normalize);

    for (double &normal : avgNormal) {
        normal /= static_cast<double>(totalPictures);
    }

    minNormal = results.front().normalized;
    maxNormal = minNormal;

    // Second pass for min/max normals and SD
    for (const AnalysisResult &result : results) {
        auto normals = result.normalized;

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
