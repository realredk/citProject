#include <algorithm>
#include <cstdlib>
#include <cstring>  // for strlen()
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "CrawlFileTree.hpp"
#include "FileReader.hpp"
#include "HttpSocket.hpp"
#include "HttpUtils.hpp"
#include "ServerSocket.hpp"
#include "ThreadPool.hpp"
#include "WordIndex.hpp"

using namespace searchserver;
using std::cerr;
using std::cout;
using std::endl;
using std::ostringstream;
using std::string;
using std::vector;

/**
 * @brief Per-connection data for the threadpool.
 */
struct TaskData {
  HttpSocket client;
  WordIndex* index;
  string root;
};

/**
 * @brief Worker function: handles all requests on one HttpSocket.
 */
static void handle_client(void* arg) {
  auto* d = static_cast<TaskData*>(arg);
  HttpSocket sock = std::move(d->client);
  WordIndex* idx = d->index;
  std::string root = std::move(d->root);
  delete d;

  // helper: redirect "/" to index.html
  auto respond_root = []() {
    return std::string(
        "HTTP/1.1 302 Found\r\n"
        "Location: /static/index.html\r\n\r\n");
  };

  // helper: serve static file or 404
  auto respond_static = [&](std::string_view uri) {
    std::string rel = std::string(uri.substr(8));
    auto blob = read_file(root + "/" + rel);
    if (!blob) {
      return std::string("HTTP/1.1 404 Not Found\r\nContent-length: 0\r\n\r\n");
    }
    auto& b = *blob;
    std::ostringstream hdr;
    hdr << "HTTP/1.1 200 OK\r\n"
        << "Content-type: text/plain\r\n"
        << "Content-length: " << b.size() << "\r\n\r\n"
        << b;
    return hdr.str();
  };

  // helper: strip leading/trailing non-alnum
  auto strip_punct = [](std::string& s) {
    std::size_t start = 0;
    std::size_t end = s.size();
    while (start < end &&
           std::isalnum(static_cast<unsigned char>(s[start])) == 0) {
      ++start;
    }
    while (end > start &&
           std::isalnum(static_cast<unsigned char>(s[end - 1])) == 0) {
      --end;
    }
    s = s.substr(start, end - start);
  };

  // helper: handle query
  auto respond_query = [&](std::string_view uri) {
    std::string terms(uri.substr(13));
    auto toks = split(terms, "+");
    for (auto& w : toks) {
      std::transform(w.begin(), w.end(), w.begin(), ::tolower);
      strip_punct(w);
    }
    toks.erase(std::remove_if(toks.begin(), toks.end(),
                              [](auto const& x) { return x.empty(); }),
               toks.end());

    auto results = idx->lookup_query(toks);
    std::ostringstream body;
    body << "<html><head><title>Results</title></head><body>\n<ul>\n";
    for (auto const& r : results) {
      body << "<li>" << r.doc_name << " [" << r.rank << "]</li>\n";
    }
    body << "</ul>\n</body></html>\n";

    auto b = body.str();
    std::ostringstream hdr;
    hdr << "HTTP/1.1 200 OK\r\n"
        << "Content-type: text/html\r\n"
        << "Content-length: " << b.size() << "\r\n\r\n"
        << b;
    return hdr.str();
  };

  while (true) {
    auto req_opt = sock.next_request();
    if (!req_opt)
      break;
    const auto raw = *req_opt;

    std::istringstream reqs(raw);
    std::string method;
    std::string uri;
    std::string version;
    if (!(reqs >> method >> uri >> version))
      break;

    std::string response;
    if (uri == "/") {
      response = respond_root();
    } else if (uri.rfind("/static/", 0) == 0) {
      response = respond_static(uri);
    } else if (uri.rfind("/query?terms=", 0) == 0) {
      response = respond_query(uri);
    } else {
      response = "HTTP/1.1 404 Not Found\r\nContent-length: 0\r\n\r\n";
    }

    if (!sock.write_response(response))
      break;
    if (raw.find("Connection: close") != std::string::npos ||
        raw.find("connection: close") != std::string::npos) {
      break;
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    cerr << "Usage: " << argv[0] << " <port> <root_dir>\n";
    return EXIT_FAILURE;
  }
  uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));
  string root = argv[2];

  // Build the index
  auto idx_opt = crawl_filetree(root);
  if (!idx_opt) {
    cerr << "Error: cannot crawl directory " << root << "\n";
    return EXIT_FAILURE;
  }
  WordIndex index = std::move(*idx_opt);

  // Listen on localhost
  ServerSocket server(AF_INET, "127.0.0.1", port);
  cout << "Listening on 127.0.0.1:" << port << " …\n";

  // Thread pool with 4 workers
  ThreadPool pool(4);

  // Accept loop
  while (true) {
    auto client_opt = server.accept_client();
    if (!client_opt)
      continue;
    auto* data = new TaskData{std::move(*client_opt), &index, root};
    pool.dispatch({handle_client, data});
  }

  return EXIT_SUCCESS;
}
