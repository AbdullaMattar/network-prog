#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXSTR 80
#define BUFSIZE 256
#define BACKLOG 10

void sigchld_handler(int s) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void process_request(int client_fd, struct sockaddr_in client_addr, char *buffer, ssize_t n) {
    buffer[n] = '\0';
    unsigned char req_type = buffer[0];
    unsigned char req_arg = buffer[1];
    char *str = buffer + 2;

    printf("Server_side > received \"%s\" from %s:%d with request No. %d\n",
           str,
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port),
           req_type);
    
    char reply[BUFSIZE] = {0};
    unsigned char status = 0;
    
    if(req_type == 1) { // Capitalize string
        for (int i = 0; i < strlen(str); i++) {
            str[i] = toupper(str[i]);
        }
        snprintf(reply, sizeof(reply), "%s", str);
    } else if(req_type == 2) { // Count characters
        int len = strlen(str);
        snprintf(reply, sizeof(reply), "%d", len);
    } else if(req_type == 3) { // Frequency of a character
        char ch = req_arg;
        int count = 0;
        for (int i = 0; i < strlen(str); i++) {
            if(str[i] == ch)
                count++;
        }
        if(count == 0) {
            status = 1;
            snprintf(reply, sizeof(reply), "Character not found");
        } else {
            snprintf(reply, sizeof(reply), "%d", count);
        }
    } else {
        status = 1;
        snprintf(reply, sizeof(reply), "Invalid operation");
    }
    
    char response[BUFSIZE];
    response[0] = status;
    strcpy(response+1, reply);
    write(client_fd, response, strlen(reply)+1);
    
    if(status == 0)
        printf("Server_side > Sending \"%s\" to the client\n", reply);
    else
        printf("Server_side > Sending error: \"%s\" to the client\n", reply);
}

int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int listen_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;
    int port = atoi(argv[1]);
    
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), 0, 8);
    
    if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    
    if(listen(listen_fd, BACKLOG) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    printf("Server_side > waiting for client connections on port %d.\n", port);
    
    while(1) {
        sin_size = sizeof(client_addr);
        if((client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
            perror("accept");
            continue;
        }
        
        printf("Server_side > Connected to %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        
        // Handle persistent connection for one client
        while(1) {
            char buffer[BUFSIZE];
            ssize_t n = read(client_fd, buffer, sizeof(buffer)-1);
            if(n <= 0) {
                printf("Server_side > received exit from client... Goodbye\n");
                break;
            }
            pid_t pid = fork();
            if(pid < 0) {
                perror("fork");
                break;
            } else if(pid == 0) {
                process_request(client_fd, client_addr, buffer, n);
                close(client_fd);
                exit(0);
            } else {
                int status;
                waitpid(pid, &status, 0);
            }
        }
        close(client_fd);
    }
    
    return 0;
}

