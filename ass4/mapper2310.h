#ifndef MAPPER_HEADER
#define MAPPER_HEADER

#include "util.h"
#include "port.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>

/* The maximum length of an airport name */
#define MAX_AIRPORT_ID_LENGTH 490
/* Number of characters in largest port number (including \0) */
#define PORT_NUMBER_LENGTH 6
/* Maximum length of a command */
#define COMMAND_BUFFER_SIZE 500 
/* Maximum number of mappings that can be added to the mapper */
#define MAX_MAP_ENTRIES 1000

#define ENTRY_NOT_FOUND -1
#define MAPPER_FATAL_ERROR 1

/**
 * Holds the information of a single entry to the mapping server.
 * airportName is the name/ID of an airport that should be mapped,
 * portNumber is the port which that airport listens on.
 */
typedef struct {
    char airportName[MAX_AIRPORT_ID_LENGTH];
    char portNumber[PORT_NUMBER_LENGTH];
} MapEntry;

/**
 * Represents the airportName:portnumber mappings. entryCount is
 * the number of entries specified by the MapEntry struct pointer,
 * which is an instance of the struct declared above.
 */
struct Mapping {
    int entryCount;
    MapEntry* entries;
};

/**
 * Acts as a 'package' for arguments that created pthreads are
 * initiated with. clientDescriptor contains a file descriptor
 * to a socket connection initiated with the client which the
 * thread manages. lock is a mutex lock (that worker threads
 * should use when modifying the Mapping structure). mapping
 * is a Mapping struct pointer.
 */
struct ClientArguments {
    int clientDescriptor;
    pthread_mutex_t* clientLock;
    struct Mapping* mapping;
};

/**
 * An enumeration of the command types that the mapper server is
 * able to execute.
 */
typedef enum {
    UNKNOWN_COMMAND = -1,
    LIST_MAPPINGS = 0,
    QUERY_MAPPING = 1,
    ADD_MAPPING = 2
} Command;

/* Function Declarations */
void* command_listener(void* threadArgs);

void handle_connections(int socket);

Command command_lexer(char* commandString);

void add_map_entry(struct Mapping* mapping, char* commandString);

void print_mappings(struct Mapping* mapping, FILE* stream);

void print_queried_mapping(struct Mapping* mapping, char* commandString, 
        FILE* stream);

int name_index_in_mapping(struct Mapping* mapping, char* airportName);

void sort_mapping(struct Mapping* mapping);

#endif