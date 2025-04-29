#include "FileReader.hpp"
#include <fstream>
#include <sstream>

namespace searchserver {

std::optional<std::string> read_file(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        return std::nullopt;
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

}  // namespace searchserver
