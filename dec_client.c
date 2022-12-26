#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


/*****************************************************************************************************************************
 * This function is used to report errors when something has gone wrong (i.e. invalid usage), and exit the program with the given error code
 * parameters: message to print to stderr, error code
 * **************************************************************************************************************************/
void error(const char *msg, int code) { 
    fprintf(stderr, msg) ;
    exit(code); 
} 


/*****************************************************************************************************************************
 * This function is used to open the key file specified by the user in argv[2] and store it's data into the key variable
 * parameters: key variable to store the contents of the file, name of the key file
 * **************************************************************************************************************************/
void store_key_content(char* key, char* key_file_name){

    FILE * key_file = fopen(key_file_name, "r");

    if(key_file == NULL)
        error("The key file specified could not be opened!\n", 1);
    
    fgets(key, 255, (FILE*)key_file);
    key[strcspn(key, "\n")] = '\0'; // remove '\n' from fgets()
    fclose(key_file);

}


/*****************************************************************************************************************************
 * This function is used to open the cyphertext file specified by the user in argv[1] and store it's data into the cyphertext variable
 * parameters: cyphertext variable to store the contents of the file, name of the cyphertext file
 * **************************************************************************************************************************/
void store_cyphertext_content(char* cyphertext, char* cyphertext_file_name){

    FILE * cyphertext_file = fopen(cyphertext_file_name, "r");

    if(cyphertext_file == NULL)
        error("The cyphertext file specified could not be opened!\n", 1);

    fgets(cyphertext, 255, (FILE*)cyphertext_file); // store name of cyphertext
    cyphertext[strcspn(cyphertext, "\n")] = '\0'; // remove \n from fgets
    fclose(cyphertext_file);

}


/*****************************************************************************************************************************
 * This function is used to combine the contents of the cyphertext and the key variables into one long string.
 * Then new variable created by this, cyphertext_key looks like this: "contentsofkeyvariable|contentsofcyphertextvariable"
 * This is done so that the contents of the key and cyphertext files can be sent only using one send(). 
 * 
 * parameters: variable to store newly created key|cyphertext variable, the contents of the key file, the contents of the cyphertext file
 * **************************************************************************************************************************/
void combine_key_cyphertext(char* cyphertext_key, char* key, char* cyphertext){

    strcpy(cyphertext_key, key);
    strcat(cyphertext_key, "|");
    strcat(cyphertext_key, cyphertext);
    strcat(cyphertext_key, "\0");

}
/***********************************************************************************************************
 * This function checks to see if there are any invalid characters in the key or cyphertext files provided by the user.
 * The allowed characters are the capital letters and the space character. 
 * 
 * parameters: the contents of the key file,  the content of the cyphertext file
****************************************************************************************************************************/
void check_invalid_characters(char* key, char* cyphertext){

    int i = 0;
    for (i; i< strlen(cyphertext)- 1; i++){ // if any of the characters in either file are invalid
        if (cyphertext[i] > 90 || (cyphertext[i] < 65 && cyphertext[i] != ' '))
            error("ERROR invalid character in cyphertext", 1);
        else if (key[i] > 90 || (key[i] < 65 && key[i] != ' '))
            error("ERROR invalid character in key", 1);
    }

}




int main(int argc, char *argv[]){

    if (argc < 4) {  // checks usage and args
        fprintf(stderr,"USAGE: %s cyphertext key port\n", argv[0]); 
        exit(0); 
    } 

    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;
    struct hostent* serverHostInfo;
    char buffer[256];
    char key[256] = "\0"; 
    char cyphertext[257] = "\0";
    char cyphertext_key [515] = "\0";
    char okmessage[4] = "\0";

    memset(buffer, '\0', sizeof(buffer)); // these memsets clear the string of any garbage initialized values.
    memset(key, '\0', sizeof(key)); 
    memset(cyphertext, '\0', sizeof(cyphertext));
    memset(cyphertext_key, '\0', sizeof(cyphertext_key));
    memset(okmessage, '\0', sizeof(okmessage));
    memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct


    if (strlen(cyphertext) > 255)
        error("cyphertext is over 255 characters!", 2);

    store_cyphertext_content(cyphertext, argv[1]);

    store_key_content(key, argv[2]);



    if (strlen(key) < strlen(cyphertext)){ // the key cannot be smaller than the cyphertext file, otherwise there would be unencrypted characters.
        error("ERROR keysize too small", 1);
    }

    check_invalid_characters(key, cyphertext);

    combine_key_cyphertext(cyphertext_key, key, cyphertext);

    
    portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
    serverAddress.sin_family = AF_INET; // Create a network-capable socket
    serverAddress.sin_port = htons(portNumber); // Store the port number
    serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address. This program only works on localhost atm.


    if (serverHostInfo == NULL) { 
        error("CLIENT: ERROR, no such host\n", 2); 
    }

    memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

    // Set up the socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket

    if (socketFD < 0){ // if error opening the socket
        char errmessage[200];
        char port_as_str[10];
        strcpy(errmessage, "CLIENT: ERROR opening socket, attempted port number ");
        sprintf(port_as_str, "%d", portNumber);
        strcat(errmessage, port_as_str); // appends port to error message to give user helpful info.

        error(errmessage, 2);
    }

    
   
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect to server, if it returns less than zero, exit
        error("CLIENT: ERROR connecting", 2);


    send(socketFD, "thisdec_client", strlen("thisdec_client"), 0); // send the message "thisenc_client" to let server know that this program is enc_client, not dec_client
    charsRead = recv(socketFD, okmessage, sizeof(okmessage) - 1, 0); 
    // charRead is the message sent back by the server. Should be "OK". If "n", that means the the server rejected the client, generally because client attempted to connect to the wrong server 

    if (strcmp("NO", okmessage) == 0){ //If the message receive is NO. Only will happen if connected to dec_server.
        error("CLIENT: dec client cannot connect to enc server!", 1);
    }



    charsWritten = send(socketFD, cyphertext_key, strlen(cyphertext_key), 0); // Write to the server the cyphertext|key

    if (charsWritten < 0) // This means that something went wrong with send()
        error("CLIENT: ERROR writing to socket", 2);
    if (charsWritten < strlen(buffer))  // fewer characters were sent than what should have been sent. This often means that the server's recv did not have a large size for the send()
        printf("CLIENT: WARNING: Not all data written to socket!\n", 2);

    memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse

    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // The server sends back the plaintext 
    
    if (charsRead < 0) 
        error("CLIENT: ERROR reading from socket", 2);

    printf("%s\n", buffer); // print the plaintext to stdout

    close(socketFD); // Close the socket
    exit(0); // exit the program with stderr set to 0.
}