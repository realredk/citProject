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
  string root = std::move(d->root);
  delete d;

  while (true) {
    auto req_opt = sock.next_request();
    if (!req_opt)
      break;
    const string raw = *req_opt;

    // Parse request line: "GET <uri> HTTP/1.1"
    std::istringstream reqs(raw);
    string method;
    string uri;
    string version;

    if (!(reqs >> method >> uri >> version))
      break;

    string response;

    if (uri == "/") {
      response =
          "HTTP/1.1 302 Found\r\n"
          "Location: /static/index.html\r\n\r\n";
    }
    // 1) Static‐file requests: /static/...
    else if (uri.rfind("/static/", 0) == 0) {
      string rel = uri.substr(8);
      auto blob = read_file(root + "/" + rel);
      if (blob) {
        auto& b = *blob;
        ostringstream hdr;
        hdr << "HTTP/1.1 200 OK\r\n"
            << "Content-type: text/plain\r\n"
            << "Content-length: " << b.size() << "\r\n\r\n"
            << b;
        response = hdr.str();
      } else {
        response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-length: 0\r\n\r\n";
      }

      // 2) Search queries: /query?terms=foo+bar
    } else if (uri.rfind("/query?terms=", 0) == 0) {
      string terms = uri.substr(13);
      auto toks = split(terms, "+");

      // strip non-alphanumeric from ends
      auto strip_punct = [](string& s) {
        size_t start = 0, end = s.size();
        while (start < end &&
               !std::isalnum(static_cast<unsigned char>(s[start])))
          ++start;
        while (end > start &&
               !std::isalnum(static_cast<unsigned char>(s[end - 1])))
          --end;
        s = s.substr(start, end - start);
      };

      for (auto& w : toks) {
        std::transform(w.begin(), w.end(), w.begin(), ::tolower);
        strip_punct(w);
      }
      toks.erase(std::remove_if(toks.begin(), toks.end(),
                                [](const string& s) { return s.empty(); }),
                 toks.end());

      auto results = idx->lookup_query(toks);

      ostringstream body;
      body << "<html><head><title>Results</title></head><body>\n"
           << "<ul>\n";
      for (auto& r : results) {
        body << "<li>" << r.doc_name << " [" << r.rank << "]</li>\n";
      }
      body << "</ul>\n</body></html>\n";

      auto b = body.str();
      ostringstream hdr;
      hdr << "HTTP/1.1 200 OK\r\n"
          << "Content-type: text/html\r\n"
          << "Content-length: " << b.size() << "\r\n\r\n"
          << b;
      response = hdr.str();

      // 3) Anything else: 404
    } else {
      response =
          "HTTP/1.1 404 Not Found\r\n"
          "Content-length: 0\r\n\r\n";
    }

    // Send it
    if (!sock.write_response(response))
      break;

    // Honor Connection: close
    if (raw.find("Connection: close") != string::npos ||
        raw.find("connection: close") != string::npos) {
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
