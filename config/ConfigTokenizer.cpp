#include "ConfigTokenizer.hpp"

// no real default constructor because I'm not creating "useless surface"
// where a default constructor has no sense here
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

// this function orchestrate the file validation
void ConfigTokenizer::validateFile() {
    checkPathExists();
    checkReadable();
    checkExtension();
    checkNotEmpty();
}

// this function check for 1) path exists 2) if it exists, is not a directory
// tests to do :
// [FAIL] => path doesn't exist (stat returns -1)
// [FAIL] => path exists but is a directory (S_ISDIR is true)
// [PASS] => path exists and is a file
void ConfigTokenizer::checkPathExists() {
    struct stat file_info;

    if (stat(file_path_.c_str(), &file_info) == 0) {
        if (S_ISDIR(file_info.st_mode)) {
            LOG_ERROR() << "Config: path error:" << file_path_
                        << " is a directory";
            throw std::runtime_error("Config: " + file_path_ +
                                     " is a directory");
        }
    } else {
        LOG_ERROR() << "Config: path error: " << file_path_ << ": "
                    << std::strerror(errno);
        throw std::runtime_error("Config: path error: " + file_path_ + ": " +
                                 std::strerror(errno));
    }
    LOG_DEBUG() << "Config: " << file_path_ << " exists";
}

// this function checks file permission and openability
// tests to do :
// [FAIL] => is_open() returns false (it means either the perms are wrong or the
// file did not open, errno will tell) [PASS] => file opened
void ConfigTokenizer::checkReadable() {
    std::ifstream config_file(file_path_.c_str());

    if (!config_file.is_open()) {
        LOG_ERROR() << "Config: cannot read: " << file_path_ << ": "
                    << std::strerror(errno);
        throw std::runtime_error("Config: cannot read: " + file_path_ + ": " +
                                 std::strerror(errno));
    }
    LOG_DEBUG() << "Config: " << file_path_ << " opened successfully";
}

// this function handle checks for file extension
// [FAIL] => multiple dots in filename (ex: test.py.conf, conf.conf.conf)
// [FAIL] => no "." in name
// [FAIL] => "." is the first char (hidden file like .conf)
// [FAIL] => "." is in last position, nothing after (ex: file.)
// [FAIL] => extension after dot is not "conf"
// [PASS] => extension is ".conf" at the right place
void ConfigTokenizer::checkExtension() {
    std::string::size_type slash_pos = file_path_.rfind('/');
    std::string filename = (slash_pos != std::string::npos)
                               ? file_path_.substr(slash_pos + 1)
                               : file_path_;

    if (std::count(filename.begin(), filename.end(), '.') != 1) {
        LOG_ERROR() << "Config: filename must have exactly one dot";
        throw std::runtime_error("Config: filename must have exactly one dot");
    }

    std::string::size_type dot_position = filename.rfind('.');

    if (dot_position == 0 || dot_position >= filename.length() - 1) {
        LOG_ERROR() << "Config: no valid extension found";
        throw std::runtime_error("Config: no valid extension found");
    }
    std::string extension = filename.substr(dot_position + 1);
    if (extension != "conf") {
        LOG_ERROR() << "Config: wrong file extension";
        throw std::runtime_error("Config: wrong file extension");
    }
    LOG_DEBUG() << "Config: correct file extension";
}

// this function checks for non emptiness of the config file
// [FAIL] => file is empty (peek() returns EOF immediately)
// [PASS] => peek() returns a valid character, file has content
void ConfigTokenizer::checkNotEmpty() {
    std::ifstream config_file(file_path_.c_str());

    if (config_file.peek() == std::ifstream::traits_type::eof()) {
        LOG_ERROR() << "Config: " << file_path_ << " is empty";
        throw std::runtime_error("Config: " + file_path_ + " is empty");
    }
    LOG_DEBUG() << "Config: file is not empty";
}