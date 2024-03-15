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

#define PORT "9000"
#define DATA_FILE "/var/tmp/aesdsocketdata"

volatile sig_atomic_t signal_received = 0; 

void signal_handler(int sig) {
  signal_received = 1;
  syslog(LOG_INFO, "Caught signal, exiting");
}

int setup_server_socket(const char *port) {
  // Socket setup
  int server_fd;
  struct addrinfo *serverinfo;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  int ret = getaddrinfo(NULL, port, &hints, &serverinfo);
  if (ret != 0) {
    syslog(LOG_ERR, "getaddrinfo error: %s", gai_strerror(ret)); // Use gai_strerror for more descriptive errors
    return -1;
  }

  server_fd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);
  if (server_fd < 0) {
    perror("socket failed");
    freeaddrinfo(serverinfo); 
    return -1;
  }

  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt failed");
    freeaddrinfo(serverinfo);
    close(server_fd);
    return -1;
  }

  if (bind(server_fd, serverinfo->ai_addr, serverinfo->ai_addrlen) < 0) {
    perror("bind failed");
    freeaddrinfo(serverinfo);
    close(server_fd);
    return -1;
  }

  freeaddrinfo(serverinfo); // Free memory

  if (listen(server_fd, 20) != 0) {
    perror("listen failed");
    close(server_fd);
    return -1;
  }

  return server_fd;
}

// Handles client communication: receives, processes, sends
int handle_client(int client_fd) {
  int filedata = open(DATA_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
  if (filedata < 0) {
    perror("open failed (receive)");
    return -1;
  }

  if (receive_data(client_fd, filedata) < 0) { 
    close(filedata);
    return -1; 
  }

  close(filedata);

  filedata = open(DATA_FILE, O_RDONLY, 0666);
  if (filedata < 0) {
    perror("open failed (send)");
    return -1;
  }

  if (send_data(client_fd, filedata) < 0) {
    close(filedata);
    return -1;
  }

  close(filedata);
  return 0;
}

int receive_data(int client_fd, int filedata) {
  char buffer[1000];
  int bytes_read;

  while ((bytes_read = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
    write(filedata, buffer, bytes_read);
    if (strchr(buffer, '\n') != NULL) {
      break;
    }
  }

  return (bytes_read < 0) ? -1 : 0; 
}

int send_data(int client_fd, int filedata) {
  char buffer[1000];
  int bytes_read;

  while ((bytes_read = read(filedata, buffer, sizeof(buffer))) > 0) {
    send(client_fd, buffer, bytes_read, 0);
  }

  return (bytes_read < 0) ? -1 : 0;
}

void graceful_shutdown(int client_fd) {
  shutdown(client_fd, SHUT_RDWR);
  close(client_fd);
} 