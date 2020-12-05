//
// Created by jmalcy on 12/5/20.
//

#ifndef NETWORK_SYSTEMS_PA4_CLIENT_H
#define NETWORK_SYSTEMS_PA4_CLIENT_H
#include "macro.h"

typedef struct {
	int firstServerSocket;
	int secondServerSocket;
	int thirdServerSocket;
	int fourthServerSocket;
	char username[MAX_USERNAME];
	char *password;
} dfc;

void initDFC(const char *fileName, dfc *config);

void destroyDFC(dfc *config);

#endif //NETWORK_SYSTEMS_PA4_CLIENT_H
