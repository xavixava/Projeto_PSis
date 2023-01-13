all: chase-client chase-server

chase-client: src/chase-client.c src/chase.h
	gcc -Wall -pedantic src/chase-client.c -g -o client/chase-client -lncurses -lpthread

chase-server: src/chase-client.c src/chase.h
	gcc -Wall -pedantic src/server.c src/queue.c src/queue.h -g -o server/server -lncurses -lpthread


hard-run:
	./server/server

easy-run:
	./server/server

clean:
	rm client/chase-client server/server 
