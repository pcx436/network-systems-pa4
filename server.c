//
// Created by jmalcy on 12/5/20.
//

#include "server.h"
#include <stdio.h>

int main(int argc, const char *argv[]) {
	signal(SIGINT, handler);
	// check for invalid arguments
	if (argc != 3) {
		fprintf(stderr, "ERROR: incorrect number of arguments.\n");
		printf("Usage: %s [DIRECTORY] [PORT]\n", argv[0]);
		return 1;
	}

	return 0;
}

void handler(int useless) { killed = 1; }
