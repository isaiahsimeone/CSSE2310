#ifndef ROC_HEADER
#define ROC_HEADER

#include "util.h"
#include "error.h"
#include "port.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

/* The maximum number of characters that can constitute airport information */
#define MAX_DEST_INFO_LENGTH 500
/* The number of characters that can be received from a client */ 
#define RESPONSE_BUFFER_SIZE 500

/* Function Declarations */
RocError resolve_destinations(char** destinations, int destinationCount, 
        char* mapperPort);

bool visit_destinations(char** destinationInfo, char** destinations, 
        int destinationCount, char* aircraftID);

void print_destination_info(char** destinationInfo, int destinationCount);

#endif