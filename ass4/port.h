#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

/* The number of digits contained in the largest port 
 * number (including a null terminator character) */
#define PORT_NUMBER_LENGTH 6

/* Function declarations */
int bind_to_free_port(void);

char* get_port_from_socket(int socket);

int connect_to_port(char* port, bool* success);

struct addrinfo construct_socket_hints(void);