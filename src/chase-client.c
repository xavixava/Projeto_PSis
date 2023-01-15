#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "chase.h"

WINDOW * message_win;
WINDOW * my_win;
int n, fd, k;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Draws players in correct position if delete is true
 * If delete if false the player will be deleted
 */
void draw_player(WINDOW *win, player_position_t * player, int delete){
    int ch;
    if(delete){
        ch = player->c;
    }else{
        ch = ' ';
    }
    int p_x = player->x;
    int p_y = player->y;
    wmove(win, p_y, p_x);
    waddch(win,ch);
    wrefresh(win);
}

void moove_player (player_position_t * player, int direction){
    if (direction == KEY_UP){
        if (player->y  != 1){
            player->y --;
        }
    }
    if (direction == KEY_DOWN){
        if (player->y  != WINDOW_SIZE-2){
            player->y ++;
        }
    }
    

    if (direction == KEY_LEFT){
        if (player->x  != 1){
            player->x --;
        }
    }
    if (direction == KEY_RIGHT)
        if (player->x  != WINDOW_SIZE-2){
            player->x ++;
    }
}

/*
Create a socket to be able to comunicate with server
*/
int create_socket(char *ip, int port){

	int sock_fd;
	struct sockaddr_in address;
	
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ip);
	address.sin_port = htons(port);
	
	int conn_status = connect(sock_fd, (struct sockaddr *)&address, sizeof(address));
    
	// Check for connection error
    if (conn_status < 0) {
        perror("Error\n");
		    exit(0);
    }

	return sock_fd;
}

void *wait_ten(void *arg)
{
	int i;
	for(i=0; i<10; i++)
	{
		pthread_mutex_lock(&lock);
		if(k == 1) pthread_exit(NULL);
  		mvwprintw(message_win, 5,1,"k:%d", k);
		pthread_mutex_unlock(&lock);
		werase(message_win);
		box(message_win, 0 , 0);	
  		mvwprintw(message_win, 1,1,"HP - 0");
		mvwprintw(message_win, 2,1, "\"q\"-quit");
		mvwprintw(message_win, 3,1,"\"c\"-continue");
		mvwprintw(message_win, 4,1,"%d seconds left", 10-i);
		wrefresh(message_win);
		sleep(1);	
	}
	pthread_mutex_lock(&lock);
	if(k == 1) pthread_exit(NULL);
	pthread_mutex_unlock(&lock);
	mvwprintw(message_win, 2,1,"Exiting");
	wrefresh(message_win);	
	sleep(1);
	close(fd);
	exit(0);
}

void *recv_update(void *arg)
{
	server_message *sm = arg;
	int count;
	int n;
	pthread_t id;
	
	k=0;
	
	while(1)
	{	
		n = read(fd, sm, sizeof(server_message));
		// todo: verify read
		if(n==0)
		{
			mvwprintw(message_win, 1, 1, "Disconnected from server.");
			sleep(2);
			exit(0);
		}
		
		if(sm->type==3){
			//todo: now health_0 doesn't do that
        		pthread_mutex_lock(&lock);
			k = -1;
        		pthread_mutex_unlock(&lock);
			pthread_create(&id, NULL, wait_ten, NULL);
			pthread_join(id, NULL);
		}
		count=0;	// update screen and players health

		//Clears screens of old information
		werase(my_win);
		werase(message_win);
		box(message_win, 0 , 0);	
		box(my_win, 0 , 0);	

		for(int i=0; i<MAX_PLAYERS; i++){ 	//update the players on screen
			if(sm->players[i].c!='\0' && sm->players[i].health_bar>0){
				draw_player(my_win, &sm->players[i], true);
				count++;
				mvwprintw(message_win, count+1, 1, "%c %d", sm->players[i].c, sm->players[i].health_bar);
			}
		}
		for(int i=0; i<MAX_PRIZES; i++){	//update the prizes on screen
			if(sm->prizes[i].c!='\0'){
				draw_player(my_win, &sm->prizes[i], true);
			}
		}
		for(int i=0; i<MAX_BOTS; i++){ 		//update the bots on screen
			if(sm->bots[i].c!='\0'){
				draw_player(my_win, &sm->bots[i], true);
			}
		}
		wrefresh(message_win);	
		wrefresh(my_win);	
	}
	return NULL;
}


int main(int argc, char* argv[]){

	server_message sm;
	client_message cm;
	int key, count = 0;
	pthread_t id;

	if(argc != 3)
	{
		printf("./src/chase-client.c <ip> <port>\n");
		exit(0);
	}
	
	// todo: still lacks ip and port verification
	fd = create_socket(argv[1], atoi(argv[2]));
	
	initscr();		    	/* Start curses mode 		*/
	cbreak();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);   /* We get F1, F2 etc..		*/
	noecho();			    /* Don't echo() while we do getch */

	/* creates a window and draws a border */
	my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
	box(my_win, 0 , 0);	
	wrefresh(my_win);
	keypad(my_win, true);
    	
	/* creates a window and draws a border */
	message_win = newwin(12, WINDOW_SIZE, WINDOW_SIZE, 0);
	box(message_win, 0 , 0);	
	wrefresh(message_win);

	// prepares and sends connecting message
	cm.type = 0;	
	cm.arg = 'c';

	n = write(fd, &cm, sizeof(client_message)); 	
	// todo: add verification	
	if(n == -1)	// no server is running 
	{
		printf("Server disconected\n");
		return 0;
	}

	mvwprintw(message_win, 1,1,"Connecting [%d %c]", cm.type, cm.arg);
	wrefresh(message_win);	

	n = read(fd, &sm, sizeof(server_message));
	// todo: add verification
	if(n == -1)perror("Recv error(please press ctrl+C)");
	else if(sm.type==1)	// field is full
	{
		mvwprintw(message_win, 1,1,"Server full");
		exit(0);
	}
	else if(sm.type==0){ // player was accepted
		mvwprintw(message_win, 1,1,"Conn Successful");
		wrefresh(message_win);	
	}

	werase(message_win);
	box(message_win, 0 , 0);	
	for(int i=0; i<MAX_PLAYERS; i++){	//update the players on screen
		if(sm.players[i].c!='\0' && sm.players[i].health_bar>0){
			draw_player(my_win, &sm.players[i], true);
			count++;
			mvwprintw(message_win, count+1, 1, "%c %d", sm.players[i].c, sm.players[i].health_bar);
		}
	}
	for(int i=0; i<MAX_PRIZES; i++){ 	//update the prizes on screen
		if(sm.prizes[i].c!='\0'){
			draw_player(my_win, &sm.prizes[i], true);
		}
	}
	for(int i=0; i<MAX_BOTS; i++){ 		//update the bots on screen
		if(sm.bots[i].c!='\0'){
			draw_player(my_win, &sm.bots[i], true);
		}
	}

	cm.type = 1;
	pthread_create(&id, NULL, recv_update, &sm); 
		
	while(key != 27 && key!= 'q'){ //awaits movement updates until disconnection or health_0
		cm.type = 1;
        	key = wgetch(my_win);
        	switch(key)
		{
			case KEY_LEFT:
            			cm.arg = 'l';
			break;
			
			case KEY_RIGHT:
            			cm.arg = 'r';
			break;
			
			case KEY_UP:
            			cm.arg = 'u';
			break;
			
			case KEY_DOWN:
            			cm.arg = 'd';
			break;
			case 'c':
				pthread_mutex_lock(&lock);
				if(k==-1)
				{
					mvwprintw(message_win, 6,1,"Continuing");
					k = 1;
					cm.type = 2;	
					n = write(fd, &cm, sizeof(client_message));	
				}
				pthread_mutex_unlock(&lock);
			break;
		}
		pthread_mutex_unlock(&lock);	
		if(k!=-1) n = write(fd, &cm, sizeof(client_message));	
		pthread_mutex_unlock(&lock);
		if(n == -1)perror("Send error(please press ctrl+C)");
        	
		// mvwprintw(message_win, 1,1,"Message sent: %d %c", cm.type, cm.arg);
		wrefresh(message_win);	
	}
	
	// prepares and sends disconnecting message
	cm.type = 0; 
	cm.arg = 'd';

	n = write(fd, &cm, sizeof(client_message)); 	
	// todo: add verification	
	if(n == -1)
	{
		werase(message_win);
		box(message_win, 0 , 0);	
		mvwprintw(message_win, 1,1,"Exiting Game");
		wrefresh(message_win);	
	}else
	{
		mvwprintw(message_win, 1,1,"Disconnecting[%d %c]", cm.type, cm.arg);
		wrefresh(message_win);	
	}

	sleep(2);	
	close(fd);
	sleep(2);	
	return 0;
}
