//
// Created by jmalcy on 12/5/20.
//

#include "macro.h"
#include <sys/socket.h>
#include <netdb.h>

#ifndef NETWORK_SYSTEMS_PA4_CONFIGPARSER_H
#define NETWORK_SYSTEMS_PA4_CONFIGPARSER_H

typedef struct {
	struct addrinfo *serverInfo[4];
	char username[MAX_USERNAME];
	char password[MAX_PASSWORD];
} dfc;

int initDFC(const char *fileName, dfc *config);

void destroyDFC(dfc *config);

int parseDFS(const char *fileName, char **usernames, char **passwords, size_t capacity);
#endif //NETWORK_SYSTEMS_PA4_CONFIGPARSER_H
