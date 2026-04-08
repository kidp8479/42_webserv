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
    checkPathExists();
    checkReadable();
    checkExtension();
    checkNotEmpty();
}

void ConfigTokenizer::checkPathExists() {
    struct stat dir_info;
    if (stat(file_path_.c_str(), &dir_info) == 0) {
        if (S_ISDIR(dir_info.st_mode)) {
            LOG_ERROR() << "Config: " << file_path_ << " is a directory";
            throw std::runtime_error("Config: " + file_path_ +
                                     " is a directory");
        }
    } else {
        LOG_ERROR() << "Config: " << file_path_
                    << " is invalid: " << std::strerror(errno);
        throw std::runtime_error("Config: " + file_path_ +
                                 " is invalid: " + std::strerror(errno));
    }
    LOG_DEBUG() << "Config: " << file_path_ << " exists!";
}

void ConfigTokenizer::checkReadable() {
    std::ifstream config_file(file_path_.c_str());
    if (!config_file.is_open()) {
        LOG_ERROR() << "Config: could not open " << file_path_ << ": "
                    << std::strerror(errno);
        throw std::runtime_error("Config: could not open " + file_path_ + ": " +
                                 std::strerror(errno));
    }
    LOG_DEBUG() << "Config: " << file_path_ << " opened successfully";
}

void ConfigTokenizer::checkExtension() {
    std::string::size_type dot_position = file_path_.rfind('.');

    if (dot_position == std::string::npos || dot_position == 0 ||
        dot_position >= file_path_.length() - 1) {
        LOG_ERROR() << "Config: no valid extension found";
        throw std::runtime_error("Config: no valid extension found");
    }
    std::string extension = file_path_.substr(dot_position + 1);
    if (extension != "conf") {
        LOG_ERROR() << "Config: wrong file extension";
        throw std::runtime_error("Config: wrong file extension");
    }
    LOG_DEBUG() << "Config: correct file extension";
}

void ConfigTokenizer::checkNotEmpty() {
}