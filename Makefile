all: chase-client

chase-client: src/chase-client.c src/chase.h
	gcc -Wall -pedantic src/chase-client.c -g -o chase-client -lncurses

clean:
	rm chase-client
