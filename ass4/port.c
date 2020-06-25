#include "port.h"

/**
 * Determines the port that is bound with the specified socket.
 *
 * It remains the task of the calling scope to ensure that the socket has 
 * both been initiated, and has an address bound to it (via bind()) See 
 * bind_to_free_port() for both socket initialisation and binding.
 *
 *  socket:   A file descriptor of a socket which has been bound
 *            with the address (port) that shall be determined.
 *    
 * Returns:   A string (char*) which specifies the port that is bound to
 *            the socket provided.
 *            
 */
char* get_port_from_socket(int socket) {
    struct sockaddr_in address;
    socklen_t addressLength = sizeof(struct sockaddr_in);
    
    getsockname(socket, (struct sockaddr*)&address, &addressLength);

    /* Get the port number of the socket in host byte order */
    char* addressBuffer = malloc(sizeof(char) * PORT_NUMBER_LENGTH);
    snprintf(addressBuffer, PORT_NUMBER_LENGTH, "%d",
            ntohs(address.sin_port));

    return addressBuffer;
}

/**
 * Creates a new socket and assigns an address (port) to the socket.
 * The assigned address will be a port that is available on the host
 * machine (localhost in this case)
 * 
 * This function is only suitable for connections on the machine
 * which this function is executed (i.e. localhost or
 * 127.0.0.1)
 *
 * Returns:   A file descriptor (int) of the newly created socket
 *            which is bound to a free port on the machine.
 */
int bind_to_free_port() {
    struct addrinfo* ai = 0;
    struct addrinfo hints = construct_socket_hints();

    const char* node = "localhost";
    /* Any available port on localhost */
    const char* port = "0";

    getaddrinfo(node, port, &hints, &ai);
    freeaddrinfo(ai);

    /* Create a socket descriptor and bind to it */
    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    bind(socketDescriptor, (struct sockaddr*)ai->ai_addr,
            sizeof(struct sockaddr));

    return socketDescriptor;
}

/**
 * Given a port, this function will establish a connection with
 * that port and return a descriptor to the created socket
 * descriptor.
 *
 * This function is only suitable for connections on the machine
 * which this function is executed (i.e. localhost or
 * 127.0.0.1)
 *
 * The returned socket file descriptor is duplicated (via dup())
 * before being returned to the caller.
 *
 *    port:   The port number that should be connected to
 * success:   A boolean which is modified by this function.
 *            the boolean has the value true if no errors were
 *            encountered whilst connecting to the given port.
 *            false otherwise.
 * 
 * Returns:   A file descriptor (int) of the newly created socket
 *            Descriptor.
 */
int connect_to_port(char* port, bool* success) {
    *success = true;
    struct addrinfo* ai = 0;
    struct addrinfo hints = construct_socket_hints();
    
    const char* node = "localhost";

    if (getaddrinfo(node, port, &hints, &ai)) {
        *success = false;
    }
    freeaddrinfo(ai);

    /* Create a new socket descriptor and connect to it */
    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(socketDescriptor, (struct sockaddr*)ai->ai_addr,
            sizeof(struct sockaddr))) {
        *success = false;
    }
    /* Duplicate the file descriptor */
    int duplicatedDescriptor = dup(socketDescriptor);
    
    return duplicatedDescriptor;
}

/**
 * Constructs an addrinfo struct and initiates the 'hints'
 * struct member
 *
 * This function always returns an addrinfo struct for
 * IPV6, TCP and PASSIVE behaviour.
 * 
 * Returns:   An addrinfo structure which is initiated with 
 *            socket hints concerning the type of socket that
 *            the caller should support
 */
struct addrinfo construct_socket_hints() {
    struct addrinfo socketHints;
    memset(&socketHints, 0, sizeof(struct addrinfo));

    /* Set IPV6 for address info family */
    socketHints.ai_family = AF_INET;
    /* Set TCP protocol for socket type */
    socketHints.ai_socktype = SOCK_STREAM;
    /* Make socket suitable for binding */
    socketHints.ai_flags = AI_PASSIVE;

    return socketHints;
}
