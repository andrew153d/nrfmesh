#include "config_parser.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

ConfigParser::ConfigParser(const std::string& filePath) : filePath(filePath) {}

void ConfigParser::load() {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + filePath);
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream lineStream(line);
        std::string key, value;
        if (std::getline(lineStream, key, '=') && std::getline(lineStream, value)) {
            config[key] = value;
        }
    }
}

std::string ConfigParser::get(const std::string& key) const {
    auto it = config.find(key);
    if (it != config.end()) {
        return it->second;
    }
    throw std::runtime_error("Key not found in config: " + key);
}
