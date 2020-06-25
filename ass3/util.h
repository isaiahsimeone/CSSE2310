#ifndef HEADER_UTIL
#define HEADER_UTIL

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

/* Function Declarations */
int char_to_int(char charNumeral);

char int_to_char(int number);

int count_occurrences(char* string, char* occurrences);

bool is_numeric(char* input); 

bool read_stream(char* destination, FILE* stream, int size);

int integer_length(int number);

int get_file_length(FILE* fileStream);

char* read_file_line(char* fileName, bool* valid);

int count_file_lines(FILE* fileStream);

#endif