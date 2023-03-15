/**
Two client server that reads stream of YUV_420_888 images. 
Example: security camera or video.

Uses OpenGL for rendering.

Reads image data in YUV_420_888 format.
*/

#include <iostream>
#include <winsock2.h>
#include <cstdio>
#include <vector>
#include <cstring>
#include <sstream>
#include <thread>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "client.h"
#include "gl.cpp"
#include "image_conversion.cpp"

// Number of active clients
short activeClients = 0;

// Create two free places for clients
Client clients[2] = {Client(), Client()};

SOCKET start_server() {
  // Create a socket
  SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  auto serverIP = INADDR_ANY;
  unsigned short serverPort = 8080;

  std::cout << serverIP << ":" << serverPort << std::endl;

  // Bind the socket to a local address and port
  sockaddr_in server_address;
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = serverIP;
  server_address.sin_port = htons(serverPort);
  bind(server_socket, (sockaddr*)&server_address, sizeof(server_address));

  return server_socket;
}

void close_socket(SOCKET socket) {
  // Close the connection
  shutdown(socket, SD_SEND);

  // Close the socket
  closesocket(socket);
}

void get_image_properties(char imageProperties[64], int &width, int &height, int &y_pixel_stride, int &y_row_stride, int &uv_pixel_stride, int &uv_row_stride, int &y_size, int &uv_size, int &yuv_size, bool &invalid) {
  std::istringstream iss(imageProperties);
  std::vector<std::string> tokens;
  std::string token;

  // Split image properties by empty space
  // 0 = Image width
  // 1 = Image height
  // 2 = Y plane size
  // 3 = UV plane size
  // 4 = Y plane row stride
  // 5 = Y plane pixel stride
  // 6 = UV plane row stride
  // 7 = UV plane pixel stride
  while (iss >> token) {
    tokens.push_back(token);
  }

  if (tokens.size() > 0) {
    try {
      // Convert properties to int
      width = std::stoi(tokens[0]); // Image width
      height = std::stoi(tokens[1]); // Image height
      y_size = std::stoi(tokens[2]); // Y plane size
      uv_size = std::stoi(tokens[3]); // U and V plane size
      y_row_stride = std::stoi(tokens[4]); // Y plane row stride
      y_pixel_stride = std::stoi(tokens[5]); // Y plane pixel stride
      uv_row_stride = std::stoi(tokens[6]); // UV plane row stride
      uv_pixel_stride = std::stoi(tokens[7]); // UV plane pixel stride
    } catch (const std::exception& e) {
      std::cout << "Invalid first message: " << imageProperties << std::endl;
      invalid = true;
      return;
    }
  }
  else {
    std::cout << "Invalid first message: " << imageProperties << std::endl;
    invalid = true;
    return;
  }

  yuv_size = y_size + uv_size * 2;
}

// Listen image data
// First message: image properties
// Other messages: image data as byte[]
void listen_client_socket(Client *client, SOCKET client_socket) {
  int width;
  int height;

  int y_pixel_stride;
  int y_row_stride;
  int uv_pixel_stride;
  int uv_row_stride;

  int y_size;
  int uv_size;
  int yuv_size;

  bool firstMessage;

  bool stopThread = false;

  // Keep the connection open until the client closes it
  while (true) {
    // Get image properties from first message
    if (!firstMessage) {
      char imageProperties[64];

      // Get image properties as char array from socket
      recv(client_socket, imageProperties, 64, 0);

      // Get image properties
      get_image_properties(imageProperties, width, height, y_pixel_stride, y_row_stride, uv_pixel_stride, uv_row_stride, y_size, uv_size, yuv_size, stopThread);

      if (stopThread) {
        std::cout << "Stopping thread: " << client->connectingIP << std::endl;
        client->free = true; // Add free screen place for clients
        --activeClients;
        return;
      }

      client->textureWidth = width;
      client->textureHeight = height;

      // Reset texture
      client->texture = nullptr;

      std::cout << "width=" << width << " height=" << height << " y_size=" << y_size << " uv_size=" << uv_size << " yuv_size=" << yuv_size << " y_row_stride=" << y_row_stride << " y_pixel_stride=" << y_pixel_stride << " uv_row_stride=" << uv_row_stride << " uv_pixel_stride=" << uv_pixel_stride << std::endl; 

      firstMessage = true;
      continue;
    }

    // YUV_420_888 image data
    char *yuv = new char[yuv_size];

    int yuv_bytes_received = 0;

    // Get complete YUV_420_888 image data
    while (yuv_bytes_received < yuv_size) {
      // Get part of YUV_420_888 image data from socket
      int result = recv(client_socket, yuv + yuv_bytes_received, yuv_size - yuv_bytes_received, 0);

      // Check if client closes connection
      if (result == 0 || result == -1) {
        std::cout << "Closing connection: " << client->IP << std::endl;
        client->free = true; // Add free screen place for clients
        free(yuv); // Fix memory leak
        --activeClients;
        close_socket(client_socket);
        return;
      }

      yuv_bytes_received += result;
    }

    // Check if client has texture
    if (client->texture == nullptr) {
      // Create the texture
      client->texture = new unsigned char[width * height * 3];
    }

    // Get Y, U and V channel
    char* y = yuv;
    char* u = yuv + y_size;
    char* v = u + uv_size;

    // Convert YUV_420_888 image to RGB
    YUV420888_to_RGB(y, u, v, client->texture, width, height, y_pixel_stride, y_row_stride, uv_pixel_stride, uv_row_stride);

    // Free memory
    free(yuv);
  }
}

SOCKET wait_for_client_connection(const SOCKET &server_socket) {
  // Listen for incoming connections
  listen(server_socket, 5);

  // Accept an incoming connection
  sockaddr_in client_address;
  int client_address_size = sizeof(client_address);
  SOCKET client_socket = accept(server_socket, (sockaddr*)&client_address, &client_address_size);

  char* IP = inet_ntoa(client_address.sin_addr);
  std::string IP_str(IP);

  // Print the client's IP address
  std::cout << "Connection made from IP address: " << IP_str << std::endl;

  clients[activeClients].connectingIP = IP_str;

  return client_socket;
}

int get_client_index_by_IP(std::string IP) {
  int index = 0;

  for (const Client &client : clients) {
    if (!client.free && client.IP == IP) {
      return index;
    }
    index++;
  }

  return -1;
}

void wait_for_client_connections(SOCKET server_socket) {
  while (true) {
    // Wait for a client
    SOCKET client_socket = wait_for_client_connection(server_socket);

    int clientNum = activeClients;
    std::string IP = clients[clientNum].connectingIP;

    int used_client_IP_index = get_client_index_by_IP(IP);

    // Check if client exist
    if (used_client_IP_index != -1) {
      std::cout << IP << " already exists" << std::endl;

      clientNum = used_client_IP_index;
      clients[clientNum].free = true;

      close_socket(client_socket); // Kick
    }

    // Find free screen slot for the client
    while (!clients[clientNum].free) {
      clientNum--;
    }

    if (clientNum < 0) {
      // No free places
      continue;
    }

    // Listen incoming camera preview image data
    clients[clientNum].thread = new std::thread(listen_client_socket, &clients[clientNum], client_socket);
    clients[clientNum].free = false;
    clients[clientNum].IP = IP;

    ++activeClients;
  }
}

int main() {
  // Initialize Winsock (Firewall check)
  WSADATA wsa_data;
  WSAStartup(MAKEWORD(2, 2), &wsa_data);

  // Start server
  SOCKET server_socket = start_server();

  // Wait for client connections
  std::thread t(wait_for_client_connections, server_socket);

  // Open OpenGL window for camera preview
  GLFWwindow* window = setupGLWindow();
  if (window == NULL) {
    return -1;
  }

  // Setup OpenGL program
  if (!setupGLProgram()) {
    return -1;
  } 

  // Main thread
  while (true) {
    // Check if any clients exist
    while(activeClients > 0) {
      float x = 0.0f;
      float y = 0.0f;

      // Draw clients screens
      for (const Client &client : clients) {
        if (!client.free) {
          drawTexture(client.texture, x, y, client.textureWidth, client.textureHeight);
        }

        // Move next screen to right side
        x += 1.0f;
      }

      // Swap OpenGL buffers
      glfwSwapBuffers(window);
      glfwPollEvents();
    }
  }

  // Close server socket
  close_socket(server_socket);

  // Clean up Winsock
  WSACleanup();

  // Close OpenGL context
  glfwTerminate();

  return 0;
}
