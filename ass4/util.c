#include "util.h"

/**
 * Counts how many times the characters from a specified string of 
 * characters appears in a specified string. e.g, the characters in the 
 * string "hl" appear in the string "hello world" 4 times.
 *
 *      string:   The string to search for the specified characters.
 * occurrences:   The characters to be counted for each appearance in 
 *                the given string.
 *    
 *     Returns:   The number of times that any of the characters
 *                in 'occurences', appear in the parameter 'string'.
 */ 
int count_occurrences(char* string, char* occurrences) {
    int count = 0;
    /* Go through the string to search, and the characters in occurences */
    for (int i = 0; i < strlen(string); i++) {
        for (int j = 0; j < strlen(occurrences); j++) {
            count += (string[i] == occurrences[j]);
        }       
    }
    return count;
}

/**
 * Determines whether a given string is numeric (i.e. all characters 
 * in the string are digits).
 *
 *  string:   The string to test numeric status.
 *
 * Returns:   true iff the string is numeric,
 *            false otherwise
 */
bool is_numeric(char* string) {
    for (int i = 0; i < strlen(string); i++) {
        if (!isdigit(string[i])) {
            return false; 
        }
    }
    return true;
}

/**
 * Removes the first newline character found in a specified char*
 * If no newline (\n) character is found, then the string is
 * not modified
 *
 * NOTE: Upon encountering a newline character, this function will
 * change that encountered newline character to a null terminator (\0)
 * hence this function is intended for use in removing a TRAILING 
 * newline character. (Since it will cut a string to the length of the
 * index of the first encountered newline character.)
 *
 *  string:   The string to remove the newline character (\n) from.
 *
 * Returns:   void.
 */
void trim_newline(char* string) {
    for (int i = 0; i < strlen(string); i++) {
        if (string[i] == '\n') {
            string[i] = '\0';
            break;
        }
    }
}

/**
 * Determines whether a given string (char*) is convertable to an
 * unsigned short such that it is not negative (or 0 since this
 * function is intended for use with port numbers (and 0 is not
 * a permissible port number from a client)) and that it is less
 * than the maximum value of an unsigned short (USHRT_MAX from
 * limits.h.)
 *
 *  string:   The string to be tested for numeric status and 
 *            range (0 <= atoi(string) < USHRT_MAX)
 *
 * Returns:   true if the string specified is converatble to USHORT
 *            false otherwise.
 */
bool is_unsigned_short(char* string) {
    if (!is_numeric(string)) {
        return false;
    }
    if (atoi(string) <= 0 || atoi(string) > USHRT_MAX) {
        return false;
    }
    return true;
}

/**
 * Allocates memory on the heap for an array of char* (AKA an array
 * of strings)
 * 
 * NOTE: This function is most appropriate for storing strings (char* types)
 * of the same/similar length. For example, if one string being stored is
 * expected to have 5 characters, and another having for example 50 characters,
 * this function will allocate space for elementCount * 50 characters.
 *
 * elementCount:   The number of strings that space should be allocated for
 * stringLength:   The length of the largest string (char*) that is expected
 *                 to be contained within the string array.
 *
 *      Returns:   A char** type referencing memory allocated on the heap
 *                 in accordance with the above.
 */
char** malloc_string_array(int elementCount, int stringLength) {
    char** memoryBlock = malloc(sizeof(char*) * elementCount);
    for (int i = 0; i < elementCount; i++) {
        memoryBlock[i] = malloc(sizeof(char) * stringLength);
    }
    return memoryBlock;
}

/**
 * Given a string, 'string', this function determines a size 
 * (in bytes) that would be required to store that string, iff
 * it is greater than or equal to PORT_NUMBER_LENGTH which is 
 * the number of characters in the largest port number (including
 * null terminator so it can be represented as a string)
 *
 * This function is intended for use when allocating memory.
 *
 *  string:   A string (char*) that should be considered when calculating
 *            the return value.
 *
 * Returns:   If the length of the given string is greater than (or equal to)
 *            the number of characters in the largest port number 
 *            (PORT_NUMBER_LENGTH) then the length of the string * 
 *            sizeof(char) is returned. Otherwise, PORT_NUMBER_LENGTH * 
 *            sizeof(char) is returned.
 */
int string_or_port_size(char* string) {
    if (strlen(string) >= PORT_NUMBER_LENGTH) {
        /* Include Null terminator */
        return sizeof(char) * strlen(string);
    }
    return sizeof(char) * PORT_NUMBER_LENGTH;
}