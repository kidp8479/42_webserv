#ifndef CONFIG_TOKENIZER_HPP
#define CONFIG_TOKENIZER_HPP

#include <sys/stat.h>
#include <sys/types.h>

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
private:
    std::vector<Token> token_list_;
    const std::string file_path_;

    ConfigTokenizer(const ConfigTokenizer& copy);
    ConfigTokenizer& operator=(const ConfigTokenizer& other);

    void validateFile();
    void checkPathExists();
    void checkReadable();
    void checkExtension();
    void checkNotEmpty();

    // void tokenize();

public:
    ConfigTokenizer(const std::string& file_path);
    ~ConfigTokenizer();
    // getter
    /* const before the return type so that no one can .push_back() anything on
     * my precious vector */
    const std::vector<Token>& getTokenList() const;
};

#endif