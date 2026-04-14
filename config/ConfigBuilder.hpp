#ifndef CONFIG_BUILDER
#define CONFIG_BUILDER

#include <string>

#include "../logger/Logger.hpp"

class ConfigBuilder {
public:
    ConfigBuilder();
    ~ConfigBuilder();

private:
    ConfigBuilder(const ConfigBuilder& copy);
    ConfigBuilder& operator=(const ConfigBuilder& other);

    void configError(const std::string& msg) const;
};

#endif