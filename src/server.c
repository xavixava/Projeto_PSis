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

void moove_player (player_position_t * player, int direction){
    if (direction == UP){
        if (player->y  != 1){
            player->y --;
        }
    }
    if (direction == DOWN){
        if (player->y  != WINDOW_SIZE-2){
            player->y ++;
        }
    }
    

    if (direction == LEFT){
        if (player->x  != 1){
            player->x --;
        }
    }
    if (direction == RIGHT)
        if (player->x  != WINDOW_SIZE-2){
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
	for(i=0; i<MAX_PLAYERS; i++)if(vector[i]==c) return i;
	return -1;
}

int main(){

	int fd, i, n, player_count=0;
	struct sockaddr_un client_addr;
        socklen_t client_addr_size = sizeof(struct sockaddr_un);
	ball_message m;
	player_position_t players[10];

	for(i=0; i<MAX_PLAYERS; i++) players.c = '\0';
	
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
			
		mvwprintw(message_win, 1,1,"Received: %c %c %c", m.type, m.arg, m.c);

		switch (m.type)
		{
			case '0':
				if(m.arg == 'c')
				{
					if(search_player(players, m.c)==-1)
					{	
						i=0;
						while(players[i]!='\0' && i<MAX_PLAYERS) i++;
						players[i].x = WINDOW_SIZE/2;
						players[i].y = WINDOW_SIZE/2;
						players[i].c = m.c;
						players[i].health_bar = 10;
					}
					else
					{
						//repeated character
					}	
				}
				else if (m.arg == 'd')
				{
					//disconect client
				}
				else mvwprintw(message_win, 2,1,"Message poorly formatted.", m.type, m.arg, m.c);
		}
		wrefresh(message_win);
	}

    exit(0);
}
