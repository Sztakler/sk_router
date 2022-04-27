#include "vector_entry.h"
#include "utilities.h"
#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/types.h>


VectorEntry::VectorEntry(const char *ip_address_string,
                         uint8_t target_network_mask, uint32_t distance,
                         bool direct) {

  int s = inet_pton(AF_INET, ip_address_string, &this->target_network);
  /* inet_pton stores numeric adress in dst parameter in network byte order
   * (big-endian), so we have to convert it. */
  this->target_network.s_addr = ntohl(this->target_network.s_addr);
  // printf("<VectorEntry> ip_address: %x %u\n", this->target_network.s_addr,
  //        this->target_network.s_addr);
  //   std::cout << ipToString(this->target_network) << '\n';
  if (s <= 0) {
    if (s == 0)
      fprintf(stderr, "Invalid format");
    else
      perror("inet_pton");
    exit(EXIT_FAILURE);
  }

  // this->target_network =
  //     getNetworkAdress_util(this->target_network, target_network_mask);
  this->subnet_mask = target_network_mask;
  this->distance = distance;
  this->direct = direct;
}

VectorEntry::VectorEntry(uint8_t message_buffer[10], struct in_addr sender) {
  struct in_addr *network_adress = (struct in_addr *)message_buffer;
  network_adress->s_addr = ntohl(network_adress->s_addr);
  uint8_t *subnet_mask = (uint8_t *)(message_buffer + 4);
  uint32_t *distance = (uint32_t *)(message_buffer + 5);
  *distance = ntohl(*distance);

  this->target_network = *network_adress;
  this->subnet_mask = *subnet_mask;
  this->via_network = sender;
  this->via_network.s_addr = ntohl(this->via_network.s_addr);
  this->distance = *distance;
  this->direct = false;
}

struct in_addr VectorEntry::getBroadcastAdress() {
  // struct in_addr broadcast_address = this->target_network;
  // in_addr_t bitmask = (0xFFFFFFFF >> this->subnet_mask);
  // broadcast_address.s_addr = this->target_network.s_addr;
  // broadcast_address.s_addr |= (~bitmask);

  // return broadcast_address;
  return getBroadcastAdress_util(this->target_network, this->subnet_mask);
}

struct in_addr VectorEntry::getNetworkAdress() {
  // struct in_addr network_adress = this->target_network;
  // in_addr_t bitmask = (0xFFFFFFFF >> this->subnet_mask);
  // network_adress.s_addr = this->target_network.s_addr;
  // network_adress.s_addr &= bitmask;

  // return network_adress;
  return getNetworkAdress_util(this->target_network, this->subnet_mask);
}

struct in_addr VectorEntry::getHostAdress() {
  // struct in_addr host_address = this->target_network;
  // in_addr_t bitmask = (0xFFFFFFFF >> this->subnet_mask);
  // host_address.s_addr = this->target_network.s_addr;
  // host_address.s_addr &= ~bitmask;

  // return host_address;
  return getHostAdress_util(this->target_network, this->subnet_mask);
}


void VectorEntry::printVectorEntry() {
  std::cout << "target network: " << ipToString(this->target_network)
            << "\nsubnet    mask: " << (int)this->subnet_mask
            << "\nvia    network: " << ipToString(this->via_network)
            << "\ndistance      : " << this->distance
            << "\ndirect        : " << this->direct
            << "\n";
}
