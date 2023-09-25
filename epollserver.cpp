// A simple epoll server example in C++
#include <iostream>
#include <vector>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h> 

#define PORT 9901 // The port number for the server
#define MAX_EVENTS 10 // The maximum number of events to handle at a time
#define BUFFER_SIZE 1024 // The size of the buffer for reading and writing data

// A helper function to set a socket as non-blocking
void set_nonblocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        exit(1);
    }
    flags |= O_NONBLOCK;
    if (fcntl(sock, F_SETFL, flags) == -1) {
        perror("fcntl");
        exit(1);
    }
}

int main() {
    // Create a socket for listening
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) {
        perror("socket");
        exit(1);
    }

    // Set the socket as non-blocking
    set_nonblocking(listen_sock);

    // Bind the socket to the port
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    // Start listening on the socket
    if (listen(listen_sock, SOMAXCONN) == -1) {
        perror("listen");
        exit(1);
    }

    // Create an epoll instance
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        perror("epoll_create");
        exit(1);
    }

    // Add the listen socket to the epoll instance
    struct epoll_event event;
    event.events = EPOLLIN; // Monitor for incoming connections
    event.data.fd = listen_sock; // Associate the listen socket with the event
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &event) == -1) {
        perror("epoll_ctl");
        exit(1);
    }

    // A vector to store the events returned by epoll_wait
    std::vector<struct epoll_event> events(MAX_EVENTS);

    // A buffer to store the data read from or written to the sockets
    char buffer[BUFFER_SIZE];

    // The main loop of the server
    while (true) {
        // Wait for events on the epoll instance
        int num_events = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait");
            exit(1);
        }

        // Loop through the events
        for (int i = 0; i < num_events; i++) {
            // If the event is from the listen socket, accept a new connection
            if (events[i].data.fd == listen_sock) {
                // Accept the connection and get the client socket
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
                if (client_sock == -1) {
                    perror("accept");
                    continue;
                }

                // Set the client socket as non-blocking
                set_nonblocking(client_sock);

                // Add the client socket to the epoll instance
                event.events = EPOLLIN | EPOLLOUT | EPOLLET; // Monitor for read and write events using edge-triggered mode
                event.data.fd = client_sock; // Associate the client socket with the event
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock, &event) == -1) {
                    perror("epoll_ctl");
                    close(client_sock);
                    continue;
                }

                // Print a message to indicate a new connection
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET,&client_addr.sin_addr,client_ip,INET_ADDRSTRLEN);
                int client_port = ntohs(client_addr.sin_port);
                std::cout << "New connection from " << client_ip << ":" << client_port << std::endl;
            }
            // If the event is from a client socket, handle it accordingly
            else {
                // Get the client socket from the event data
                int client_sock = events[i].data.fd;

                // If the event is a read event, read data from the client socket
                if (events[i].events & EPOLLIN) {
                    // Read data until EAGAIN or an error occurs
                    while (true) {
                        int bytes_read = read(client_sock, buffer, BUFFER_SIZE);
                        if (bytes_read == -1) {
                            if (errno == EAGAIN) {
                                // No more data to read, break the loop
                                break;
                            }
                            else {
                                // An error occurred, print a message and close the socket
                                perror("read");
                                close(client_sock);
                                break;
                            }
                        }
                        else if (bytes_read == 0) {
                            // The client closed the connection, close the socket
                            close(client_sock);
                            break;
                        }
                        else {
                            // Print the data read from the client
                            std::cout << "Read " << bytes_read << " bytes from client " << client_sock << ": ";
                            std::cout.write(buffer, bytes_read);
                            std::cout << std::endl;
                        }
                    }
                }

                // If the event is a write event, write data to the client socket
                if (events[i].events & EPOLLOUT) {
                    // Write some data to the client
                    std::string message = "Hello from server!\n";
                    int bytes_written = write(client_sock, message.data(), message.size());
                    if (bytes_written == -1) {
                        // An error occurred, print a message and close the socket
                        perror("write");
                        close(client_sock);
                    }
                    else {
                        // Print the data written to the client
                        std::cout << "Wrote " << bytes_written << " bytes to client " << client_sock << ": ";
                        std::cout.write(message.data(), bytes_written);
                        std::cout << std::endl;
                    }
                }
            }
        }
    }

    // Close the listen socket and the epoll instance
    close(listen_sock);
    close(epoll_fd);

    return 0;
}

