#include "chase.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>


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

int generate_prize()
{
	return (random()%4)+1;
}

int main(int argc, char **argv)
{	
	int fd, n = 0, i, prize=0;
	client_message cm;
	struct sockaddr_un server_addr;

	srand(time(NULL));

	server_addr.sun_family = AF_UNIX;
       	strcpy(server_addr.sun_path, SOCK_ADDRESS);	

	fd = create_socket();	
	cm.type = 2;
	cm.arg = '0' + prize;
	cm.c = '0' + prize;	
	
	for(i=0; i<5; i++)
	{
		prize = generate_prize();	
		cm.arg = '0' + prize;
		n = -1;
		sleep(5);
		while(n==-1) n = sendto(fd, &cm, sizeof(client_message), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));	
	}

    	while (1)
    	{
        	sleep(5);	
		prize = generate_prize();	
		cm.arg = '0' + prize;
		n = sendto(fd, &cm, sizeof(client_message), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));	
		if(n==-1)
		{
			printf("Server disconected\n");
			break;
		}
	}
		

	return 0;
}
