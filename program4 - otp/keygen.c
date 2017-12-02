/*
    Author: Tam Lu
    Date: 11/28/2017
    Description: Key generator for encryption/decryption use.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int main( int argc, char *argv[]) {

    int randchar, keylen; //initiate random character and key length variable for later use
    srand(time(NULL)); // randomizes seed every launch

    if (argc != 2 && argc != 4) 
        {
            printf("ERROR: enter proper key length and/or output file.\n"); fflush(stdout);
            exit(1); // if the lengths are not 2 or 4, then return error as it's not an stdout or an output argument
        }

    keylen = atoi(argv[1]);
    char* key = malloc(sizeof(char)*keylen+2); //create the string for the key, one extra for null terminator and another for newline
    memset(key, '\0', sizeof(key)); //set the end of key to null terminator

    int i;
    // generate random allowed character (27 characters possible)
    for (i = 0; i < keylen; i++) {
        randchar = rand() % 27;
        if (randchar < 26) key[i] = 'A' + randchar;
        else key[i] = ' ';
    }

    // refer to line 14
    key[keylen] = '\n'; 
    key[keylen+1] = '\0';

    printf("%s", key); fflush(stdout); // write to file

    free(key);
    exit(0);
}