#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include <string>
#include <vector>

#define NO_REDIRECT -1

class LocationConfig {
private:
    std::string path_;
    std::vector<std::string> methods_;
    std::string root_;
    std::string index_;
    bool directory_listing_;
    std::string upload_path_;
    std::vector<std::string> cgi_extensions_;
    int return_code_;  // NO_REDIRECT (-1) if no return directive is set
    std::string return_url_;

public:
    LocationConfig();
    LocationConfig(const LocationConfig& copy);
    LocationConfig& operator=(const LocationConfig& other);
    ~LocationConfig();

    // getters
    const std::string& getPath() const;
    const std::vector<std::string>& getMethods() const;
    const std::string& getRoot() const;
    const std::string& getIndex() const;
    bool getDirectoryListing() const;
    const std::string& getUploadPath() const;
    const std::vector<std::string>& getCgiExtensions() const;
    int getReturnCode() const;
    const std::string& getReturnUrl() const;

    // setters
    void setPath(const std::string& path);
    void setMethods(const std::vector<std::string>& methods);
    void setRoot(const std::string& root);
    void setIndex(const std::string& index);
    void setDirectoryListing(bool directory_listing);
    void setUploadPath(const std::string& upload_path);
    void setCgiExtensions(const std::vector<std::string>& cgi_extensions);
    void setReturnCode(int return_code);
    void setReturnUrl(const std::string& return_url);
};

#endif