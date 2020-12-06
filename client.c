//
// Created by jmalcy on 12/5/20.
//

#include "client.h"
#include "common.h"
#include "configParser.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
// TODO: Implement AES encryption?

int main(int argc, const char *argv[]) {
	dfc config;
	char fullCommand[MAX_COMMAND], *tokenSave, *param;
	int exit = 0, *serverPing;

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
			serverPing = pingServers(&config);
			printf("%d servers online\n", countOnes(serverPing));
			free(serverPing);
		} else if (strncmp("put ", fullCommand, 4) == 0) {
			serverPing = pingServers(&config);
			printf("%d servers online\n", countOnes(serverPing));
			free(serverPing);
		} else if (strncmp("get ", fullCommand, 4) == 0) {
			serverPing = pingServers(&config);
			printf("%d servers online\n", countOnes(serverPing));
			free(serverPing);
		} else if (strncmp("help", fullCommand, 4) == 0) {
			displayHelp();
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

int * pingServers(dfc *config) {
	int i, socket, *online, error;
	char *pingCommand;
	char response[1024];
	// 7 = 2 \n + 1 \0 + "ping"
	size_t pingSize = strlen(config->username) + strlen(config->password) + 7, ioSize;

	if (config == NULL)
		return NULL;
	if ((online = malloc(sizeof(int) * 4)) == NULL)
		return NULL;

	// default 0 offline, 1 if online
	for (i = 0; i < 4; i++)
		online[i] = 0;

	if ((pingCommand = malloc(sizeof(char) * pingSize)) == NULL) {
		free(online);
		return NULL;
	}
	// build ping command
	sprintf(pingCommand, "%s\n%s\nping", config->username, config->password);

	for (i = 0; i < 4; i++) {
		if ((socket = makeSocket(config->serverInfo[i]->ai_family)) != -1) {
			error = connect(socket, config->serverInfo[i]->ai_addr, config->serverInfo[i]->ai_addrlen);

			if (error == 0 && (ioSize = send(socket, pingCommand, strlen(pingCommand), 0)) != -1) {
				ioSize = recv(socket, response, 1024, 0);
				if (strncmp("pong", response, 4) == 0)
					online[i] = 1;
			}
			close(socket);
		} else {
			fprintf(stderr, "failed to ping servers!\n");
			i = 5;  // break
		}
	}

	free(pingCommand);
	return online;
}

int countOnes(const int *online) {
	int c = 0, i;
	if (online == NULL)
		return -1;

	for (i = 0; i < 4; i++)
		if (online[i] == 1)
			c++;

	return c;
}
