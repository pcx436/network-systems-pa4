//
// Created by jmalcy on 12/5/20.
//

#include "client.h"
#include "common.h"
#include "configParser.h"
#include <stdio.h>

int main(int argc, const char *argv[]) {
	dfc config;
	int numServers;
	char fullCommand[MAX_COMMAND], *tokenSave, *commandWord, *commandOption;
	int exit = 0;

	if (argc != 2) {
		fprintf(stderr, "Incorrect number of arguments.\n");
		printf("Usage: %s [configuration file]", argv[0]);
		return 1;
	}

	// failure to load dfc.conf
	if ((numServers = initDFC(argv[1], &config)) != 4)
		return 1;

	// display intro
	printf("Welcome to Jacob's Distributed File System Client!\n");
	displayHelp();

	while (exit == 0) {
		printf("> ");
		fgets(fullCommand, MAX_COMMAND, stdin);
		
		// TODO: remove this once I have the command parsing in. It's only here to shut my editor up
		exit = 1;
	}

	return 0;
}

void displayHelp() {
	printf("Your available commands are:\n");
	printf("\tget [FILE]: retrieves a file from the DFS\n\t"
		"put [FILE]: sends a file to the DFS\n\t"
		"list: lists all available files in the DFS\n\thelp: display this message\n\texit: terminates the DFC\n");
}
