//
// Created by jmalcy on 12/5/20.
//

#include "client.h"
#include "configParser.h"
#include <stdio.h>

static volatile int killed = 0;

int main(int argc, const char *argv[]) {
	dfc config;
	int numServers;
	char command[MAX_COMMAND];

	if (argc != 2) {
		fprintf(stderr, "Incorrect number of arguments.\n");
		printf("Usage: %s [configuration file]", argv[0]);
		return 1;
	}
	if ((numServers = initDFC(argv[1], &config)) != 4)
		return 1;
	

	return 0;
}

void sigintHandler(int useless) { killed = 1; }
