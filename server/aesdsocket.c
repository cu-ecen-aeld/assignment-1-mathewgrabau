#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#ifdef NDEBUG
#define print_debug(fd, format, ...)
#else
#define print_debug(fd, format, ...) fprintf(fd, format, ##__VA_ARGS__)
#endif

/* Constants */
#define TERMINATOR '\n'
#define CLIENT_BUFFER_SIZE 1024
#define LISTEN_PORT 9000

#define DEFAULT_OUTPUT_FILE "/var/tmp/aesdsocketdata"

#define ERROR_STRING_LEN 80
static char error_string[ERROR_STRING_LEN + 1];

int terminate = 0;

void signal_handler(int signo) {
  print_debug(stdout, "signal_handler invoked, signo = %d\n", signo);
  syslog(LOG_INFO, "Caught signal, exiting.");
  terminate = 1;
}

void register_signal_handlers() {
  struct sigaction current;
  sigemptyset(&current.sa_mask);
  current.sa_flags = 0;
  current.sa_handler = signal_handler;
  if (sigaction(SIGTERM, &current, NULL) == -1) {
    perror("Error installing signal handler");
    syslog(LOG_ERR, "Error installing the signal handler");
  }
  if (sigaction(SIGINT, &current, NULL) == -1) {
    perror("Error installing signal handler");
    syslog(LOG_ERR, "Error installing the signal handler");
  }
}

/**
 * @param log_facility - One of the values provided in syslog.h (intended to
 * support running as a daemon flexibly later).
 */
void setup_syslog(int log_facility) {
  /* use the default (program name) to identify the logging */
  openlog(NULL, LOG_PID | LOG_PERROR, log_facility);
}

int main() {
  struct sockaddr client_addr;
  struct sockaddr_in server_address;
  int listen_socket, connection_socket;
  socklen_t client_addr_len;
  int rc;
  int i;
  int fd;
  int addr_family;
  int newline = 0;
  char client_data[CLIENT_BUFFER_SIZE];
  char address_buffer[INET_ADDRSTRLEN + 1];
  ssize_t socket_read_size;
  struct addrinfo hints, *my_addrinfo, *rp;

  setup_syslog(LOG_USER);

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  rc = getaddrinfo(NULL, "9000", &hints, &my_addrinfo); /* Open the socket */
  if (rc < 0) {
    strerror_r(errno, error_string, sizeof(error_string));
    syslog(LOG_ERR, "getaddrinfo failed: %s", error_string);
    return -1;
  }

  for (rp = my_addrinfo; rp != NULL; rp = rp->ai_next) {
    fprintf(stdout, "len = %d\n", rp->ai_addrlen);
    // Format the address
    inet_ntop(rp->ai_family, rp->ai_addr, address_buffer,
              sizeof(address_buffer));
    int server_port = 0;
    if (rp->ai_family == AF_INET) {
      server_port = ntohs(((struct sockaddr_in *)rp->ai_addr)->sin_port);
    } else if (rp->ai_family == AF_INET6) {
      server_port = ntohs(((struct sockaddr_in6 *)rp->ai_addr)->sin6_port);
    }
    print_debug(stdout, "server family: %d, %s:%d\n", rp->ai_family,
                address_buffer, server_port);

    listen_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (listen_socket < 0) {
      strerror_r(errno, error_string, sizeof(error_string));
      syslog(LOG_ERR, "socket failed: %s", error_string);
      freeaddrinfo(my_addrinfo);
      return -1;
    }

    /* Bind the socket now that it's been created. */
    rc = bind(listen_socket, my_addrinfo->ai_addr, my_addrinfo->ai_addrlen);
    if (rc != 0) {
      strerror_r(errno, error_string, sizeof(error_string));
      syslog(LOG_ERR, "socket failed: %s", error_string);
      close(listen_socket);
      freeaddrinfo(my_addrinfo);
      return -1;
    }

    addr_family = rp->ai_family;
    break;
  }

  freeaddrinfo(my_addrinfo);

  /* start listening on the socket */
  rc = listen(listen_socket, 1);
  if (rc < 0) {
    perror("listen() failed");
    strerror_r(errno, error_string, sizeof(error_string));
    syslog(LOG_ERR, "listen failed: %s", error_string);
    close(listen_socket);
    freeaddrinfo(my_addrinfo);
    return -1;
  }

  register_signal_handlers();

  while (terminate == 0) {
    memset(address_buffer, 0, sizeof(address_buffer));

    fprintf(stdout, "Listening for connection\n");
    client_addr_len = sizeof(client_addr);
    connection_socket = accept(listen_socket, &client_addr, &client_addr_len);
    if (connection_socket < 0) {
      strerror_r(errno, error_string, sizeof(error_string));
      syslog(LOG_ERR, "accept(listen_socket) failed: %s", error_string);
      rc = -1;
      break;
    }

    inet_ntop(client_addr.sa_family, &client_addr, address_buffer,
              sizeof(address_buffer));

    /* I'm finding the address_buffer printout very questionable. */
    syslog(LOG_INFO, "Received connection from client: %s\n", address_buffer);
    in_port_t port_number;
    if (client_addr.sa_family == AF_INET) {
      port_number = ((struct sockaddr_in *)&client_addr)->sin_port;
    } else {
      port_number = ((struct sockaddr_in6 *)&client_addr)->sin6_port;
    }

    port_number = ntohs(port_number);

    syslog(LOG_INFO, "Client port is %d\n", port_number);

    /* Keep reading bytes until finding the terminal */
    while (newline == 0) {
      fprintf(stdout, "Reading socket for data\n");
      /* Receive data, and process accordingly. Note that we opted to avoid the
       * heap allocation and just read it 1K at a time into the buffer, then
       * flush to the file immediately. Another optimization could be to leave
       * the file open for the duration of it.*/
      socket_read_size =
          recv(connection_socket, &client_data, sizeof(client_data), 0);
      if (socket_read_size < 0) {
        strerror_r(errno, error_string, sizeof(error_string));
        syslog(LOG_ERR, "recv() error: %s", error_string);
        close(connection_socket);
        connection_socket = 0;
        continue;
      }
      /* Check for the terminating case */
      if (socket_read_size == 0) {
        syslog(LOG_INFO, "client closed the socket");
        close(connection_socket);
        connection_socket = 0;
        break;
      }

      syslog(LOG_INFO, "Read %d bytes from connection_socket",
             (int)socket_read_size);

      fd = open("/var/tmp/aesdsocketdata",
                O_CLOEXEC | O_APPEND | O_RDWR | O_CREAT,
                S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
      if (fd < 0) {
        strerror_r(errno, error_string, sizeof(error_string));
        syslog(LOG_ERR, "open() failed: %s", error_string);
        close(connection_socket);
        connection_socket = 0;
        rc = -1;
        break;
      }
      // Otherwise, we append to the end of the file
      write(fd, client_data, socket_read_size);
      close(fd);
      fd = 0;

      /* Check to see if we have found the terminator */
      i = 0;
      while (i < socket_read_size && newline == 0) {
        if (client_data[i] == TERMINATOR) {
          fprintf(stdout, "Detected the newline character\n");

          fd = open(DEFAULT_OUTPUT_FILE, O_CLOEXEC | O_RDONLY, S_IWUSR, S_IRUSR,
                    S_IRGRP, S_IROTH);
          if (fd >= 0) {

            int num_read = read(fd, client_data, sizeof(client_data));
            while (num_read > 0) {
              send(connection_socket, client_data, num_read, 0);
              num_read = read(fd, client_data, sizeof(client_data));
            }

            close(fd);
          }

          close(connection_socket);
          connection_socket = 0;
          newline = 1;
        }
        ++i;
      }
    }

    newline = 0;
  }

  syslog(LOG_INFO, "Server cleaning up and shutting down");

  close(listen_socket);

  /* Cleanup the file */
  rc = remove(DEFAULT_OUTPUT_FILE);
  if (rc < 0) {
    rc = errno;
    if (rc != ENOENT) {
      strerror_r(rc, error_string, sizeof(error_string));
      syslog(LOG_ERR, "remove() failed: %s", error_string);
    }
  }

  return rc;
}
