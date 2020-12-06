//
// Created by jmalcy on 12/5/20.
//

#ifndef NETWORK_SYSTEMS_PA4_CONFIGPARSER_H
#define NETWORK_SYSTEMS_PA4_CONFIGPARSER_H

#include "macro.h"
#include <sys/socket.h>
#include <netdb.h>

typedef struct {
	struct sockaddr_in *dfsSockets[4];
	socklen_t lengths[4];
	char username[MAX_USERNAME];
	char password[MAX_PASSWORD];
} dfc;

int initDFC(const char *fileName, dfc *config);

void destroyDFC(dfc *config);
#endif //NETWORK_SYSTEMS_PA4_CONFIGPARSER_H
