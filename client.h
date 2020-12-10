//
// Created by jmalcy on 12/5/20.
//

#ifndef NETWORK_SYSTEMS_PA4_CLIENT_H
#define NETWORK_SYSTEMS_PA4_CLIENT_H
#include "configParser.h"
#include "common.h"

#define START_LIST_FILES  8192

void displayHelp();
int makeSocket(struct addrinfo *info);
int list(dfc config, distributedFile *files, size_t *capacity);
void *get(dfc config, const char *fileName);
int put(dfc config, const char *fileName);

#endif //NETWORK_SYSTEMS_PA4_CLIENT_H
