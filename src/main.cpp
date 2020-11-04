#include <paintings/options.h>

#include <paintings/pool.h>
#include <paintings/analysis.h>

#include <nlohmann/json.hpp>

#include <csv2/writer.hpp>

#include <curl/curl.h>

#include <fmt/printf.h>

#include <random>
#include <thread>
#include <sstream>

using nlohmann::json;

std::string concatURL(const std::string &a, const std::string &b) {
    return (a.substr(a.size() - 1, 1) == "/" && b.substr(0, 1) == "/") ? a + b.substr(1) : a + b;
}

void pushColors(std::vector<std::string> &vec) {
    for (const std::string &sample : samples) {
        vec.push_back(sample);
    }
}

template <typename T>
void pushValues(std::vector<std::string> &vec, const std::array<T, samples.size()> &values) {
    for (T value : values)
        vec.push_back(std::to_string(value));
}

void pushTable(std::vector<std::string> &vec, const char *name) {
    vec.emplace_back("");
    vec.emplace_back(name);
    pushColors(vec);
}

template <typename T>
void pushTableValues(std::vector<std::string> &vec, const std::array<T, samples.size()> &values) {
    vec.emplace_back("");
    vec.emplace_back("");
    pushValues(vec, values);
}

struct SampleContext {
    const std::vector<size_t> &ids;

    const size_t sampleSize = 0;
    const std::string baseUrl;

    std::mutex mutex;
    std::vector<size_t> samplesPicked;
    std::vector<AnalysisResult> results;

    SampleContext(const std::vector<size_t> &ids, size_t sampleSize, std::string baseUrl)
        : ids(ids), sampleSize(sampleSize), baseUrl(std::move(baseUrl)) {
        samplesPicked.reserve(sampleSize);
        results.reserve(sampleSize);
    }
};

size_t writeStream(void *buffer, size_t, size_t size, void *user) {
    auto *stream = reinterpret_cast<std::stringstream *>(user);
    stream->write(reinterpret_cast<char *>(buffer), size);

    return size;
}

std::vector<size_t> getIds(const std::string &url) {
    CURL *curl = curl_easy_init();

    std::stringstream stream;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStream);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    std::vector<size_t> data;
    json::parse(stream.str())["objectIDs"].get_to(data);

    return data;
}

void workerThread(SampleContext *context) {
    while (true) {
        size_t objectId;

        {
            std::lock_guard lock(context->mutex);

            if (context->results.size() >= context->sampleSize)
                return;

            std::random_device rd;
            std::mt19937 generator(rd());
            std::uniform_int_distribution<size_t> distribution(0, context->ids.size() - 1);

            auto begin = context->samplesPicked.begin();
            auto end = context->samplesPicked.end();

            do {
                objectId = context->ids[distribution(generator)];
            } while (std::find(begin, end, objectId) != end);
        }

        std::unique_ptr<ImageData> image;

        {
            CURL *curl = curl_easy_init();
            CURLcode error;

            std::stringstream stream;

            std::string objectUrl = concatURL(context->baseUrl, fmt::format("/objects/{}", objectId));
            curl_easy_setopt(curl, CURLOPT_URL, objectUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStream);

            error = curl_easy_perform(curl);
            if (error != CURLE_OK) {
                fmt::print("\nFailed to query object {}, resampling\n", objectUrl);
                std::cout.flush();
                continue;
            }

            std::string imageUrl;
            json::parse(stream.str())["primaryImage"].get_to(imageUrl);

            if (imageUrl.empty()) {
                fmt::print("\nFailed to find image url for query object {}, resampling\n", objectUrl);
                std::cout.flush();
                continue;
            }

            stream.str("");
            stream.clear();

            curl_easy_setopt(curl, CURLOPT_URL, imageUrl.c_str());

            error = curl_easy_perform(curl);
            if (error != CURLE_OK) {
                fmt::print("\nFailed to query image data {}, resampling", imageUrl);
                std::cout.flush();
                continue;
            }

            curl_easy_cleanup(curl);

            stream.seekg(0, std::ios::end);
            std::vector<uint8_t> data(stream.tellg());
            stream.seekg(0, std::ios::beg);
            stream.read(reinterpret_cast<char *>(data.data()), data.size());

            try {
                image = std::make_unique<ImageData>(data.data(), data.size());
            } catch (const std::runtime_error &error) {
                fmt::print("\nFailed to parse image data {}, resampling", imageUrl);
                std::cout.flush();
                continue;
            }
        }

        // Lock in the decision to use this sample now that everything has been downloaded with no problems.
        {
            std::lock_guard lock(context->mutex);

            if (context->results.size() >= context->sampleSize)
                return;

            context->samplesPicked.push_back(objectId);
        }

        AnalysisResult result(*image);

        {
            std::lock_guard lock(context->mutex);

            if (context->results.size() >= context->sampleSize)
                return;

            context->results.emplace_back(result);

            if (context->results.size() % (context->sampleSize / 10) == 0)
                std::cout << "." << std::flush; // for loading
        }
    }
}

std::vector<AnalysisResult> runSample(const Options &options, const std::vector<size_t> &ids) {
    SampleContext context(ids, options.sampleSize, options.url);

    std::vector<std::thread> threads;
    threads.reserve(options.threads);

    for (size_t b = 0; b < options.threads; b++)
        threads.emplace_back(workerThread, &context);

    for (std::thread &thread : threads)
        thread.join();

    return context.results;
}

int main(int count, const char **args) {
    try {
        Options options(count, args);

        fmt::print("Downloading IDs...\n");
        std::vector<size_t> ids = getIds(concatURL(options.url, "/search" + options.search));

        if (options.singleSample) {
            fmt::print("Starting Single Sample");

            std::vector<AnalysisResult> sample = runSample(options, ids);

            std::cout << std::endl;

            if (options.output.empty()) {
                for (size_t a = 0; a < sample.size(); a++)
                    fmt::print("# Object {}\n{}\n", a + 1, sample[a].toString());
            } else {
                fmt::print("Serializing...\n");
                std::ofstream stream(options.output);
                csv2::Writer writer(stream);

                std::vector<std::vector<std::string>> file;

                size_t size = 0;

                {
                    auto &header = file.emplace_back();
                    header.emplace_back("Object #");
                    header.emplace_back("# Pixels");

                    pushTable(header, "Frequencies");
                    pushTable(header, "Normalized");

                    size = header.size();
                }

                for (size_t a = 0; a < sample.size(); a++) {
                    const auto &result = sample[a];

                    auto &row = file.emplace_back();
                    row.reserve(size);

                    row.emplace_back(std::to_string(a + 1));
                    row.emplace_back(std::to_string(result.numPixels));

                    pushTableValues(row, result.sampleFrequency);
                    pushTableValues(row, result.normalized);
                }

                writer.write_rows(file);
            }
        } else {
            std::vector<AnalysisPool> pools;
            pools.reserve(options.sampleCount);

            for (size_t a = 0; a < options.sampleCount; a++) {
                fmt::print("Starting Sample {}", a + 1);

                pools.emplace_back(runSample(options, ids));

                std::cout << std::endl;
            }

            fmt::print("Done.\n");

            if (options.output.empty()) {
                for (size_t a = 0; a < pools.size(); a++)
                    fmt::print("# Sample {}\n{}\n", a + 1, pools[a].toString());
            } else {
                fmt::print("Serializing...\n");
                std::ofstream stream(options.output);
                csv2::Writer writer(stream);

                std::vector<std::vector<std::string>> file;

                size_t size = 0;

                {
                    auto &header = file.emplace_back();
                    header.emplace_back("Sample #");
                    header.emplace_back("# Pictures");
                    header.emplace_back("# Pixels");

                    pushTable(header, "Frequencies");
                    pushTable(header, "Raw");
                    pushTable(header, "Average");
                    pushTable(header, "Min");
                    pushTable(header, "Max");
                    pushTable(header, "S.D.");

                    size = header.size();
                }

                for (size_t a = 0; a < pools.size(); a++) {
                    const auto &pool = pools[a];

                    auto &row = file.emplace_back();
                    row.reserve(size);

                    row.emplace_back(std::to_string(a + 1));
                    row.emplace_back(std::to_string(pool.totalPictures));
                    row.emplace_back(std::to_string(pool.totalPixels));

                    pushTableValues(row, pool.rawFrequency);
                    pushTableValues(row, pool.rawNormalized);
                    pushTableValues(row, pool.avgNormal);
                    pushTableValues(row, pool.minNormal);
                    pushTableValues(row, pool.maxNormal);
                    pushTableValues(row, pool.standardDeviation);
                }

                writer.write_rows(file);
            }
        }
    } catch (const std::runtime_error &e) {
        fmt::print("ERROR: {}\n", e.what());
        return 1;
    }

    return 0;
}