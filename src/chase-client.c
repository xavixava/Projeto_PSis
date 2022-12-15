#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>

#include "chase.h"

WINDOW * message_win;

typedef struct player_position_t{
    int x, y;
    char c;
    int health_bar;
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
        sprintf(local_client_addr.sun_path, "/tmp/client_sock_%d", getpid());

        unlink(local_client_addr.sun_path);
        int err = bind(sock_fd, (struct sockaddr *)&local_client_addr,
                                                        sizeof(local_client_addr));
        if(err == -1) {
                perror("bind");
                exit(-1);
        }

	return sock_fd;
}

player_position_t p1;

int main(){

	int fd, n;
	server_message sm;
	client_message cm;
	struct sockaddr_un server_addr;
	int key;

	fd = create_socket();

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
	message_win = newwin(5, WINDOW_SIZE, WINDOW_SIZE, 0);
	box(message_win, 0 , 0);	
	wrefresh(message_win);
	
	sm.type=2;	
	do
	{
        	mvwprintw(message_win, 1,1,"Choose ur character(a-z)");
        	wrefresh(message_win);	
    		
        	key = wgetch(my_win);
		if('a' <= tolower(key) && tolower(key) <= 'z')
		{
			new_player(&p1, key);
    			cm.type = 0;
			cm.arg = 'c';
			cm.c = key;

        		server_addr.sun_family = AF_UNIX;
        		strcpy(server_addr.sun_path, SOCK_ADDRESS);	
   
			n = sendto(fd, &cm, sizeof(client_message), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));	
			if(n == -1)perror("Sendto error(please press ctrl+C)");
	
			n = recv(fd, &sm, sizeof(server_message), 0);
			if(n == -1)perror("Recv error(please press ctrl+C)");
			else if(sm.type==1)
			{
        			mvwprintw(message_win, 1,1,"Server full");
				exit(0);
			}
		}
	}while(sm.type==2);
		
        mvwprintw(message_win, 1,1,"                                   ");
	box(message_win, 0 , 0);	
        wrefresh(message_win);	
	//for(i<player_count){
	p1.x = sm.x;
	p1.y = sm.y;
	draw_player(my_win, &p1, true);
	//}
	
	cm.type = 1;	
	while(key != 27 && key!= 'q'){
        	key = wgetch(my_win);
        	switch(key)
		{
			case KEY_LEFT:
            			cm.arg = 'l';
				draw_player(my_win, &p1, false);
            			moove_player (&p1, key);
            			draw_player(my_win, &p1, true);
			break;
			
			case KEY_RIGHT:
            			cm.arg = 'r';
            			draw_player(my_win, &p1, false);
            			moove_player (&p1, key);
            			draw_player(my_win, &p1, true);
			break;
			
			case KEY_UP:
            			cm.arg = 'u';
            			draw_player(my_win, &p1, false);
            			moove_player (&p1, key);
            			draw_player(my_win, &p1, true);
			break;
			
			case KEY_DOWN:
            			cm.arg = 'd';
            			draw_player(my_win, &p1, false);
            			moove_player (&p1, key);
            			draw_player(my_win, &p1, true);
			break;

		}
		n = sendto(fd, &cm, sizeof(client_message), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));	
		if(n == -1)perror("Send error(please press ctrl+C)");
        	
        	mvwprintw(message_win, 2,1,"%c key pressed", key);
		
		n = recv(fd, &sm, sizeof(server_message), 0);
		if(n == -1)perror("Recv error(please press ctrl+C)");
    	
		p1.x = sm.x;
		p1.y = sm.y;
		p1.health_bar = sm.health;
		//todo for (i<0; i<sm.elements; i++)
		
		mvwprintw(message_win, 1,1,"HP - %d", p1.health_bar);
		
        	wrefresh(message_win);	
	}

	cm.type = 0; 
	cm.arg = 'd';
	n = sendto(fd, &cm, sizeof(client_message), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));	
	if(n == -1)perror("Send error(please press ctrl+C)");
    	
	return 0;
}
