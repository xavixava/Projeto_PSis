all: chase-client chase-server

chase-client: src/chase-client.c src/chase.h
	gcc -Wall -pedantic src/chase-client.c -g -o chase-client -lncurses

chase-server: src/chase-client.c src/chase.h
	gcc -Wall -pedantic src/server.c -g -o server -lncurses

clean:
	rm chase-client server 
