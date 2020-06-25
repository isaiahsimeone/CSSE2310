#include "roc2310.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        return roc_error(ROC_INVALID_ARG_COUNT);
    }
    bool mapperSpecified = true, mapperRequired = false;
    char* aircraftID = argv[1];
    char* mapperPort = argv[2];
    /* Determine whether a mapper port is provided */
    if (strcmp(mapperPort, "-") == 0) {
        mapperSpecified = false;
    }
    /* Calculate number of destinations to visit */
    int destinationCount = argc - 3;
    /* Copy destinations (ID's/port numbers) to string array (char**) */
    char** destinations = malloc(sizeof(char*) * destinationCount);
    for (int i = 3; i < argc; i++) {
        destinations[i - 3] = (char*) malloc(string_or_port_size(argv[i]));
        strcpy(destinations[i - 3], argv[i]);
        /* If we encounter a non-port number, resolution is required later */
        if (!is_unsigned_short(argv[i])) {
            mapperRequired = true;
        }
    }
    if (mapperSpecified && !is_unsigned_short(mapperPort)) {
        return roc_error(ROC_INVALID_MAPPER_PORT);
    }
    if (!mapperSpecified && mapperRequired) {
        return roc_error(ROC_REQUIRE_MAPPER);
    }
    /* At least one destination port number must be resolved */
    if (mapperRequired) {
        RocError status = resolve_destinations(destinations,
                destinationCount, mapperPort);
        /* Failure resolving at least one destination */
        if (status != ROC_NORMAL_OPERATION) {
            return roc_error(status);
        }
    }
    /* Allocate memory for a char** array to hold the info of destinations */
    char** destinationInfo = malloc_string_array(destinationCount,
            MAX_DEST_INFO_LENGTH);
    /* Visit destinations and receive the destination airports information */
    bool visitingSuccess = visit_destinations(destinationInfo, destinations,
            destinationCount, aircraftID);
    /* Print the information for all visited airports */
    print_destination_info(destinationInfo, destinationCount);
    return visitingSuccess ? roc_error(ROC_NORMAL_OPERATION) 
            : roc_error(ROC_FAILED_TO_CONNECT);
}

/**
 * Prints to stdout the destination info of all airports that have
 * been visited by the Roc. destination information is printed out
 * in FIFO order (as in first visited printed first) and is newline
 * separated.
 *
 * It is the responsibility of the calling scope to ensure that 
 * the argument: 'destinationInfo' specifies all of the destinations
 * that have been visited by the roc.
 *
 *  destinationInfo:   An array of strings (char**) containing the info
 *                     retrieved from visited destinations.
 * destinationCount:   The number of destinations which have been visited
 *                     by the roc.
 *    
 *          Returns:   Void
 */
void print_destination_info(char** destinationInfo, int destinationCount) {
    for (int i = 0; i < destinationCount; i++) {
        /* Destinations that could not be connected to are entered as NULL */
        if (destinationInfo[i] != NULL) {
            printf("%s\n", destinationInfo[i]);
        }
    }
    fflush(stdout);
}

/**
 * Visits destinations in the order they are specified in the argument 
 * 'destinations'.
 *
 * The argument, destinationInfo should be allocated memory space before
 * using it with this function. Ensuring this is the responsibility of the
 * calling scope.
 *
 * All fields specified in the argument 'destinations' should be the type of
 * unsigned_short (i.e. a valid port number.) Destination codes are not
 * parseable and will result in that destination not being connected to.
 * Use resolve_destinations() first to resolve port numbers from airport
 * names.
 *
 *  destinationInfo:   An array of strings (char**) which will contain
 *                     the information of visited destinations after 
 *                     calling.
 *     destinations:   The port numbers that should be connected to
 *                     (this signifies the roc visiting the airport
 *                     that is expected to be listening on the port)
 * destinationCount:   The number of destinations which the roc should
 *                     visit.
 *       aircraftID:   The ID (name) of the roc which is sent to each
 *                     destination (control server)
 *    
 *          Returns:   True if all specified destinations were connected
 *                     to successfully. False otherwise.
 */
bool visit_destinations(char** destinationInfo, char** destinations,
        int destinationCount, char* aircraftID) {
    /* After all visiting is complete, visitingSuccess = false if at least
     * one destination was not able to be connected to */
    bool visitingSuccess = true;
    char controlResponse[RESPONSE_BUFFER_SIZE];
    for (int i = 0; i < destinationCount; i++) {
        /* Connect to the destination airport (control server) */
        bool success;
        int connectionDescriptor = connect_to_port(destinations[i], &success);
        if (!success) {
            /* Failed to connect to a destination - skip and visit the next */
            visitingSuccess = false;
            destinationInfo[i] = NULL;
            continue;
        }
        FILE* receiveStream = fdopen(connectionDescriptor, "r");
        FILE* sendStream = fdopen(connectionDescriptor, "w");

        /* Send the aircraft ID of the roc to the control */
        fprintf(sendStream, "%s\n", aircraftID);
        fflush(sendStream);

        /* Receive the airport info and record it to destinationInfo */
        fgets(controlResponse, RESPONSE_BUFFER_SIZE, receiveStream);
        strcpy(destinationInfo[i], controlResponse);
        /* Remove the trailing newline from the response from the control */
        trim_newline(destinationInfo[i]);
    }
    return visitingSuccess;
}

/**
 * For a given string array (char**), 'destinations', each string contained
 * by 'destinations' that is not a valid unsigned short (port number)
 * (e.g. BNE) will be resolved and replaced with the port number of the
 * control server that is governing that destination. E.g. if destinations
 * contains {"BNE", "53940"} this function will modify 'destinations' to
 * {"29403", "53940"} (supposing the control server for the destination 
 * 'BNE' is listening on 29403, and that it can be connected to)
 *
 * It is the responsibility of the calling scope to ensure that all arguments
 * to this function are valid. Particularly, each string (char*) specified
 * in 'destinations' should be allocated enough memory for a destination name
 * to be resolved to a port number. (e.g. if a destination name is "A", and it
 * is resolved to the port number "34903". Memory should already be allocated
 * for the length of the string "34903", or the maximum length of a port
 * number more generally. (see  minimum_length() in util.c for a function 
 * that can help to accomplish this.)
 *
 *     destinations:   An array of strings (char**) which contain a mixture
 *                     of port numbers and destination names (which will be
 *                     resolved and replaced with the port numbers of those
 *                     destinations (their governing control servers))
 * destinationCount:   The number of destinations specified by the
 *                     'destination' argument.
 *       mapperPort:   The port number of the mapper server which should
 *                     be contacted for the port number resolution of
 *                     destinations.
 *    
 *          Returns:   A member of the RocError enum.
 *                     ROC_ERROR_CONNECTING_MAPPER if the mapper was unable
 *                     to be connected to.
 *                     ROC_CANNOT_RESOLVE_PORT if the mappers response
 *                     could not be received or if it has no record
 *                     of at least one destination which was queried
 */
RocError resolve_destinations(char** destinations, int destinationCount,
        char* mapperPort) {
    /* Connect to the mapper server */
    bool success;
    int connection = connect_to_port(mapperPort, &success);
    if (!success) {
        return ROC_ERROR_CONNECTING_MAPPER;
    }
    FILE* sendStream = fdopen(connection, "w");
    FILE* receiveStream = fdopen(connection, "r");

    bool allResolved = true;
    /* For each unresolved port in 'destinations' query the mapper */
    char mapperResponse[RESPONSE_BUFFER_SIZE];
    for (int i = 0; i < destinationCount; i++) {
        if (!is_unsigned_short(destinations[i])) {
            /* Send the query to the mapper for the unresolved destination */
            fprintf(sendStream, "?%s\n", destinations[i]);
            fflush(sendStream);

            /* Handle the mappers response to the query */
            if (fgets(mapperResponse, RESPONSE_BUFFER_SIZE, receiveStream)) {
                /* Remove the newline which terminates the mapper response */
                trim_newline(mapperResponse);
                strcpy(destinations[i], mapperResponse);
            } else {
                /* Couldn't get response from mapper */
                allResolved = false;
                continue;
            }

            /* The mapper sent ';', the destination cannot be resolved */
            if (strcmp(destinations[i], ";") == 0) {
                allResolved = false;
            }
        }
    }
    /* Could not resolve at least one destination */
    if (!allResolved) {
        return ROC_CANNOT_RESOLVE_PORT;
    }
    /* All destinations resolved successfully */
    return ROC_NORMAL_OPERATION;
}