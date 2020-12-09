//
// Created by jmalcy on 12/5/20.
//

#include "server.h"
#include <stdio.h>

int main(int argc, const char *argv[]) {
	// check for invalid arguments
	if (argc != 2) {
		fprintf(stderr, "ERROR: incorrect number of arguments.\n");
		printf("Usage: %s [PORT]\n", argv[0]);
		return 1;
	}

	return 0;
}