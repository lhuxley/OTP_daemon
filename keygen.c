#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {
    srand(time(NULL));

    // Check if a key length was specified
    if (argc < 2) {
        fprintf(stderr, "Error: No key length specified\n");
        return 1;
    }

    int key_length = atoi(argv[1]);

    char mykey[key_length + 2];
    memset(mykey, 0, strlen(mykey));

    int i = 0;
    for (i; i < key_length; i++) {
        int random = rand() % 27; // get random number between (including) 0 - 26

        if (random == 0) {
            random = 32; // make it a space
        } else {
            random = random + 64; // make it capital
        }

        mykey[i] = random; // store that value
    }

    mykey[key_length] = '\n';
    mykey[key_length + 1] = '\0';
    printf("%s", mykey);

    return 0;
}
