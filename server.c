//
// Created by jmalcy on 12/5/20.
//

#include "server.h"
#include "common.h"
#include "configParser.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

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

	if (mkdir(dir, 0700) == -1 && errno != EEXIST) {
		perror("Failed directory creation");
		return 1;
	}

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
				tArgs->usernames = usernames;
				tArgs->passwords = passwords;
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

	return sockfd;
}

void handler(int useless) { killed = 1; }

void *connectionHandler(void *arguments) {
	threadArgs *tArgs = arguments;
	char buffer[MAX_BUFFER], *nameToken, *pwToken, *pointInRequest, *savePoint, *end, *endlToken;
	int checkedAuthorization = 0, userIndex = -1, i, partDesignation = -1, fullName = 0;
	size_t bytesReceived;

	while ((bytesReceived = recv(tArgs->sockfd, buffer, MAX_BUFFER, 0)) > 0 && (!checkedAuthorization || userIndex != -1)) {
		pointInRequest = buffer;
		end = buffer + bytesReceived;

		// FIXME: what if it takes multiple buffer recvs to get the whole command
		while (pointInRequest < end) {
			if (!checkedAuthorization) {
				checkedAuthorization = 1;
				nameToken = strtok_r(buffer, "\n", &savePoint);
				pwToken = strtok_r(NULL, "\n", &savePoint);

				if (nameToken != NULL && pwToken != NULL) {
					for (i = 0; i < tArgs->numUsers && userIndex == -1; i++) {
						if (strcmp(nameToken, tArgs->usernames[i]) == 0 && strcmp(pwToken, tArgs->passwords[i]) == 0)
							userIndex = i;
					}
				}
				if (userIndex == -1) {
					send(tArgs->sockfd, invalidPasswordResponse, strlen(invalidPasswordResponse), 0);
					pointInRequest = end;  // break inner loop
				}
				else {
					pointInRequest = strtok_r(NULL, "\n", &savePoint);  // jump past password
				}
			}
			else if (strncmp(pointInRequest, "list", 4) == 0) {
				// list functionality called
				list(*tArgs, userIndex);
				pointInRequest = end;
			}
			else if (strncmp(pointInRequest, "get ", 4) == 0) {
				// get functionality called
				printf("Get called with file %s\n", pointInRequest + 4);
				receiveGet(*tArgs, userIndex, pointInRequest + 4);
				pointInRequest = end;
			}
			else if (strncmp(pointInRequest, "put ", 4) == 0) {  // b
				// put functionality called
				pointInRequest += 4;
				bytesReceived -= 4;
				printf("Put called with file %s\n", pointInRequest);
				receivePut(*tArgs, userIndex, pointInRequest);

				pointInRequest = end;
			}
			else {
				send(tArgs->sockfd, queryFailure, strlen(queryFailure), 0);
				pointInRequest = end;
			}
		}
	}

	close(tArgs->sockfd);

	pthread_mutex_lock(tArgs->mutex);
	tArgs->numThreads--;
	pthread_mutex_unlock(tArgs->mutex);

	free(tArgs);
	return NULL;
}

int list(threadArgs tArgs, int userIndex) {
	DIR *dirptr;
	char fileName[PATH_MAX];
	struct dirent *ent;
	struct stat st = {0};

	bzero(fileName, PATH_MAX);
	sprintf(fileName, "%s/%s", tArgs.dir, tArgs.usernames[userIndex]);

	if (stat(fileName, &st) == -1 && mkdir(fileName, 0700) == -1) {
		perror("Failed to create user directory");
		return -1;
	}
	if ((dirptr = opendir(fileName)) != NULL) {  // succeeded in opening directory
		while ((ent = readdir(dirptr)) != NULL) {
			if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
				continue;

			bzero(fileName, PATH_MAX);
			sprintf(fileName, "%s\n", ent->d_name);
			send(tArgs.sockfd, fileName, strlen(fileName), 0);
		}
		closedir(dirptr);
	}
	else {
		perror("List failure");
	}
	return 0;
}

/**
 * receive GET command from the client
 * @param tArgs
 * @param userIndex
 * @return
 */
int receiveGet(threadArgs tArgs, int userIndex, char *fileName) {
	if (fileName == NULL)
		return -1;

	char nameBuff[PATH_MAX], buffer[MAX_BUFFER];
	FILE *file;
	int i, returnValue = 0;
	size_t bytesRead;

	// init foundParts
	for(i = 0; i < 4; i++) {
		bzero(nameBuff, PATH_MAX);
		sprintf(nameBuff, "%s/%s/%s.%d", tArgs.dir, tArgs.usernames[userIndex], fileName, i);

		if ((file = fopen(nameBuff, "r")) != NULL) {
			fseek(file, 0, SEEK_END);
			bzero(buffer, MAX_BUFFER);
			sprintf(buffer, "%d\n%ld\n", i, ftell(file));
			fseek(file, 0, SEEK_SET);

			send(tArgs.sockfd, buffer, strlen(buffer), 0);
			while ((bytesRead = fread(buffer, sizeof(char), MAX_BUFFER, file)) > 0) {
				if (send(tArgs.sockfd, buffer, bytesRead, 0) == -1) {
					bzero(buffer, MAX_BUFFER);
					sprintf(buffer, "Error sending chunk data for %s", nameBuff);
					perror(buffer);
				}
			}
			fclose(file);
		}
		else {
			bzero(buffer, MAX_BUFFER);
			sprintf(buffer, "Failed to open file %s", nameBuff);
			perror(buffer);
		}
	}

	return returnValue;
}

int receivePut(threadArgs tArgs, int userIndex, char *fileName) {
	char buffer[MAX_BUFFER], *pointInRequest, *end, *token, sizeBuffer[MAX_BUFFER],
	// 5 = [dir]/[uname]/[fname].partNum\0
	nameBuffer[strlen(tArgs.dir) + strlen(tArgs.usernames[userIndex]) + strlen(fileName) + 5];
	int i, partDesignation = -1;
	size_t bytesReceived, currentPartSize = 0, remainder, writeResult, bytesWritten = 0, toWrite;
	FILE *file = NULL;
	bzero(sizeBuffer, MAX_BUFFER);

	// send ready response
	send(tArgs.sockfd, ready, strlen(ready), 0);

	while ((bytesReceived = recv(tArgs.sockfd, buffer, MAX_BUFFER, 0)) > 0) {
		// set pointInRequest to start of buffer
		pointInRequest = buffer;

		// keep track of end pointer
		end = buffer + bytesReceived;

		// still data left in buffer
		while (pointInRequest < end) {
			if ((partDesignation == -1 || currentPartSize == 0) && pointInRequest[0] == '\n') {
				// skip newline
				pointInRequest += 1;
				bytesReceived--;
			}
			else if (partDesignation == -1) {  // current character is the buffer designation
				if (isdigit(pointInRequest[0])) {
					partDesignation = (int)strtol(pointInRequest, NULL, 10);
					pointInRequest += 2;  // skip designation and newline
					bytesReceived -= 2;
					sprintf(nameBuffer, "%s/%s/%s.%d", tArgs.dir, tArgs.usernames[userIndex], fileName, partDesignation);

					if ((file = fopen(nameBuffer, "w")) == NULL) {
						pointInRequest = end;
						perror(nameBuffer);
					}
				}
				else {
					fprintf(stderr, "Invalid file part designation received.\n");
					pointInRequest = end;
				}
			}
			else if (currentPartSize == 0) {
				// found the end of the part size
				if ((token = strchr(pointInRequest, '\n')) != NULL) {
					token[0] = '\0';
					strcat(sizeBuffer, pointInRequest);

					currentPartSize = strtol(sizeBuffer, NULL, 10);
					bzero(sizeBuffer, MAX_BUFFER);  // wipe for next block
					bytesReceived -= strlen(pointInRequest) + 1;  // "file size" + \n
					token[0] = '\n';

					// start at data chunk
					pointInRequest = token + 1;
				}
				else {  // have yet to see end of part size specification
					strcat(sizeBuffer, pointInRequest);
					pointInRequest = end;
				}
			}
			else {  // partDesignation != -1, partSize[partDesignation] != 0, not skipping
				// determine if current file takes up entire received buffer or only part
				remainder = currentPartSize - bytesWritten;
				if (remainder <= bytesReceived)
					toWrite = remainder;
				else
					toWrite = bytesReceived;

				// "append" received bytes to the end of the part in question
				if ((writeResult = fwrite(pointInRequest, sizeof(char), toWrite, file)) <= 0) {
					perror(nameBuffer);
					pointInRequest = end;
					fclose(file);
				}

				bytesWritten += writeResult;
				if (bytesWritten == currentPartSize) {
					partDesignation = -1;
					currentPartSize = 0;
					bytesWritten = 0;
					fclose(file);
					file = NULL;
				}

				// shift pointInRequest in reaction to copy
				pointInRequest += writeResult;
				bytesReceived -= writeResult;
			}
		}
	}

	return 0;
}
