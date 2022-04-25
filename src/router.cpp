#include "router.h"

Router::Router() {}

void Router::initializeDistanceVector() {
  std::vector<std::vector<std::string>> lines;
  std::vector<std::string> line(3);
  int n;

  std::cin >> n;

  for (int i = 0; i < n; i++) {
    std::cin >> line[0];
    std::cin >> line[1];
    std::cin >> line[2];

    lines.push_back(line);
  }

  for (auto line : lines) {
    for (auto word : line) {
      { std::cout << word << " "; }
    }
    std::cout << "\n";
  }

}

void Router::addVectorEntry(VectorEntry vector_entry) {}