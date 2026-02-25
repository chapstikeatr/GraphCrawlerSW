#include <algorithm>
#include <chrono>
#include <curl/curl.h>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "rapidjson/error/error.h"
#include "rapidjson/reader.h"
#include <rapidjson/document.h>

struct ParseException : std::runtime_error, rapidjson::ParseResult {
  ParseException(rapidjson::ParseErrorCode code, const char *msg, size_t offset)
      : std::runtime_error(msg), rapidjson::ParseResult(code, offset) {}
};

#define RAPIDJSON_PARSE_ERROR_NORETURN(code, offset)                           \
  throw ParseException(code, #code, offset)

bool debug = false;

// Service URL
const std::string SERVICE_URL =
    "http://hollywood-graph-crawler.bridgesuncc.org/neighbors/";

// URL-encode parts of URLs (e.g., spaces -> %20)
static std::string url_encode(CURL *curl, const std::string &input) {
  char *out =
      curl_easy_escape(curl, input.c_str(), static_cast<int>(input.size()));
  std::string s = out ? out : "";
  if (out)
    curl_free(out);
  return s;
}

// Callback for writing response data
static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            std::string *output) {
  const size_t totalSize = size * nmemb;
  output->append(static_cast<char *>(contents), totalSize);
  return totalSize;
}

// Fetch neighbors JSON using libcurl
static std::string fetch_neighbors(CURL *curl, const std::string &node) {
  std::string url = SERVICE_URL + url_encode(curl, node);
  std::string response;

  if (debug)
    std::cout << "Sending request to: " << url << std::endl;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  // Set a User-Agent header to avoid potential blocking by the server
  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "User-Agent: C++-Client/1.0");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
  } else if (debug) {
    std::cout << "CURL request successful!" << std::endl;
  }

  curl_slist_free_all(headers);

  if (debug)
    std::cout << "Response received: " << response << std::endl;

  return (res == CURLE_OK) ? response : "{}";
}

// Parse JSON and extract neighbors
static std::vector<std::string> get_neighbors(const std::string &json_str) {
  std::vector<std::string> neighbors;
  try {
    rapidjson::Document doc;
    doc.Parse(json_str.c_str());

    if (doc.HasMember("neighbors") && doc["neighbors"].IsArray()) {
      for (const auto &neighbor : doc["neighbors"].GetArray()) {
        neighbors.push_back(neighbor.GetString());
      }
    }
  } catch (const ParseException &e) {
    std::cerr << "Error while parsing JSON: " << json_str << std::endl;
    throw;
  }
  return neighbors;
}

// Parallel, level-by-level BFS traversal
std::vector<std::vector<std::string>> bfs_parallel(const std::string &start,
                                                   int depth, int max_threads) {
  std::vector<std::vector<std::string>> levels;
  std::unordered_set<std::string> visited;
  std::mutex m;

  levels.push_back({start});
  visited.insert(start);

  for (int d = 0; d < depth; d++) {
    if (debug)
      std::cout << "starting level: " << d << "\n";

    const auto &current = levels[d];
    levels.push_back({});
    auto &next = levels[d + 1];

    if (current.empty()) {
      continue;
    }

    const int threads_to_use = std::max(
        1, std::min<int>(max_threads, static_cast<int>(current.size())));
    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(threads_to_use));

    // Evenly distribute nodes among threads using contiguous blocks
    const int n = (int)current.size();
    const int base = n / threads_to_use;
    const int rem = n % threads_to_use;

    int begin = 0;
    for (int t = 0; t < threads_to_use; t++) {
      const int extra =
          (t < rem) ? 1 : 0; // Chat GPT optimization from the use of 2 if's
      const int end = begin + base + extra;

      threads.emplace_back(
          [&, begin, end]() { // Chat GPT optimization of what was previously an
                              // external function.
            // Each thread uses its own CURL handle
            CURL *curl = curl_easy_init();
            if (!curl) {
              std::lock_guard<std::mutex> lk(m);
              std::cerr << "Failed to initialize CURL in worker thread\n";
              return;
            }

            for (int i = begin; i < end; i++) {

              const std::string &s = current[static_cast<size_t>(i)];
              try {
                if (debug) {
                  std::lock_guard<std::mutex> lk(m);
                  std::cout << "Trying to expand " << s << "\n";
                }

                for (const auto &neighbor : get_neighbors(fetch_neighbors(
                         curl, s))) { // each thread gets its neighbors
                  bool inserted = false;
                  {
                    std::lock_guard<std::mutex> lk(m);
                    auto it = visited.find(neighbor);
                    if (it == visited.end()) {
                      visited.insert(neighbor);
                      next.push_back(neighbor);
                      inserted = true;
                    }
                  }

                  if (debug && inserted) {
                    std::lock_guard<std::mutex> lk(m);
                    std::cout << "  neighbor " << neighbor << "\n";
                  }
                }
              } catch (const ParseException &) {
                std::lock_guard<std::mutex> lk(m);
                std::cerr << "Error while fetching neighbors of: " << s
                          << std::endl;
                // keep going; one bad node shouldn't kill the crawl
              }
            }

            curl_easy_cleanup(curl);
          });

      begin = end;
    }

    for (auto &th : threads) {
      th.join();
    }
  }

  return levels;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0]
              << " <node_name> <depth> <number of threads>\n";
    return 1;
  }

  std::string start_node = argv[1];
  int depth;
  try {
    depth = std::stoi(argv[2]);
  } catch (...) {
    std::cerr << "Error: Depth must be an integer.\n";
    return 1;
  }

  // libcurl global init is recommended when using curl across threads
  curl_global_init(CURL_GLOBAL_DEFAULT);

  const auto start{std::chrono::steady_clock::now()};

  const int MAX_THREADS = std::stoi(argv[3]);
  for (const auto &level : bfs_parallel(start_node, depth, MAX_THREADS)) {
    for (const auto &node : level) {
      std::cout << "- " << node << "\n";
    }
    std::cout << level.size() << "\n";
  }

  const auto finish{std::chrono::steady_clock::now()};
  const std::chrono::duration<double> elapsed_seconds{finish - start};
  std::cerr << "Time to crawl: " << elapsed_seconds.count() << "s\n";

  curl_global_cleanup();
  return 0;
}
