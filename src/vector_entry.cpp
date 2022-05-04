#include "vector_entry.h"
#include "utilities.h"

VectorEntry::VectorEntry(const char *ip_address_string,
                         uint8_t target_network_mask, uint32_t distance,
                         bool direct) {

  int s = inet_pton(AF_INET, ip_address_string, &this->target_network);
  /* inet_pton stores numeric adress in dst parameter in network byte order
   * (big-endian), so we have to convert it. */
  this->target_network.s_addr = ntohl(this->target_network.s_addr);

  if (s <= 0) {
    if (s == 0)
      fprintf(stderr, "Invalid format");
    else
      perror("inet_pton");
    exit(EXIT_FAILURE);
  }

  this->via_network.s_addr = this->target_network.s_addr;
  this->target_network =
      getNetworkAdress_util(this->target_network, target_network_mask);
  this->subnet_mask = target_network_mask;
  this->distance = distance;
  this->direct = direct;
  this->turns_last_seen = 0;
  this->turns_down = 0;
  this->up = true;
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
  this->turns_last_seen = 0;
  this->turns_down = 0;
  this->up = true;
}

struct in_addr VectorEntry::getBroadcastAdress() {
  return getBroadcastAdress_util(this->target_network, this->subnet_mask);
}

struct in_addr VectorEntry::getNetworkAdress() {
  return getNetworkAdress_util(this->target_network, this->subnet_mask);
}

struct in_addr VectorEntry::getHostAdress() {
  return getHostAdress_util(this->target_network, this->subnet_mask);
}


void VectorEntry::printVectorEntry() {
  std::cout << "target network: " << ipToString(this->target_network)
            << "\nsubnet    mask: " << (int)this->subnet_mask
            << "\nvia    network: " << ipToString(this->via_network)
            << "\ndistance      : " << this->distance
            << "\ndirect        : " << this->direct
            << "\nreachable     : " << this->turns_down
            << "\n";
}
