#include "vector_entry.h"
#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <netinet/in.h>

VectorEntry::VectorEntry(const char *ip_address_string,
                         uint8_t target_network_mask, uint32_t distance,
                         bool direct) {

  int s = inet_pton(AF_INET, ip_address_string, &this->target_network);
  if (s <= 0) {
    if (s == 0)
      fprintf(stderr, "Invalid format");
    else
      perror("inet_pton");
    exit(EXIT_FAILURE);
  }

  this->subnet_mask = target_network_mask;
  this->distance = distance;
  this->direct = direct;
}

struct in_addr VectorEntry::getBroadcastAdress() {
  struct in_addr broadcast_address = this->target_network;
  in_addr_t bitmask = (0xFFFFFFFF >> this->subnet_mask);
  broadcast_address.s_addr = this->target_network.s_addr;
  broadcast_address.s_addr |= (~bitmask);

  return broadcast_address;
}

struct in_addr VectorEntry::getNetworkAdress() {
  struct in_addr network_adress = this->target_network;
  in_addr_t bitmask = (0xFFFFFFFF >> this->subnet_mask);
  network_adress.s_addr = this->target_network.s_addr;
  network_adress.s_addr &= bitmask;

  return network_adress;
}
