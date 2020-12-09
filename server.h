//
// Created by jmalcy on 12/5/20.
//

#ifndef NETWORK_SYSTEMS_PA4_SERVER_H
#define NETWORK_SYSTEMS_PA4_SERVER_H

#define MAX_USERS   512
#define LISTENQ     1024

void *connectionHandler(void *arguments);
int makeSocket(int port);
void handler(int useless);

static volatile int killed = 0;
#endif //NETWORK_SYSTEMS_PA4_SERVER_H
