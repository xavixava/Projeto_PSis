#include "chase.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;


/*
Create a socket to be able to comunicate with clients
*/

int create_socket()
{
	int sock_fd;
        sock_fd= socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sock_fd == -1){
                perror("socket: ");
                exit(-1);
        }

        struct sockaddr_un local_client_addr;
        local_client_addr.sun_family = AF_UNIX;
        sprintf(local_client_addr.sun_path, "/tmp/bot_sock_%d", getpid());

        unlink(local_client_addr.sun_path);
        int err = bind(sock_fd, (struct sockaddr *)&local_client_addr,
                                                        sizeof(local_client_addr));
        if(err == -1) {
                perror("bind");
                exit(-1);
        }

	return sock_fd;
}

int main(int argc, char **argv)
{	
	int fd, n = 0, i;
	client_message cm;
	struct sockaddr_un server_addr;
	direction_t direction;

	srand(time(NULL));

	if(argc != 3)	// cli arguments weren't correct
	{
		printf("./bot <server address> <number of bots\n");
		exit(0);
	}
	else
	{
		server_addr.sun_family = AF_UNIX;
        	strcpy(server_addr.sun_path, argv[1]);	
	}

	const int bot_count = atoi(argv[2]) <= 10 ? atoi(argv[2])-1 : 9; //reads number of bots from cli
	printf("%d\n", bot_count);
	char bot_vector[bot_count];   

	fd = create_socket();
	
	cm.type = 0;
	cm.arg = 'b';
	cm.c = '0' + bot_count;	

	do	// sends connecting message to server
	{
		n = sendto(fd, &cm, sizeof(client_message), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));	
	}while(n==-1);

    	cm.type=1;
	cm.arg = 'b';
    	while (1)
    	{
        	sleep(3);
		for(i=0; i < bot_count+1; i++)
		{
        		direction = random()%4;
        		n++;
        		switch (direction)
        		{
        			case LEFT:
        	   		//printf("%d Going Left   \n", n);
        	   		bot_vector[i]='l'; 
		   		break;
        		case RIGHT:
        	    		//printf("%d Going Right   \n", n);
           			bot_vector[i]='r'; 
           		break;
        		case DOWN:
            			//printf("%d Going Down   \n", n);
           			bot_vector[i]='d'; 
           		break;
        		case UP:
            			//printf("%d Going Up    \n", n);
           			bot_vector[i]='u'; 
            		break;
        		}
		}
		
	// this client sends 2 messages a first one to unlock the server, another with the directions
	n = sendto(fd, &cm, sizeof(client_message), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));	
	if(n == -1)
	{
		printf("bot: Disconected from server\n");
		break;	
	}
	
	n = sendto(fd, &bot_vector, bot_count+1, 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));	
	if(n == -1)
	{
		printf("bot: Disconected from server\n");
		break;	
	}
    }

 
	return 0;
}
