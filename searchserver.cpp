#ifndef SEARCHSERVER_FILEREADER_HPP
#define SEARCHSERVER_FILEREADER_HPP

#include <string>
#include <optional>

namespace searchserver {

/**
 * @brief Reads the entire contents of a file into a string.
 *
 * Opens the file at @p path in binary mode and returns its bytes as a std::string.
 *
 * @param path  Path to the file to read.
 * @return      On success, the file’s contents; on failure, std::nullopt.
 */
std::optional<std::string> read_file(const std::string& path);

}  // namespace searchserver

#endif  // SEARCHSERVER_FILEREADER_HPP
