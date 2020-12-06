//
// Created by jmalcy on 12/5/20.
//

#ifndef NETWORK_SYSTEMS_PA4_CLIENT_H
#define NETWORK_SYSTEMS_PA4_CLIENT_H
#include "configParser.h"

void displayHelp();
int makeSocket(struct addrinfo *info);
int * pingServers(dfc config);
int countOnes(const int *online);

#endif //NETWORK_SYSTEMS_PA4_CLIENT_H
