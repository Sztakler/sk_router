#include "vector_entry.h"
#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

VectorEntry::VectorEntry(const char *ip_address_string,
                         uint8_t target_network_mask,
                         uint32_t distance,
                         bool direct) {

  int s = inet_pton(AF_INET, ip_address_string, &this->target_network);
  if (s <= 0) {
    if (s == 0)
      fprintf(stderr, "Invalid format");
    else
      perror("inet_pton");
    exit(EXIT_FAILURE);
  }

  this->target_network_mask = target_network_mask;
  this->distance = distance;
  this->direct = direct;
}
