//
// Created by jmalcy on 12/5/20.
//

#include "common.h"
#include <ctype.h>
#include <string.h>
#include <limits.h>

void trimSpace(char *s) {
	int i, begin = 0, end = strlen(s) - 1;
	while (isspace(s[end]))
		end--;
	s[end + 1] = '\0';

	while (isspace(s[begin]))
		begin++;

	for (i = 0; i < end - 1; i++)
		s[i] = s[i + begin];
}

/**
 * Modulo of unsigned char array
 * Source: https://stackoverflow.com/a/38572628
 * @param num 		Array to get modulus of
 * @param size  	Size of the provided array
 * @param divisor	What to divide the array by.
 * @return
 */
unsigned mod_big(const unsigned char *num, size_t size, unsigned divisor) {
	unsigned rem = 0;
	// Assume num[0] is the most significant
	while (size-- > 0) {
		// Use math done at a width wider than `divisor`
		rem = ((UCHAR_MAX + 1ULL)*rem + *num) % divisor;
		num++;
	}
	return rem;
}
