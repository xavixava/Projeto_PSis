#define WINDOW_SIZE 20
#define MAX_PLAYERS 10
#define MAX_BOTS 10
#define MAX_PRIZES 10
#define SOCK_ADDRESS "/tmp/server_sock"
#define PRIZE_ADDR i "/tmp/prize_sock"

typedef struct player_position_t{
    int x, y;
    char c;
    int health_bar;
} player_position_t;

// if server sends disconecting then it means it cannot accept more clients

typedef struct client_message{	//later on define field status messages
    int type; // 0-conection, 1-movement, 2-prize 
    char arg;  // conection: c-conecting, d-disconecting; movement: u, d, l, r; prize: value
    char c; //warn which char
} client_message;

typedef struct server_message{	//later on define field status messages
    int type; // 0-conection accepted, 1-no more characters, 2-refusing char, 3-movement response
    int player_pos; //position of player recieving the message in the players array, -1 implies the user reached hp0 and has to disconnect
    player_position_t players[MAX_PLAYERS];
    player_position_t bots[MAX_BOTS];
    player_position_t prizes[MAX_PRIZES]; // number of elements to be able to receive field status message
} server_message;
