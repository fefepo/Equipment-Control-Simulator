#pragma once

#include <string>

class Config {
public:
    int port = 5000;
    double tempThreshold = 80.0;
    double pressureThreshold = 3.5;
    int updateIntervalMs = 100;

    bool load(const std::string& filename);
};