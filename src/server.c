#include <stdlib.h>
#include <ncurses.h>
#include<unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "chase.h"

#define MAX_PLAYERS 10

WINDOW * message_win;

typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;

typedef struct player_position_t{
    int x, y;
    char c;
    unsigned int health_bar;
} player_position_t;

void new_player (player_position_t * player, char c){
    player->x = WINDOW_SIZE/2;
    player->y = WINDOW_SIZE/2;
    player->c = c;
}

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

void moove_player (player_position_t * player, char direction){
    if (direction == 'u'){
        if (player->y != 1){
            player->y --;
        }
    }
    if (direction == 'd'){
        if (player->y != WINDOW_SIZE-2){
            player->y ++;
        }
    }
    

    if (direction == 'l'){
        if (player->x != 1){
            player->x --;
        }
    }
    if (direction == 'r')
        if (player->x != WINDOW_SIZE-2){
            player->x ++;
    }
}

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

        struct sockaddr_un local_addr;
        local_addr.sun_family = AF_UNIX;
        strcpy(local_addr.sun_path, SOCK_ADDRESS);

        unlink(SOCK_ADDRESS);
        int err = bind(sock_fd, (struct sockaddr *)&local_addr,
                                                        sizeof(local_addr));
        if(err == -1) {
                perror("bind");
                exit(-1);
        }
	return sock_fd;
}

int search_player(player_position_t vector[], char c)
{
	int i;
	for(i=0; i<MAX_PLAYERS; i++)if(vector[i].c==c) return i;
	return -1;
}

int main(){

	int fd, i, n, player_count=0;
	struct sockaddr_un client_addr;
        socklen_t client_addr_size = sizeof(struct sockaddr_un);
	ball_message m;
	player_position_t players[10];

	for(i=0; i<MAX_PLAYERS; i++) players[i].c = '\0';
	
	fd = create_socket();

	initscr();		    	/* Start curses mode 		*/
	cbreak();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);   /* We get F1, F2 etc..		*/
	noecho();			    /* Don't echo() while we do getch */

	/* creates a window and draws a border */
	WINDOW * my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
	box(my_win, 0 , 0);	
	wrefresh(my_win);
    
	/* creates a window and draws a border */
	message_win = newwin(15, WINDOW_SIZE, WINDOW_SIZE, 0);
	box(message_win, 0 , 0);	
	wrefresh(message_win);
	
    	while(1){
	
		n = recvfrom(fd, &m, sizeof(ball_message), 0, ( struct sockaddr *)&client_addr, &client_addr_size);
		if(n == -1)perror("recv error(please press ctrl+C)");
			
		mvwprintw(message_win, 1,1,"Received: %d %c %c", m.type, m.arg, m.c);

		switch (m.type)
		{
			case 0:
				if(m.arg == 'c')
				{
					if (player_count == MAX_PLAYERS)
					{
						m.arg = 'd'; 
						n = sendto(fd, &m, sizeof(ball_message), 0, (const struct sockaddr *) &client_addr, client_addr_size);	
					}	
					else if(search_player(players, m.c)==-1) // non-repeated character
					{	
						i=0;
						while(players[i].c!='\0' && i<MAX_PLAYERS) i++;
						players[i].x = WINDOW_SIZE/2;	// todo: add random position
						players[i].y = WINDOW_SIZE/2;
						players[i].c = m.c;
						players[i].health_bar = 10;
						mvwprintw(message_win, 2,1,"player %c joined", m.c);
						n = sendto(fd, &m, sizeof(ball_message), 0, (const struct sockaddr *) &client_addr, client_addr_size);	
						if(n==-1)perror("sendto error");
						//todo: check if any bot or player is in the place where we started it 
            					draw_player(my_win, &players[i], true);
					}
					else	// refuse character
					{
						m.type = 2;
						n = sendto(fd, &m, sizeof(ball_message), 0, (const struct sockaddr *) &client_addr, client_addr_size);	
						if(n==-1)perror("sendto error");
					}	
				}
				else if (m.arg == 'd')	// disconect client
				{
					i = search_player(players, m.c);
					if(i==-1)  mvwprintw(message_win, 2,1,"Char %c not found.", m.c);
					else 
					{
            					draw_player(my_win, &players[i], false);
						mvwprintw(message_win, 2,1,"player %c disconected", m.c);
						players[i].c = '\0';
					}
				}
				else mvwprintw(message_win, 2,1,"Message poorly formatted.");
			break;
			case 1:
				i = search_player(players, m.c);
				if(i==-1)  mvwprintw(message_win, 2,1,"Char %c not found.", m.c);
				else
				{
            				draw_player(my_win, &players[i], false);
            				moove_player (&players[i], m.arg);
            				draw_player(my_win, &players[i], true);
					mvwprintw(message_win, 2,1,"Player %c moved %c.", m.c, m.arg);
				}			
			break;
		}
		wrefresh(message_win);
	}

    exit(0);
}
