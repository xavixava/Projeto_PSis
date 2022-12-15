#define WINDOW_SIZE 20
#define SOCK_ADDRESS "/tmp/server_sock"
#define PRIZE_ADDR i "/tmp/prize_sock"

// if server sends disconecting then it means it cannot accept more clients
typedef struct client_message{	//later on define field status messages
    int type; // 0-conection, 1-movement, 2-prize 
    char arg;  // conection: c-conecting, d-disconecting; movement: u, d, l, r; prize: value
    char c; //warn which char
} client_message;

typedef struct server_message{	//later on define field status messages
    int type; // 0-conection accepted, 1-no more characters, 2-refusing char, 3-movement response
    int x, y; // x, y positions
    int health; // character health
    int elements; // number of elements to be able to receive field status message
} server_message;
