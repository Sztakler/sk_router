#ifndef UTILITIES_H
#define UTILITIES_H

#include <iostream>
#include <cstdint>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>

std::string ipToString(struct in_addr address);
struct in_addr getBroadcastAdress_util(struct in_addr address, uint8_t subnet_mask);
struct in_addr getNetworkAdress_util(struct in_addr address, uint8_t subnet_mask);
struct in_addr getHostAdress_util(struct in_addr address, uint8_t subnet_mask);

#endif