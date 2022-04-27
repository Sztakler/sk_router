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
#include <ios>
#include <netinet/in.h>
#include <netinet/ip.h>
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
    this->distance_vector.push_back(vector_entry);
    this->neighbours.push_back(vector_entry);
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
    inet_ntop(AF_INET, &(vector_entry.via_network.s_addr), via_ip_string,
              sizeof(via_ip_string));

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
  for (;;) {
    // print distance vector
    printDistanceVector();
    // send distance vector to neighbours
    sendDistanceVectorToNeighbours();

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
      // printf("<debug>sending entry [%u] to neighbours\n", j);
      sendVectorEntry(this->distance_vector[j], this->neighbours[i]);
    }
  }
}

bool Router::isFromNeighbouringNetwork(struct in_addr address1,
                                       struct in_addr address2) {
  return address1.s_addr == address2.s_addr;
}

void Router::updateDistanceVector(VectorEntry &new_vector_entry) {
  for (uint i = 0; i < this->neighbours.size(); i++) {
    VectorEntry neighbour = this->neighbours[i];

    // printf("received: %s\nneighbour: %s\n\n",
    // ipToString(new_vector_entry.target_network, neighbour.target_network));

    std::cout << "received: " << ipToString(new_vector_entry.target_network)
              << "\nneighbour: " << ipToString(neighbour.target_network)
              << "\n";

    std::cout << "[neighbour]\n";
    neighbour.printVectorEntry();

    std::cout << "[received]\n";
    new_vector_entry.printVectorEntry();

    // If new entry routes through one of our neighbours, we have to append
    // distance to that neighbour to that entry.
    if (isFromNeighbouringNetwork(
            getNetworkAdress_util(new_vector_entry.via_network,
                                  neighbour.subnet_mask),
            neighbour.getNetworkAdress())) {
      printf("Is from neighbouring network.\n");
      new_vector_entry.direct = true;
      new_vector_entry.distance += neighbour.distance;
      break;
    }

    // printf("\n");
  }

  bool is_new_entry = true;

  for (uint j = 0; j < this->distance_vector.size(); j++) {
    VectorEntry current_vector_entry = this->distance_vector[j];
    if (current_vector_entry.getNetworkAdress().s_addr ==
        new_vector_entry.getNetworkAdress().s_addr) {
      // std::cout << "Adding new entry\n";
      // new_vector_entry.printVectorEntry();

      // new_vector_entry.direct = false;
      // new_vector_entry.distance += neighbour.distance;
      // this->addVectorEntry(new_vector_entry);
    }
  }

  // /* At first assume, that this entry is completely new. */
  // bool is_new_entry = true;
  //
  //
  //
  // uint current_distance_vector_size = this->distance_vector.size();
  // for (uint i = 0; i < current_distance_vector_size; i++) {
  //   VectorEntry current_vector_entry = this->distance_vector[i];

  // printf("<debug>[%d]\n", i);

  // printf("<debug>current vector entry:\n");
  // printf("<debug>net: %#010x\nsub: %d\nvia: %#010x\ndis: %d\ndir: %d\n",
  //  current_vector_entry.target_network.s_addr,
  //  current_vector_entry.subnet_mask,
  //  current_vector_entry.via_network.s_addr,
  //  current_vector_entry.distance, current_vector_entry.direct);

  // printf("<debug>received vector entry:\n");
  // printf("<debug>net: %#010x\nsub: %d\nvia: %#010x\ndis: %d\ndir: %d\n",
  //  new_vector_entry.target_network.s_addr, new_vector_entry.subnet_mask,
  //  new_vector_entry.via_network.s_addr, new_vector_entry.distance,
  //  new_vector_entry.direct);

  // printf("<debug>same network condition: %d | %#010x %#010x\n",
  //  current_vector_entry.getNetworkAdress().s_addr ==
  //      new_vector_entry.getNetworkAdress().s_addr,
  //  current_vector_entry.getNetworkAdress().s_addr,
  //  new_vector_entry.getNetworkAdress().s_addr);

  /*
   * If received vector describes networks, that already is inside this
   * router's distance vector, we may be able to update it.
   */
  // if (current_vector_entry.getNetworkAdress().s_addr ==
  //     new_vector_entry.getNetworkAdress().s_addr) {

  //   /* This entry is already in router's distance vector, so we don't want to
  //    * add it.
  //    */
  //   is_new_entry = false;

  //   /*
  //    * We have to check if received entry's distance is
  //    * shorter than it's current_vector_entry one -- then we need to update
  //    * this entry.
  //    */
  //   if (new_vector_entry.distance < current_vector_entry.distance)
  //     this->distance_vector[i] = new_vector_entry;

  //   /*
  //    * Otherwise, new entry may contain information, that current cunnection
  //    * with particular network was lost. We have to check, whether new and
  //    * current entry route through the same network (via) and whether new
  //    * entry's distance is set to infinity. If that's not the case, we simply
  //    * update distance in router's distance vector to received value.
  //    */
  //   else if (new_vector_entry.via_network.s_addr ==
  //            current_vector_entry.via_network.s_addr) {
  //     if (new_vector_entry.distance >= 16)
  //       this->distance_vector[i].distance = 16;
  //     else
  //       this->distance_vector[i].distance = new_vector_entry.distance;
  //   }
  // }
  // }

  /*
   * If this is a completely new entry, we add it to the distance vector.
   */
  // if (is_new_entry)
  //   addVectorEntry(new_vector_entry);
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

  while (timeout > 0) {
    clock_gettime(CLOCK_REALTIME, &begin);
    int ready_nfds = poll(&fds, 1, timeout);
    clock_gettime(CLOCK_REALTIME, &end);

    // printf("\n[time] timeout: %dms | begin: %lds | end: %lds\n", timeout,
    //        begin.tv_sec, end.tv_sec);

    if (ready_nfds == -1) {
      perror("poll");
      exit(EXIT_FAILURE);
    } else if (ready_nfds > 0) {
      listenForNeighboursMessages();
    }
    // printf("<debug>ready fds: %d\n", ready_nfds);
    timeout -= (end.tv_sec - begin.tv_sec) * 1000 +
               (end.tv_nsec - begin.tv_nsec) / 1000000;
  }
}