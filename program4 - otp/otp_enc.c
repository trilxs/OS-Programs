/*
    Author: Tam Lu
    Date: 11/28/2017
    Description: OTP encryption client
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#define MAX_SIZE 9999

char* getFileContent(char* filename);
int validateContent(char* file);
void sendText(char* text, char* key, int socketFD);
void outputText(char* text, int socketFD);

int main( int argc, char *argv[]) {
    int socketFD, connectResult, portNumber;
    struct sockaddr_in serverAddress;
    struct hostent* serverHostInfo;
    char buffer[MAX_SIZE];

    if (argc < 4) {
        fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]); fflush(stdout);
        exit(1);
    }

    // set up the server address struct
    memset ((char*) &serverAddress, '\0', sizeof(serverAddress)); // clear out the struct before use
    portNumber = atoi(argv[3]); //get the port number from the arguments and then convert into integer
    serverAddress.sin_family = AF_INET; // create network-capable socket
    serverAddress.sin_port = htons(portNumber); //store port number (converted LSB using htons)
    serverHostInfo = gethostbyname("localhost"); 
    if (serverHostInfo == NULL) {
        fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
        exit(1);
    }
    memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // if everything is okay then copy into address

    //set up the socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
        fprintf(stderr, "CLIENT: ERROR opening socket\n"); fflush(stdout);
        exit(1);
    }

    //connect socket to server
    if (connect(socketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
        fprintf(stderr, "CLIENT: ERROR connecting socket to server\n"); fflush(stdout);
        exit(1);
    }

    // ensure connection to the otp_enc_d server
    char serverAuth[] = "e";
    write(socketFD, serverAuth, sizeof(serverAuth));
    read(socketFD, buffer, sizeof(buffer));
    if (strcmp(buffer, "e") != 0) {
        fprintf(stderr,"CLIENT: ERROR connecting to port\n"); fflush(stdout);
        exit(2);
    }

    // get file content
    char* text = getFileContent(argv[1]);
    char* key = getFileContent(argv[2]);

    if (strlen(text) > strlen(key)) {
        fprintf(stderr, "CLIENT: '%s' is too short\n", argv[2]);
        exit(1);
    }
    // validate file content
    if (validateContent(text) != 0 || validateContent(key) != 0) {
        fprintf(stderr, "CLIENT: File content is invalid\n"); fflush(stdout);
        exit(1);
    }

    // send to server
    sendText(text, key, socketFD);
    // output cyphertext
    outputText(text, socketFD);
    /*memset (buffer, '\0', sizeof(buffer));
    if (read(socketFD, buffer, sizeof(buffer)) < 0) {
        fprintf(stderr, "CLIENT: ERROR reading from socket\n"); fflush(stdout);
        exit(1);
    }
    printf("%s\n", buffer);*/

	close(socketFD); // Close the socket
    return 0;    
}


int validateContent(char* file) {
    int i, length;
    length = strlen(file);
    // check if the file contains invalid characters (not an upper case letter or space)
    for (i = 0; i < length-1; i++) {
        if ((file[i] < 'A' || file[i] > 'Z') && file[i] != ' ')  {
            return -1;
        }
    }
    return 0;
}

void outputText(char* text, int socketFD) {
    int i=0, charsRead;
    int bufferLength = 1000;

    // set up the buffer
    char buffer[bufferLength+1];
    // clear buffer for use
    memset(buffer, '\0', bufferLength+1);

    // set up cipher text length 
    char* cipherText = malloc((strlen(text)+1)*sizeof(char));

    // clear for use
    memset(cipherText, '\0', strlen(text)+1);

    // Receive chunks to buffer, then concat to cipherText
    while (i < strlen(text)) {

        memset(buffer, '\0', bufferLength + 1);
        charsRead = recv(socketFD, buffer, bufferLength, 0);

        if (charsRead < 0) {
            fprintf(stderr, "CLIENT: ERROR reading from socket\n"); fflush(stdout); 
            exit(1);
        }

        strcat(cipherText, buffer);

        i += strlen(buffer);
    }

    // output the cipherText
    printf("%s\n", cipherText); fflush(stdout);
}

void sendText(char* text, char* key, int socketFD) {
    int i=0, charsWritten;
    int mergedTextLength = strlen(text) + strlen(key) + 5;
    int bufferLength = 1000;

    // set up the buffer
    char buffer[bufferLength+1];
    // clear buffer for use
    memset(buffer, '\0', bufferLength+1);
    
    // set up the char array with the sum of the two file's lengths
    char* mergedText = malloc((mergedTextLength)*sizeof(char));

    // merge the two variables and split with ## and @@ to parse later
    sprintf(mergedText, "%s##%s@@", text, key);
    
    while (i < mergedTextLength) {
        buffer[i%bufferLength] = mergedText[i];

        // if end, then send 
        if (mergedText[i+1] == '\0' || (i+1)%bufferLength == 0) {
            charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
            
            // clear buffer for use
            memset(buffer, '\0', bufferLength+1);
            
            if (charsWritten < 0)  {
                fprintf(stderr, "CLIENT: ERROR writing to socket\n"); fflush(stdout);
                exit(1);
            }
        }
        i++;
    }
    return;
}

char* getFileContent(char* filename) {
    FILE *fp = fopen(filename, "r");
    char *content;
    size_t n = 0;
    int c, num = 0;
    int limit = MAX_SIZE;

    // if can't open file
    if (fp == NULL) {
        fprintf(stderr, "CLIENT: ERROR reading '%s'.\n", filename);
        exit(1);
    }

    content = malloc(MAX_SIZE);

    // take in file contents and put into content array
    while ((c = fgetc(fp)) != '\n') {
        if (num <= limit-10) {
            limit+=1000;
            char* tempContent = realloc(content, limit*sizeof(content+1));
            if (tempContent) {
                content = tempContent;
            }
            else {
                fprintf(stderr, "SERVER: ERROR reallocating memory\n"); fflush(stdout);
                exit(1);
            }
        }
        content[n++] = (char) c;
    }

    // add in null terminator
    content[n] = '\0';
    fclose(fp);
    return content;
}
