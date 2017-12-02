/*
    Author: Tam Lu
    Date: 11/28/2017
    Description: OTP decryption
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

void decrypt(int establishedConnectionFD);
char* getDecipherText(char* mergedText, int mergedTextLength);
char* createDecipherText(char* text, char* key);
void sendText(char* decipherText, int establishedConnectionFD);
int charToInt(char n);
char intToChar(int n);

int main( int argc, char *argv[]) {
    int listenSocketFD, establishedConnectionFD, portNumber, status;
    socklen_t sizeOfClientInfo;
    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;
    char buffer[256];

    if (argc != 2) {
        fprintf(stderr, "SERVER USAGE: %s listening_port \n", argv[0]);  
        exit(1);
    }

    // set up the server address struct
    memset ((char*) &serverAddress, '\0', sizeof(serverAddress)); // clear out the struct before use
    portNumber = atoi(argv[1]); //get the port number from the arguments and then convert into integer
    serverAddress.sin_family = AF_INET; // create network-capable socket
    serverAddress.sin_port = htons(portNumber); //store port number (converted LSB using htons)
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    //set up the socket
    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocketFD < 0) {
        fprintf(stderr, "SERVER: ERROR opening socket\n"); fflush(stdout);
        exit(1);
    }

    //Time to bind the program to an address
    if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {// Connect socket to port
        fprintf(stderr, "SERVER: ERROR on binding\n"); fflush(stdout);
        exit(1);
    }
    
    listen(listenSocketFD, 5);
    //connect socket to server
    while (1) {
        sizeOfClientInfo = sizeof(clientAddress);
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *) &clientAddress, &sizeOfClientInfo);
        
        if (establishedConnectionFD > 0) {
            pid_t pid = fork();
            if (pid < 0) { 
                fprintf(stderr, "SERVER: ERROR forking\n"); fflush(stdout);
                break; 
            }
            else if (pid == 0 ) {
                int childProcess = getpid();

                // check if client is otp_enc
                read(establishedConnectionFD, buffer, sizeof(buffer)-1);
                
                if (strcmp(buffer, "d") == 0) {
                    // write back correct port
                    char response[] = "d";
                    write(establishedConnectionFD, response, sizeof(response));
                } 
                else { 
                    //write error back to client by sending a random character
                    char response[] = "i";
                    write(establishedConnectionFD, response, sizeof(response));
                    exit(2);
                }
                // start the main decryption process after fork
                decrypt(establishedConnectionFD);

                close(establishedConnectionFD);
                while (pid > 0){ 	//parent process. wait for children to finish
                    pid = waitpid(-1, &status, WNOHANG);
                }
            }
        }
    }
    
    close(listenSocketFD);
    return 0;
}

void decrypt(int establishedConnectionFD) {
    int i=0, charsRead;
    size_t mergedTextLength = 1000;
    size_t bufferLength = mergedTextLength;
    // set up the buffer
    char buffer[bufferLength+1];
    // clear buffer for use
    memset(buffer, '\0', bufferLength+1);

    char* mergedText = malloc(mergedTextLength*sizeof(mergedText+1));

    // receive parts/all of the value sent from the client and place it into mergedText
    while (strstr(mergedText, "@@") == NULL) {
        if (strlen(mergedText) > mergedTextLength-10) {
            // allocate 1000 more spaces into the array if there is not enough space
            mergedTextLength += 1000;
            char* tempArray = realloc(mergedText, mergedTextLength*sizeof(mergedText+1));
            if (tempArray) {
                mergedText = tempArray;
            }
            else {
                fprintf(stderr, "SERVER: ERROR reallocating memory\n"); fflush(stdout);
                exit(1);
            }
        }
        // clear buffer for use
        memset(buffer, '\0', bufferLength+1);

        charsRead = recv(establishedConnectionFD, buffer, bufferLength, 0);
        if (charsRead < 0) {
            fprintf(stderr, "CLIENT: ERROR reading from socket\n"); fflush(stdout); 
            exit(1);
        }
        strcat(mergedText, buffer);
    }
    
    // split the merged text into a text and a key variable to encrypt
    char* decipherText = getDecipherText(mergedText, mergedTextLength);

    // send it to client
    sendText(decipherText, establishedConnectionFD);
}

void sendText(char* decipherText, int establishedConnectionFD) {
    int i=0, charsWritten;
    int bufferLength = 1000;

    // set up the buffer
    char buffer[bufferLength+1];
    // clear buffer for use
    memset(buffer, '\0', bufferLength+1);

    while (i < strlen(decipherText)) {
        buffer[i%bufferLength] = decipherText[i];

        // if end, then send 
        if (decipherText[i+1] == '\0' || (i+1)%bufferLength == 0) {
            charsWritten = send(establishedConnectionFD, buffer, strlen(buffer), 0); // Write to the server
            
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

char* getDecipherText(char* mergedText, int mergedTextLength) {
    // textIsFilled is toggled once we find "#", because that marks the end of the text
    int textIsFilled = 0;
    int i = 0, keyLength = 0, textLength = 0;
    // allocate space into key and text
    char* key = malloc(mergedTextLength*sizeof(mergedText));
    char* text = malloc(mergedTextLength*sizeof(mergedText));
    
    memset(key, '\0', sizeof(key));
    memset(text, '\0', sizeof(text));
    // read merged text until "##" to put into text variable
    for (i = 0; i < strlen(mergedText);i++) {
        // if the merged text is at @ then it has reached the end
        if (mergedText[i] == '@') break;
        // if the merged text is not #
        if(mergedText[i] != '#') {
            // if the merged text has not reached the end and text has been filled
            if (textIsFilled == 1) {
                key[keyLength] = mergedText[i];
                keyLength++;
            }
            else if (textIsFilled == 0) {
                text[textLength] = mergedText[i];
                textLength++;
            }
        }
        else textIsFilled = 1;
    }

    // put null terminator at the end
    text[textLength] = '\0';
    key[keyLength] = '\0';

    return createDecipherText(text, key);
}

char* createDecipherText(char* text, char* key) {
    int i;
    int keyNum, textNum, decipherNum;
    char decipherChar;
    char* decipherText = malloc((strlen(text)+1)*sizeof(char));
    for (i = 0; i < strlen(text);i++) {
        // if char is space, then make the num 26, else if it's an actual num, then subtract by 'A' to find its char value
        if (text[i] != ' ') textNum = charToInt(text[i]);
        else textNum = 26;
        // do the same thing for key
        if (key[i] != ' ') keyNum = charToInt(key[i]);
        else keyNum = 26;
        
        // convert into decipher integer
        decipherNum = textNum-keyNum;
        if (decipherNum < 0) decipherNum += 27;

        // convert this decipher integer into a char to place into array
        if(decipherNum != 26) decipherChar = intToChar(decipherNum); 
        else decipherChar = ' ';
        decipherText[i] = decipherChar;
    }
    
    // put null terminator at the end
    decipherText[strlen(text)] = '\0';
    return decipherText;
}

int charToInt(char n) {
    //subtract the ascii value of A to get the integer value
    return n-65;
}

char intToChar(int n) {
    //add the ascii value of A to get the character value
    return n+65;
}
