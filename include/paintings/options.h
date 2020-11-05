#pragma once

#include <string>

struct Options {
    std::string url = "https://collectionapi.metmuseum.org/public/collection/v1/";
    std::string search = "?hasImages=true&material=Paintings&q=*";
    size_t threads = 2;
    
    size_t sampleSize = 10;
    size_t sampleCount = 10;

    bool raw = false;

    std::string output;

    Options(int count, const char **args);
};
