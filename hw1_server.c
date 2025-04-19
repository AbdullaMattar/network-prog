//HW1
// Names: Abdulla Matar (151605) + Obada Gharaibeh (152818)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>

#define MAX_LEN 80

int sockfd;

// Handle Ctrl+\ quit signal
void handle_quit(int sig) {
    printf("\nGoodbye!\n");
    usleep(100000);
    (void)sig;
    close(sockfd);
    exit(0);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    char input[MAX_LEN + 1];
    char choice[10];
    char buffer[256];
    int option;
    unsigned char arg = 0;

    // Check for correct number of arguments
    if (argc != 3) {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        return 1;
    }

    signal(SIGQUIT, handle_quit); // Register signal handler

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create TCP socket

    // Setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
    memset(&(server_addr.sin_zero), 0, 8);

    // Connect to server
    connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    while (1) {
        printf("Enter a string: ");
        fgets(input, MAX_LEN, stdin);
        input[strcspn(input, "\n")] = '\0'; // Remove newline

        // Show operation menu
        printf("Choose an operation:\n");
        printf("1. Convert to UPPERCASE\n");
        printf("2. Count characters\n");
        printf("3. Count frequency of a character\n");
        printf("4. Enter new string\n");

        printf("Your choice: ");
        fgets(choice, sizeof(choice), stdin);
        option = atoi(choice);

        if (option == 4) continue;

        if (option == 3) {
            // Get character to count
            printf("Enter character to count: ");
            arg = getchar();
            while (getchar() != '\n'); // Clear input buffer
        }

        // Build request buffer
        buffer[0] = (unsigned char)option;
        buffer[1] = arg;
        strcpy(buffer + 2, input);
        printf("sending \"%s\" to the server with requested operation %d \n", input , option);
        write(sockfd, buffer, strlen(input) + 2); // Send to server

        char response[256];
        int n = read(sockfd, response, sizeof(response)); // Read response
        if (n <= 0) {
            perror("Read error");
            break;
        }

        response[n] = '\0';
        unsigned char status = response[0];
        char *result = response + 1;

        // Check status and print result
        if (status == 0) {
            printf("received: \"%s\" from the server\n", result);
        } else {
            printf("Error: %s\n", result);
        }
    }

    close(sockfd); // Cleanup
    return 0;
}
