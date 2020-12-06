//
// Created by jmalcy on 12/5/20.
//

#include "common.h"
#include <ctype.h>
#include <string.h>

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
