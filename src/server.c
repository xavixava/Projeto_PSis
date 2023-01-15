#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "queue.h"
#include "chase.h"

Queue *q;

WINDOW * message_win;
WINDOW *my_win;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t id[308];  // 0: prize_gen; 1: bot_gen; 2: update players; 3: tcp listener
cond_t cv[MAX_PLAYERS];
pthread_cond_t queue_cv;

short bot_nr;

extern int errno;
int player_count;
int fd, n;

int socket_array[MAX_PLAYERS];

server_message sm;


int search_player(player_position_t vector[], char c){
	int i;
	for(i=0; i<MAX_PLAYERS; i++)if(vector[i].c==c) return i;
	return -1;
}

void error_msg(char c)
{
	mvwprintw(message_win, 7, 1, "%c: %s", strerror(errno));
	return;
}

/*
 * Chooses which direction the bots will move to
 * Informs the thread that computes all this via queue
 */
void *bot_gen(void *arg)
{	
	char *bot_vector = arg;
	int i, direction, ret;
	player_position_t bot_warn;
	
	bot_warn.x = 0;
	bot_warn.y = 0;
	bot_warn.c = '*';
	bot_warn.health_bar = -1;	

	srand(time(NULL));

	while (1){
		// every 3 seconds move the bots
		sleep(3);
		for(i=0; i < bot_nr; i++)
		{
			direction = random()%4;
			switch (direction)
			{
				case 0:
					bot_vector[i]='l'; 
					break;
				case 1:
					bot_vector[i]='r'; 
					break;
				case 2:
					bot_vector[i]='d'; 
					break;
				case 3:
					bot_vector[i]='u'; 
					break;
			}
		}
		
		// warn queue
		pthread_mutex_lock(&lock);
		InsertLast(q, &bot_warn);
		ret = pthread_cond_signal(&queue_cv);
		pthread_mutex_unlock(&lock);
	}

 
	return 0;
}

int generate_prize()
{
	// returns a random value between 1 and 5
	return (random()%5)+1;
}

char generate_player_char()
{
	int player_num;
	char player = 'a';
	while (search_player(sm.players, player)!=-1)
	{
		player_num = (random()%46);
		if(player_num < 23)
			player ='a' + player_num;
		else player = 'A' + player_num;	
	}
	return player;
}

void *wait_ten(void *arg)
{
	int index;
	index = (int) arg;
	sleep(10);
	
	pthread_mutex_lock(&lock);
	if(cv[index].n==0)
	{
		cv[index].n = 2;
		// send signal
	}	
	pthread_mutex_unlock(&lock);		
	pthread_exit(NULL);
}


void *hp_o(void *arg)
{
	int n;
	int ret;
	pthread_t waiter;
	
	n = (int) arg;
	/* initialize a condition variable to its default value */
	ret = pthread_cond_init(&cv[n].cv, NULL);
	if(ret!=0)
	{
		error_msg('c');
		pthread_exit(NULL);
	}
	cv[n].n = 0;
	pthread_create(&waiter, NULL, wait_ten, n);
	pthread_mutex_lock(&lock);
	/* wait on condition variable */
	while(cv[n].n == 0) ret = pthread_cond_wait(&cv[n].cv, &lock);
	if (cv[n].n == 1) sm.players[n].health_bar = 10;	
	else if (cv[n].n == 2)
	{
		mvwprintw(message_win, 3, 1, "Player elim");
		sm.players[n].c = '\0';
		pthread_cancel(id[n+4]);
		close(socket_array[n]);
		socket_array[n] = 0;
	}	
	pthread_mutex_unlock(&lock);		
	pthread_cond_destroy(&cv[n].cv);	
	pthread_exit(NULL);
}


void *prize_gen(void *arg)
{	
	int i, ret; 
	player_position_t prize[MAX_PRIZES];

	srand(time(NULL));

	for(i=0; i<5; i++) 	// create 5 prizes at the begining of the game
	{
		prize[i].c = '0' + generate_prize();	
		// put prize in the queue
		pthread_mutex_lock(&lock);
		InsertLast(q, &prize[i]);
		ret = pthread_cond_signal(&queue_cv);
		pthread_mutex_unlock(&lock);
	}

	while (1)	// sends prizes every 5 seconds
	{
		sleep(5);	
		prize[i].c = '0' + generate_prize();	
		// put prize in the queue
		pthread_mutex_lock(&lock);
		InsertLast(q, &prize[i]);
		ret = pthread_cond_signal(&queue_cv);
		pthread_mutex_unlock(&lock);
    
		i = (i >= 9) ? 0 : i+1;
	}
		

	return 0;
}

void update_health(player_position_t * player, int collision_type){
	
	if(collision_type==-2){ //hit player
		player->health_bar++;
	}else if(collision_type==-1){ //got hit by player
		player->health_bar--;
	}else if(collision_type==0){ //got hit by bot
		player->health_bar--;
	}else if(collision_type>0 && collision_type<=5){ //hit prize (collision_type is the value of the prize)
		player->health_bar+=collision_type;
	}

	if (player->health_bar<=0){ //min health is 0
		player->health_bar=0;
	}
	else if (player->health_bar>10){ //max health is 10
		player->health_bar=10;
	}
}

/*
Checks if there's a element in the current player position, 
returns the element position in the respective array in case of colision

Return -2 if no collision
Return -1 if hit bot
Return 0-303 if hit player (position of player in array)
Return 304-313 if it prize (position of prize in array + offset)
*/
int check_collision (int element_role, int array_pos){
	//element_role: 0 = player, 1 = bot, 2 = prize
	switch (element_role)
	{
	case 0://player
		for(int i=0; i<MAX_PLAYERS; i++){
			if (array_pos!=i && sm.players[i].c!='\0'){
				if(sm.players[array_pos].x==sm.players[i].x && sm.players[array_pos].y==sm.players[i].y){
						return i;
				} 
			}
		}
		for(int i=0; i<MAX_BOTS; i++){
			if (sm.bots[i].c!='\0'){
				if(sm.players[array_pos].x==sm.bots[i].x && sm.players[array_pos].y==sm.bots[i].y){
					return -1; 
				}
			}
		}
		for(int i=0; i<MAX_PRIZES; i++){
			if (sm.prizes[i].c!='\0'){
				if(sm.players[array_pos].x==sm.prizes[i].x && sm.players[array_pos].y==sm.prizes[i].y){
					return i+MAX_PLAYERS;
				}
			}
		}
		return -2;
		break;

	case 1://bot
		for(int i=0; i<MAX_PLAYERS; i++){
			if (sm.players[i].c!='\0'){
				if(sm.bots[array_pos].x==sm.players[i].x && sm.bots[array_pos].y==sm.players[i].y){
						return i;
				}
			}
		}
		for(int i=0; i<MAX_BOTS; i++){
			if (array_pos!=i && sm.bots[i].c!='\0'){
				if(sm.bots[array_pos].x==sm.bots[i].x && sm.bots[array_pos].y==sm.bots[i].y){
					return -1;
				}
			}
		}
		for(int i=0; i<MAX_PRIZES; i++){
			if (sm.prizes[i].c!='\0'){
				if(sm.bots[array_pos].x==sm.prizes[i].x && sm.bots[array_pos].y==sm.prizes[i].y){
					return i+MAX_PLAYERS;
				}
			}
		}
		return -2;
		break;

	case 2://prize
		for(int i=0; i<MAX_PLAYERS; i++){
			if (sm.players[i].c!='\0'){
				if(sm.prizes[array_pos].x==sm.players[i].x && sm.prizes[array_pos].y==sm.players[i].y){
						return i;
				}
			}
		}
		for(int i=0; i<MAX_BOTS; i++){
			if (sm.bots[i].c!='\0'){
				if(sm.prizes[array_pos].x==sm.bots[i].x && sm.prizes[array_pos].y==sm.bots[i].y){
					return -1;
				}
			}
		}
		for(int i=0; i<MAX_PRIZES; i++){
			if (array_pos!=i && sm.prizes[i].c!='\0'){
				if(sm.prizes[array_pos].x==sm.prizes[i].x && sm.prizes[array_pos].y==sm.prizes[i].y){
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

/*
Adds a new element in random position avoiding other elements that might exist in that location
*/
void new_player (int element_role, int array_pos, char c){
    
	srand(time(NULL));
	switch (element_role)
	{
	case 0:
		do{
			sm.players[array_pos].x = (rand() % (WINDOW_SIZE-2)) + 1;
			sm.players[array_pos].y = (rand() % (WINDOW_SIZE-2)) + 1;
		}while (check_collision(element_role, array_pos)!=-2);
		sm.players[array_pos].c = c;
		sm.players[array_pos].health_bar = 10;
		break;
	
	case 1:
		do{
			sm.bots[array_pos].x = (rand() % (WINDOW_SIZE-2)) + 1;
			sm.bots[array_pos].y = (rand() % (WINDOW_SIZE-2)) + 1;
		}while (check_collision(element_role, array_pos)!=-2);
		sm.bots[array_pos].c = '*';
		break;

	case 2:
		do{
			sm.prizes[array_pos].x = (rand() % (WINDOW_SIZE-2)) + 1;
			sm.prizes[array_pos].y = (rand() % (WINDOW_SIZE-2)) + 1;
		}while (check_collision(element_role, array_pos)!=-2);
		sm.prizes[array_pos].c = c;
		sm.prizes[array_pos].health_bar = atoi(&c);
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
    if(p_x<=0 || p_x >= WINDOW_SIZE || p_y >= WINDOW_SIZE || p_y <= 0) mvwprintw(message_win, 3, 1, "c:%c x:%d y:%d", ch, p_x, p_y);
    else
    {	    
	mvwprintw(message_win, 3, 1, "c:%c x:%d y:%d", ch, p_x, p_y);
    	wmove(win, p_y, p_x);
    	waddch(win,ch);
    	wrefresh(win);
    }
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
Clears the server screen and updates it
*/
void screen_update(){
		werase(my_win);	
		box(my_win, 0 , 0);	

		for(int i=0; i<MAX_PLAYERS; i++){ 	//update the players on screen
			if(sm.players[i].c!='\0' && sm.players[i].health_bar>0){
				draw_player(my_win, &sm.players[i], true);
			}
		}
		for(int i=0; i<MAX_PRIZES; i++){	//update the prizes on screen
			if(sm.prizes[i].c!='\0'){
				draw_player(my_win, &sm.prizes[i], true);
			}
		}
		for(int i=0; i<MAX_BOTS; i++){ 		//update the bots on screen
			if(sm.bots[i].c!='\0'){
				draw_player(my_win, &sm.bots[i], true);
			}
		}
}

/*
Create a socket to be able to comunicate with clients
Server's port and address are provided as arguments in command line
*/
int create_socket(int port){
	
  int sock_fd;
	struct sockaddr_in address;
	
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sock_fd == -1){
		error_msg('s');	
		exit(-1);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
 	address.sin_port = htons(port);
 
	int err = bind(sock_fd, (struct sockaddr *)&address, sizeof(address));
	if(err == -1) {
		perror("socket:");
		exit(-1);
	}
	return sock_fd;
}



void clear_hp_changes(int vector[]){
	int i;
	for(i=0; i<MAX_PLAYERS; i++) vector[i]=0;
}



void *update_players(void *arg)
{
	int i, n;

	for(i=0; i<MAX_PLAYERS; i++)
	{
		pthread_mutex_lock(&lock);	
		if(socket_array[i]!=0)
		{
			sm.type = 4;
			n = write(socket_array[i], &sm, sizeof(server_message));
			if(n==-1) error_msg('u');	
		}	
		pthread_mutex_unlock(&lock);
	}
	// todo: add a count to check if it was sent to all players
	return NULL;
}


/*
 * This function will be used for the health and position computations
 * it will receive a queue and a vector with changes of positions for the bots
 */
void *computation(void *arg)
{
	char *bot_vector = arg;
	player_position_t *current_player;
	int i, j, ret, k, temp_x, temp_y, rammed_player;
  	pthread_t updater, hpo;
  
	current_player = NULL;
	ret = pthread_cond_init(&queue_cv, NULL);
	
	while(1)
	{
		pthread_mutex_lock(&lock);
		// todo: if empty add wait here
		current_player = GetFirst(q);  // gets most prioritary element from FIFO
		if(current_player==NULL) ret = pthread_cond_wait(&queue_cv, &lock);
		pthread_mutex_unlock(&lock);
		

		if(current_player != NULL)
		{
			//werase(message_win);
			//box(message_win, 0 , 0);	
			//content of message from FIFO being processed
			mvwprintw(message_win, 1, 1, "%c %d %d %d", current_player->c, current_player->x, current_player->y, current_player->health_bar);

			if('1' <= current_player->c && current_player->c <= '5') 
			{ // new prize on field
				j=0;
				while(sm.prizes[j].c!='\0' && j<MAX_PRIZES) j++;  // find empty prize 
				if (j != 10)
				{
					new_player (2, j, current_player->c);  // checks if space is occupied
					//draw_player(my_win, &sm.prizes[j], true);
					mvwprintw(message_win, 3,1,"New Prize: %d %d %c", sm.prizes[j].x, sm.prizes[j].y, current_player->c);
				}
			}
			else if(current_player->c == '*') 
			{ // update bots
				for(k=0; k<bot_nr; k++)  //move each bot accordingly and check for collision
				{							
					temp_x=sm.bots[k].x;
					temp_y=sm.bots[k].y;
					//draw_player(my_win, &sm.bots[k], false);
					moove_player (&sm.bots[k], bot_vector[k]);
					rammed_player = check_collision(1, k);
							
					if(rammed_player>-1 && rammed_player<MAX_PLAYERS)
					{ //bot hit player (update player's health)
						if(sm.players[rammed_player].health_bar>=0)
						{
							sm.bots[k].x=temp_x;
							sm.bots[k].y=temp_y;
							update_health(&sm.players[rammed_player], -1);
					
							if (sm.players[rammed_player].health_bar<=0){// todo: Reached 0HP 10sec count 
								//draw_player(my_win, &sm.players[i], false);
								mvwprintw(message_win, 2, 1, "Player %c reached 0 HP", sm.players[i].c);
								sm.type = 3;
								n = write(socket_array[rammed_player], &sm, sizeof(server_message));
								pthread_create(&hpo, NULL, hp_o, rammed_player);
							}
						}
					}
					else if(rammed_player==-1)
					{ //bot hit another bot or prize
						sm.bots[k].x=temp_x;
						sm.bots[k].y=temp_y;
					}
					//draw_player(my_win, &sm.bots[k], true);
					//mvwprintw(message_win, 4,1,"                       ");
					mvwprintw(message_win, 4,1,"Bots: %d %d", sm.bots[k].x, sm.bots[k].y);
				}
			}
			else if((current_player->c >= 'a' && current_player->c <= 'z') || (current_player->c >= 'A' && current_player->c <= 'Z')) 
			{ // move player
				i = current_player-> health_bar;  // since we don't need to save hp in queue, then health_bar saves index
				if(i<0 || i>= MAX_PLAYERS) mvwprintw(message_win, 7,1,"Char %c not found.", current_player->c); // needed better check
				else
				{
					//move player accordingly and check for collisions
					temp_x=sm.players[i].x;
					temp_y=sm.players[i].y;
					draw_player(my_win, &sm.players[i], false);
					sm.players[i].x = current_player->x;
					sm.players[i].y = current_player->y;
					rammed_player = check_collision(0, i);

					if(rammed_player>-1 && rammed_player<MAX_PLAYERS) 
					{	// collided with player
						if(sm.players[rammed_player].health_bar!=0){
							sm.players[i].x=temp_x;
							sm.players[i].y=temp_y;
							update_health(&sm.players[i], -2);
							update_health(&sm.players[rammed_player], -1);
							if (sm.players[rammed_player].health_bar<=0){// todo: Reached 0HP 10sec count 
								//draw_player(my_win, &sm.players[i], false);
								mvwprintw(message_win, 2, 1, "Player %c reached 0 HP", sm.players[i].c);
								sm.type = 3;
								n = write(socket_array[rammed_player], &sm, sizeof(server_message));
								pthread_create(&hpo, NULL, hp_o, rammed_player);
							}
						}
					
					}
					else if(rammed_player==-1) 
					{	// collided with bot

						sm.players[i].x=temp_x;
						sm.players[i].y=temp_y;
					}
					else if(rammed_player>=MAX_PLAYERS && rammed_player<MAX_PLAYERS+MAX_PRIZES)
					{	// found prize
						update_health(&sm.players[i], sm.prizes[rammed_player-MAX_PLAYERS].health_bar);
						//draw_player(my_win, &sm.prizes[rammed_player-MAX_PLAYERS], false);
						sm.prizes[rammed_player-MAX_PLAYERS].c='\0';
					}
				}
				draw_player(my_win, &sm.players[i], true);
				//draw_player(my_win, &sm.players[i], true);
				
				free(current_player);
				current_player = NULL;
			}

			// werase(my_win);
			// box(my_win, 0, 0);
			// werase(message_win);
			// box(message_win, 0, 0);
			
			//Send the player its and the field's updated status
			pthread_create(&updater, NULL, update_players, NULL);
			for (k=0; k<MAX_PRIZES; k++){
				if (sm.prizes[k].c!='\0')
					draw_player(my_win, &sm.prizes[k], true);
			}
			/*
			for (k=0; k<MAX_BOTS; k++){
				if (sm.bots[k].c!='\0')
					draw_player(my_win, &sm.bots[k], true);
			}
			for (k=0; k<MAX_PRIZES; k++){
				if (sm.players[k].c!='\0')
					draw_player(my_win, &sm.players[k], true);
			}*/
			//for (k=0; k<MAX_PRIZES; k++){
				//if (sm.prizes[k].c!='\0')
					//draw_player(my_win, &sm.prizes[k], true);
			//}
			//Send the players the field's updated status
			//screen_update();
			// pthread_create(&updater, NULL, update_players, NULL);
		}		
		// wrefresh(message_win);
		// wrefresh(my_win);
	}
	pthread_cond_destroy(&queue_cv);	
}

player_position_t *alloc()
{
	return (player_position_t *) (malloc(sizeof(player_position_t)));
}

void *cli_reciever(void *arg)
{
	client_message cm;
	int player_pos = (int) arg;
	char player_char = '?';
	player_position_t *item;
	pthread_t updater;

	while(1)
	{
		n = read(socket_array[player_pos], &cm, sizeof(client_message));
		if(n==0)
		{
			mvwprintw(message_win, 7, 1, "%d disconect", player_pos);
			close(socket_array[player_pos]);
			socket_array[player_pos]=0;
			pthread_exit(NULL);
		}
		// add read verification
		// mvwprintw(message_win, 8, 1, "                       ");
		// wmove(message_win, 8, 1);          // move to begining of line
		// wclrtoeol(message_win);  
		// werase(message_win);
		// box(message_win, 0 , 0);	
		mvwprintw(message_win, 6, 1, "%d %c", cm.type, cm.arg);
		switch (cm.type){
			case 0: //Message about player's connection
				if(cm.arg == 'c') // connect player
				{
					if (player_count >= MAX_PLAYERS){ // field is full		
						sm.type = 1;	
						n = write(socket_array[player_pos], &sm, sizeof(server_message));
						// todo: add verification	
					}	
					else {	// player was accepted
						player_char = generate_player_char();
						new_player (0, player_pos, player_char); 
						sm.type = 0;	
						n = write(socket_array[player_pos], &sm, sizeof(server_message));
						if(n==-1)error_msg('w');
						// todo: add proper n verification

						// draw_player(my_win, &sm.players[player_pos], true);					
						//pthread_create(&id[14], NULL, update_players, NULL);
						draw_player(my_win, &sm.players[player_pos], true);		
						//Update the players about new character		
						pthread_create(&updater, NULL, update_players, NULL);
						player_count++;
					}
				}
				else if (cm.arg == 'd')	// disconnect player
				{
					if(sm.players[player_pos].c=='\0')  mvwprintw(message_win, 7,1,"Char %c not found.", player_char);
					
					else  draw_player(my_win, &sm.players[player_pos], false);

					mvwprintw(message_win, 2,1,"%c disconect", player_char);
					sm.players[player_pos].c = '\0';
					//closes socket
					close(socket_array[player_pos]);
					socket_array[player_pos]=0;
					//updates other players
					pthread_create(&updater, NULL, update_players, NULL);
					player_count--;
					pthread_exit(NULL);
				}
				else mvwprintw(message_win, 5,1,"Wrong format: %d %c %c", cm.type, cm.arg, player_char);
			break;
			case 1: //Message about player's movement
				if(player_char == sm.players[player_pos].c)
				{
				wrefresh(message_win);
					item = alloc();
					item->c = player_char;
					item->x = sm.players[player_pos].x;
					item->y = sm.players[player_pos].y;
					item->health_bar = player_pos;
					moove_player(item, cm.arg);

					pthread_mutex_lock(&lock);
					InsertLast(q, item);
					n = pthread_cond_signal(&queue_cv);
					pthread_mutex_unlock(&lock);
				}
			break;
			case 2:
				pthread_mutex_lock(&lock);
				if(cv[player_pos].n==0) cv[player_pos].n = 1; 
				n = pthread_cond_signal(&cv[player_pos].cv);
				pthread_mutex_unlock(&lock);
			break;

			default:
				mvwprintw(message_win, 5,1,"Wrong format: %d %c %c", cm.type, cm.arg, player_char);
				return 0;
			break;
	 	}
	}
}

void *tcp_accepter(void *arg)
{
	int i, new_client;
	struct sockaddr_in client_addr;
	socklen_t client_addr_size = sizeof(struct sockaddr_in);
	while(1){
		new_client = accept(fd, (struct sockaddr*)&client_addr, &client_addr_size);
		for (i=0; i<MAX_PLAYERS; i++){
			if (socket_array[i]==0){
				socket_array[i]=new_client;
				mvwprintw(message_win, 6, 1, "accepted descriptor %d", new_client);

				pthread_create (&id[i+4], NULL, cli_reciever, i);
				break;
			}
		}
	}
	mvwprintw(message_win, 6, 1, "Returning");
	return 0;
}


int main(int argc, char* argv[])
{
	int i; 
	char bot_message[MAX_BOTS];
	struct sigaction act;
  
	srand(time(NULL));
	
	memset(&act,0,sizeof act);
	act.sa_handler=SIG_IGN;
	if(sigaction(SIGPIPE,&act,NULL)==-1)
	{
		error_msg('m');
		exit(1);
	}
	
	if(argc!=2)
	{
		printf("./server/server <PORT NUMBER>\n");
		exit(0);
	}

	player_count = 0;
	q = QueueNew();	
	bot_nr = 10;

	memset(socket_array, 0, MAX_PLAYERS*sizeof(int));

	fd = create_socket(atoi(argv[1]));
	if(listen(fd, 15)!=0)error_msg('l');	

	//Inicializing sm.players array
	for(i=0; i<MAX_PLAYERS; i++) sm.players[i].c = '\0'; 
	//Inicializing sm.prizes array
	for(i=0; i<MAX_PRIZES; i++){ 
		sm.prizes[i].c = '\0';
		sm.prizes[i].x = 1;
		sm.prizes[i].y = 1;
	}
	//Inicializing sm.bots array
	for(i=0; i<MAX_BOTS; i++) sm.bots[i].c = '\0';

	initscr();				/* Start curses mode 		*/
	cbreak();				/* Line buffering disabled	*/
	keypad(stdscr, TRUE);   /* We get F1, F2 etc..		*/
	noecho();				/* Don't echo() while we do getch */

	/* creates a window and draws a border */
	my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
	box(my_win, 0 , 0);	
	wrefresh(my_win);
    
	/* creates a window and draws a border */
	message_win = newwin(10, 25, WINDOW_SIZE, 0);
	box(message_win, 0 , 0);	
	wrefresh(message_win);
	
	for(i=0; i<bot_nr; i++){ 
		new_player (1, i, '*');
		draw_player(my_win, &sm.bots[i], true);
		wrefresh(message_win);
	}

	pthread_create(&id[3], NULL, tcp_accepter, NULL); //starts thread to check for new connections
	pthread_create(&id[2], NULL, computation, &bot_message);  	//starts thread that will compute 
	pthread_create(&id[1], NULL, bot_gen, &bot_message);  		//starts bot generating thread
	pthread_create(&id[0], NULL, prize_gen, q);  				//starts prize generating thread	
	
	pthread_join(id[2], NULL);
	
    freeQueue(q);
    exit(0);
}
