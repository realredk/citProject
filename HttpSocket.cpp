/*
 * Copyright ©2025 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 5950 for use solely during Spring Semester 2025 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <arpa/inet.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>

#include "./HttpSocket.hpp"
#include "./HttpUtils.hpp"

using std::array;
using std::map;
using std::nullopt;
using std::optional;
using std::string;
using std::vector;

namespace searchserver {

static const char* const kHeaderEnd = "\r\n\r\n";
static const int kHeaderEndLen = 4;

// Read next HTTP request header, return it (incl. "\r\n\r\n");
// return nullopt if connection closed or on error.
optional<string> HttpSocket::next_request() {
  // Use "wrapped_read" to read data into the buffer_
  // instance variable.  Keep reading data until either the
  // connection drops or you see a "\r\n\r\n" that demarcates
  // the end of the request header. Be sure to try and read in
  // a large amount of bytes each time you call wrapped_read.
  //
  // Once you've seen the request header, use parse_request()
  // to parse the header into the *request argument.
  //
  // Very tricky part:  clients can send back-to-back requests
  // on the same socket.  So, you need to preserve everything
  // after the "\r\n\r\n" in buffer_ for the next time the
  // caller invokes next_request()!

  while (true) {
    // Check if we already have full header
    auto pos = buffer_.find(kHeaderEnd);
    if (pos != string::npos) {
      size_t end = pos + kHeaderEndLen;
      string req = buffer_.substr(0, end);
      buffer_.erase(0, end);
      return req;
    }
    // Read more data into buffer_
    ssize_t n = wrapped_read(fd_, &buffer_);
    if (n <= 0) {
      return nullopt;
    }
  }
}

// Write the entire response via wrapped_write; return false on error.
bool HttpSocket::write_response(const string& response) const {
  ssize_t n = wrapped_write(fd_, response);
  return n >= 0;
}

// Below functions are given to you
// they just get some information about the connection.
string HttpSocket::client_addr() const {
  array<char, INET6_ADDRSTRLEN> addrbuf{};
  if (addr_.ss_family == AF_INET) {
    inet_ntop(AF_INET,
              &(reinterpret_cast<const struct sockaddr_in*>(&addr_)->sin_addr),
              addrbuf.data(), INET_ADDRSTRLEN);
  } else {
    inet_ntop(
        AF_INET6,
        &(reinterpret_cast<const struct sockaddr_in6*>(&addr_)->sin6_addr),
        addrbuf.data(), INET6_ADDRSTRLEN);
  }
  return {addrbuf.data()};
}

uint16_t HttpSocket::client_port() const {
  if (addr_.ss_family == AF_INET) {
    return ntohs(reinterpret_cast<const struct sockaddr_in*>(&addr_)->sin_port);
  }
  return ntohs(reinterpret_cast<const struct sockaddr_in6*>(&addr_)->sin6_port);
}

string HttpSocket::server_addr() const {
  struct sockaddr_storage srvr{};
  socklen_t srvrlen = sizeof(srvr);
  getsockname(fd_, reinterpret_cast<struct sockaddr*>(&srvr), &srvrlen);

  array<char, INET6_ADDRSTRLEN> addrbuf{};
  if (srvr.ss_family == AF_INET) {
    inet_ntop(AF_INET,
              &(reinterpret_cast<const struct sockaddr_in*>(&srvr)->sin_addr),
              addrbuf.data(), INET_ADDRSTRLEN);
  } else {
    inet_ntop(AF_INET6,
              &(reinterpret_cast<const struct sockaddr_in6*>(&srvr)->sin6_addr),
              addrbuf.data(), INET6_ADDRSTRLEN);
  }
  return {addrbuf.data()};
}

uint16_t HttpSocket::server_port() const {
  struct sockaddr_storage srvr{};
  socklen_t srvrlen = sizeof(srvr);
  getsockname(fd_, reinterpret_cast<struct sockaddr*>(&srvr), &srvrlen);
  if (srvr.ss_family == AF_INET) {
    return ntohs(reinterpret_cast<const struct sockaddr_in*>(&srvr)->sin_port);
  }
  return ntohs(reinterpret_cast<const struct sockaddr_in6*>(&srvr)->sin6_port);
}

}  // namespace searchserver
