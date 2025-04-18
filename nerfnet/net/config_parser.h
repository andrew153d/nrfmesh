#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <unordered_map>

class ConfigParser {
public:
    // Constructor that takes the path to the config file
    explicit ConfigParser(const std::string& filePath);

    // Load and parse the configuration file
    void load();

    // Get a value from the configuration file
    std::string get(const std::string& key) const;

private:
    std::string filePath;
    std::unordered_map<std::string, std::string> config;
};

#endif // CONFIG_PARSER_H
