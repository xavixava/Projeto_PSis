#define WINDOW_SIZE 20
#define SOCK_ADDRESS "/tmp/server_sock"

typedef struct ball_message{	//later on define field status messages
    int type; // 0-conection, 1-movement, 3-refusing char 
    char arg;  // conection: c-conecting, d-disconecting; movement: u, d, l, r
    char c; //warn which char
} ball_message;
