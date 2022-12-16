all: chase-client chase-server bot-client prize-generator

chase-client: src/chase-client.c src/chase.h
	gcc -Wall -pedantic src/chase-client.c -g -o client/chase-client -lncurses

chase-server: src/chase-client.c src/chase.h
	gcc -Wall -pedantic src/server.c -g -o server/server -lncurses

bot-client: src/bot-client.c src/chase.h
	gcc -Wall -pedantic src/bot-client.c -g -o server/bot-client 

prize-generator: src/prize_generator.c src/chase.h
	gcc -Wall -pedantic src/prize_generator.c -g -o server/prize-generator

hard-run:
	./server/bot-client /tmp/server_sock 10 &
	./server/prize-generator &
	./server/server

easy-run:
	./server/bot-client /tmp/server_sock 1 &
	./server/prize-generator &
	./server/server

clean:
	rm client/chase-client server/server server/bot-client server/prize-generator 
