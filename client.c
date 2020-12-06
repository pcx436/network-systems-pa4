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
	int i, socketIndex, sizeIndex, whichFile, partDesignation = -1, socket;
	FILE *file;
	void *parts[4];
	char *query, *pointInResponse, responseBuffer[MAX_BUFFER], *token, *end, partsOfSize[MAX_BUFFER];
	size_t partSize[4], currentSize[4];  // partSize = total # bytes of part, currentSize = bytes received so far
	ssize_t bytesReceived, toCopy, remainder, parsedSize, skipCount = 0;
	size_t queryLength = strlen(config.username) + strlen(config.password);
	queryLength += 4 + strlen(fileName);  // "get [fileName]"
	queryLength += 2;  // newlines
	queryLength += 1;  // null byte

	if ((query = malloc(queryLength)) == NULL)
		return NULL;
	bzero(partsOfSize, MAX_BUFFER);

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
				// FIXME: adapt to malformed responses
				// FIXME: currently assuming that when there is an info block, we can see the whole thing.
				while ((bytesReceived = recv(socket, responseBuffer, MAX_BUFFER, 0)) > 0) {
					// set pointInResponse to start of buffer
					pointInResponse = responseBuffer;

					// keep track of end pointer
					end = responseBuffer + bytesReceived;


					// continuing receive of part from last loop iteration
					// still bytes left, we're not supposed to skip them, and we know which part to put them in
					if (pointInResponse < end && skipCount == 0 && partDesignation != -1) {
					}

					// still data left in buffer, begin parsing as new info block
					while (pointInResponse < end) {
						// in case we've already seen this part, skip it
						if (skipCount > 0 && skipCount <= bytesReceived) {
							pointInResponse += skipCount;
							skipCount = 0;
							partDesignation = -1;  // skipped finished
						}
						else if (skipCount > 0) {
							skipCount -= bytesReceived;
							pointInResponse += bytesReceived;
						}
						else if ((partDesignation == -1 || partSize[partDesignation] == -1) && pointInResponse[0] == '\n') {
							// skip newline
							pointInResponse += 1;
						}
						else if (partDesignation == -1) {
							partDesignation = (int)strtol(pointInResponse, NULL, 10) - 1;
							pointInResponse += 2;  // skip designation and newline
							bytesReceived -= 2;
						}
						else if (partSize[partDesignation] == -1) {
							// found the end of the part size
							if ((token = strchr(pointInResponse, '\n')) != NULL) {
								token[0] = '\0';
								strcat(partsOfSize, pointInResponse);

								parsedSize = strtol(partsOfSize, NULL, 10);
								bzero(partsOfSize, MAX_BUFFER);  // wipe for next block
								bytesReceived -= strlen(pointInResponse) + 1;  // "file size" + \n
								token[0] = '\n';

								// start at data chunk
								pointInResponse = token + 1;

								// new part
								if (partSize[partDesignation] == -1) {
									skipCount = 0;
									partSize[partDesignation] = parsedSize;

									// if there is a higher power, I beg of them to never let this condition be true
									if ((parts[partDesignation] = malloc(parsedSize)) == NULL) {
										perror("Failed allocating space for file part");
										partSize[partDesignation] = -1;
										partDesignation = -1;
									}
								} else {  // seen part, skip it
									skipCount = parsedSize;
								}
							}
							else {
								strcat(partsOfSize, pointInResponse);
								pointInResponse = end;
							}
						}
						else {  // partDesignation != -1, partSize[partDesignation] != -1, not skipping
							// determine if current file takes up entire received buffer or only part
							remainder = partSize[partDesignation] - currentSize[partDesignation];
							if (bytesReceived <= remainder)
								toCopy = bytesReceived;
							else
								toCopy = remainder;

							// "append" received bytes to the end of the part in question
							memcpy(parts[partDesignation] + currentSize[partDesignation], responseBuffer, toCopy);
							currentSize[partDesignation] += toCopy;

							if (toCopy == remainder)
								partDesignation = -1;

							// shift pointInResponse in reaction to copy
							pointInResponse += toCopy;
							bytesReceived -= toCopy;
						}
					}
				}
			}
			close(socket);
		}

	}

	free(query);
	free(partsOfSize);
	return NULL;
}
