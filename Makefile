all: chase-client chase-server bot-client

chase-client: src/chase-client.c src/chase.h
	gcc -Wall -pedantic src/chase-client.c -g -o chase-client -lncurses

chase-server: src/chase-client.c src/chase.h
	gcc -Wall -pedantic src/server.c -g -o server -lncurses

bot-client: src/bot-client.c src/chase.h
	gcc -Wall -pedantic src/bot-client.c -g -o bot-client  -lncurses

clean:
	rm chase-client server 
