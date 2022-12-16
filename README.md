# Chase-Them Game

It is a project assigned by the Systems Programming subject. It implements a simple game where balls move in a field, trying to chase other balls and pick prizes.

## Required knowledge

Since you might have more than 1 player, it creates the need to use threads. Unix sockets are used to comunicate between client and server.

## Get Started 

```
#generate both client and server binaries
make

#run client and server in different windows
./chase-client
./server

#bot-client and prize-generator will be needed for the game to function properly
./bot-client
./bot-client /tmp/server_sock <number of bots>

#or use one of the predetermined modes
make easy-run
make hard-run

```
