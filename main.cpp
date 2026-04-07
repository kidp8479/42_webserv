#include "config/ConfigTokenizer.hpp"

int main(int argc, char** argv) {
    std::string path = (argc > 1) ? argv[1] : "conf/default.conf";

    try {
        ConfigTokenizer tokenizer(path);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
