#ifndef VECTOR_ENTRY_H
#define VECTOR_ENTRY_H

#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>

class VectorEntry {
public:
  in_addr target_network; // network IP adress in host byte order
  uint8_t subnet_mask; // network subnet mask
  in_addr via_network; // network IP adress in host by order
  uint32_t distance; // current distance to target network
  bool direct; // true if target_network is directly connected with router

public:
  VectorEntry(const char* ip_address_string, uint8_t target_network_mask, uint32_t distance, bool direct);

  public:
  struct in_addr getBroadcastAdress();
  struct in_addr getNetworkAdress();
  struct in_addr getHostAdress();
};

#endif

