#include "mapper2310.h"

int main(int argc, char** argv) {
    /* Bind to a free port (0) and return a socket descriptor */
    int socket = bind_to_free_port();

    /* Allow SOMAXCONN (socket.h constant) number of connections to queue */
    listen(socket, SOMAXCONN);

    /* Print the bound port to stdout */
    printf("%s\n", get_port_from_socket(socket));
    fflush(stdout);

    /* Await and action new connections */
    handle_connections(socket);

    /* This server runs until killed. Fatal error if we reach here */
    return MAPPER_FATAL_ERROR;
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
 * threadArgs:   A void* type which this function immediately casts
 *               to a ClientArguments struct pointer type.
 *    
 *    Returns:   NULL, which serves no purpose other than to satisfy the
 *               compiler.
 */
void* command_listener(void* threadArgs) {
    /* Cast the arguments to a ClientArguments struct and fetch the members */
    struct ClientArguments* arguments = (struct ClientArguments*) threadArgs;
    int clientDescriptor = dup(arguments->clientDescriptor);
    FILE* sendStream = fdopen(clientDescriptor, "w");
    FILE* receiveStream = fdopen(clientDescriptor, "r");
    struct Mapping* mapping = arguments->mapping;
    pthread_mutex_t* lock = arguments->clientLock;

    char clientMessage[COMMAND_BUFFER_SIZE];
    /* Until the client terminates the connection */
    while (!feof(receiveStream)) {
        if (fgets(clientMessage, COMMAND_BUFFER_SIZE, receiveStream)) {
            /* Client is modifying the mapping Structure, lock it */
            pthread_mutex_lock(lock);
            /* Determine the type of the issued command */
            Command commandType = command_lexer(clientMessage);

            switch (commandType) {
                case LIST_MAPPINGS:
                    /* Print all mappings back via the client's stream */
                    print_mappings(mapping, sendStream);
                    break;
                case QUERY_MAPPING:
                    /* Print queried mapping back to the client */
                    print_queried_mapping(mapping, clientMessage, sendStream);
                    break;
                case ADD_MAPPING:
                    /* Add a new entry to the mapping structure */
                    add_map_entry(mapping, clientMessage);
                    break;
                case UNKNOWN_COMMAND:
                    break;                                   
            }
            /* Other clients may now modify the Mapping structure */
            pthread_mutex_unlock(lock);
        }
    }
    /* We don't want to return anything - to satisfy compiler */
    return NULL;
}

/**
 * Determines what type (A member of the Command enum (mapper2310.h))
 * of command is specified in the argument 'commandString'
 *
 * NOTE: This function will also trim newline characters
 * from a given commandString. (Newline characters are not allowed
 * to constitute a command. Unspecified behaviour results from
 * a commandString containing newline characters that are not
 * located at the end of the string. See trim_newline() to see why.)
 *
 * commandString:   A string (char*) which represents the
 *                  command that should be recognised.
 *    
 *       Returns:   LIST_MAPPINGS (0) iff the command was recognised
 *                  as a list mappings command
 *                  QUERY_MAPPING (1) iff the command was recognised
 *                  as a request for a port number (from an airport name)
 *                  ADD_MAPPING (2) iff the command was recognised
 *                  as a request to add an airport name -> port number mapping.
 *                  Otherwise, returns UNKNOWN_COMMAND (-1)
 */
Command command_lexer(char* commandString) {
    /* Remove trailing newline from 'commandString' */
    trim_newline(commandString);

    if (strcmp(commandString, "@") == 0) {
        return LIST_MAPPINGS;
    }
    if (commandString[0] == '?') {
        return QUERY_MAPPING;
    }
    /* commandString represents an ADD_MAPPING command if: it has only one ':'
     * character, Is over 3 characters (including '\0'), has no ':' character
     * at the second, or last index of the string. And, has no '\r' or '\n' */
    if (commandString[0] == '!' && count_occurrences(commandString, ":") == 1
            && strlen(commandString) > 3 && commandString[1] != ':' 
            && commandString[strlen(commandString) - 1] != ':'
            && count_occurrences(commandString, "\r\n") == 0) {
        return ADD_MAPPING;
    }
    return UNKNOWN_COMMAND;
}

/**
 * Adds a mapping for an airport name and a port number (as contained
 * within the specified commandString sent by a client.) This mapping
 * entry is added for all connected clients.
 *
 * If a mapping already exists for the specified airport name, then the
 * add command is silently discarded, making no changes.
 *
 * It should first be verified that the contents of the argument 
 * 'commandString' are actually an add mapping (command) (and not for 
 * example a list all command.) Ensuring this is the task of the
 * calling scope. See command_lexer() for command recognition.
 *
 *       mapping:   A pointer to the Mapping structure that the new airport
 *                  name -> port number mapping entry will be added to
 * commandString:   The command that was sent by the client (which was 
 *                  identifed to be a command to add a mapping entry)
 *                  which contains the airport name to port number mapping.
 *                  
 *       Returns:   Void
 */
void add_map_entry(struct Mapping* mapping, char* commandString) {
    /* Advance passed the '!' keyword in commandString */
    commandString++;

    /* Tokenise commandString into airport name and port number */
    char* airportName = strtok(commandString, ":");
    char* portNumber = strtok(NULL, ":");

    /* Check that the provided port number is valid */
    if (!is_unsigned_short(portNumber)) {
        return;
    }

    /* Create a MapEntry structure to append to the mapping structure */
    MapEntry newEntry;
    strcpy(newEntry.airportName, airportName);
    strcpy(newEntry.portNumber, portNumber);

    /* Check that there is no entry already in the Mapping structure with
     * the same name as specified in commandString (airportName) */
    if (name_index_in_mapping(mapping, airportName) == ENTRY_NOT_FOUND) {
        mapping->entries[mapping->entryCount] = newEntry;
        mapping->entryCount++;
    }
}

/**
 * For a given command (commandString), this function prints to the stream
 * specified (stream) the port number of the airport name specified in
 * 'commandString'.
 *
 * If no entry for the specified airport name is found, this function
 * prints to the specified stream ";\n".
 *
 * It should first be verified that the contents of the argument 
 * 'commandString' are actually a query (command) for a port number.
 * (and not for example a list all command.) Ensuring this is the task
 * of the calling scope. See command_lexer() for command recognition.
 *
 * The provided client stream, 'stream' should be verified to be open, 
 * valid and able to receive bytes.
 *
 *       mapping:   A pointer to the Mapping structure that should be
 *                  searched for a matching airport name.
 * commandString:   (search string) The command that was sent by the client
 *                  (which was identifed to be a query for a port number.)
 *        stream:   A FILE* stream used to send bytes to the client
 *                  that requested the port number of the given airport name
 *
 *       Returns:   Void
 */
void print_queried_mapping(struct Mapping* mapping, char* commandString, 
        FILE* stream) {
    /* Advance passed the control character (?) to get the search string */
    commandString++;

    /* Determine the index of the entry that contains the search string */
    int entryIndex = name_index_in_mapping(mapping, commandString);

    if (entryIndex != ENTRY_NOT_FOUND) {
        fprintf(stream, "%s\n", mapping->entries[entryIndex].portNumber);
    } else {
        fprintf(stream, ";\n");
    }

    fflush(stream); 
}

/**
 * Prints out the entries contained within the Mapping structure to
 * a specified client stream. Each Mapping entry is printed in the
 * following format:
 *
 *                    AIRPORTNAME:PORTNUMBER
 *
 * Followed by a newline character. All entries contained within the
 * 'mapping (argument) are printed to the client stream in lexicographic,
 * ascending order.
 *
 * NOTE: This function has the side effect of sorting the given mapping
 * structure into lexicographic order.
 *
 * The provided client stream, 'stream', should be verified to be
 * open, valid and able to receive bytes.
 *
 * mapping:   A pointer to the Mapping structure whose entries will
 *            be accessed.
 *  stream:   A FILE* stream used to send bytes to the client
 *            who requested the mappings.
 *    
 * Returns:   Void
 */
void print_mappings(struct Mapping* mapping, FILE* stream) {
    sort_mapping(mapping);

    for (int i = 0; i < mapping->entryCount; i++) {
        MapEntry entry = mapping->entries[i];
        fprintf(stream, "%s:%s\n", entry.airportName, entry.portNumber);
    }
    fflush(stream);
}

/**
 * Sorts the specified pointer to a Mapping structure into lexicographic
 * ascending order via bubble sort (O(n*n), for n map entries)
 *
 * Entries in the Mapping structure are sorted by lexicographic order
 * by the airport name that the entry holds.
 *
 * It is the responsibility of the calling scope to ensure that the 
 * Mapping structure is appropriately initialised.
 *
 * mapping:   A pointer to the Mapping structure that will be
 *            sorted into ascending, lexicographic order.
 *    
 * Returns:   Void
 */
void sort_mapping(struct Mapping* mapping) {
    for (int i = 0; i < mapping->entryCount; i++) {
        for (int j = i + 1; j < mapping->entryCount; j++) {
            if (strcmp(mapping->entries[i].airportName, 
                    mapping->entries[j].airportName) > 0) {
                MapEntry greater = mapping->entries[i];
                MapEntry lesser = mapping->entries[j];
                mapping->entries[i] = lesser;
                mapping->entries[j] = greater;
            }
        }
    }
}

/**
 * Searches in the entries member of a given Mapping structure for
 * the index of the mapping entry that contains the given airport
 * name (airportName.)
 *
 *     mapping:   A pointer to the Mapping structure whose Map entries
 *                should be searched for the corresponding airport name.
 * airportName:   A string (char*) specifying the name of the port that
 *                should be searched for.
 *    
 *     Returns:   The index of the specified port (by its airport name)
 *                contained within the 'entries' member of the mapping
 *                structure provided. OR, ENTRY_NOT_FOUND if the specified
 *                airport name was not found in any mapping structure entry
 */
int name_index_in_mapping(struct Mapping* mapping, char* airportName) {
    for (int i = 0; i < mapping->entryCount; i++) {
        /* Return the index of the matching mapping entry */
        if (strcmp(airportName, mapping->entries[i].airportName) == 0) {
            return i;
        }
    }
    return ENTRY_NOT_FOUND;
}

/**
 * Handles new client connections to the port that was bound to and 
 * specified by the socket file descriptor
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
 *  socket:   A file descriptor for the socket that new connections should
 *            be accepted through.
 *    
 * Returns:   Void. This function shall only return upon an accept() error
 */
void handle_connections(int socket) {
    /* Initialise a new mapping structure with space for mapping entries */
    struct Mapping mapping;
    mapping.entryCount = 0;
    mapping.entries = malloc(sizeof(MapEntry) * MAX_MAP_ENTRIES);

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
        arguments->mapping = &mapping;
        arguments->clientLock = &lock;

        pthread_create(&threadId, 0, command_listener, (void*) arguments);
    }
}