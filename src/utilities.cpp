#include "utilities.h"

std::string ipToString(struct in_addr address) {
  char via_ip_string[20];
  in_addr_t ip = htonl(address.s_addr);
  inet_ntop(AF_INET, &(ip), via_ip_string, sizeof(via_ip_string));

  return std::string(via_ip_string);
}

struct in_addr getBroadcastAdress_util(struct in_addr address, uint8_t subnet_mask) {
  struct in_addr broadcast_address = address;
  in_addr_t bitmask = (0xFFFFFFFF >> subnet_mask);
  broadcast_address.s_addr = address.s_addr;
  broadcast_address.s_addr |= bitmask;

  return broadcast_address;
}

struct in_addr getNetworkAdress_util(struct in_addr address, uint8_t subnet_mask) {
  struct in_addr network_adress = address;
  in_addr_t bitmask = (0xFFFFFFFF >> subnet_mask);
  network_adress.s_addr = address.s_addr;
  network_adress.s_addr &= ~bitmask;

  return network_adress;
}

struct in_addr getHostAdress_util(struct in_addr address, uint8_t subnet_mask) {
  struct in_addr host_address = address;
  in_addr_t bitmask = (0xFFFFFFFF >> subnet_mask);
  host_address.s_addr = address.s_addr;
  host_address.s_addr &= bitmask;

  return host_address;
}