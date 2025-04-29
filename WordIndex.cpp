#include "./WordIndex.hpp"

#include <algorithm>

namespace searchserver {

WordIndex::WordIndex() {}

size_t WordIndex::num_words() {
  // return count of unique words in the index
  return index_.size();
}

void WordIndex::record(const string& word, const string& doc_name) {
  // increment occurrence count for word in given document
  index_[word][doc_name]++;
}

vector<Result> WordIndex::lookup_word(const string& word) {
  vector<Result> results;
  auto it = index_.find(word);
  // if word not found, return empty list
  if (it == index_.end())
    return results;

  // collect doc-name and count pairs into results
  for (auto& [doc, count] : it->second) {
    results.emplace_back(doc, count);
  }
  // sort by descending count, then ascending doc name
  std::sort(results.begin(), results.end(), [](auto& a, auto& b) {
    if (a.rank != b.rank)
      return a.rank > b.rank;
    return a.doc_name < b.doc_name;
  });
  return results;
}

vector<Result> WordIndex::lookup_query(const vector<string>& query) {
  if (query.empty())
    return {};

  // start with posting list for first word
  auto first_it = index_.find(query[0]);
  if (first_it == index_.end())
    return {};

  // doc -> accumulated count
  std::unordered_map<std::string, size_t> doc_counts = first_it->second;

  // for each additional word, intersect and sum counts
  for (size_t i = 1; i < query.size(); i++) {
    auto it = index_.find(query[i]);
    if (it == index_.end())
      return {};
    std::unordered_map<std::string, size_t> next_counts;
    for (auto& [doc, cnt] : it->second) {
      auto it2 = doc_counts.find(doc);
      if (it2 != doc_counts.end()) {
        next_counts[doc] = it2->second + cnt;
      }
    }
    doc_counts.swap(next_counts);
    if (doc_counts.empty())
      return {};
  }

  // convert to vector<Result> and sort same as single-word lookup
  vector<Result> results;
  results.reserve(doc_counts.size());
  for (auto& [doc, cnt] : doc_counts) {
    results.emplace_back(doc, cnt);
  }
  std::sort(results.begin(), results.end(), [](auto& a, auto& b) {
    if (a.rank != b.rank)
      return a.rank > b.rank;
    return a.doc_name < b.doc_name;
  });
  return results;
}

}  // namespace searchserver