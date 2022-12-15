#include <stdlib.h>
#include <ncurses.h>
#include<unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

#include "chase.h"

#define MAX_PLAYERS 10

WINDOW * message_win;

typedef struct player_position_t{
    int x, y;
    char c;
    int health_bar;
} player_position_t;

void update_health(player_position_t * player, int collision_type){
	if(collision_type==-2){ //hit player
		player->health_bar++;
	}else if(collision_type==-1){ //got hit by player
		player->health_bar--;
	}else if(collision_type==0){ //got hit by bot
		player->health_bar--;
	}else if(collision_type>0&&collision_type<=5){ //hit prize	
		player->health_bar+=collision_type;
	}
	if (player->health_bar<=0){ //min health is 0
		player->health_bar=0;
	}
	else 
	if (player->health_bar>10){ //max health is 10
		player->health_bar=10;
	}
}


/*Checks if there's a player/bot in the current player position, 
returns the player/bot position in the array in case of colision*/
int check_collision (player_position_t * players_bots, int num_players, int curr_player){

	for(int i=0; i<num_players; i++){
		if (i!=curr_player){
			if(players_bots[curr_player].x==players_bots[i].x && players_bots[curr_player].y==players_bots[i].y){
				return i;
			}
		}
	}
	return -1;
}

void new_player (player_position_t * players,  int num_players, int curr_player, char c){
    
	srand(time(NULL));
	do{
		players[curr_player].x = (rand() % (WINDOW_SIZE-2)) + 1;
		players[curr_player].y = (rand() % (WINDOW_SIZE-2)) + 1;
	}while (check_collision(players, num_players, curr_player)!=-1);
	
    players[curr_player].c = c;
	players[curr_player].health_bar = 10;
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

void clear_hp_changes(int vector[])
{
	int i;
	for(i=0; i<MAX_PLAYERS; i++) vector[i]=0;
}



int main(){

	int fd, i, n, player_count=0, hp_changes[MAX_PLAYERS];
	int temp_x, temp_y, rammed_player;//, rammed_bot;
	struct sockaddr_un client_addr;
        socklen_t client_addr_size = sizeof(struct sockaddr_un);
	client_message cm;
	server_message sm;
	player_position_t players[10];

	srand(time(NULL));
	
	for(i=0; i<MAX_PLAYERS; i++) players[i].c = '\0';
	clear_hp_changes(hp_changes);

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
	
		n = recvfrom(fd, &cm, sizeof(client_message), 0, ( struct sockaddr *)&client_addr, &client_addr_size);
		if(n == -1)perror("recv error(please press ctrl+C)");
			
		mvwprintw(message_win, 1,1,"Received: %d %c %c", cm.type, cm.arg, cm.c);

		switch (cm.type)
		{
			case 0:
				if(cm.arg == 'c')
				{
					if (player_count == MAX_PLAYERS)
					{		
						sm.type = 2;
						// sm.c = '\0';	
						n = sendto(fd, &sm, sizeof(server_message), 0, (const struct sockaddr *) &client_addr, client_addr_size);	
					}	
					else if(search_player(players, cm.c)==-1) // non-repeated character
					{	
						i=0;
						while(players[i].c!='\0' && i<MAX_PLAYERS) i++;
						new_player (players,  player_count, i, cm.c);
						/*players[i].x = rand() % WINDOW_SIZE;	// todo: add random position
						players[i].y = rand() % WINDOW_SIZE;
						players[i].c = cm.c;
						players[i].health_bar = 10;*/
						mvwprintw(message_win, 2,1,"player %c joined", cm.c);
						
						sm.type = 0;
						// sm.c = players[i].c;
						sm.x = players[i].x;
						sm.y = players[i].y;

						n = sendto(fd, &sm, sizeof(server_message), 0, (const struct sockaddr *) &client_addr, client_addr_size);
						if(n==-1)perror("sendto error");
						//todo: check if any bot or player is in the place where we started it 
            					draw_player(my_win, &players[i], true);
						player_count++;
					}
					else	// refuse character
					{
						sm.type = 2;
						// sm.c = cm.c;
						n = sendto(fd, &sm, sizeof(server_message), 0, (const struct sockaddr *) &client_addr, client_addr_size);
						if(n==-1)perror("sendto error");
					}	
				}
				else if (cm.arg == 'd')	// disconect client
				{
					i = search_player(players, cm.c);
					if(i==-1)  mvwprintw(message_win, 2,1,"Char %c not found.", cm.c);
					else 
					{
            					draw_player(my_win, &players[i], false);
						mvwprintw(message_win, 2,1,"player %c disconected", cm.c);
						players[i].c = '\0';
						player_count--;
					}
				}
				else mvwprintw(message_win, 2,1,"Message poorly formatted.");
			break;
			case 1:
				i = search_player(players, cm.c);
				if(i==-1) mvwprintw(message_win, 2,1,"Char %c not found.", cm.c);
				else
				{
					temp_x=players[i].x;
					temp_y=players[i].y;
					draw_player(my_win, &players[i], false);
					moove_player (&players[i], cm.arg);
					rammed_player=check_collision(players, player_count, i);
					//rammed_bot=check_collision(bots, num_bots, curr_player)M
					if(rammed_player!=-1){
						players[i].x=temp_x;
						players[i].y=temp_y;
						update_health(&players[i], -2);
						update_health(&players[rammed_player], -1);
					//}else if(ramed_bot!=-1){
					//	players[i].x=temp_x;
					//	players[i].y=temp_y;
					}else{ 
					//	if(check_prize!=-1){
					//		prize_val=remove_prize(check_prize);
					//		update_health(&players[i].health_bar, prize_val);
					//	}
					}
					draw_player(my_win, &players[i], true);
					
					/*if (players[i].health_bar==0){
						draw_player(my_win, &players[i], false);
						mvwprintw(message_win, 2,1,"player %c disconected", cm.c);
						players[i].c = '\0';
						player_count--;
					}else{*/

					mvwprintw(message_win, 2,1,"Player %c moved %c", cm.c, cm.arg);
					
					sm.type = 3;
					sm.health = players[i].health_bar;
					sm.x = players[i].x;
					sm.y = players[i].y;
					sm.elements=0;	
					//}
					

					n = sendto(fd, &sm, sizeof(server_message), 0, (const struct sockaddr *) &client_addr, client_addr_size);
					if(n==-1)perror("sendto error");
				}
			break;
		}
		wrefresh(message_win);
	}

    exit(0);
}
