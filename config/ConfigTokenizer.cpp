#include "ConfigTokenizer.hpp"

ConfigTokenizer::ConfigTokenizer(const std::string& file_path)
    : file_path_(file_path) {
    validateFile();
    // tokenize();
}

ConfigTokenizer::~ConfigTokenizer() {
}

const std::vector<Token>& ConfigTokenizer::getTokenList() const {
    return this->token_list_;
}

void ConfigTokenizer::validateFile() {
    std::ifstream config_file(file_path_.c_str());
    if (config_file.is_open()) {
        std::cout << "Config: file opened successfully!" << std::endl;
    } else {
        throw std::runtime_error("Config: could not open config file");
    }
}