// searchserver.cpp

#include <iostream>
#include <cstdlib>
#include <string>
#include "ServerSocket.hpp"
#include "HttpSocket.hpp"
#include "ThreadPool.hpp"
#include "CrawlFileTree.hpp"
#include "FileReader.hpp"

using namespace searchserver;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0]
                  << " <port> <root_directory>\n";
        return EXIT_FAILURE;
    }
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));
    std::string root = argv[2];

    // 1) Crawl & index the directory
    auto maybe_idx = crawl_filetree(root);
    if (!maybe_idx) {
        std::cerr << "Error: cannot crawl directory " << root << "\n";
        return EXIT_FAILURE;
    }
    WordIndex index = std::move(*maybe_idx);

    // 2) Start listening socket and thread pool
    ServerSocket server(AF_INET, "127.0.0.1", port);
    ThreadPool pool(4);  // adjust thread count as desired

    std::cout << "Server listening on port " << port << "...\n";

    // 3) Accept + dispatch loop
    while (true) {
        auto client_opt = server.accept_client();
        if (!client_opt) continue;

        // Move the socket into the lambda:
        pool.dispatch({[client = std::move(*client_opt), &index, root](void*) {
            HttpSocket sock = std::move(client);

            // For each request on this connection:
            while (auto req_opt = sock.next_request()) {
                const std::string& raw = *req_opt;

                // --- parse the first line ---
                // Expect "GET <path> HTTP/1.1"
                std::istringstream iss(raw);
                std::string method, uri, version;
                iss >> method >> uri >> version;

                std::string response;

                if (uri.rfind("/static/", 0) == 0) {
                    // static file request
                    std::string rel = uri.substr(8); // drop "/static/"
                    auto file = read_file(root + rel);
                    if (file) {
                        response = "HTTP/1.1 200 OK\r\n"
                                   "Content-length: " + std::to_string(file->size()) + "\r\n\r\n"
                                   + *file;
                    } else {
                        response = "HTTP/1.1 404 Not Found\r\n"
                                   "Content-length: 0\r\n\r\n";
                    }
                }
                else if (uri.rfind("/query?terms=", 0) == 0) {
                    // search query
                    std::string terms = uri.substr(13); // drop "/query?terms="
                    // split on '+'
                    auto toks = split(terms, "+");
                    // to lowercase
                    for (auto& w : toks)
                        std::transform(w.begin(), w.end(), w.begin(), ::tolower);
                    auto results = index.lookup_query(toks);

                    // build a simple HTML page
                    std::ostringstream html;
                    html << "<html><head><title>Results</title></head><body>\n";
                    html << "<ul>\n";
                    for (auto &r : results) {
                        html << "<li>" << r.doc_name
                             << " [" << r.rank << "]</li>\n";
                    }
                    html << "</ul>\n</body></html>";

                    auto body = html.str();
                    response = "HTTP/1.1 200 OK\r\n"
                               "Content-type: text/html\r\n"
                               "Content-length: " + std::to_string(body.size()) + "\r\n\r\n"
                               + body;
                }
                else {
                    // fallback: not found
                    response = "HTTP/1.1 404 Not Found\r\n"
                               "Content-length: 0\r\n\r\n";
                }

                sock.write_response(response);

                // check for "Connection: close"
                if (raw.find("Connection: close") != std::string::npos) {
                    break;
                }
            }
        }, nullptr});
    }

    return EXIT_SUCCESS;
}
