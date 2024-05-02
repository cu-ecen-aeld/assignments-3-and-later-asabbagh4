#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>
#include <netinet/in.h>
#include <time.h>
#include <arpa/inet.h>

#define PORT "9000"
#define CONNECTIONS 20
#define TIMESTAMP_INT 10
#define DATA_FILE "/var/tmp/aesdsocketdata"

pthread_mutex_t file_mutex;
int signal_received = 0;

int sockfd;  // Global variable to hold the socket file descriptor

// Signal handler function
void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        //cleanup and close the socket
        signal_received = 1;
        syslog(LOG_INFO, "Caught signal, exiting");
        close(sockfd);
        remove(DATA_FILE);
        closelog();
        exit(EXIT_SUCCESS);
    }
}

// Daemonize the process
void daemonize() {
    pid_t pid, sid;

    // Fork the parent process
    pid = fork();

    // If fork fails, exit with an error message
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    // If fork succeeds, terminate the parent process
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Create a new session and become the leader of the session
    sid = setsid();
    if (sid < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }

    // Change the working directory to root
    if (chdir("/") < 0) {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void *timestamp_writer(void *arg) {
    while (1) {
        pthread_mutex_lock(&file_mutex);

        FILE *file = fopen(DATA_FILE, "a");
        if (file == NULL) {
            perror("File opening failed");
            pthread_mutex_unlock(&file_mutex);
            sleep(TIMESTAMP_INT);
            continue;
        }

        time_t now = time(NULL);
        struct tm *time_data = localtime(&now);
        char time_stamp[64];
        strftime(time_stamp, sizeof(time_stamp), "timestamp:%Y-%m-%d %H:%M:%S\n", time_data);

        fprintf(file, "%s", time_stamp);

        fclose(file);
        pthread_mutex_unlock(&file_mutex);

        sleep(TIMESTAMP_INT);
    }
}

void* connection_handler(void* socket_desc){
    int new_fd = *(int *)socket_desc;

    //lock mutex
    pthread_mutex_lock(&file_mutex);

    char buffer[1024];
    ssize_t bytes_received;
    int data_file_fd = open(DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (data_file_fd == -1) {
        syslog(LOG_ERR, "open failed for %s", DATA_FILE);
        close(new_fd);
        pthread_exit(NULL);
    } else {
        while ((bytes_received = recv(new_fd, buffer, sizeof(buffer), 0)) > 0) {
            write(data_file_fd, buffer, bytes_received);
            // Check for newline character to consider the packet complete
            if (memchr(buffer, '\n', bytes_received) != NULL) {
                // Send the content of the file back to the client
                int file_content_fd = open(DATA_FILE, O_RDONLY);
                if (file_content_fd != -1) {
                    while ((bytes_received = read(file_content_fd, buffer, sizeof(buffer))) > 0) {
                        send(new_fd, buffer, bytes_received, 0);
                    }
                    close(file_content_fd);
                }
                lseek(data_file_fd, 0, SEEK_SET);  // Reset file offset for the next iteration
            }
        }
        close(data_file_fd);
        //unlock mutex
        pthread_mutex_unlock(&file_mutex);
        }
    // Close the connection
    close(new_fd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int new_fd;


    // Initialize the mutex
    if (pthread_mutex_init(&file_mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(EXIT_FAILURE);
    }

    // Open syslog
    openlog(NULL, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    // Set up signal handling for SIGINT and SIGTERM
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Daemonize if needed
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemonize();
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &res) != 0) {
        syslog(LOG_ERR, "getaddrinfo failed");
        closelog();
        exit(EXIT_FAILURE);
    }

    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        syslog(LOG_ERR, "socket failed");
        freeaddrinfo(res);
        closelog();
        exit(EXIT_FAILURE);
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        syslog(LOG_ERR, "bind failed");
        close(sockfd);
        freeaddrinfo(res);
        closelog();
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, CONNECTIONS) == -1) {
        syslog(LOG_ERR, "listen failed");
        close(sockfd);
        freeaddrinfo(res);
        closelog();
        exit(EXIT_FAILURE);
    }
/* commenting out timestamp for assignment 8
    pthread_t timestamp_thread;
    if (pthread_create(&timestamp_thread, NULL, timestamp_writer, NULL) < 0) {
        perror("Timestamp thread failed");
        exit(EXIT_FAILURE);
    }
*/
    // loop to accept connections
    do {
        // now accept an incoming connection:
        addr_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1) {
            syslog(LOG_ERR, "accept failed");
            continue;  // Continue to the next iteration of the loop
        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, connection_handler, (void *)&new_fd) < 0) {
            perror("Thread failed");
            exit(EXIT_FAILURE);
        }
        pthread_join(thread, NULL);

        // Log accepted connection
        char addr_str[INET6_ADDRSTRLEN];
        inet_ntop(their_addr.ss_family,
                  &(((struct sockaddr_in*)&their_addr)->sin_addr),
                  addr_str, sizeof addr_str);
        syslog(LOG_INFO, "Accepted connection from %s", addr_str);

        // Log closed connection
        syslog(LOG_INFO, "Closed connection from %s", addr_str);

    } while (signal_received == 0);
    
    // Cleanup and close the socket
    close(sockfd);
    freeaddrinfo(res);
    closelog();

    return 0;
}