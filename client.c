//
// Created by jmalcy on 12/5/20.
//

#include "client.h"
#include <stdio.h>
#include <netdb.h>

/**
 * Initialize a DFC
 * @param fileName	Name of the configuration file.
 * @param config	Pointer to the config to build
 */
void initDFC(const char *fileName, dfc *config) {
	FILE *file;
	char lineBuffer[MAX_LINE];
	struct addrinfo hints, *results;

}

void destroyDFC(dfc *config) {

}
