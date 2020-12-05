//
// Created by jmalcy on 12/5/20.
//

#ifndef NETWORK_SYSTEMS_PA4_CLIENT_H
#define NETWORK_SYSTEMS_PA4_CLIENT_H
#include "macro.h"
#include <sys/socket.h>

typedef struct {
	struct sockaddr firstServerSocket;
	struct sockaddr secondServerSocket;
	struct sockaddr thirdServerSocket;
	struct sockaddr fourthServerSocket;
	char username[MAX_USERNAME];
	char password[MAX_PASSWORD];
} dfc;

void initDFC(const char *fileName, dfc *config);

void destroyDFC(dfc *config);

#endif //NETWORK_SYSTEMS_PA4_CLIENT_H
