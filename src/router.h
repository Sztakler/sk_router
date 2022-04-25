#ifndef ROUTER_H
#define ROUTER_H

#include <iostream>
#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>
#include <vector>
#include "string"

#include "vector_entry.h"


class Router {
private:
  std::vector<VectorEntry> distance_vector();

public: Router();

public:
  void addVectorEntry(VectorEntry vector_entry);
  void updateDistanceVector();
  void sendDistanceVectorToNeighbours();
  void receiveDistanceVectorFromNeighbours();

private:
  void initializeDistanceVector();
};

#endif
