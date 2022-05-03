#ifndef VECTOR_ENTRY_H
#define VECTOR_ENTRY_H

#include "utilities.h"

class VectorEntry {
public:
  in_addr target_network; // network IP adress in host byte order
  uint8_t subnet_mask;    // network subnet mask
  in_addr via_network;    // network IP adress in host by order
  uint32_t distance;      // current distance to target network
  bool direct; // true if target_network is directly connected with router
  uint turns_last_seen; // how many turns ago has this entry's via_network sent
                       // message to this router
  uint turns_down; // how many turns ago this entry's interface has been set down
  bool up;        // true if it's interface is up

public:
  VectorEntry(const char *ip_address_string, uint8_t target_network_mask,
              uint32_t distance, bool direct);
  VectorEntry(uint8_t message_buffer[10], struct in_addr sender);

public:
  /* Require network address in host byte order (little-endian). */
  struct in_addr getBroadcastAdress();
  /* Require network address in host byte order (little-endian). */
  struct in_addr getNetworkAdress();
  /* Require network address in host byte order (little-endian). */
  struct in_addr getHostAdress();
  // /* Require network address in host byte order (little-endian). */
  // struct in_addr getBroadcastAdress(struct in_addr address,
  //                                   uint8_t subnet_mask);
  // /* Require network address in host byte order (little-endian). */
  // struct in_addr getNetworkAdress(struct in_addr address, uint8_t
  // subnet_mask);
  // /* Require network address in host byte order (little-endian). */
  // struct in_addr getHostAdress(struct in_addr address, uint8_t subnet_mask);

  void printVectorEntry();
};

#endif
