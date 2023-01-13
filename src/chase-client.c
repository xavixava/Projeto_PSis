#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <arpa/inet.h>
#include "chase.h"

WINDOW * message_win;

//unused
void new_player (player_position_t * player, char c){
    player->x = WINDOW_SIZE/2;
    player->y = WINDOW_SIZE/2;
    player->c = c;
}

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
Create a socket to be able to comunicate with clients
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

player_position_t p1;

int main(int argc, char* argv[]){

	int fd, n;
	server_message sm;
	client_message cm;
	//struct sockaddr_in server_addr;
	//socklen_t server_addr_size = sizeof(struct sockaddr_in);
	int key;

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
    WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);
    keypad(my_win, true);
    	
	/* creates a window and draws a border */
	message_win = newwin(12, WINDOW_SIZE, WINDOW_SIZE, 0);
	box(message_win, 0 , 0);	
	wrefresh(message_win);
	
	sm.type=2;	
	do	// tries different characters until server accepts it, to avoid repeated characters
	{
        	mvwprintw(message_win, 1,1,"Choose ur character(a-z)");
        	wrefresh(message_win);	
    		
        	key = wgetch(my_win);
		if('a' <= tolower(key) && tolower(key) <= 'z') // only accepts characters from A-Z or a-z
		{
    			cm.type = 0;	// preparing connecting message
			cm.arg = 'c';
			cm.c = key;
        		
			mvwprintw(message_win, 2,1,"Selected %c", cm.c);
   
			n = write(fd, &cm, sizeof(client_message)); 	
			if(n == -1)	// no server is running 
			{
				printf("Server disconected\n");
				return 0;
			}
			n = read(fd, &sm, sizeof(server_message));
			if(n == -1)perror("Recv error(please press ctrl+C)");
			else if(sm.type==1)	// there are already 10 players
			{
        			mvwprintw(message_win, 1,1,"Server full");
				exit(0);
			}
		}
	}while(sm.type==2);
		
	int count = 0;
	werase(message_win);
	box(message_win, 0 , 0);	
	for(int i=0; i<MAX_PLAYERS; i++){ //update the screen
		if(sm.players[i].c!='\0' && sm.players[i].health_bar>0){
			draw_player(my_win, &sm.players[i], true);
			count++;
        		mvwprintw(message_win, count,1,"%c %d", sm.players[i].c, sm.players[i].health_bar);
		}
	}
	for(int i=0; i<MAX_PRIZES; i++){ //update the screen
		if(sm.prizes[i].c!='\0'){
			draw_player(my_win, &sm.prizes[i], true);
		}
	}
	for(int i=0; i<MAX_BOTS; i++){ //update the screen
		if(sm.bots[i].c!='\0'){
			draw_player(my_win, &sm.bots[i], true);
		}
	}
	wrefresh(message_win);	
	cm.type = 1;	
	while(key != 27 && key!= 'q'){ //awaits movement updates until disconnection or health_0
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

		}
		n = write(fd, &cm, sizeof(client_message));	
		if(n == -1)perror("Send error(please press ctrl+C)");
        	
		// clear screen so we can print new screen	

		for(int i=0; i<MAX_PLAYERS; i++){ 
			if(sm.players[i].c!='\0'){
				draw_player(my_win, &sm.players[i], false);
			}
		}
		for(int i=0; i<MAX_PRIZES; i++){ 
			//if(sm.prizes[i].c!='\0'){
				draw_player(my_win, &sm.prizes[i], false);
			//}
		}
		for(int i=0; i<MAX_BOTS; i++){ 
			if(sm.bots[i].c!='\0'){
				draw_player(my_win, &sm.bots[i], false);
			}
		}
		
		n = read(fd, &sm, sizeof(server_message));
		//if(n == -1)perror("Recv error(please press ctrl+C)");
		if(sm.type==3){
			werase(message_win);
			box(message_win, 0 , 0);	
			mvwprintw(message_win, 1,1,"HP - 0");
			mvwprintw(message_win, 2,1,"Exiting");
			wrefresh(message_win);	
			sleep(5);
			return 0;
		}
	
		count=0;	// update screen and players health
		werase(message_win);
		box(message_win, 0 , 0);	
		for(int i=0; i<MAX_PLAYERS; i++){ 
			if(sm.players[i].c!='\0' && sm.players[i].health_bar>0){
				draw_player(my_win, &sm.players[i], true);
				count++;
        			mvwprintw(message_win, count,1,"%c %d", sm.players[i].c, sm.players[i].health_bar);
			}
		}
		for(int i=0; i<MAX_PRIZES; i++){
				if(sm.prizes[i].c!='\0'){
				draw_player(my_win, &sm.prizes[i], true);
			}
		}
		for(int i=0; i<MAX_BOTS; i++){ 
			if(sm.bots[i].c!='\0'){
				draw_player(my_win, &sm.bots[i], true);
			}
		}
        	wrefresh(message_win);	
	}
	
	cm.type = 0; 
	cm.arg = 'd';
	n = write(fd, &cm, sizeof(client_message));	
	if(n == -1)//perror("Send error(please press ctrl+C)");
	{
		werase(message_win);
		box(message_win, 0 , 0);	
		mvwprintw(message_win, 2,1,"Exiting Game");
		wrefresh(message_win);	
		sleep(2);
		return 0;
	}	
	return 0;
}
