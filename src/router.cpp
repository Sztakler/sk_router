#include "router.h"
#include "vector_entry.h"
#include <asm-generic/socket.h>
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>

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

    /* Create new vector ventry and push it into router's distance vector. */
    VectorEntry vector_entry(ip_string.c_str(), subnet_mask, distance, true);
    this->distance_vector.push_back(vector_entry);
  }
}

void Router::initializeRouter() {

  createSocket();
  giveBroadcastPermissionToSocket();
  bindToPort();

  initializeDistanceVector();
}

void Router::addVectorEntry(VectorEntry vector_entry) {}

void Router::sendVectorEntry(int i) {
  VectorEntry vector_entry = this->distance_vector[i];
  uint8_t message_buffer[9];

  *(in_addr_t*)message_buffer = ntohl(vector_entry.target_network.s_addr);
  message_buffer[4] = vector_entry.target_network_mask;
  *(uint32_t*)(message_buffer + 5) = htonl(vector_entry.distance);

  printf("debug %x %d %d\n",
         vector_entry.target_network.s_addr,
         vector_entry.target_network_mask,
         vector_entry.distance);

  for (int i = 0; i < 9; i++)
    {
      printf("%x ", message_buffer[i]);
    }
  printf("\n\n");
}

void Router::printDistanceVector() {
  for (int i = 0; i < this->distance_vector.size(); i++) {
    VectorEntry vector_entry = this->distance_vector[i];

    char network_ip_string[20];
    inet_ntop(AF_INET, &(vector_entry.target_network.s_addr), network_ip_string,
              sizeof(network_ip_string));

    char via_ip_string[20];
    inet_ntop(AF_INET, &(vector_entry.via_network.s_addr), via_ip_string,
              sizeof(via_ip_string));

    printf("%s/%d %s %d %s %s\n", network_ip_string,
           vector_entry.target_network_mask, "distance", vector_entry.distance,
           vector_entry.direct ? "connected directly" : "via", via_ip_string);
  }
}

void Router::loop() {
  for (int i = 0; i < 5; i++) {
    // print distance vector
    printDistanceVector();
    // send distance vector to neighbours
    sendVectorEntry(0);
    // listen for distance vectors sent by neighbours
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
  for (int i = 0; i < this->distance_vector.size(); i++) {
  }
}
