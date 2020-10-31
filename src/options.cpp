#include <paintings/options.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>

Options::Options(int count, const char **args) {
    CLI::App app("Hue analyzer for images.");

    app.add_option("-u,--url", url, "Base URL for MET API.");
    app.add_option("-s,--search", search, "Postfix for search query.");
    app.add_option("-t,--threads", threads, "Number of threads per sample.");
    app.add_option("-n,--sample-size", sampleSize, "Size of each sample.");
    app.add_option("-c,--sample-count", sampleCount, "Number of samples to be made.");
    app.add_option("-o,--output", output, "Optional output CSV file.");

    try {
        app.parse(count, args);
    } catch (const CLI::ParseError &e) {
        throw std::runtime_error(e.what());
    }
}
