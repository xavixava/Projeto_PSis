all: chase-client chase-server bot-client prize-generator

chase-client: src/chase-client.c src/chase.h
	gcc -Wall -pedantic src/chase-client.c -g -o chase-client -lncurses

chase-server: src/chase-client.c src/chase.h
	gcc -Wall -pedantic src/server.c -g -o server -lncurses

bot-client: src/bot-client.c src/chase.h
	gcc -Wall -pedantic src/bot-client.c -g -o bot-client 

prize-generator: src/prize_generator.c src/chase.h
	gcc -Wall -pedantic src/prize_generator.c -g -o prize-generator

hard-run:
	./bot-client /tmp/server_sock 10 &
	./prize-generator &
	./server

easy-run:
	./bot-client /tmp/server_sock 1 &
	./prize-generator &
	./server

clean:
	rm chase-client server bot-client prize-generator 
