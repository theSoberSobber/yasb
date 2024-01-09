#include <iostream>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " subscribe <namedPipeName>\n";
        return 1;
    }

    std::string command = argv[1];
    if (command != "subscribe") {
        std::cerr << "Unknown command: " << command << "\n";
        return 1;
    }

    std::string pipeName = argv[2];

    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << "\n";
        return 1;
    }

    struct addrinfo* resultAddr = NULL, * ptr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    result = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &resultAddr);
    if (result != 0) {
        std::cerr << "getaddrinfo failed: " << result << "\n";
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    SOCKET connectSocket = INVALID_SOCKET;
    ptr = resultAddr;

    // Create a SOCKET for the server to listen for client connections
    connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (connectSocket == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << "\n";
        freeaddrinfo(resultAddr);
        WSACleanup();
        return 1;
    }

    // Connect to server.
    result = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (result == SOCKET_ERROR) {
        closesocket(connectSocket);
        connectSocket = INVALID_SOCKET;
    }

    freeaddrinfo(resultAddr);

    if (connectSocket == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server\n";
        WSACleanup();
        return 1;
    }

    // Send subscription request
    result = send(connectSocket, pipeName.c_str(), static_cast<int>(pipeName.size()), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << "\n";
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Subscription request sent to the server.\n";

    // Shutdown the connection since no more data will be sent
    result = shutdown(connectSocket, SD_SEND);
    if (result == SOCKET_ERROR) {
        std::cerr << "Shutdown failed: " << WSAGetLastError() << "\n";
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    // Cleanup
    closesocket(connectSocket);
    WSACleanup();

    return 0;
}
