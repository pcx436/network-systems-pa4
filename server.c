//
// Created by jmalcy on 12/5/20.
//

#include "server.h"
#include "common.h"
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
	pthread_t *threadIDs;
	size_t threadCapacity = LISTENQ;

	if ((threadIDs = malloc(threadCapacity)) == NULL) {
		return 4;
	}

	pthread_mutex_t threadMutex;
	int numThreads = 0, i, numUsers = parseDFS("./dfs.conf", usernames, passwords, MAX_USERS), port, sockfd;
	int connnectionfd;
	threadArgs *tArgs;

	socklen_t socketSize;
	struct sockaddr_in clientAddr;

	if (numUsers < 0) {
		free(threadIDs);
		// free users
		for (i = 0; i < numUsers; i++) {
			free(usernames[i]);
			free(passwords[i]);
		}
		return 2;
	}

	pthread_mutex_init(&threadMutex, NULL);

	port = (int)strtol(argv[2], NULL, 10);
	if ((sockfd = makeSocket(port)) >= 0) {
		// loop until SIGINT
		while (!killed) {
			if ((connnectionfd = accept(sockfd, (struct sockaddr *)&clientAddr, &socketSize)) > 0) {
				tArgs = malloc(sizeof(threadArgs));
				tArgs->sockfd = connnectionfd;
				tArgs->numThreads = &numThreads;
				tArgs->dir = dir;
				tArgs->mutex = &threadMutex;
				tArgs->usernames;
				tArgs->passwords;
				tArgs->numUsers = numUsers;

				pthread_mutex_lock(&threadMutex);
				if (numThreads == threadCapacity) {
					threadCapacity *= 2;
					threadIDs = realloc(threadIDs, threadCapacity);
				}

				pthread_create(&threadIDs[numThreads++], NULL, connectionHandler, tArgs);
				pthread_mutex_unlock(&threadMutex);
			}
		}

	}

	// free users
	for (i = 0; i < numUsers; i++) {
		free(usernames[i]);
		free(passwords[i]);
	}
	pthread_mutex_destroy(&threadMutex);
	close(sockfd);

	for (i = 0; i < numThreads; i++)
		pthread_join(threadIDs[i], NULL);

	free(threadIDs);
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

void *connectionHandler(void *arguments) {
	threadArgs *tArgs = arguments;
	char buffer[MAX_BUFFER], *nameToken, *pwToken, *pointInRequest, *savePoint, *end;
	char *fileName = NULL;
	int checkedAuthorization = 0, authorized = 0, i, partDesignation = -1;
	size_t bytesReceived;

	while ((bytesReceived = recv(tArgs->sockfd, buffer, MAX_BUFFER, 0)) > 0 && (!checkedAuthorization || authorized)) {
		if (!checkedAuthorization) {
			checkedAuthorization = 1;
			nameToken = strtok_r(buffer, "\n", &savePoint);
			pwToken = strtok_r(NULL, "\n", &savePoint);

			if (nameToken != NULL && pwToken != NULL) {
				for (i = 0; i < tArgs->numUsers && !authorized; i++) {
					if (strcmp(nameToken, tArgs->usernames[i]) == 0 && strcmp(pwToken, tArgs->passwords[i]) == 0)
						authorized = 1;
				}
			}

			if (!authorized)
				send(tArgs->sockfd, invalidPasswordResponse, strlen(invalidPasswordResponse), 0);
		}
	}

	close(tArgs->sockfd);

	pthread_mutex_lock(tArgs->mutex);
	tArgs->numThreads--;
	pthread_mutex_unlock(tArgs->mutex);

	free(tArgs);
	return NULL;
}
