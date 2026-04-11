#ifndef CONFIG_TOKENIZER_HPP
#define CONFIG_TOKENIZER_HPP

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../logger/Logger.hpp"

struct Token {
    std::string value;
    int line;
};

class ConfigTokenizer {
public:
    ConfigTokenizer(const std::string& file_path);
    ~ConfigTokenizer();

    const std::vector<Token>& getTokenList() const;

private:
    const std::string file_path_;
    std::vector<Token> token_list_;

    ConfigTokenizer(const ConfigTokenizer& copy);
    ConfigTokenizer& operator=(const ConfigTokenizer& other);

    void validateFile();
    void checkPathExists();
    void checkReadable();
    void checkExtension();
    void checkNotEmpty();
    void tokenize();
};

#endif
