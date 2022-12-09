#include <stdlib.h>
#include <ncurses.h>


#include "chase.h"

WINDOW * message_win;


typedef struct player_position_t{
    int x, y;
    char c;
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

player_position_t p1;

int main(){
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


    new_player(&p1, 'y');
    draw_player(my_win, &p1, true);

    int key = -1;
    while(key != 27 && key!= 'q'){
        key = wgetch(my_win);		
        if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN){
            draw_player(my_win, &p1, false);
            moove_player (&p1, key);
            draw_player(my_win, &p1, true);

        }

        mvwprintw(message_win, 1,1,"%c key pressed", key);
        wrefresh(message_win);	
    }

    exit(0);
}