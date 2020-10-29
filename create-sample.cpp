#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>

#include <csv2/reader.hpp>

#include <fmt/format.h>

#include <unordered_map>

struct Options {
    using HeaderFilter = std::tuple<std::string, std::string>;
    using RangeFilter = std::tuple<std::string, size_t, size_t>;

    std::string csvFile;
    std::string output;
    std::vector<HeaderFilter> equalFilters;
    std::vector<RangeFilter> rangeFilters;

    Options(int count, const char **args) {
        CLI::App app("Sampler for MET database.");

        app.add_option("-i", csvFile, "Input CSV database file for MET.");
        app.add_option("-o", output, "Output subsample file for future processing.");
        app.add_option("-e", equalFilters, "Find entries that match this header exactly.");
        app.add_option("-r", rangeFilters, "Find entries that are within a numerical range of this header.");

        app.parse(count, args);
    }
};

bool isNumber(const std::string &value) {
    return !std::any_of(value.begin(), value.end(), [](char c) { return !std::isdigit(c); });
}

int main(int count, const char **args) {
    Options options(count, args);

    if (options.csvFile.empty()) {
        fmt::print("Error, missing file input.\n");
        return 1;
    }

    csv2::Reader reader;

    if (!reader.mmap(options.csvFile)) {
        fmt::print("Error, file is invalid.\n");
        return 1;
    }

    const auto header = reader.header();

    size_t cols = reader.cols();
    size_t rows = reader.rows();

    std::vector<std::vector<std::string>> allowedValues(cols);
    std::vector<std::vector<std::pair<size_t, size_t>>> allowedRanges(cols);

    {
        size_t headerIndex = 0;
        for (auto cell : header) {
            std::string value;
            cell.read_value(value);

            for (const auto &e : options.equalFilters) {
                if (std::get<0>(e) == value) {
                    allowedValues[headerIndex].push_back(std::get<1>(e));
                }
            }

            for (const auto &e : options.rangeFilters) {
                if (std::get<0>(e) == value) {
                    allowedRanges[headerIndex].push_back(std::make_pair(std::get<1>(e), std::get<2>(e)));
                }
            }

            headerIndex++;
        }
    }

    std::cout << "Loading [" << std::flush;

    std::vector<std::string> objects;

    size_t rowNumber = 0;
    for (auto row : reader) {
        size_t index = 0;

        bool success = false;

        std::string objectId;

        for (auto cell : row) {
            std::string value;
            cell.read_value(value);

            if (index == 0)
                objectId = value;

            const auto &allowed = allowedValues[index];
            const auto &ranges = allowedRanges[index];

            if (!allowed.empty()) {
                auto v = std::find(allowed.begin(), allowed.end(), value);

                success = v != allowed.end();

                if (!success)
                    break;
            }

            if (!ranges.empty()) {
                if (value.empty() || !isNumber(value)) {
                    success = false;
                } else {
                    size_t num = std::stoull(value);

                    success = true;

                    for (const auto &r : ranges) {
                        if (num < r.first || num > r.second) {
                            success = false;
                            break;
                        }
                    }

                    if (!success)
                        break;
                }
            }

            index++;
        }

        if (success)
            objects.push_back(objectId);

        rowNumber++;

        if (rowNumber % (rows / 40) == 0)
            std::cout << "#" << std::flush;
    }

    std::cout << "#]" << std::endl;

    fmt::print("Works found: {}\n", objects.size());

    if (!options.output.empty()) {
        std::ofstream stream(options.output);

        if (!stream.is_open()) {
            fmt::print("Failed to write to \"{}\".\n", options.output);
            return 0;
        }

        for (const std::string &object : objects) {
            stream << object << "\n";
        }
    }

    return 0;
}
