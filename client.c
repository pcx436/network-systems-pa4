//
// Created by jmalcy on 12/5/20.
//

#include "client.h"
#include "common.h"
#include "configParser.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
// TODO: Implement AES encryption?

int main(int argc, const char *argv[]) {
	dfc config;
	char fullCommand[MAX_COMMAND], *tokenSave, *param;
	int exit = 0;

	if (argc != 2) {
		fprintf(stderr, "Incorrect number of arguments.\n");
		printf("Usage: %s [configuration file]", argv[0]);
		return 1;
	}

	// failure to load dfc.conf
	if (initDFC(argv[1], &config) != 4)
		return 1;

	// display intro
	printf("Welcome to Jacob's Distributed File System Client!\n");
	displayHelp();

	while (exit == 0) {
		printf("> ");
		fgets(fullCommand, MAX_COMMAND, stdin);
		trimSpace(fullCommand);

		if (strncmp("list", fullCommand, 4) == 0) {

		} else if (strncmp("put ", fullCommand, 4) == 0) {

		} else if (strncmp("get ", fullCommand, 4) == 0) {

		} else if (strncmp("help", fullCommand, 4) == 0) {

		} else if (strncmp("exit", fullCommand, 4) == 0) {
			exit = 1;
		} else {
			fprintf(stderr, "Unknown command \"%s\", enter \"help\" to see available commands.\n", fullCommand);
		}
	}

	destroyDFC(&config);
	return 0;
}

void displayHelp() {
	printf("Your available commands are:\n");
	printf("\tget [FILE]: retrieves a file from the DFS\n\t"
		"put [FILE]: sends a file to the DFS\n\t"
		"list: lists all available files in the DFS\n\thelp: display this message\n\texit: terminates the DFC\n");
}

int makeSocket(int family) {
	int sockfd;
	struct timeval t;
	t.tv_sec = 1;
	t.tv_usec = 0;

	// create socket and set timeout
	if ((sockfd = socket(family, SOCK_STREAM, 0)) != -1) {
		if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t, sizeof(t)) == -1 ||
				setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&t, sizeof(t)) == -1) {
			perror("Socket setup failed");
			close(sockfd);
			sockfd = -1;
		}
	} else {
		perror("Socket setup failed");
	}

	return sockfd;
}
