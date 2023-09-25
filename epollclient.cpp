#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#define PORT 9901
#define BUFFER_SIZE 1024

int conn() {
  // Create a TCP socket
  int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket == -1) {
    std::cerr << "Failed to create socket\n";
    return -1;
  }

  // Connect to the server at localhost:8080
  struct sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_address.sin_port = htons(PORT);

  if (connect(client_socket, (struct sockaddr *)&server_address,
              sizeof(server_address)) == -1) {
    std::cerr << "Failed to connect to server\n";
    return -1;
  }

  std::cout << "Connected to server\n";

  // Create a string to store the data to send
  std::string data = "Hello, world!";

  // Write the data to the socket
  if (write(client_socket, data.c_str(), data.length()) == -1) {
    std::cerr << "Failed to write to socket\n";
    return -1;
  }

  std::cout << "Sent: " << data << "\n";

  // Read the response from the server
  char buffer[BUFFER_SIZE];
  int bytes_read = read(client_socket, buffer, BUFFER_SIZE);
  if (bytes_read == -1) {
    std::cerr << "Failed to read from socket\n";
    return -1;
  }

  // Print the response
  std::cout << "Received: " << std::string(buffer, bytes_read) << "\n";

  // Close the socket
  close(client_socket);

  // Return 0 to indicate success
  return 0;
}

int main(int argc, char *argv[]) {
  for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
    std::thread th1 = std::thread([] {
      int a = conn();
      std::cout << a << std::endl;
    });
    th1.join();
  }
  return 0;
}
