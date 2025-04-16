#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAXSTR 80
#define BUFSIZE 256

int sockfd;

void sigquit_handler(int sig) {
    printf("\nClient_side > Good bye\n");
    close(sockfd);
    exit(0);
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <server-ip> <server-port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in serv_addr;
    int port = atoi(argv[2]);
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }
    memset(serv_addr.sin_zero, 0, sizeof(serv_addr.sin_zero));
    
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connect error");
        exit(EXIT_FAILURE);
    }
    
    signal(SIGQUIT, sigquit_handler);
    
    char inputStr[MAXSTR+1];
    char menuChoice[10];
    while(1) {
        printf("Client_side> please enter a string: ");
        if(fgets(inputStr, sizeof(inputStr), stdin) == NULL)
            break;
        inputStr[strcspn(inputStr, "\n")] = 0;
        
        int operation;
        unsigned char arg = 0;
        while(1) {
            printf("Client_side> select an operation:\n");
            printf("  1. Change string to capital letters\n");
            printf("  2. Count number of characters\n");
            printf("  3. Count frequency of a character\n");
            printf("  4. Enter another string\n");
            printf("Your choice: ");
            if(fgets(menuChoice, sizeof(menuChoice), stdin) == NULL)
                continue;
            operation = atoi(menuChoice);
            if(operation >= 1 && operation <= 4)
                break;
            printf("Invalid choice. Try again.\n");
        }
        
        if(operation == 4)
            continue;
        
        if(operation == 3) {
            printf("Client_side> Enter the character: ");
            char ch = getchar();
            while(getchar() != '\n');
            arg = ch;
        }
        
        char request[BUFSIZE];
        request[0] = (unsigned char) operation;
        request[1] = arg;
        strcpy(request+2, inputStr);
        
        printf("Client_side> sending \"%s\" to the server with requested operation %d\n", inputStr, operation);
        write(sockfd, request, strlen(request+2) + 2);
        
        char response[BUFSIZE];
        ssize_t n = read(sockfd, response, sizeof(response)-1);
        if(n <= 0) {
            perror("Read error");
            break;
        }
        response[n] = '\0';
        unsigned char status = response[0];
        char *result = response + 1;
        if(status == 0)
            printf("Client_side> received \"%s\" from the server\n", result);
        else
            printf("Client_side> error: \"%s\"\n", result);
    }
    
    close(sockfd);
    return 0;
}

