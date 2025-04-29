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

#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <unistd.h>      // for close(), fcntl()
#include <cerrno>        // for errno, used by strerror()
#include <cstring>       // for memset, strerror()
#include <iostream>      // for std::cerr, etc.
#include <stdexcept>

#include "./ServerSocket.hpp"
#include "HttpSocket.hpp"

using namespace std;

namespace searchserver {

ServerSocket::ServerSocket(sa_family_t family,
                           const string& address,
                           uint16_t port)
    : port_(port), listen_sock_fd_() {
  // TODO:
  // - create a stream socket
  // - set the socket option to enable re-use of the port number
  // - bind the socket to the specified address and port
  // - mark the socket as listening

  // since the address and port are given to you, you must populate the
  // address structures necessary yourself. You only need to set the
  // port, address and family field of those structs.
  // The rest can be zero'd out.
  // Only IPv6 is required; reject others
  if (family != AF_INET6) {
    throw invalid_argument("Only IPv6 is supported");
  }

  // Prepare hints
  addrinfo hints{};
  hints.ai_family = family;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;  // for wildcard address if address is empty

  // Convert port to string
  string port_str = to_string(port);
  addrinfo* res;
  int err = getaddrinfo(address.empty() ? nullptr : address.c_str(),
                        port_str.c_str(), &hints, &res);
  if (err) {
    throw runtime_error(string("getaddrinfo: ") + gai_strerror(err));
  }

  // Try each result until bind succeeds
  for (addrinfo* p = res; p; p = p->ai_next) {
    listen_sock_fd_ = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listen_sock_fd_ < 0)
      continue;

    int opt = 1;
    setsockopt(listen_sock_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(listen_sock_fd_, p->ai_addr, p->ai_addrlen) == 0) {
      // success
      break;
    }
    close(listen_sock_fd_);
    listen_sock_fd_ = -1;
  }
  freeaddrinfo(res);

  if (listen_sock_fd_ < 0) {
    throw runtime_error("Failed to bind to " + address + ":" + port_str);
  }

  // Start listening
  if (listen(listen_sock_fd_, SOMAXCONN) < 0) {
    close(listen_sock_fd_);
    throw runtime_error(string("listen: ") + strerror(errno));
  }
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

optional<HttpSocket> ServerSocket::accept_client() const {
  // TODO accept the next client connection and return it as an HttpSocket
  // object nullopt on error
  // Accept a client and wrap in HttpSocket
  struct sockaddr_storage client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_fd =
      accept(listen_sock_fd_, reinterpret_cast<struct sockaddr*>(&client_addr),
             &client_len);
  if (client_fd < 0) {
    return std::nullopt;  // error accepting
  }
  HttpSocket sock(client_fd, client_len,
                  reinterpret_cast<struct sockaddr*>(&client_addr));
  return sock;  // RVO / move
}

}  // namespace searchserver
