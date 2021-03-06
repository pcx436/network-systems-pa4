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
#include <ctype.h>
#include <openssl/md5.h>
// TODO: Implement AES encryption?

int main(int argc, const char *argv[]) {
	dfc config;
	distributedFile *files = NULL;
	size_t fileCapacity;
	char fullCommand[MAX_COMMAND], *tokenSave, *param, *fileName;
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
				}
				free(files);
			}
		} else if (strncmp("put ", fullCommand, 4) == 0) {
			if (strlen(fullCommand) > 4) {
				fileName = fullCommand + 4;
				trimSpace(fileName);
				put(config, fileName);
			}
			else {
				fprintf(stderr, "Please specify a file to retrieve.\n");
			}
		} else if (strncmp("get ", fullCommand, 4) == 0) {
			if (strlen(fullCommand) > 4) {
				fileName = fullCommand + 4;
				get(config, fileName);
			}
			else {
				fprintf(stderr, "Please specify a file to retrieve.\n");
			}
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
		if ((socket = makeSocket(config.serverInfo[i])) >= 0) {
			if (send(socket, query, querySize, 0) != -1) {
				// response format: "NAME1.n\nNAME2.n\nNAME3.n\nNAME4.n"
				while (recv(socket, response, MAX_BUFFER, 0) > 0 && files != NULL) {
					if (strcmp(response, invalidPasswordResponse) == 0) {
						fprintf(stderr, "%s\n", invalidPasswordResponse);
						break;
					}
					else if (strcmp(response, queryFailure) == 0) {
						fprintf(stderr, "%s\n", queryFailure);
						break;
					}

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
								files[j].parts[fileDesignation] = 1;
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

							if (files != NULL) {
								bzero(files[numFiles].name, PATH_MAX);
								strcpy(files[numFiles].name, line);
								for (j = 0; j < 4; j++)
									files[numFiles].parts[j] = 0;

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
	int i, socketIndex, partDesignation = -1, socket, justStarted = 1;
	int fileIsComplete = 1;
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
		partSize[i] = 0;
		currentSize[i] = 0;
		parts[i] = NULL;
	}

	// loop through all DFS to grab file parts
	for (socketIndex = 0; socketIndex < 4; socketIndex++) {
		if ((socket = makeSocket(config.serverInfo[socketIndex])) >= 0) {
			if (send(socket, query, queryLength, 0) != -1) {
				justStarted = 1;
				partDesignation = -1;
				skipCount = 0;
				// FIXME: adapt to malformed responses
				// FIXME: currently assuming that when there is an info block, we can see the whole thing.
				while ((bytesReceived = recv(socket, responseBuffer, MAX_BUFFER, 0)) > 0) {
					// check for login failure
					if (justStarted == 1 && strcmp(responseBuffer, invalidPasswordResponse) == 0) {
						fprintf(stderr, "%s\n", invalidPasswordResponse);
						break;
					}
					else if (justStarted == 1 && strcmp(queryFailure, responseBuffer) == 0) {
						fprintf(stderr, "%s\n", queryFailure);
						break;
					}
					else
						justStarted = 0;

					// set pointInResponse to start of buffer
					pointInResponse = responseBuffer;

					// keep track of end pointer
					end = responseBuffer + bytesReceived;

					// still data left in buffer
					while (pointInResponse < end) {
						// have seen a part, have to skip it
						if (skipCount > 0 && skipCount <= bytesReceived) {
							pointInResponse += skipCount;
							bytesReceived -= skipCount;
							skipCount = 0;
							partDesignation = -1;  // skipped finished
						}
						else if (skipCount > 0) {  // skipCount exceeds buffer limits
							skipCount -= bytesReceived;
							pointInResponse = end;
						}
						else if ((partDesignation == -1 || partSize[partDesignation] == 0) && pointInResponse[0] == '\n') {
							// skip newline
							pointInResponse += 1;
							bytesReceived--;
						}
						else if (partDesignation == -1) {  // current character is the buffer designation
							if (isdigit(pointInResponse[0])) {
								partDesignation = (int)strtol(pointInResponse, NULL, 10);
								pointInResponse += 2;  // skip designation and newline
								bytesReceived -= 2;
							}
							else {
								fprintf(stderr, "Invalid file part designation received.\n");
								pointInResponse = end;
							}
						}
						else if (partSize[partDesignation] == 0) {
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
								skipCount = 0;
								partSize[partDesignation] = parsedSize;

								// if there is a higher power, I beg of them to never let this condition be true
								if ((parts[partDesignation] = malloc(parsedSize)) == NULL) {
									perror("Failed allocating space for file part");
									partSize[partDesignation] = 0;
									partDesignation = -1;
									pointInResponse = end;
								}
							}
							else {  // have yet to see end of part size specification
								strcat(partsOfSize, pointInResponse);
								pointInResponse = end;
							}
						}
						else if (partSize[partDesignation] == currentSize[partDesignation]) {
							skipCount = partSize[partDesignation];
							token = strchr(pointInResponse, '\n');

							token[0] = '\0';
							bytesReceived -= strlen(pointInResponse) + 1;
							token[0] = '\n';

							pointInResponse = strchr(pointInResponse, '\n') + 1;  // skip past chunk size
						}
						else {  // partDesignation != -1, partSize[partDesignation] != 0, not skipping
							// determine if current file takes up entire received buffer or only part
							remainder = partSize[partDesignation] - currentSize[partDesignation];
							if (remainder <= bytesReceived)
								toCopy = remainder;
							else
								toCopy = bytesReceived;

							// "append" received bytes to the end of the part in question
							memcpy(parts[partDesignation] + currentSize[partDesignation], pointInResponse, toCopy);
							currentSize[partDesignation] += toCopy;

							if (toCopy == remainder || bytesReceived == remainder)
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

	// If we have all four parts, assemble the file and write to working directory
	for (i = 0; i < 4 && fileIsComplete == 1; i++)
		if (parts[i] == NULL)
			fileIsComplete = 0;

	if (fileIsComplete) {
		if ((file = fopen(fileName, "w")) != NULL) {
			for (i = 0; i < 4; i++)
				fwrite(parts[i], sizeof(char), partSize[i], file);
			fclose(file);

			printf("Successfully retrieved file \"%s\".\n", fileName);
		}
		else {
			perror("Successfully retrieved all file parts, but failed to open file for writing");
		}
	}
	else {
		printf("File is incomplete.");
	}

	for (i = 0; i < 4; i++)
		if (parts[i] != NULL)
			free(parts[i]);

	return NULL;
}

int put(dfc config, const char *fileName) {
	if (fileName == NULL)
		return -1;

	unsigned char hash[MD5_DIGEST_LENGTH];
	FILE *file;
	char readBuffer[MAX_BUFFER], *query;
	MD5_CTX context;
	size_t bytesRead, querySize, leftToSend, currentRead;
	ssize_t fileSize, partSize[4];
	int i, j, socket, partsToSend[2], x, offset;
	/*
	 * USERNAME\n
	 * PASSWORD\n
	 * put FILE\0
	 */
	querySize = strlen(config.username) + strlen(config.password) + strlen(fileName) + 7;

	if ((query = malloc(querySize)) == NULL)
		return -2;
	sprintf(query, "%s\n%s\nput %s", config.username, config.password, fileName);

	MD5_Init(&context);


	if ((file = fopen(fileName, "r")) != NULL) {
		// hash file
		while ((bytesRead = fread(readBuffer, sizeof(char), MAX_BUFFER, file)) != 0)
			MD5_Update(&context, readBuffer, bytesRead);
		MD5_Final(hash, &context);

		// get file size, return to beginning
		fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		offset = (int)mod_big(hash, MD5_DIGEST_LENGTH, 4);

		// parts 1-3 are of size dividedSize, part 4 is remainingSize
		partSize[0] = partSize[1] = partSize[2] = fileSize / 4;
		partSize[3] = fileSize - (partSize[0] * 3);

		// open socket loop time
		for (i = 0; i < 4; i++) {
			x = (i - offset) % 4;

			partsToSend[0] = x;
			partsToSend[1] = (x != 3) ? x + 1 : 0;

			if ((socket = makeSocket(config.serverInfo[i])) >= 0) {
#ifdef DEBUG
				printf("Copying parts %d and %d\n", partsToSend[0], partsToSend[1]);
#endif
				if (send(socket, query, querySize, 0) != -1) {
					bzero(readBuffer, MAX_BUFFER);
					bytesRead = recv(socket, readBuffer, MAX_BUFFER, 0);
					// successful authorization, send the data
					if (bytesRead > 0 && strcmp(ready, readBuffer) == 0) {
						for (j = 0; j < 2; j++) {
							// build part header
							bzero(readBuffer, MAX_BUFFER);
							sprintf(readBuffer, "%d\n%zd\n", partsToSend[j], partSize[partsToSend[j]]);

							// move to beginning of part
							fseek(file, partSize[0] * partsToSend[j], SEEK_SET);

							// send part header
							if (send(socket, readBuffer, strlen(readBuffer), 0) != -1) {
								for (leftToSend = partSize[partsToSend[j]]; leftToSend > 0; leftToSend -= bytesRead) {
									currentRead = (leftToSend < MAX_BUFFER) ? leftToSend : MAX_BUFFER;
									bytesRead = fread(readBuffer, sizeof(char), currentRead, file);
									send(socket, readBuffer, bytesRead, 0);
								}
							}
						}
					}
					else if (bytesRead > 0 && strcmp(invalidPasswordResponse, readBuffer) == 0)
						fprintf(stderr, "%s\n", invalidPasswordResponse);
					else if (bytesRead > 0 && strcmp(queryFailure, readBuffer) == 0)
						fprintf(stderr, "%s\n", queryFailure);
					else
						perror("Failure in send");
				}
				else {
					perror("Send failure");
				}
				close(socket);
			}
			else {
				perror("Socket failure");
			}
		}

		fclose(file);
	}
	else {
		return -3;
	}

	free(query);
	return 0;
}
