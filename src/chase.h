#define WINDOW_SIZE 20
#define MAX_PLAYERS 304 // (18*18)-20 
#define MAX_BOTS 10
#define MAX_PRIZES 10
#define SOCK_ADDRESS "/tmp/server_sock"
#define PRIZE_ADDR i "/tmp/prize_sock"

typedef struct player_position_t{
    int x, y;
    char c;
    int health_bar;  // in queue will save index
} player_position_t;

// if server sends disconecting then it means it cannot accept more clients

typedef struct client_message{	//later on define field status messages
    int type; // 0-conection, 1-movement, 2 - continue(response to hp_o)
    char arg;  // conection: c-conecting, d-disconecting; movement: u, d, l, r
} client_message;

typedef struct server_message{	//later on define field status messages
    int type; // 0-conection accepted, 1-field is full, 2-to do: disconnect, 3-Health_0, 4-field_update
    player_position_t players[MAX_PLAYERS];
    player_position_t bots[MAX_BOTS];
    player_position_t prizes[MAX_PRIZES]; // number of elements to be able to receive field status message
} server_message;

typedef struct cond_t{	// solves hp0 synchronization
    pthread_cond_t cv; 
    short n;  // 0 - waiting, 1 - continue, 2 - ten sec passed 
} cond_t;
