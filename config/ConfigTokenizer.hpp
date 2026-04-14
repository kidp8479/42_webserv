#ifndef CONFIG_TOKENIZER_HPP
#define CONFIG_TOKENIZER_HPP

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../logger/Logger.hpp"

// note: std::pair was considered but rejected - struct gives named fields
// (.value, .line) over .first/.second, and leaves room to add fields later
struct Token {
    std::string value;
    size_t line;
};

/**
 * @brief Phase 1 of config parsing: tokenizes a .conf file into a flat token
 * list.
 *
 * Validates the file (existence, permissions, extension, emptiness) then splits
 * its content into tokens on whitespace, '{', '}', and ';'.
 * Comments starting with '#' are ignored.
 *
 * Output is a std::vector<Token> consumed by ConfigBuilder (phase 2).
 *
 * @note No syntactic or semantic validation at this stage, pure tokens only.
 * @note Copy and assignment are disabled: this class is not copyable.
 */
class ConfigTokenizer {
public:
    ConfigTokenizer(const std::string& file_path);
    ~ConfigTokenizer();

    const std::vector<Token>& getTokenList() const;

private:
    ConfigTokenizer(const ConfigTokenizer& copy);
    ConfigTokenizer& operator=(const ConfigTokenizer& other);

    void validateFile();
    void checkPathExists();
    void checkReadable();
    void checkExtension();
    void checkNotEmpty();
    void tokenize();
    void emitToken(std::string& current_word, size_t line_number);

    const std::string file_path_;
    std::vector<Token> token_list_;
};

#endif
