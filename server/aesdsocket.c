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
#include <time.h>

#define PORT "9000"
#define DATA_FILE "/var/tmp/aesdsocketdata"

volatile sig_atomic_t signal_received = 0; // Flag for signal handling

void signal_handler(int sig) {
  signal_received = 1;
  syslog(LOG_INFO, "Caught signal, exiting");
}

// Structure to store client information for thread handling
typedef struct client_info {
  int client_fd;
  char buffer[1000]; // Buffer for data transmission/reception
} client_info_t;

// Function to handle client connection (executed by a thread)
void* handle_client(void* arg) {
  client_info_t* client_data = (client_info_t*)arg;
  int client_fd = client_data->client_fd;

  // Open data file for client-specific storage (optional)
  // ...

  // Receive and retransmit data in a loop
  while (1) {
    int bytes_read = recv(client_fd, client_data->buffer, sizeof(client_data->buffer), 0);
    if (bytes_read <= 0) {
      break; // Handle connection closing or error
    }

    // Send received data back to the same client
    send(client_fd, client_data->buffer, bytes_read, 0);

    // Implement timer functionality here (optional)
    // ...
  }

  // Cleanup for the client connection (close files, free memory)
  // ...

  free(client_data);
  return NULL;
}

int main(int argc, char *argv[]) {
  int daemonize = 0;

  // Process command line argument
  if (argc > 1 && strcmp(argv[1], "-d") == 0) {
    daemonize = 1;
  }

  // Daemonize if required (unchanged)
  // ...

  // Open syslog (unchanged)
  // ...

  // Socket setup (unchanged)
  // ...

  // Signal handling setup (unchanged)
  // ...

  while (signal_received == 0) {
    struct sockaddr peer;
    socklen_t peersize = sizeof(peer);
    memset(&peer, 0, peersize);

    int client_fd = accept(server_fd, &peer, &peersize);
    if (client_fd < 0) {
      perror("accept failed");
      continue;
    }

    syslog(LOG_INFO, "Accepted connection from %u", peer.sa_data);

    // Create a new thread to handle the client connection
    pthread_t thread_id;
    client_info_t* client_data = malloc(sizeof(client_info_t));
    client_data->client_fd = client_fd;
    int result = pthread_create(&thread_id, NULL, handle_client, client_data);
    if (result != 0) {
      perror("pthread_create failed");
      close(client_fd);
      free(client_data);
      continue;
    }

    // Detach the thread to avoid resource leaks
    pthread_detach(thread_id);
  }

  // Cleanup (unchanged)
  // ...

  return 0;
}