//
// Created by jmalcy on 12/5/20.
//

#ifndef NETWORK_SYSTEMS_PA4_SERVER_H
#define NETWORK_SYSTEMS_PA4_SERVER_H
#include <pthread.h>

#define MAX_USERS   512
#define LISTENQ     1024

typedef struct {
	int sockfd;
	int numUsers;
	int *numThreads;
	const char *dir;
	char **usernames;
	char **passwords;
	pthread_mutex_t *mutex;
} threadArgs;

void *connectionHandler(void *arguments);
int makeSocket(int port);
void handler(int useless);
int list(threadArgs tArgs, int userIndex);
int receiveGet(threadArgs tArgs, int userIndex, char *fileName);
int receivePut(threadArgs tArgs, int userIndex, char *fileName);

static volatile int killed = 0;
#endif //NETWORK_SYSTEMS_PA4_SERVER_H
