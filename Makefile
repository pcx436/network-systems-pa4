CC=gcc
CFLAGS=-I. -std=c99

default: dfc dfs

dfc:
	$(CC) -o dfc client.* configParser.c common.c macro.h -lssl -lcrypto

dfs:
	$(CC) -o dfs server.* common.c configParser.c macro.h -lpthread

clean:
	rm *.o
	rm dfs
	rm dfc
