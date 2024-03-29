#include "router.h"
#include "utilities.h"
#include "vector_entry.h"
#include <cstring>
#include <errno.h>
#include <netinet/ip.h>
#include <poll.h>
#include <sstream>

Router::Router() { initializeRouter(); }

void Router::initializeDistanceVector() {
  int n;
  std::cin >> n;

  std::string cidr_ip;
  std::string distance_string;
  uint32_t distance;

  for (int i = 0; i < n; i++) {

    std::cin >> cidr_ip;
    std::cin >> distance_string;
    std::cin >> distance;

    std::stringstream ip_stream(cidr_ip);

    std::string ip_string;
    uint8_t subnet_mask;

    std::vector<std::string> tokens;
    std::string token;

    /* Tokenize IP string in CIDR notation to ip string and netmask. */
    while (std::getline(ip_stream, token, '/')) {
      tokens.push_back(token);
    }
    ip_string = tokens[0];
    subnet_mask = (uint8_t)atoi(tokens[1].c_str());

    /* Create new vector ventry and push it into router's distance vector and
     * neighbours vector. */
    VectorEntry vector_entry(ip_string.c_str(), subnet_mask, distance, true);

    this->neighbours.push_back(vector_entry);
    this->neighbours_activity_map.push_back(INACTIVE);
    // vector_entry.target_network = vector_entry.getNetworkAdress();
    this->distance_vector.push_back(vector_entry);
  }
}

void Router::initializeRouter() {

  createSocket();
  bindToPort();
  giveBroadcastPermissionToSocket();

  initializeDistanceVector();
}

void Router::addVectorEntry(VectorEntry &vector_entry) {
  this->distance_vector.push_back(vector_entry);
}

void Router::sendVectorEntry(VectorEntry &vector_entry,
                             VectorEntry &neighbour) {
  uint8_t message_buffer[10];

  *(in_addr_t *)message_buffer = htonl(vector_entry.target_network.s_addr);
  message_buffer[4] = vector_entry.subnet_mask;
  *(uint32_t *)(message_buffer + 5) = htonl(vector_entry.distance);
  message_buffer[9] = 0; // null terminate

  // struct in_addr broadcast_address = vector_entry.getBroadcastAdress();
  struct in_addr broadcast_address = neighbour.getBroadcastAdress();

  broadcast_address.s_addr = htonl(broadcast_address.s_addr);

  struct sockaddr_in network_adress;
  bzero(&network_adress, sizeof(network_adress));
  network_adress.sin_family = AF_INET;
  network_adress.sin_port = htons(this->port);
  network_adress.sin_addr = broadcast_address;

  int bytes = sendto(this->sockfd, message_buffer, 9, 0,
                     (sockaddr *)&network_adress, sizeof(network_adress));
  if (bytes != 9) {
    // perror("sendto");
    markNeighbourDown(neighbour);
  } else {
    markNeighbourUp(neighbour);
  }
}
void Router::printDistanceVector() {
  for (uint i = 0; i < this->distance_vector.size(); i++) {
    VectorEntry vector_entry = this->distance_vector[i];
    /* inet_ntop requires address to be in network format (big-endian), so we
     * have to convert it. */

    char network_ip_string[20];
    struct in_addr net = getNetworkAdress_util(vector_entry.target_network,
                                               vector_entry.subnet_mask);
    net.s_addr = ntohl(net.s_addr);
    inet_ntop(AF_INET, &(net.s_addr), network_ip_string,
              sizeof(network_ip_string));

    char via_ip_string[20];
    struct in_addr via = vector_entry.via_network;
    via.s_addr = ntohl(via.s_addr);
    inet_ntop(AF_INET, &(via.s_addr), via_ip_string, sizeof(via_ip_string));

    printf("[%d] %s/%d ", i, network_ip_string, vector_entry.subnet_mask);
    if (vector_entry.turns_down > 0 || vector_entry.turns_last_seen > INFTURN) {
      printf("unreachable ");
    } else {
      printf("distance %d ", vector_entry.distance);
    }
    if (vector_entry.direct) {
      printf("connected directly");
    } else {
      printf("via %s", via_ip_string);
    }
    printf("\n");
  }
  printf("\n");
}

void Router::loop() {
  for (;;) {
    // print distance vector
    printDistanceVector();
    // send distance vector to neighbours
    sendDistanceVectorToNeighbours();

    // Initially mark all neighbours as inactive and update their status when
    // activity on particular interface is recorded.
    for (uint i = 0; i < neighbours.size(); i++)
    { /* No activity on this neighbour. */
      this->neighbours_activity_map[i] = INACTIVE;
    }
    // listen for distance vectors sent by neighbours
    receiveDistanceVectorFromNeighbours();
  }
}

void Router::createSocket() {
  this->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (this->sockfd < 0) {
    fprintf(stderr, "socket error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void Router::giveBroadcastPermissionToSocket() {
  int broadcast_permission = 1;
  setsockopt(this->sockfd, SOL_SOCKET, SO_BROADCAST,
             (void *)&broadcast_permission, sizeof(broadcast_permission));
}

void Router::bindToPort() {
  struct sockaddr_in server_address;
  bzero(&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(this->port);
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(this->sockfd, (struct sockaddr *)&server_address,
           sizeof(server_address)) < 0) {
    fprintf(stderr, "bind error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void Router::sendDistanceVectorToNeighbours() {
  for (uint i = 0; i < this->neighbours.size(); i++) {
    for (uint j = 0; j < this->distance_vector.size(); j++) {
      sendVectorEntry(this->distance_vector[j], this->neighbours[i]);
    }
  }
}

bool Router::isFromNeighbouringNetwork(struct in_addr address1,
                                       struct in_addr address2) {
  return address1.s_addr == address2.s_addr;
}

// Find which neighbour sent this message
int Router::getDistanceToSender(struct in_addr sender) {
  for (uint j = 0; j < this->neighbours.size(); j++) {
    VectorEntry neighbour = this->neighbours[j];
    // Sender is our neighbour if it's IP is from current neighbour's
    // network.
    if (neighbour.getNetworkAdress().s_addr ==
        getNetworkAdress_util(sender, neighbour.subnet_mask).s_addr) {

      return neighbour.distance; // We will only at most one matching neighbour
                                 // (networks are disjoint).
    }
  }

  return -1;
}

void Router::updateNeighbourActivity(struct in_addr sender) {
  // Find which neighbour is the sender
  for (uint i = 0; i < this->neighbours.size(); i++) {
    VectorEntry neighbour = this->neighbours[i];
    // If this neighbour has contacted us, and the sender is not ourselves, we
    // can reset it's inactivity counter.

    if (neighbour.getNetworkAdress().s_addr ==
            getNetworkAdress_util(sender, neighbour.subnet_mask).s_addr &&
        sender.s_addr != neighbour.via_network.s_addr) {
      /* Recorded activity on this neighbour. */
      this->neighbours_activity_map[i] = ACTIVE;
    }
  }
}

void Router::markNeighbourDown(VectorEntry &neighbour) { neighbour.up = false; }

void Router::markNeighbourUp(VectorEntry &neighbour) {
  neighbour.up = true;
  neighbour.turns_down = 0;
}

void Router::updateNeighbourDowntime() {
  for (VectorEntry &neighbour : this->neighbours) {
    if (neighbour.up == false) {
      neighbour.turns_down++;
    } else {
      neighbour.turns_down = 0;
    }

    for (VectorEntry &network : this->distance_vector) {
      if (getNetworkAdress_util(network.via_network, neighbour.subnet_mask)
              .s_addr == neighbour.getNetworkAdress().s_addr) {
        network.up = neighbour.up;
        network.turns_down = neighbour.turns_down;
      }
    }
  }
}

void Router::updateNeighboursInactivityCounter() {
  for (uint i = 0; i < this->neighbours_activity_map.size(); i++) {
    if (this->neighbours_activity_map[i] == INACTIVE) {
      this->neighbours[i].turns_last_seen++;
    } else {
      this->neighbours[i].turns_last_seen = 0;
    }

    this->neighbours_activity_map[i] = INACTIVE;
  }
}

void Router::markNetworksUnreachable() {
  // Mark all networks as reachable.
  for (VectorEntry &network : this->distance_vector) {
    network.turns_last_seen = 0;
  }

  // Mark only unreachable networks as unreachable.
  for (VectorEntry &neighbour : this->neighbours) {
    if (neighbour.turns_last_seen >= INFTURN || neighbour.turns_down > 0) {
      printf("Network last seen increment. Reason: %d || %d\n",
             neighbour.turns_last_seen >= INFTURN, neighbour.turns_down > 0);
      for (VectorEntry &network : this->distance_vector) {
        if (getNetworkAdress_util(network.via_network, neighbour.subnet_mask)
                .s_addr == neighbour.getNetworkAdress().s_addr) {
          network.turns_last_seen = neighbour.turns_last_seen;
          network.turns_down = neighbour.turns_down;
          std::cout << "network unreachable last seen:"
                    << network.turns_last_seen
                    << " turns down: " << network.turns_down << "\n";

          for (uint j = 0; j < this->distance_vector.size(); j++)
            printf("[%d] %d %u\n", j, this->distance_vector[j].turns_last_seen,
                   this->distance_vector[j].turns_last_seen);
          printDistanceVector();
        }
      }
    }
  }
}

void Router::updateDistanceVector(VectorEntry &new_vector_entry) {
  struct in_addr sender = new_vector_entry.via_network;

  updateNeighbourActivity(sender);

  // Assume that this is a new entry.
  bool is_new_entry = true;

  // Find if we already know how to route to the received network.
  for (uint i = 0; i < this->distance_vector.size(); i++) {
    VectorEntry entry = this->distance_vector[i];
    // If target network exists in our distance vector, then we want to check,
    // if we should update it.
    if (entry.target_network.s_addr == new_vector_entry.target_network.s_addr) {
      // This network's entry already exists, so it's not new.
      is_new_entry = false;

      int distance_to_sender = getDistanceToSender(sender);
      if (distance_to_sender < 0) {
        std::cout << "\033[91mERROR -- sender is not a neighbour\033[0m\n";
      }

      // If new entry has shorter distance than current entry, we replace old
      // entry with new one.
      if (new_vector_entry.distance + distance_to_sender < entry.distance) {

        // Add distance to sender router to the new entry's distance -- we have
        // to
        // route there first in order to reach the target network.
        new_vector_entry.distance += distance_to_sender;
        this->distance_vector[i] = new_vector_entry;

      } // Otherwise we have to check whether new entry has infinite distance to
        // target -- then we may want to update our current entry, because that
        // networks is no longer reachable.
        // If it so happened that that new entry routes through the same
        // neighbour that sent that message, we must believe this neighbour --
        // it's closer to target on the route to it, so it has greater knowledge
        // about that route's current state.
      else if (new_vector_entry.distance >= INFDIST &&
               (new_vector_entry.via_network.s_addr == sender.s_addr)) {

        this->distance_vector[i].distance = INFDIST;
      }

    } else {
    }
  }

  // If this is a completely new entry, then simply add it to the distance
  // vector.
  if (is_new_entry) {
    int distance_to_sender = getDistanceToSender(sender);
    if (distance_to_sender < 0) {
      std::cout << "\033[91mERROR -- sender is not a neighbour\033[0m\n";
    }

    // Add distance to sender router to new entry distance -- we have to route
    // there first in order to reach target network.
    new_vector_entry.distance += distance_to_sender;
    addVectorEntry(new_vector_entry);
  }
}

void Router::listenForNeighboursMessages() {
  uint8_t message_buffer[IP_MAXPACKET + 1];
  struct sockaddr_in sender;
  socklen_t addr_len = sizeof(sender);

  char sender_ip_string[20];

  ssize_t datagram_size = recvfrom(this->sockfd, message_buffer, IP_MAXPACKET,
                                   0, (struct sockaddr *)&sender, &addr_len);
  if (datagram_size < 0) {
    fprintf(stderr, "recfrom error: %s\n", strerror(errno));
    inet_ntop(AF_INET, &sender.sin_addr, sender_ip_string,
              sizeof(sender_ip_string));

    exit(EXIT_FAILURE);
  }

  VectorEntry received_vector_entry(message_buffer, sender.sin_addr);
  updateDistanceVector(received_vector_entry);
}

void Router::receiveDistanceVectorFromNeighbours() {
  int timeout = 3 * 1000; // 20s = 20000ms
  struct timespec begin, end;

  struct pollfd fds {
    .fd = this->sockfd, .events = POLLIN, .revents = 0
  };

  // Listen for messages from neighbours for 'timeout' seconds.
  while (timeout > 0) {
    clock_gettime(CLOCK_REALTIME, &begin);
    int ready_nfds = poll(&fds, 1, timeout);
    clock_gettime(CLOCK_REALTIME, &end);

    if (ready_nfds == -1) {
      perror("poll");
      exit(EXIT_FAILURE);
    } else if (ready_nfds > 0) {
      listenForNeighboursMessages();
    }

    timeout -= (end.tv_sec - begin.tv_sec) * 1000 +
               (end.tv_nsec - begin.tv_nsec) / 1000000;
  }
  updateNeighboursInactivityCounter();
  updateNeighbourDowntime();
  markNetworksUnreachable();
  removeInactiveNetworks();
}

void Router::removeInactiveNetworks() {
  uint i = 0;
  while (i < this->distance_vector.size()) {
    if ((this->distance_vector[i].turns_down > TIMEOUT ||
         this->distance_vector[i].turns_last_seen > (INFTURN + TIMEOUT)) &&
        this->distance_vector[i].direct == false) {
      this->distance_vector.erase(this->distance_vector.begin() + i);
    } else {
      i++;
    }
  }
}