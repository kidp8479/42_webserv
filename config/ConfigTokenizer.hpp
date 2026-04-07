#ifndef CONFIG_TOKENIZER_HPP
#define CONFIG_TOKENIZER_HPP

#include <string>
#include <vector>

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
    void tokenize();

public:
    /* no real default constructor because I'm not creating "useless surface"
     * where a default constructor has no sense here */
    ConfigTokenizer(const std::string& file_path);
    ~ConfigTokenizer();
    // getter
    /* const before the return type so that no one can .push_back() anything on
     * my precious vector */
    const std::vector<Token>& getTokenList() const;
};

#endif