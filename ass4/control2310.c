#include "control2310.h"

int main(int argc, char** argv) {
    /* Ensure that the argument count is correct */
    if (argc < 3 || argc > 4) {
        return control_error(CONTROL_INVALID_ARG_COUNT);
    }

    char* airportID = argv[1];
    char* airportInfo = argv[2];
    char* mapperPort;
    bool mapperSpecified;

    /* If there are four arguments, a mapper is specified*/
    if ((mapperSpecified = (argc == 4))) {
        mapperPort = argv[3];
    }
    /* Ensure that airportID and airportInfo contain no illegal characters */
    if (count_occurrences(airportID, "\n\r:") != 0 
            || count_occurrences(airportInfo, "\n\r:") != 0) {
        return control_error(CONTROL_INVALID_INFO_OR_ID);
    }
    /* If a mapper is specified, but it is not a valid port number*/
    if (mapperSpecified && !is_unsigned_short(mapperPort)) {
        return control_error(CONTROL_INVALID_PORT_NUMBER);
    }

    /* Bind to a free port (0) and return a socket descriptor */
    int socket = bind_to_free_port();
    /* Allow SOMAXCONN (socket.h) number of connections to queue */
    listen(socket, SOMAXCONN);
    /* Get the port which was bound to the socket, print it to stdout */
    char* controlPort = get_port_from_socket(socket);
    printf("%s\n", controlPort);
    fflush(stdout);

    /* If a mapper port is specified, report airportID to it */
    if (mapperSpecified) {
        if (!report_to_mapper(mapperPort, controlPort, airportID)) {
            return control_error(CONTROL_ERROR_CONNECTING_MAPPER);
        }
    }
    /* Await and action new connections */
    handle_connections(socket, airportInfo);

    return CONTROL_NORMAL_EXIT;
}

/**
 * Intended for calling by newly created pthreads. The function listens
 * on a stream for incoming commands and actions them appropriately according
 * to the predefined grammar (see command_lexer() for information on said 
 * grammar.)
 *
 * It is the responsibility of the calling scope to ensure that the parameter
 * 'threadArguments' is castable to the ClientArguments* struct type. The
 * calling scope should also ensure that the passed structure specifies
 * correct, valid information (when casted to a ClientArguments struct *)
 *
 * threadArguments:   A void* type which this function immediately casts
 *                    to a ClientArguments struct pointer type.
 *    
 *         Returns:   NULL, which serves no purpose other than to satisfy the
 *                    compiler.
 */
void* command_listener(void* threadArguments) {
    /* Cast the arguments to a ClientArguments struct and fetch the members */
    struct ClientArguments* arguments = 
            (struct ClientArguments*) threadArguments;
    int clientDescriptor = dup(arguments->clientDescriptor);
    FILE* sendStream = fdopen(clientDescriptor, "w");
    FILE* receiveStream = fdopen(clientDescriptor, "r");
    struct Visitors* visitors = arguments->visitors;
    pthread_mutex_t* lock = arguments->lock;
    char* airportInfo = arguments->airportInfo;

    char clientMessage[COMMAND_BUFFER_SIZE];
    /* Until the client terminates the connection */
    while (!feof(receiveStream)) {
        if (fgets(clientMessage, COMMAND_BUFFER_SIZE, receiveStream)) {
            /* Client is modifying the Visitors Structure, take the lock */
            pthread_mutex_lock(lock);
            /* Determine the type of the issued command */
            Command commandType = command_lexer(clientMessage);

            switch (commandType) {
                case LOG_COMMAND:
                    /* Print all visitors to this airport to the client */
                    print_all_visitors(visitors, sendStream);
                    break;
                case RECORD_VISIT_SEND_INFO:
                    /* Record the aircraft name as a visitor */
                    record_visit(visitors, clientMessage);
                    /* Send the information of the airport (\n terminated) */
                    fprintf(sendStream, "%s\n", airportInfo);
                    fflush(sendStream);
                    break;
            }
            /* Other clients may now modify the Visitors structure */
            pthread_mutex_unlock(lock);
        }
    }
    /* We don't want to return anything - to satisfy compiler */
    return NULL; 
}

/**
 * Sorts the specified pointer to a Visitors structure into lexicographic,
 * ascending order via bubble sort (O(n*n), for n visitor entries)
 *
 * Entries in the Visitor structure are sorted by lexicographic order by
 * the airport name that the entry holds.
 *
 * It is the responsibility of the calling scope to ensure that the 
 * Visitor structure is appropriately initialised.
 *
 * visitors:   A pointer to the Visitors structure that will be
 *             sorted into ascending, lexicographic order.
 *    
 *  Returns:   Void
 */
void sort_visitors(struct Visitors* visitors) {
    for (int i = 0; i < visitors->visitorCount; i++) {
        for (int j = i + 1; j < visitors->visitorCount; j++) {
            if (strcmp(visitors->entries[i].aircraftID, 
                    visitors->entries[j].aircraftID) > 0) {
                VisitorEntry greater = visitors->entries[i];
                VisitorEntry lesser = visitors->entries[j];
                visitors->entries[i] = lesser;
                visitors->entries[j] = greater;
            }
        }
    }
}

/**
 * Determines what type (A member of the Command enum (control2310.h))
 * of command is specified in the argument 'commandString'
 *
 * NOTE: This function will also trim newline characters
 * from a given commandString. (Newline characters are not allowed
 * to constitute a command. Unspecified behaviour results from
 * a commandString containing newline characters that are not
 * located at the end of the string. See trim_newline() to see why.)
 *
 * commandString:   A string (char*) which represents the command that
 *                  should be recognised.
 *    
 *       Returns:   LOG_COMMAND (0) iff the command was recognised to be
 *                  a request to log all visitors (to the airport)
 *                  RECORD_VISIT_SEND_INFO (1) iff the command was not
 *                  recognised as a log command (i.e. default case)
 */
Command command_lexer(char* commandString) {
    /* Remove trailing newline from 'commandString' */
    trim_newline(commandString);

    if (strcmp(commandString, "log") == 0) {
        return LOG_COMMAND;
    }
    return RECORD_VISIT_SEND_INFO;
}

/**
 * Prints out the aircraftID of all aircraft which have visited
 * the airport (contained within the Visitors structure.) Each visitor
 * entry is printed out in lexicographic, ascending order (newline 
 * separated) and is followed by the string ".\n"
 *
 * NOTE: This function has the side effect of sorting the given Visitors
 * structure into lexicographic order.
 *
 * The provided FILE stream, 'stream', should be verified to be open, 
 * valid and able to receive bytes before making a call to this function
 *
 * visitors:   A pointer to the Visitors structure whose entries will
 *             be accessed for printing.
 *   stream:   A FILE* stream used to send bytes to the client who
 *             requested the list of visitors.
 *    
 *  Returns:   Void
 */
void print_all_visitors(struct Visitors* visitors, FILE* stream) {
    sort_visitors(visitors);

    for (int i = 0; i < visitors->visitorCount; i++) {
        fprintf(stream, "%s\n", visitors->entries[i].aircraftID);
    }
    /* Print out the character '.' followed by a newline */
    fprintf(stream, ".\n");
    fflush(stream);
}

/**
 * Adds a new visitor to a given Visitors structure.
 *
 * If an aircraft with the same aircraftID (specified in commandString)
 * already exists as a Visitor entry in the Visitors structure, it 
 * is duplicated (recorded again.)
 *
 * It should first be verified that the contents of the argument 
 * 'commandString' does not contain 'log'. Which should be treated as
 * a different command. Ensuring this is the task of the calling
 * scope. See command_lexer() in this file for command recognition.
 *
 *      visitors:   A pointer to the Visitors structure which will be
 *                  modified to have an additional visitor entry.
 * commandString:   The string 'commandString' which will be recorded as a
 *                  visitor to this airport
 *    
 *       Returns:   Void
 */
void record_visit(struct Visitors* visitors, char* commandString) {
    /* Create and add a new VisitorEntry to the Visitors structure */
    VisitorEntry newVisitor;
    strcpy(newVisitor.aircraftID, commandString);
    visitors->entries[visitors->visitorCount] = newVisitor;
    /* Increase the number of visitor entries in the Visitors structure */
    visitors->visitorCount++;
}

/**
 * Handles new client connections to the port that was bound to
 * specified by the socket file descriptor.
 *
 * Upon the connection of a new client, a new thread is created to 
 * handle the client, see command_listener() for the function that
 * is executed by the thread.
 *
 * It is the responsibility of the calling scope to ensure that the
 * provided socket file descriptor is initiated and valid before
 * being used with this function. To initialise a socket file descriptor,
 * see bind_to_free_port()
 *
 * Furthermore, the calling scope should ensure that listen() is called
 * upon the socket descriptor prior to its use with this function. 
 *
 * This function has unspecified behaviour for subsequent calls. The first
 * call to this function initialises a Visitors structure, hence, a
 * subsequent call to this function would initialise a new Visitors structure.
 *
 *      socket:   A file descriptor for the socket that new connections should
 *                be accepted through.
 * airportInfo:   A string (char*) specifying the information associated with 
 *                this airport.
 *    
 *     Returns:   Void. This function shall only return upon an accept() error
 */
void handle_connections(int socket, char* airportInfo) {
    /* Initialise a new Visitors structure with space for visitor entries */
    struct Visitors visitors;
    visitors.visitorCount = 0;
    visitors.entries = malloc(sizeof(VisitorEntry) * MAX_VISITOR_ENTRIES);

    /* Initialise mutex lock */
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);

    pthread_t threadId;
    int clientDescriptor;
    /* Accept connections and create a new worker to handle requests */
    while ((clientDescriptor = accept(socket, 0, 0), clientDescriptor >= 0)) {
        /* Prepare the arguments which should be passed to the worker thread */
        struct ClientArguments* arguments = 
                malloc(sizeof(struct ClientArguments));
        arguments->clientDescriptor = clientDescriptor;
        arguments->visitors = &visitors;
        arguments->airportInfo = airportInfo;
        arguments->lock = &lock;

        pthread_create(&threadId, 0, command_listener, (void*) arguments);
    }
}

/**
 * For a specified mapper port, this function connects to the mapper
 * server and sends to it a string in the following format:
 *
 *                     !airportID:controlPort\n
 *
 * Which signifies a request to the specifed mapper server to add a mapping
 * for the airport with the id 'airportID' for a port number 'controlPort'.
 *
 * NOTE: it is the responsibility of the calling scope to ensure that the
 * port number of the mapper process is a valid port number.
 *
 *  mapperPort:   A string specifying the port number of the mapper 
 *                server that should be connected to.
 * controlPort:   A string specifying the port number of this
 *                airport which should be sent to a mapper process.
 *   airportID:   The ID(/name) of the airport that should be sent
 *                to the relevant mapper server
 *    
 *     Returns:   true iff connection and sending of data to the mapper
 *                succeeded, false otherwise
 */
bool report_to_mapper(char* mapperPort, char* controlPort, char* airportID) {
    bool success;
    /* Create a new connection to the mapper server */
    int connectionDescriptor = connect_to_port(mapperPort, &success);
    
    if (!success) {
        return false;
    }

    /* Send the airportID and port of this airport to the mapper */
    FILE* sendStream = fdopen(connectionDescriptor, "w");
    fprintf(sendStream, "!%s:%s\n", airportID, controlPort);
    fflush(sendStream);

    return true;
}