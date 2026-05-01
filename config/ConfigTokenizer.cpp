#include "ConfigTokenizer.hpp"

/**
 * @brief Constructs a ConfigTokenizer from a config file path.
 *
 * Validates the file then tokenizes its content.
 * No syntactic or semantic validation at this stage, pure tokens only.
 *
 * @param file_path Path to the config file (argv[1] or "conf/default.conf")
 * @throws std::runtime_error If the file fails any validation check
 */
ConfigTokenizer::ConfigTokenizer(const std::string& file_path)
    : file_path_(file_path) {
    validateFile();
    tokenize();
}

/**
 * @brief Destroys the ConfigTokenizer.
 */
ConfigTokenizer::~ConfigTokenizer() {
}

/**
 * @brief Returns the list of tokens produced by tokenization.
 */
const std::vector<Token>& ConfigTokenizer::getTokenList() const {
    return tokens_list_;
}

/**
 * @brief Logs an error and throws a std::runtime_error with a "Config: "
 * prefix.
 *
 * @param msg The error message (without the "Config: " prefix)
 * @throws std::runtime_error always
 */
void ConfigTokenizer::configError(const std::string& msg) const {
    std::string full = "Config: " + msg;
    LOG_ERROR() << full;
    throw std::runtime_error(full);
}

/**
 * @brief Orchestrator for file level validation.
 *
 * Performs various checks before tokenizing the content of the file:
 * - path existence
 * - file readable
 * - file extension
 * - file emptiness
 *
 * @note Each check opens its own ifstream independently. This is intentional:
 * each method is self-contained, fails fast, and has nothing to clean up.
 * Three file opens at startup is negligible.
 *
 * @throws std::runtime_error if any of the checks fails
 */
void ConfigTokenizer::validateFile() {
    checkPathExists();
    checkReadable();
    checkExtension();
    checkNotEmpty();
}

/**
 * @brief Checks that the path exists and is not a directory.
 *
 * @throws std::runtime_error If the path does not exist or is a directory
 */
void ConfigTokenizer::checkPathExists() {
    struct stat file_info;

    if (stat(file_path_.c_str(), &file_info) == 0) {
        if (S_ISDIR(file_info.st_mode)) {
            configError(file_path_ + " is a directory");
        }
    } else {
        configError("path error: " + file_path_ + ": " + std::strerror(errno));
    }
    LOG_DEBUG() << "Config: " << file_path_ << " exists";
}

/**
 * @brief Checks that the file can be opened for reading.
 *
 * @throws std::runtime_error If the file cannot be opened
 */
void ConfigTokenizer::checkReadable() {
    std::ifstream config_file(file_path_.c_str());

    if (!config_file.is_open()) {
        configError("cannot read: " + file_path_ + ": " + std::strerror(errno));
    }
    LOG_DEBUG() << "Config: " << file_path_ << " opened successfully";
}

/**
 * @brief Checks that the filename has a valid ".conf" extension.
 *
 * Rejects hidden files (.conf), files without extension, multiple dots,
 * and any extension other than "conf" (rejects "CONF").
 *
 * @throws std::runtime_error If the extension is missing or invalid
 */
void ConfigTokenizer::checkExtension() {
    size_t slash_pos = file_path_.rfind('/');
    std::string filename = (slash_pos != std::string::npos)
                               ? file_path_.substr(slash_pos + 1)
                               : file_path_;

    if (std::count(filename.begin(), filename.end(), '.') != 1) {
        configError("filename must have exactly one dot");
    }

    std::string::size_type dot_position = filename.rfind('.');

    if (dot_position == 0 || dot_position >= filename.length() - 1) {
        configError("no valid extension found");
    }
    std::string extension = filename.substr(dot_position + 1);
    if (extension != "conf") {
        configError("wrong file extension");
    }
    LOG_DEBUG() << "Config: correct file extension";
}

/**
 * @brief Checks that the config file is not empty.
 *
 * @throws std::runtime_error If the file contains no content
 */
void ConfigTokenizer::checkNotEmpty() {
    std::ifstream config_file(file_path_.c_str());

    if (config_file.peek() == std::ifstream::traits_type::eof()) {
        configError(file_path_ + " is empty");
    }
    LOG_DEBUG() << "Config: file is not empty";
}

/**
 * @brief Tokenizes the config file into a list of tokens.
 *
 * Reads the file line by line and splits it into tokens on whitespace,
 * '{', '}', and ';'. Comments starting with '#' are ignored.
 */
void ConfigTokenizer::tokenize() {
    std::ifstream file(file_path_.c_str());
    std::string line;
    size_t line_number = 1;

    while (std::getline(file, line)) {
        std::string current_word = "";

        for (size_t i = 0; i < line.size(); i++) {
            char current_char = line[i];

            if (isspace(static_cast<unsigned char>(current_char))) {
                emitToken(current_word, line_number);
            } else if (current_char == '{' || current_char == '}' ||
                       current_char == ';') {
                emitToken(current_word, line_number);
                Token token_special_char = {std::string(1, current_char),
                                            line_number};
                tokens_list_.push_back(token_special_char);
            } else if (current_char == '#') {
                emitToken(current_word, line_number);
                break;
            } else {
                current_word += current_char;
            }
        }
        emitToken(current_word, line_number);
        line_number++;
    }

    // for (size_t i = 0; i < tokens_list_.size(); i++) {
    //     LOG_DEBUG() << "line [" << tokens_list_[i].line << "] - token[" << i
    //                 << "] = '" << tokens_list_[i].value << "' ";
    // }
    LOG_INFO() << BR_CYN "ConfigTokenizer: tokenization done - "
               << tokens_list_.size() << " tokens extracted" << RESET;
}

/**
 * @brief Emits a token from the current accumulated word.
 *
 * If current_word is non-empty, creates a Token and appends it to tokens_list_,
 * then resets current_word to "".
 *
 * @param current_word Accumulated characters since the last delimiter (reset on
 * emit)
 * @param line_number Current line number in the file
 */
void ConfigTokenizer::emitToken(std::string& current_word, size_t line_number) {
    if (!current_word.empty()) {
        Token token = {current_word, line_number};
        tokens_list_.push_back(token);
        current_word = "";
    }
}