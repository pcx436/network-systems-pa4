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

	// failure to load dfc.conf
	if ((numServers = initDFC(argv[1], &config)) != 4)
		return 1;

	// display intro
	printf("Welcome to Jacob's Distributed File System Client! Your available commands are:\n");
	printf("\tget [FILE]: retrieves a file from the DFS\n\tput [FILE]: sends a file to the DFS\n\t"
		"list: lists all available files in the DFS\n\texit: terminates the DFC\n");

	do {

	} while ();

	return 0;
}

void sigintHandler(int useless) { killed = 1; }
