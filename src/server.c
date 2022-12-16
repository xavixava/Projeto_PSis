#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

#include "chase.h"


WINDOW * message_win;

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
		//player->health_bar=0;
	}
	else if (player->health_bar>10){ //max health is 10
		//player->health_bar=10;
	}
}


/*Checks if there's a player/bot in the current player position, 
returns the player/bot position in the array in case of colision*/
int check_collision (server_message * sm, int element_role, int array_pos){//element_role: 0 - player, 1 -bot, 2 prize
	switch (element_role)
	{
	case 0://player
		for(int i=0; i<MAX_PLAYERS; i++){
			if (array_pos!=i && sm->players[i].c!='\0'){
				if(sm->players[array_pos].x==sm->players[i].x && sm->players[array_pos].y==sm->players[i].y){
						return i;
				} 
			}
		}
		for(int i=0; i<MAX_PLAYERS; i++){
			if (sm->bots[i].c!='\0'){
				if(sm->players[array_pos].x==sm->bots[i].x && sm->players[array_pos].y==sm->bots[i].y){
					return -1;
				}
			}
		}
		for(int i=0; i<MAX_PLAYERS; i++){
			if (sm->prizes[i].c!='\0'){
				if(sm->players[array_pos].x==sm->prizes[i].x && sm->players[array_pos].y==sm->prizes[i].y){
					return i+MAX_PLAYERS;
				}
			}
		}
		return -2;
		break;

	case 1://bot
		for(int i=0; i<MAX_PLAYERS; i++){
			if (sm->players[i].c!='\0'){
				if(sm->bots[array_pos].x==sm->players[i].x && sm->bots[array_pos].y==sm->players[i].y){
						return i;
				}
			}
		}
		for(int i=0; i<MAX_PLAYERS; i++){
			if (array_pos!=i && sm->bots[i].c!='\0'){
				if(sm->bots[array_pos].x==sm->bots[i].x && sm->bots[array_pos].y==sm->bots[i].y){
					return -1;
				}
			}
		}
		for(int i=0; i<MAX_PLAYERS; i++){
			if (sm->prizes[i].c!='\0'){
				if(sm->bots[array_pos].x==sm->prizes[i].x && sm->bots[array_pos].y==sm->prizes[i].y){
					return i+MAX_PLAYERS;
				}
			}
		}
		return -2;
		break;

	case 2://prize
		for(int i=0; i<MAX_PLAYERS; i++){
			if (sm->players[i].c!='\0'){
				if(sm->prizes[array_pos].x==sm->players[i].x && sm->prizes[array_pos].y==sm->players[i].y){
						return i;
				}
			}
		}
		for(int i=0; i<MAX_PLAYERS; i++){
			if (sm->bots[i].c!='\0'){
				if(sm->prizes[array_pos].x==sm->bots[i].x && sm->prizes[array_pos].y==sm->bots[i].y){
					return -1;
				}
			}
		}
		for(int i=0; i<MAX_PLAYERS; i++){
			if (array_pos!=i && sm->prizes[i].c!='\0'){
				if(sm->prizes[array_pos].x==sm->prizes[i].x && sm->prizes[array_pos].y==sm->prizes[i].y){
					return i+MAX_PLAYERS;
				}
			}
		}
		return -2;
		break;

	default:
		return -3;
		break;
	}
}

void new_player (server_message * sm, int element_role, int array_pos, char c){ //todo check for bots/prizes
    
	srand(time(NULL));
	switch (element_role)
	{
	case 0:
		do{
			sm->players[array_pos].x = (rand() % (WINDOW_SIZE-2)) + 1;
			sm->players[array_pos].y = (rand() % (WINDOW_SIZE-2)) + 1;
		}while (check_collision(sm, element_role, array_pos)!=-2);
		sm->players[array_pos].c = c;
		sm->players[array_pos].health_bar = 10;
		break;
	
	case 1:
		do{
			sm->bots[array_pos].x = (rand() % (WINDOW_SIZE-2)) + 1;
			sm->bots[array_pos].y = (rand() % (WINDOW_SIZE-2)) + 1;
		}while (check_collision(sm, element_role, array_pos)!=-2);
		sm->bots[array_pos].c = '*';
		break;

	case 2:
		do{
			sm->prizes[array_pos].x = (rand() % (WINDOW_SIZE-2)) + 1;
			sm->prizes[array_pos].y = (rand() % (WINDOW_SIZE-2)) + 1;
		}while (check_collision(sm, element_role, array_pos)!=-2);
		sm->prizes[array_pos].c = c;
		sm->prizes[array_pos].health_bar = atoi(&c);
		break;

	default:
		break;
	}
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

	int fd, i,j,k, n, player_count=0, bot_count = 0, prize_count = 0;
	int temp_x, temp_y, rammed_player;//, prize_val;
	struct sockaddr_un client_addr;
        socklen_t client_addr_size = sizeof(struct sockaddr_un);
	client_message cm;
	server_message sm;
	player_position_t players[MAX_PLAYERS];
	char bot_message[MAX_PLAYERS];

	srand(time(NULL));
	

	for(i=0; i<MAX_PLAYERS; i++) sm.players[i].c = '\0'; //Inicializing sm.players array
	for(i=0; i<MAX_PRIZES; i++)
	{ sm.prizes[i].c = '\0';
		sm.prizes[i].x = 1;
		sm.prizes[i].y = 1;
	}
	for(i=0; i<MAX_BOTS; i++) sm.bots[i].c = '\0';

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
	message_win = newwin(10, 25, WINDOW_SIZE, 0);
	box(message_win, 0 , 0);	
	wrefresh(message_win);
	
    	while(1){
	
		n = recvfrom(fd, &cm, sizeof(client_message), 0, ( struct sockaddr *)&client_addr, &client_addr_size);
		if(n == -1)perror("recv error(please press ctrl+C)");
			
		mvwprintw(message_win, 1,1,"Received: %d %c %c ", cm.type, cm.arg, cm.c);

		switch (cm.type)
		{
			case 0: //Message about player's connection
				if(cm.arg == 'c')
				{
					if (player_count == MAX_PLAYERS) // field is full
					{		
						sm.type = 2;	
						n = sendto(fd, &sm, sizeof(server_message), 0, (const struct sockaddr *) &client_addr, client_addr_size);	
					}	
					else if(search_player(sm.players, cm.c)==-1) // accepted player
					{	
						i=0;
						while(sm.players[i].c!='\0' && i<MAX_PLAYERS) i++;
						new_player (&sm, 0, i, cm.c);
						mvwprintw(message_win, 2,1,"player %c joined", cm.c);
						
						sm.type = 0;
							
						mvwprintw(message_win, 3,1,"%s", client_addr.sun_path);
						
						n = sendto(fd, &sm, sizeof(server_message), 0, (const struct sockaddr *) &client_addr, client_addr_size);
						if(n==-1)perror("sendto error");
						
						draw_player(my_win, &sm.players[i], true);
						player_count++;
					}
					else	// repeated character
					{
						sm.type = 2;
						n = sendto(fd, &sm, sizeof(server_message), 0, (const struct sockaddr *) &client_addr, client_addr_size);
						if(n==-1)perror("sendto error");
					}	
				}
				else if (cm.arg == 'd')	// disconect client
				{
					i = search_player(sm.players, cm.c);
					if(i==-1)  mvwprintw(message_win, 2,1,"Char %c not found.", cm.c);
					else 
					{
						draw_player(my_win, &sm.players[i], false);
						mvwprintw(message_win, 2,1,"player %c disconected", cm.c);
						sm.players[i].c = '\0';
						player_count--;
					}
				}
				else if(cm.arg == 'b') // bot client conected 
				{  // todo: add robustnes by saving address and adding verification
				   // todo: run bot-client from makefile
					bot_count = atoi(&cm.c);
					for(i=0; i<bot_count; i++){ 
					k=0;
					while(sm.bots[k].c!='\0' && k<MAX_PLAYERS) k++;
					new_player (&sm, 1, k, '*');
					draw_player(my_win, &sm.bots[k], true);
					}
					mvwprintw(message_win, 2,1,"%d bots joined", bot_count);
				}
				else mvwprintw(message_win, 2,1,"Message poorly formatted.");
			break;
			case 1: //Message about player's/bot's movement
				if(cm.arg == 'b') // received a bot_movement message
				{
					n = recvfrom(fd, &bot_message, bot_count, 0, ( struct sockaddr *)&client_addr, &client_addr_size);
					if(n == -1)perror("recv error(please press ctrl+C)");
					for(k=0; k<bot_count; k++)
					{
							temp_x=sm.bots[k].x;
							temp_y=sm.bots[k].y;
							draw_player(my_win, &sm.bots[k], false);
							moove_player (&sm.bots[k], bot_message[k]);
							rammed_player = check_collision(&sm, 1, i);
						
							if(rammed_player>-1 && rammed_player<MAX_PLAYERS){
								sm.bots[k].x=temp_x;
								sm.bots[k].y=temp_y;
								update_health(&sm.players[rammed_player], -1);
						
							}else if(rammed_player>=MAX_PLAYERS && rammed_player<MAX_PLAYERS+MAX_PRIZES){
								sm.bots[k].x=temp_x;
								sm.bots[k].y=temp_y;
							}
							draw_player(my_win, &sm.bots[k], true);
							//todo: check for collision after movement
					}
				}
				else // received a player_movement message
				{	i = search_player(sm.players, cm.c);
					if(i==-1) mvwprintw(message_win, 2,1,"Char %c not found.", cm.c);
					else
					{
						temp_x=sm.players[i].x;
						temp_y=sm.players[i].y;
						draw_player(my_win, &sm.players[i], false);
						moove_player (&sm.players[i], cm.arg);
						rammed_player = check_collision(&sm, 0, i);

						if(rammed_player>-1 && rammed_player<MAX_PLAYERS){
							sm.players[i].x=temp_x;
							sm.players[i].y=temp_y;
							update_health(&sm.players[i], -2);
							update_health(&sm.players[rammed_player], -1);
						
						}else if(rammed_player==-1){
							sm.players[i].x=temp_x;
							sm.players[i].y=temp_y;
						}else if(rammed_player>=MAX_PLAYERS && rammed_player<MAX_PLAYERS+MAX_PRIZES){
							update_health(&sm.players[i], sm.prizes[rammed_player-MAX_PLAYERS].health_bar);
							draw_player(my_win, &sm.prizes[rammed_player-MAX_PLAYERS], false);
							sm.prizes[rammed_player-MAX_PLAYERS].c='\0';
							prize_count--;
						}
						}
						draw_player(my_win, &sm.players[i], true);
					
						/*if (players[i].health_bar==0){
							draw_player(my_win, &players[i], false);
							mvwprintw(message_win, 2,1,"player %c disconected", cm.c);
							players[i].c = '\0';
							player_count--;
						}else{*/

						mvwprintw(message_win, 2,1,"Player %c moved %c", cm.c, cm.arg);
						sm.type = 3;
						sm.player_pos=i;
						//}

						n = sendto(fd, &sm, sizeof(server_message), 0, (const struct sockaddr *) &client_addr, client_addr_size);
					if(n==-1)perror("sendto error");
				}
			break;
			case 2:
				if(prize_count < MAX_PLAYERS)
				{	
					j=0;
					while(sm.prizes[j].c!='\0' && j<MAX_PLAYERS) j++;
					new_player (&sm, 2, j, cm.arg);
					draw_player(my_win, &sm.prizes[j], true);
					prize_count++;

					mvwprintw(message_win, 2,1,"New Prize: %d %d %c ", sm.prizes[j].x, sm.prizes[j].y, cm.arg);
				}
			break;
		}
		wrefresh(message_win);
		memset(client_addr.sun_path, '\0', 108);
	}

    exit(0);
}
