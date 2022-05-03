#ifndef UTILITIES_H
#define UTILITIES_H

#include <iostream>
#include <cstdint>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>

uint const INFDIST = UINT32_MAX;
uint const INFTURN = 5;
uint const TIMEOUT = 5;

enum ACTIVITY : bool {
    INACTIVE = false,
    ACTIVE = true
};

std::string ipToString(struct in_addr address);
struct in_addr getBroadcastAdress_util(struct in_addr address, uint8_t subnet_mask);
struct in_addr getNetworkAdress_util(struct in_addr address, uint8_t subnet_mask);
struct in_addr getHostAdress_util(struct in_addr address, uint8_t subnet_mask);

#endif