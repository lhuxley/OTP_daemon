#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


//global variables
int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
socklen_t sizeOfClientInfo;
char buffer[1000];
struct sockaddr_in serverAddress, clientAddress;


/*****************************************************************************************************************************
 * This function is used to report errors when something has gone wrong (i.e. invalid usage), and exit the program with the given error code
 * parameters: message to print to stderr, error code
 * **************************************************************************************************************************/

void error(const char *msg, int code) { 
    perror(msg);
    exit(code); 
} // Error function used for reporting issues


/*****************************************************************************************************************************
 * This function applies the encryption algorithm to plaintext, using the key file. After this function is completed, cyphertext should
 * have the encrypted characters.
 * 
 * parameters: plaintext and key given by enc_client, cyphertext variable
 * **************************************************************************************************************************/

void perform_encryption(char* plaintext, char* key, char* cyphertext) {
    int length = strlen(plaintext);
    int i = 0;
    for (i; i < length; i++) {

        // Convert plaintext and key to numerical values
        char p = (plaintext[i] == 32) ? 0 : plaintext[i] - 64;
        char k = (key[i] == 32) ? 0 : key[i] - 64;

        // Perform encryption
        char c = (p + k) % 27;

        // Convert result back to character
        cyphertext[i] = (c == 0) ? 32 : c + 64;
    }
}


/*****************************************************************************************************************************
 * Takes the buffer given by enc_client (plaintext_key variable in enc_client), and seperates into key and plaintext
 * 
 * parameters: buffer to be parsed, seperated key and plaintext values
 * **************************************************************************************************************************/
void parse_buffer(char* buffer, char* key, char* plaintext) {
    // Extract key
    char* token = strtok(buffer, "|");
    strcpy(key, token);

    // Extract plaintext
    token = strtok(NULL, "\0");
    strcpy(plaintext, token);
}


/*****************************************************************************************************************************
 * The core of the servers processes. Is in an infinite while loop so that server can accept multiple simultaneous connections. Upon accepting 
 * a connection from the client, will fork, allowing the child to process the client's demands. The parent simply goes back into while loop, waiting
 * for another connection from enc_client. Sends back to the client the cyphertext version of the plaintext.
 * 
 * **************************************************************************************************************************/
void accept_and_fork(){
    // Accept a connection, blocking if one is not available until one connects
    sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
    establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
    int fo = fork(); // fork
    
    if (fo == 0){ // if child process
        if (establishedConnectionFD < 0) 
            error("ERROR on accept", 2);
        printf("SERVER: Connected Client at port %d\n", ntohs(clientAddress.sin_port));
        // Get the message from the client and display it
        memset(buffer, '\0', sizeof(buffer));

        char auth [50] = "\0";

        charsRead = recv(establishedConnectionFD, auth, 255, 0); // Read the client's message from the socket
        if (charsRead < 0) 
            error("ERROR reading from socket", 1);

        
        if (strcmp(auth,"thisenc_client") != 0){ // if connected to non enc_client
            charsRead = send(establishedConnectionFD, "NO", 3, 0); 
            error("ERROR connection refused", 1);
        }

        
        charsRead = send(establishedConnectionFD, "OK", 3, 0); // Send success back
        if (charsRead < 0) 
            error("ERROR writing to socket", 1);

        charsRead = recv(establishedConnectionFD, buffer, 600, 0); // Read the client's message from the socket

        char key[256] = "\0";
        char plaintext[256] = "\0";
        char cyphertext[256] = "\0";

        parse_buffer(buffer, key, plaintext);
        perform_encryption(plaintext, key, cyphertext);


        // Send a Success message back to the client
        charsRead = send(establishedConnectionFD, cyphertext, strlen(cyphertext), 0); // Send success back
    

        if (charsRead < 0) 
            error("ERROR writing to socket", 1);

        close(establishedConnectionFD); // Close the existing socket which is connected to the client
    }
    //if parent process continue in while loop
}




int main(int argc, char *argv[]){

    if (argc < 2) { 
        fprintf(stderr,"USAGE: %s port\n", argv[0]); 
        exit(0); 
    } // Check usage & args
    // Set up the address struct for this process (the server)
    memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
    portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
    serverAddress.sin_family = AF_INET; // Create a network-capable socket
    serverAddress.sin_port = htons(portNumber); // Store the port number
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process
    // Set up the socket
    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket

    if (listenSocketFD < 0) 
        error("ERROR opening socket", 2);

    if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
        error("ERROR on binding", 2);
        listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
    while (1){
        accept_and_fork();
    }

    close(listenSocketFD); // This should probably never happen, the server must exit with the SIGINT or SIGSTP signal
    exit(0);
}