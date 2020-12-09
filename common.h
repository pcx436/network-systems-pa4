//
// Created by jmalcy on 12/5/20.
//

#ifndef NETWORK_SYSTEMS_PA4_COMMON_H
#define NETWORK_SYSTEMS_PA4_COMMON_H
#include <stddef.h>

typedef struct {
	int parts[4];
	char *name;
} distributedFile;

void trimSpace(char *s);
unsigned mod_big(const unsigned char *num, size_t size, unsigned divisor);

static const char *invalidPasswordResponse = "Invalid Username/Password. Please try again.";
static const char *queryFailure = "Invalid query.";
static const char *ready = "PUT_READY";

#endif //NETWORK_SYSTEMS_PA4_COMMON_H
