#include "vector_entry.h"
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>

VectorEntry::VectorEntry(const char *ip_address_string,
                         uint8_t target_network_mask, in_addr via_network,
                         uint32_t distance) {

  int s = inet_pton(AF_INET, ip_address_string, &this->target_network);
  if (s <= 0) {
    if (s == 0)
      fprintf(stderr, "Not in presentation format");
    else
      perror("inet_pton");
    exit(EXIT_FAILURE);
  }

  
}