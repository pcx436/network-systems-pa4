//
// Created by jmalcy on 12/5/20.
//

#include "server.h"
#include "configParser.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

int main(int argc, const char *argv[]) {
	signal(SIGINT, handler);
	// check for invalid arguments
	if (argc != 3) {
		fprintf(stderr, "ERROR: incorrect number of arguments.\n");
		printf("Usage: %s [DIRECTORY] [PORT]\n", argv[0]);
		return 1;
	}

	char *usernames[MAX_USERS], *passwords[MAX_USERS];
	const char *dir = argv[1];
	pthread_t threadIDs[LISTENQ];
	pthread_mutex_t threadMutex;
	int numThreads = 0, i, numUsers = parseDFS("./dfs.conf", usernames, passwords, MAX_USERS), port, sockfd;
	int connnectionfd;

	socklen_t socketSize;
	struct sockaddr_in clientAddr;

	if (numUsers < 0)
		return 2;

	pthread_mutex_init(&threadMutex, NULL);

	port = (int)strtol(argv[2], NULL, 10);
	if ((sockfd = makeSocket(port)) >= 0) {

	}

	// free users
	for (i = 0; i < numUsers; i++) {
		free(usernames[i]);
		free(passwords[i]);
	}
	pthread_mutex_destroy(&threadMutex);
	close(sockfd);
	return 0;
}

int makeSocket(int port) {
	int sockfd, opt = 1, flags;
	struct sockaddr_in addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(int)) < 0) {
		close(sockfd);
		return -1;
	}

	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((unsigned short) port);

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		close(sockfd);
		return -1;
	}
	if ((flags = fcntl(sockfd, F_GETFL, 0)) < 0) {
		close(sockfd);
		return -1;
	}
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
		close(sockfd);
		return -1;
	}

	if (listen(sockfd, LISTENQ) < 0) {
		close(sockfd);
		return -1;
	}

	return 0;
}

void handler(int useless) { killed = 1; }
