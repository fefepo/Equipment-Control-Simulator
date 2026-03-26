#include "Config.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <cctype>

namespace {
    std::string trim(const std::string& s) {
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
            ++start;
        }

        size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
            --end;
        }

        return s.substr(start, end - start);
    }

    bool extractValue(const std::string& json, const std::string& key, std::string& outValue) {
        std::string pattern = "\"" + key + "\"";
        size_t keyPos = json.find(pattern);
        if (keyPos == std::string::npos) {
            return false;
        }

        size_t colonPos = json.find(':', keyPos);
        if (colonPos == std::string::npos) {
            return false;
        }

        size_t valueStart = colonPos + 1;
        while (valueStart < json.size() &&
            std::isspace(static_cast<unsigned char>(json[valueStart]))) {
            ++valueStart;
        }

        size_t valueEnd = valueStart;

        if (valueStart < json.size() && json[valueStart] == '"') {
            ++valueStart;
            valueEnd = json.find('"', valueStart);
            if (valueEnd == std::string::npos) {
                return false;
            }
            outValue = json.substr(valueStart, valueEnd - valueStart);
            return true;
        }

        while (valueEnd < json.size() &&
            json[valueEnd] != ',' &&
            json[valueEnd] != '}' &&
            json[valueEnd] != '\n' &&
            json[valueEnd] != '\r') {
            ++valueEnd;
        }

        outValue = trim(json.substr(valueStart, valueEnd - valueStart));
        return !outValue.empty();
    }
}

bool Config::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    std::string value;

    if (extractValue(json, "port", value)) {
        port = std::stoi(value);
    }

    if (extractValue(json, "tempThreshold", value)) {
        tempThreshold = std::stod(value);
    }

    if (extractValue(json, "pressureThreshold", value)) {
        pressureThreshold = std::stod(value);
    }

    if (extractValue(json, "updateIntervalMs", value)) {
        updateIntervalMs = std::stoi(value);
    }

    return true;
}