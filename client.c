//
// Created by jmalcy on 12/5/20.
//

#include "client.h"
#include "configParser.h"
#include <stdio.h>

int main(int argc, const char *argv[]) {
	dfc config;
	int numServers;

	if (argc != 2) {
		fprintf(stderr, "Incorrect number of arguments.\n");
		printf("Usage: %s [configuration file]", argv[0]);
		return 1;
	}
	numServers = initDFC(argv[1], &config);

	return 0;
}
