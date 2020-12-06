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
	distributedFile *files;
	size_t fileCapacity;
	char fullCommand[MAX_COMMAND], *tokenSave, *param;
	int exit = 0, numFiles, anyMissing;
	int i, j;

	if (argc != 2) {
		fprintf(stderr, "Incorrect number of arguments.\n");
		printf("Usage: %s [configuration file]\n", argv[0]);
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
			if ((files = malloc(sizeof(distributedFile) * START_LIST_FILES)) == NULL) {
#ifdef DEBUG
				fprintf(stderr, "Failed to allocate room for list buffer\n");
#else
				fprintf(stderr, "An issue occured trying to list the files, sorry!\n");
#endif
			}
			else {
				fileCapacity = START_LIST_FILES;
				numFiles = list(config, files, &fileCapacity);

				// print list
				for (i = 0; i < numFiles; i++) {
					anyMissing = 0;
					printf("%s", files[i].name);
					for (j = 0; j < 4 && anyMissing == 0; j++)
						if (files[i].parts[j] == 0)
							anyMissing = 1;

					if (anyMissing == 1)
						printf(" [incomplete]");
					printf("\n");

					free(files[i].name);
				}
				free(files);
			}
		} else if (strncmp("put ", fullCommand, 4) == 0) {
		} else if (strncmp("get ", fullCommand, 4) == 0) {
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

int makeSocket(struct addrinfo *info) {
	int sockfd;
	struct timeval t;
	t.tv_sec = 1;
	t.tv_usec = 0;

	// create socket and set timeout
	if ((sockfd = socket(info->ai_family, SOCK_STREAM, 0)) != -1) {
		if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t, sizeof(t)) == -1 ||
				setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&t, sizeof(t)) == -1) {
#ifdef DEBUG
			perror("Socket setup failed");
#endif
			close(sockfd);
			sockfd = -1;
		} else {
			// attempt connection
			if (connect(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
#ifdef DEBUG
				perror("ERROR");
#endif
				close(sockfd);
				sockfd = -2;
			}
		}
	} else {
#ifdef DEBUG
		perror("Socket setup failed");
#endif
		sockfd = -3;
	}

	return sockfd;
}

int * pingServers(dfc config) {
	int i, socket, *online, error;
	char *pingCommand;
	char response[MAX_BUFFER];
	// 7 = 2 \n + 1 \0 + "ping"
	size_t pingSize = strlen(config.username) + strlen(config.password) + 7, ioSize;

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
	sprintf(pingCommand, "%s\n%s\nping", config.username, config.password);

	for (i = 0; i < 4; i++) {
		if ((socket = makeSocket(config.serverInfo[i])) != -1) {

			if ((ioSize = send(socket, pingCommand, strlen(pingCommand), 0)) > 0) {
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

int list(dfc config, distributedFile *files, size_t *capacity) {
	int i, j, socket, numFiles = 0,
	fileDesignation, fileIndex;
	size_t querySize = strlen(config.username) + strlen(config.password) + 7;
	char *query, response[MAX_BUFFER], *line, *lineSavePoint, *designationPoint, *dotPoint;

	if ((query = malloc(sizeof(char) * querySize)) == NULL)
		return -2;

	// build query
	sprintf(query, "%s\n%s\nlist", config.username, config.password);

	for (i = 0; i < 4; i++) {
		if ((socket = makeSocket(config.serverInfo[i])) != -1) {
			if (send(socket, query, querySize, 0) != -1) {
				// response format: "NAME1.n\nNAME2.n\nNAME3.n\nNAME4.n"
				while (recv(socket, response, MAX_BUFFER, 0) > 0 && files != NULL) {
					for (line = strtok_r(response, "\n", &lineSavePoint); line != NULL && files != NULL; line = strtok_r(NULL, "\n", &lineSavePoint)) {
						trimSpace(line);
						if (strlen(line) == 0)
							continue;

						// grab the last character
						designationPoint = line + strlen(line) - 1;
						dotPoint = line + strlen(line) - 2;

						fileDesignation = (int)strtol(designationPoint, NULL, 10);

						// temporarily null the ".n" at the end to get the full file name
						dotPoint[0] = '\0';

						for (j = 0, fileIndex = -1; j < numFiles && fileIndex == -1; j++) {
							if (strcmp(files[j].name, line) == 0) {
								files[j].parts[fileDesignation - 1] = 1;
								fileIndex = j;
							}
						}

						if (fileIndex == -1) {
							// resize list if necessary
							if (numFiles == *capacity) {
								*capacity *= 2;
								if ((files = realloc(files, *capacity)) == NULL) {
									perror("Failed to reallocate list of files");
								}
							}

							if (files != NULL && (files[numFiles].name = malloc(strlen(line) + 1)) != NULL) {
								strcpy(files[numFiles].name, line);
								bzero(files[numFiles].parts, 4);

								files[numFiles++].parts[fileDesignation - 1] = 1;
							}
						}
						// de-null the ".n" in case it's needed for the strtok_r
						dotPoint[0] = '.';
					}
				}
			}
			close(socket);
		}
	}

	free(query);
	return numFiles;
}

void *get(dfc config, const char *fileName) {
	// response format "[Part\nNumBytes\nDATA][Part\nNumBytes\nDATA]"
	// (no [] transmitted, used to show separation of parts)
	int i, socketIndex, sizeIndex, whichFile, socket;
	FILE *file;
	void *parts[4];
	char *query, *pointInResponse, responseBuffer[MAX_BUFFER];
	size_t partSize[4], currentSize[4];  // partSize = total # bytes of part, currentSize = bytes received so far
	ssize_t bytesReceived, toCopy, remainder;
	size_t queryLength = strlen(config.username) + strlen(config.password);
	queryLength += 4 + strlen(fileName);  // "get [fileName]"
	queryLength += 2;  // newlines
	queryLength += 1;  // null byte

	if ((query = malloc(queryLength)) == NULL)
		return NULL;

	// build query
	sprintf(query, "%s\n%s\nget %s", config.username, config.password, fileName);

	// init partSize and parts arrays
	for (i = 0; i < 4; i++) {
		partSize[i] = -1;
		currentSize[i] = 0;
		parts[i] = NULL;
	}

	// loop through all DFS to grab file parts
	for (socketIndex = 0; socketIndex < 4; socketIndex++) {
		if ((socket = makeSocket(config.serverInfo[socketIndex])) >= 0) {
			if (send(socket, query, queryLength, 0) != -1) {
				while ((bytesReceived = recv(socket, responseBuffer, MAX_BUFFER, 0)) > 0) {
					// set pointInResponse to start of buffer
					pointInResponse = responseBuffer;

					// determine if currently receiving file info or at start of info block
					for (sizeIndex = 0, whichFile = -1; sizeIndex < 4 && whichFile == -1; sizeIndex++) {
						if (partSize[sizeIndex] != -1 && currentSize[sizeIndex] < partSize[sizeIndex])
							whichFile = sizeIndex;
					}

					// all parts seen so far have been totally received, time to read new block
					if (whichFile == -1) {
						// TODO: Handle new block case
					} else {  // continuing receive of part from last loop iteration
						// determine if current file takes up entire received buffer or only part
						remainder = partSize[whichFile] - currentSize[whichFile];
						if (bytesReceived <= remainder)
							toCopy = bytesReceived;
						else
							toCopy = remainder;

						// "append" received bytes to the end of the part in question
						memcpy(parts[whichFile] + currentSize[whichFile], responseBuffer, toCopy);
						currentSize[whichFile] += toCopy;

						// shift pointInResponse in reaction to copy
						pointInResponse += toCopy;
					}
					}
				}
			}

			close(socket);
		}
	}

	return NULL;
}
