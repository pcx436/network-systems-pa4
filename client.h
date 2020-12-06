//
// Created by jmalcy on 12/5/20.
//

#ifndef NETWORK_SYSTEMS_PA4_CLIENT_H
#define NETWORK_SYSTEMS_PA4_CLIENT_H
#include "macro.h"
#include <sys/socket.h>
#include <netdb.h>

typedef struct {
	struct sockaddr_in *dfsSockets[4];
	char username[MAX_USERNAME];
	char password[MAX_PASSWORD];
} dfc;

int initDFC(const char *fileName, dfc *config);

void destroyDFC(dfc *config);

void trimSpace(char *s);

#endif //NETWORK_SYSTEMS_PA4_CLIENT_H
