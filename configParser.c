//
// Created by jmalcy on 12/5/20.
//

#include "configParser.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

/**
 * Initialize a DFC
 * @param fileName	Name of the configuration file.
 * @param config	Pointer to the config to build
 */
int initDFC(const char *fileName, dfc *config) {
	FILE *file;
	char *lineBuffer, *savePoint, *option, *portPointer;
	int whichServer, numServers = 0, i, seenServers[4] = {0,0,0,0}, error;
	struct addrinfo hints, *results;
	ssize_t bytesRead;
	size_t bufferSize = 1024;
	bzero(&seenServers, 4);

	if ((file = fopen(fileName, "r")) == NULL) {
		fprintf(stderr, "Failed to open file %s\n", fileName);
		return numServers;
	}
	if ((lineBuffer = malloc(sizeof(char) * MAX_LINE)) == NULL) {
		fclose(file);
		fprintf(stderr, "Failed to allocate line buffer\n");
		return numServers;
	}

	bzero(config->username, MAX_USERNAME);
	bzero(config->password, MAX_PASSWORD);
	bzero(&hints, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	for(i = 0; i < 4; i++)
		config->serverInfo[i] = NULL;

	while ((bytesRead = getline(&lineBuffer, &bufferSize, file)) != -1) {
		trimSpace(lineBuffer);

		// non-empty lines and non-comment lines
		if (strlen(lineBuffer) != 0 && lineBuffer[0] != '#') {
			if (strncmp(lineBuffer, "Server", 6) == 0 && numServers < 4) {
				option = strtok_r(lineBuffer, "\t", &savePoint);
				option = strtok_r(NULL, "\t", &savePoint);

				// check DFS designation matches regex "DFS[0-9]"
				if (option != NULL && strlen(option) == 4 && strncmp(option, "DFS", 3) == 0 && isdigit(option[3])) {
					whichServer = (int)strtol(option + 3, NULL, 10);

					// determine which server this corresponds to
					if (!errno && seenServers[whichServer - 1] == 0 && whichServer >= 1 && whichServer <= 4) {
						whichServer--;
						option = strtok_r(NULL, "\t", &savePoint);

						// parse host:port
						if (option != NULL) {
							option = strtok_r(option, ":", &savePoint);
							portPointer = strtok_r(NULL, "\t", &savePoint);

							if (option != NULL && portPointer != NULL) {
								error = getaddrinfo(option, portPointer, &hints, &results);

								if (error == 0) {
									config->serverInfo[whichServer] = results;
									seenServers[whichServer] = 1;
									numServers++;
								}
								else {
									fprintf(stderr, "Hostname resolution failure: %s\n", gai_strerror(error));
									break;
								}
							} else {
								fprintf(stderr, "Failure parsing Server line\n");
								break;
							}
						} else {
							fprintf(stderr, "Failure parsing Server line\n");
							break;
						}
					} else if (errno != 0) {
						perror("Failure parsing hostname line");
						break;
					} else if (seenServers[whichServer - 1] == 1) {
						fprintf(stderr, "Duplicate server designation in %s\n", fileName);
						break;
					} else if (whichServer < 1 || whichServer > 4) {
						fprintf(stderr, "DFS designation %d out of range [1, 4]\n", whichServer);
						break;
					}
				} else {
					fprintf(stderr, "Malformed Server line.\n");
					break;
				}
			} else if (strncmp(lineBuffer, "Username", 8) == 0 && strlen(config->username) == 0) {
				option = strtok_r(lineBuffer, "\t", &savePoint);
				option = strtok_r(NULL, "\t", &savePoint);

				if (option != NULL) {
					strncpy(config->username, option, MAX_USERNAME);
				} else {
					fprintf(stderr, "Username option in file, but no username provided.\n");
					break;
				}
			} else if (strncmp(lineBuffer, "Password", 8) == 0 && strlen(config->password) == 0) {
				option = strtok_r(lineBuffer, "\t", &savePoint);
				option = strtok_r(NULL, "\t", &savePoint);

				if (option != NULL) {
					strncpy(config->password, option, MAX_PASSWORD);
				} else {
					fprintf(stderr, "Password option in file, but no password provided.\n");
					break;
				}
			} else {
				fprintf(stderr, "Unrecognized option on line \"%s\"\n", lineBuffer);
				break;
			}
		}
	}
	fclose(file);
	free(lineBuffer);

	// error handling
	if (numServers != 4)
		for (i = 0; i < 4; i++)
			if (seenServers[i] == 0) {
				fprintf(stderr, "Missing designation for server #%d\n", i + 1);
				numServers = -1;
			}

	if (strlen(config->username) == 0) {
		fprintf(stderr, "No username provided\n");
		numServers = -1;
	}
	if (strlen(config->password) == 0) {
		fprintf(stderr, "No password provided\n");
		numServers = -1;
	}

	return numServers;
}

void destroyDFC(dfc *config) {
	int i;
	for (i = 0; i < 4; i++)
		freeaddrinfo(config->serverInfo[i]);
}

int parseDFS(const char *fileName, char *usernames, char *passwords) {
	return NULL;
}
