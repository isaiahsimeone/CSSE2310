#ifndef CONTROL_HEADER
#define CONTROL_HEADER

#include "error.h"
#include "util.h"
#include "port.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* The maximum length of an aircraft name (id) */
#define MAX_AIRCRAFT_NAME_LENGTH 500
/* Buffer size for receiving commands from a client */
#define COMMAND_BUFFER_SIZE 500
/* The maximum number of visitors that can be recorded */
#define MAX_VISITOR_ENTRIES 1000

/**
 * Represents a visitor to the airport. Each VisitorEntry
 * structure contains the ID of a visiting aircraft.
 */
typedef struct {
    char aircraftID[MAX_AIRCRAFT_NAME_LENGTH];
} VisitorEntry;

/**
 * Used to track information including the total number of
 * visitors that have visited the airport. And a pointer to
 * the first (of possibly many) VisitorEntries
 */
struct Visitors {
    int visitorCount;
    VisitorEntry* entries;
};

/**
 * Acts as a 'package' for arguments that created pthreads are
 * initiated with. clientDescriptor contains a file descriptor
 * to a socket connection initiated with the client which the
 * thread manages. lock is a mutex lock (that worker threads
 * should use when modifying the Visitors structure). visitors 
 * is a structure pointer of the struct declared above. 
 * airportInfo is the information argument that the airport
 * (control2310) is started with.
 */
struct ClientArguments {
    int clientDescriptor;
    pthread_mutex_t* lock;
    struct Visitors* visitors;
    char* airportInfo;
};

/**
 * An enumeration of the command types that the control server is
 * able to handle.
 */
typedef enum {
    LOG_COMMAND = 0,
    RECORD_VISIT_SEND_INFO = 1
} Command;

/* Function Declarations */
bool report_to_mapper(char* mapperPort, char* controlPort, char* airportID);

void handle_connections(int socket, char* airportInfo);

Command command_lexer(char* commandString);

void record_visit(struct Visitors* visitors, char* commandString);

void sort_visitors(struct Visitors* visitors);

void print_all_visitors(struct Visitors* visitors, FILE* stream);

#endif