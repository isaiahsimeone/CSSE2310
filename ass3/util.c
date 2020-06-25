#include "util.h"

/**
 * Determines whether a given string is numeric (i.e. all characters 
 * in the string are digits). 
 *
 * If an empty string is specified by the input parameter, then
 * is_numeric returns false.
 *
 *   input:   The string to test numeric status.
 *
 * Returns:   true iff the string is numeric,
 *            false otherwise
 */
bool is_numeric(char* input) {
    /* If there is a special character present, do not check it */
    if (strlen(input) == 0) {
        return false;
    }
    for (int i = 0; i < strlen(input); i++) {
        if (!isdigit(input[i])) {
            return false; 
        }
    }
    return true;
}

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
 * Converts a given character into it's integer equivalent. E.g.:
 * '3' -> 3, '0' -> 0, etc.
 *
 * The character 'charNumeral' must be a character 0 through to 9 inclusive.
 * Ensuring that charNumeral is an integer is the task of the calling scope.
 * 
 * charNumeral:   The character to be converted to its integer equivalent.
 *
 *     Returns:   An integer value of the number specified in charNumeral.
 */
int char_to_int(char charNumeral) {
    return (int)(charNumeral - '0');
}

/**
 * Converts a given integer into it's character equivalent. E.g.:
 * 3 -> '3', 0 -> '0', etc.
 * 
 *  number:   The number to be converted to its character equivalent.
 *
 * Returns:   A character containing the character form of number.
 */
char int_to_char(int number) {
    return (char)(number + '0');
}

/**
 * Reads at most 'size' number of bytes of a message from stdin 
 * to a location pointed to by 'destination'
 *
 * A specified FILE stream should be verified to be open prior
 * to using it with this function.
 *
 * destination:   A character pointer which points to the location 
 *                to read potential stdin messages to
 *      stream:   The File descriptor stream to read from (e.g.
 *                stdin, stderr, etc.)
 *        size:   The maximum number of bytes to read from stdin
 *
 *     Returns:   True if data was read from stdin, false otherwise
 */
bool read_stream(char* destination, FILE* stream, int size) {
    return fgets(destination, size, stream) ? true : false;
}

/**
 * Determines the number of digits contained within an integer by
 * counting the number of times it can be divided by 10.
 *
 * EG:  integer_length(348593) = 6.
 *
 * NOTE: This function is NOT intended for use with negative integers
 * The caller is responsible for ensuring that a given number arugment
 * is > 0.
 *
 *  number:   An integer to be measured in length.
 *
 * Returns:   The length of the specified integer, 'number'.
 */
int integer_length(int number) {
    int length = 1;
    // Continue to divide by 10 until we have a number which
    // consists of only a single digit (i.e. <= 9)
    while (number > 9) {
        number /= 10;
        length++;
    }
    return length;
}

/**
 * Determines the length of a specified file Stream.
 *
 * This function has undefined behaviour when provided a stream
 * that is frequently changing (such as stderr/stdin.) If the
 * stream changes often, this function's reliability may be
 * diminished.
 *
 * A specified FILE stream should be verified to be open prior
 * to using it with this function.
 *
 * After calculating length of the fileStream, this function
 * will rewind the fileStream to the beginning before returning.
 *
 * fileStream:   A pointer to a file stream which should be measured
 *               in terms of file length.
 *
 *    Returns:   An integer: the size of the given fileStream
 */
int get_file_length(FILE* fileStream) {
    // Go to the end of the fileStream
    fseek(fileStream, 0, SEEK_END);
    // Determine the files size
    int length = ftell(fileStream);
    // rewind the file
    rewind(fileStream);
    return length;
}

/**
 * Counts the number of lines in a specified file Stream. Specifically,
 * counts the number of occurrences of the newline character in that
 * file stream.
 *
 * This function has undefined behaviour when provided a stream
 * that is frequently changing (such as stderr/stdin.) If the
 * stream changes often, this function's reliability may be
 * diminished.
 *
 * NOTE: This function will rewind a fileStream to the beginning before 
 * counting the number of lines contained.
 *
 * After counting the number of lines, this fileStream is rewound to
 * the beginning.
 *
 * A specified FILE stream should be verified to be open prior
 * to using it with this function.
 *
 * fileStream:   A pointer to a file stream to count the total number
 *               of lines in.
 *
 *    Returns:   An integer: The number of lines in the given FILE*
 */
int count_file_lines(FILE* fileStream) {
    rewind(fileStream);
    int lineCount = 0;
    // Keep searching for new line characters until EOF.
    while (!feof(fileStream)) {
        if (fgetc(fileStream) == '\n') {
            lineCount++;
        }
    }
    rewind(fileStream);
    return lineCount;
}

/**
 * Reads the first line of a given file. 
 *
 * This function should only be used when reading a single line
 * from the beginning of a file, if a multilined file is input,
 * this function will set the boolean flag 'valid' to false and
 * return an empty character ("")
 *
 * fileName:   The name of the file to open and read the first line
 *             from.
 *    valid:   A boolean pointer which will be set to true if the
 *             reading and validation process was successful, set
 *             to false otherwise.
 *
 *  Returns:   A char* (string) which contains the first line read
 *             from the specified file.
 */
char* read_file_line(char* fileName, bool* valid) {
    FILE* fileToRead = fopen(fileName, "r"); 
    // Ensure that the file opened successfully
    if (fileToRead == NULL) {
        *valid = false;
        return "";
    }
    // Ensure that the number of lines is not greater than one
    if (count_file_lines(fileToRead) > 1) {
        *valid = false;
        return "";
    }
    // Determine the length of the file line (since we already
    // confirmed that the number of lines is one)
    int fileLength = get_file_length(fileToRead);
    // Allocate memory for the file line to be read to
    char* destination = malloc(fileLength * sizeof(char));
    // read the line.
    fgets(destination, fileLength, fileToRead);
    // We don't need the file anymore, it will be rewound if it is
    // reopened elsewhere
    fclose(fileToRead);
    *valid = true;
    return destination;
}