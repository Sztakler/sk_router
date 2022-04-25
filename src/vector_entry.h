#ifndef VECTOR_ENTRY_H
#define VECTOR_ENTRY_H

#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>

class VectorEntry {
private:
  in_addr target_network;
  uint8_t target_network_mask;
  in_addr via_network;
  uint32_t distance;

public:
  VectorEntry(const char* ip_address_string, uint8_t target_network_mask,
              in_addr via_network, uint32_t distance);
};

#endif

