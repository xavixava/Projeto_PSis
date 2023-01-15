# Chase-Them Game

It is a project assigned by the Systems Programming subject. It implements a simple game where balls move in a field, trying to chase other balls and pick prizes.

Project by:

> Xavier Antunes, 96343

> Tiago Espadinha, 96329

## Required knowledge

Since you might have more than 1 player, it creates the need to use threads. Inet sockets are used to comunicate between client and server.

## Get Started 

```
#generate both client and server binaries
make

#run client and server in different windows
#the programs necessary for client and server are in different directories
./client/chase-client <server_ip> <server_port>
./server/server <server_port>


```
