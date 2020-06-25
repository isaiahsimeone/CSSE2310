#ifndef UTIL_HEADER
#define UTIL_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h> 

/* The number of digits contained in the largest port 
 * number (including a null terminator character) */
#define PORT_NUMBER_LENGTH 6

/* Function Declarations */
void trim_newline(char* string);

int count_occurrences(char* string, char* occurrences);

bool is_numeric(char* string);

bool is_unsigned_short(char* string);

char** malloc_string_array(int elementCount, int stringLength);

int string_or_port_size(char* string);

#endif