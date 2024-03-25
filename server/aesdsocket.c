#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h> 

#define PORT "9000"
#define DATA_FILE "/var/tmp/aesdsocketdata"

volatile sig_atomic_t signal_received = 0; 

void signal_handler(int sig) {
    signal_received = 1;
    syslog(LOG_INFO, "Caught signal, exiting");
}

// Function to handle a single client's communication
void *handle_client(void *client_socket) {
    int client_fd = *(int *)client_socket;
    free(client_socket); 

    int filedata = open(DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (filedata < 0) {
        perror("open failed");
        close(client_fd);
        pthread_exit(NULL); 
    }

    char buffer[1000];
    int bytes_read;

    while (((bytes_read = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) && (signal_received == 0)){ 
        write(filedata, buffer, bytes_read);
        if (strchr(buffer, '\n') != NULL) {
            break;
        }
    } 
    close(filedata);
    memset(buffer, 0, sizeof(buffer));

    filedata = open(DATA_FILE, O_RDONLY, 0666);
    if (filedata < 0) {
        perror("open failed");
        close(client_fd);
        pthread_exit(NULL); 
    }

    while ((bytes_read = read(filedata, buffer, sizeof(buffer))) > 0) {
        send(client_fd, buffer, bytes_read, 0);
    }

    close(filedata);
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
    pthread_exit(NULL); 
}

int main(int argc, char *argv[]) {
    int daemonize = 0;

    // Process command line argument
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemonize = 1;
    }

    // Daemonize if required 
    if (daemonize) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid > 0) {
            exit(0); // Exit parent process
        }
    }

    // Open syslog
    openlog("aesdsocket", LOG_PID, LOG_USER);

    struct sockaddr peer;
    socklen_t peersize = sizeof(peer);
    memset(&peer, 0, peersize);

    // Socket setup
    int server_fd, client_fd;
    struct addrinfo *serverinfo;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_addr = NULL;

    int ret = getaddrinfo(NULL, PORT, &hints, &serverinfo);
    if (ret != 0) {
        exit(1);
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(1);
    }

    if (bind(server_fd, serverinfo->ai_addr, serverinfo->ai_addrlen) < 0) {
        perror("bind failed");
        exit(1);
    }

    if (listen(server_fd, 20) != 0) {
        perror("listen failed");
        exit(1);
    }

    // Signal handling setup
    struct sigaction signalaction;
    memset(&signalaction, 0, sizeof(struct sigaction));
    signalaction.sa_handler = signal_handler; 
    int retvalsig = sigaction(SIGINT, &signalaction, NULL);
    if (retvalsig != 0) {
        syslog(LOG_ERR, "SIGINT Signal error %d", retvalsig);
        exit(1);
    }
    retvalsig = sigaction(SIGTERM, &signalaction, NULL);
    if (retvalsig != 0) {
        syslog(LOG_ERR, "SIGTERM Signal error %d", retvalsig);
        exit(1);
    }

    syslog(LOG_INFO, "Signal Handling set up complete.");
    printf("Signal handling set up.\n");

    while (signal_received == 0) {
        client_fd = accept(server_fd, &peer, &peersize);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        pthread_t thread_id;
        int *client_socket_ptr = malloc(sizeof(int)); 
        *client_socket_ptr = client_fd;

        if (pthread_create(&thread_id, NULL, handle_client, client_socket_ptr) != 0) {
            perror("pthread_create failed");
            free(client_socket_ptr); 
            continue; 
        }
    }

    // Cleanup 
    close(server_fd);
    remove(DATA_FILE); 
    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);
    closelog();
    return 0;
}