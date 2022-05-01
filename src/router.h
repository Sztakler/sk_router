#ifndef ROUTER_H
#define ROUTER_H

#include "utilities.h"
#include "vector_entry.h"
#include <netinet/in.h>

class Router {
private:
  int sockfd;
  in_port_t port = 54321;
  /* Vector holding direct connections (via is address this router's interface). */
  std::vector<VectorEntry> neighbours;
  /* Router's distance vector. */
  std::vector<VectorEntry> distance_vector;
  /* Mapping between neighbour's index in neighbours and distance vector and
   * their activity in current turn. 'ACTIVE' for active, 'INACTIVE' otherwise.*/
  std::vector<ACTIVITY> neighbours_activity_map;

public:
  Router();

public:
  void addVectorEntry(VectorEntry &vector_entry);
  void updateDistanceVector(VectorEntry &vector_entry);
  void printDistanceVector();

  /* Infinite loop in which router prints it's distance vector, sends it to
   * neighbours and listen for their's distance vectors. */
  void loop();

private:
  void createSocket();
  void giveBroadcastPermissionToSocket();
  void bindToPort();
  /* Initializes router's distance vector with informations from configuration
   * string given on stdin. */
  void initializeDistanceVector();
  /* Initializes router by creating it's socket, binding it to specific port and
   * initializing it's distance vector. */
  void initializeRouter();
  void sendDistanceVectorToNeighbours();
  void receiveDistanceVectorFromNeighbours();
  void listenForNeighboursMessages();
  void sendVectorEntry(VectorEntry &vectory_entry, VectorEntry &neighbour);

  bool isFromNeighbouringNetwork(struct in_addr network,
                                 struct in_addr neighbourhood);
  int getDistanceToSender(struct in_addr sender);
  void updateNeighbourActivity(struct in_addr sender);
  void updateNeighboursInactivityCounter();
  void markNetworksUnreachable();
};

#endif
