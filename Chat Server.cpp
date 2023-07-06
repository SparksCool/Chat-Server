#include <iostream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <mutex>

#pragma comment(lib, "ws2_32.lib") // Winsock Library

std::vector<SOCKET> client_sockets;
std::mutex mtx;

void handle_client(SOCKET client_socket) {
    char message[2000] = { 0 };
    char username[2000] = { 0 };

    int receive_size = recv(client_socket, username, 2000, 0);
    if (receive_size == SOCKET_ERROR) {
        puts("Client disconnected");
        puts("Name: Unknown");
        closesocket(client_socket);
        return;
    }

    // If receive_size is 0, the client disconnected gracefully
    if (receive_size == 0) {
        puts("Client disconnected");
        puts("Name: Unknown");
        closesocket(client_socket);
        return;
    }

    printf("Client name: %s\n", username);

    while (1) {
        memset(message, 0, sizeof(message)); // clear the variable

        int receive_size = recv(client_socket, message, 2000, 0);
        if (receive_size == SOCKET_ERROR) {
            puts("Client disconnected");
            puts("Name: ");
            puts(username);
            break;
        }

        // If receive_size is 0, the client disconnected gracefully
        if (receive_size == 0) {
            puts("Client disconnected");
            puts("Name: ");
            puts(username);
            break;
        }

        printf("Client message (USER: %s): %s\n", username, message);

        std::string formattedMessage = username;
        formattedMessage += ": ";
        formattedMessage += message;

        // Broadcast received message to all clients
        mtx.lock();
        for (SOCKET s : client_sockets) {
            if (s == client_socket) send(s, "\n", strlen("\n"), 0);
            send(s, formattedMessage.c_str(), strlen(formattedMessage.c_str()), 0);
        }
        mtx.unlock();
    }

    closesocket(client_socket);
}

int main() {
    WSADATA wsa;
    SOCKET s, new_socket;
    struct sockaddr_in server, client;
    int c;

    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return 1;
    }

    printf("Initialised.\n");

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket : %d", WSAGetLastError());
    }

    printf("Socket created.\n");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    if (bind(s, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed with error code : %d", WSAGetLastError());
    }

    puts("Bind done");

    listen(s, 3);

    puts("Waiting for incoming connections...");

    c = sizeof(struct sockaddr_in);

    while ((new_socket = accept(s, (struct sockaddr*)&client, &c))) {
        if (new_socket == INVALID_SOCKET) {
            printf("accept failed with error code : %d", WSAGetLastError());
            break;
        }

        puts("Connection accepted");

        mtx.lock();
        client_sockets.push_back(new_socket);
        mtx.unlock();

        std::thread client_thread(handle_client, new_socket);
        client_thread.detach(); // we're not joining the thread in main, so detach it
    }

    closesocket(s);
    WSACleanup();

    return 0;
}