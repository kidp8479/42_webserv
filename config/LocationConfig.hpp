#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <map>
#include <string>
#include <vector>

/**
 * @brief Holds the parsed content of one location { } block.
 *
 * Pure data container, no validation logic.
 *
 * @note return_code_ uses kNoRedirect (-1) as a sentinel. To use:
 * if return_code_ != kNoRedirect, send a redirect response with return_code_
 * and return_url_. If return_code_ == kNoRedirect, no return directive was
 * set, serve normally.
 */
class LocationConfig {
public:
    static const int kNoRedirect = -1;

    LocationConfig();
    LocationConfig(const LocationConfig& copy);
    LocationConfig& operator=(const LocationConfig& other);
    ~LocationConfig();

    const std::string& getPath() const;
    const std::vector<std::string>& getMethods() const;
    const std::string& getRoot() const;
    const std::string& getIndex() const;
    bool getDirectoryListing() const;
    const std::string& getUploadPath() const;
    const std::map<std::string, std::string>& getCgiInterpreters() const;
    int getReturnCode() const;
    const std::string& getReturnUrl() const;

    void setPath(const std::string& path);
    void setMethods(const std::vector<std::string>& methods);
    void setRoot(const std::string& root);
    void setIndex(const std::string& index);
    void setDirectoryListing(bool directory_listing);
    void setUploadPath(const std::string& upload_path);
    void addCgiInterpreter(const std::string& ext, const std::string& binary);
    void setReturnCode(int return_code);
    void setReturnUrl(const std::string& return_url);

private:
    std::string path_;
    std::vector<std::string> methods_;
    std::string root_;
    std::string index_;
    bool directory_listing_;
    std::string upload_path_;

    std::map<std::string, std::string> cgi_interpreters_;
    int return_code_;  // kNoRedirect (-1) if no return directive is set
    std::string return_url_;
};

#endif
