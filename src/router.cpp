#include "router.h"
#include "utilities.h"
#include "vector_entry.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <bits/types/struct_timeval.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <errno.h>
#include <exception>
#include <ios>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <new>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <vector>

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
    printf("<sendto> interface with ip %d %d is unavailable\n",
           network_adress.sin_addr.s_addr, broadcast_address.s_addr);
  }
}
void Router::printDistanceVector() {
  for (uint i = 0; i < this->distance_vector.size(); i++) {
    VectorEntry vector_entry = this->distance_vector[i];
    /* inet_ntop requires address to be in network format (big-endian), so we
     * have to convert it. */
    // vector_entry.target_network.s_addr =
    // ntohl(vector_entry.target_network.s_addr);

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

    printf("\033[92m[%d] %s/%d %s %d %s %s\033[0m\n", i, network_ip_string,
           vector_entry.subnet_mask, "distance", vector_entry.distance,
           vector_entry.direct ? "connected directly" : "via", via_ip_string);
    /*
        // vector_entry.target_network.s_addr =
        //     htonl(vector_entry.target_network.s_addr);
        // vector_entry.via_network.s_addr =
       htonl(vector_entry.via_network.s_addr);
        // printf("<debug> broadcast/network/host:\nb: %#010x, n: %#010x, h:
        // // %#010x\n", vector_entry.getBroadcastAdress().s_addr,
        // vector_entry.getNetworkAdress().s_addr,
        // vector_entry.getHostAdress().s_addr);
      */
  }
  printf("\n");
}

void Router::loop() {
  int i = 0;
  for (;;) {
    std::cout << "\033[93mRound " << i  << "\033[0m\n";
    // print distance vector
    printDistanceVector();
    // send distance vector to neighbours
    sendDistanceVectorToNeighbours();

    // listen for distance vectors sent by neighbours
    receiveDistanceVectorFromNeighbours();
    i++;
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
      // printf("<debug>sending entry [%u] to neighbours\n", j);
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
      std::cout << "sender " << ipToString(sender) << " is from neighbour's "
                << ipToString(neighbour.target_network) << " network\n";

      return neighbour.distance; // We will only at most one matching neighbour
                                 // (networks are disjoint).
    }
  }

  return -1;
}

void Router::updateDistanceVector(VectorEntry &new_vector_entry) {
  struct in_addr sender = new_vector_entry.via_network;

  // Find which neighbour sent this message
  for (VectorEntry neighbour : this->neighbours) {
    if (neighbour.getNetworkAdress().s_addr ==
        getNetworkAdress_util(sender, neighbour.subnet_mask).s_addr) {
      std::cout << "sender " << ipToString(sender) << " is from neighbour's "
                << ipToString(neighbour.target_network) << " network\n";
    }
  }

  // Assume that this is a new entry.
  bool is_new_entry = true;

  // Find if we already know how to route to the received network.
  for (uint i = 0; i < this->distance_vector.size(); i++) {
    VectorEntry entry = this->distance_vector[i];
    std::cout << i << "\n";
    // If target network exists in our distance vector, then we want to check,
    // if we should update it.
    if (entry.target_network.s_addr == new_vector_entry.target_network.s_addr) {
      std::cout << "\033[92mGOOD " << ipToString(entry.target_network) << " "
                << ipToString(new_vector_entry.target_network) << "\033[0m\n";

      // This network's entry already exists, so it's not new.
      is_new_entry = false;

      int distance_to_sender = getDistanceToSender(sender);
      if (distance_to_sender < 0) {
        std::cout << "\033[91mERROR -- sender is not a neighbour\033[0m\n";
      }

      // If new entry has shorter distance than current entry, we replace old
      // entry with new one.
      if (new_vector_entry.distance + distance_to_sender < entry.distance) {
        std::cout << "\033[93mEntry updated. New distance: "
                  << new_vector_entry.distance << "\033[0m\n";

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
        std::cout << "\033[93mEntry updated. Network "
                  << ipToString(new_vector_entry.target_network)
                  << " is unreachable.\033[0m\n";

        this->distance_vector[i].distance = INFDIST;
      }

    } else {
      std::cout << "\033[91mBAD  " << ipToString(entry.target_network) << " "
                << ipToString(new_vector_entry.target_network) << "\033[0m\n";
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
    std::cout << "\033[93mNew entry. Target " << ipToString(new_vector_entry.target_network) << " via " << ipToString(new_vector_entry.via_network) << "\033[0m\n";
    addVectorEntry(new_vector_entry);
  }
}

void Router::listenForNeighboursMessages() {
  uint8_t message_buffer[IP_MAXPACKET + 1];
  struct sockaddr_in sender;
  socklen_t addr_len = sizeof(sender);

  // printf("<debug>message buffer before recvfrom:\n");
  // for (uint i = 0; i < 10; i++) {
  //   printf("%x ", message_buffer[i]);
  // }
  // printf("\n");

  char sender_ip_string[20];

  ssize_t datagram_size = recvfrom(this->sockfd, message_buffer, IP_MAXPACKET,
                                   0, (struct sockaddr *)&sender, &addr_len);
  if (datagram_size < 0) {
    fprintf(stderr, "recfrom error: %s\n", strerror(errno));
    inet_ntop(AF_INET, &sender.sin_addr, sender_ip_string,
              sizeof(sender_ip_string));
    // printf("<debug> received message from %x [%s] datagram_size =
    // %ld\n[message]\n",
    //  sender.sin_addr.s_addr, sender_ip_string, datagram_size);

    // for (uint i = 0; i < 10; i++) {
    //   printf("%x ", message_buffer[i]);
    // }
    // printf("\n\n");
    exit(EXIT_FAILURE);
  }

  // printf("<debug> received message from %x datagram_size = %ld\n[message]\n",
  //  sender.sin_addr.s_addr, datagram_size);

  // for (uint i = 0; i < 10; i++) {
  //   printf("%x ", message_buffer[i]);
  // }
  // printf("\n\n");

  VectorEntry received_vector_entry(message_buffer, sender.sin_addr);
  updateDistanceVector(received_vector_entry);
}

void Router::receiveDistanceVectorFromNeighbours() {
  int timeout = 3 * 1000; // 20s = 20000ms
  struct timespec begin, end;

  struct pollfd fds {
    .fd = this->sockfd, .events = POLLIN, .revents = 0
  };

  // printf("<debug>listening on port %d, socket %d\n", this->port,
  // this->sockfd); printf("<debug>pollfd: fd=%d events=%d, revents=%d\n",
  // fds.fd, fds.events,
  //  fds.revents);

  // Listen for messages from neighbours for 'timeout' seconds.
  while (timeout > 0) {
    std::cout << "\033[93mlistening\033[0m\n";
    clock_gettime(CLOCK_REALTIME, &begin);
    int ready_nfds = poll(&fds, 1, timeout);
    clock_gettime(CLOCK_REALTIME, &end);

    // printf("\n[time] timeout: %dms | begin: %lds | end: %lds\n", timeout,
    //        begin.tv_sec, end.tv_sec);

    if (ready_nfds == -1) {
      perror("poll");
      exit(EXIT_FAILURE);
    } else if (ready_nfds > 0) {
      std::cout << "start\n";
      listenForNeighboursMessages();
      std::cout << "end\n";
    }
    // printf("<debug>ready fds: %d\n", ready_nfds);
    timeout -= (end.tv_sec - begin.tv_sec) * 1000 +
               (end.tv_nsec - begin.tv_nsec) / 1000000;
  }
}