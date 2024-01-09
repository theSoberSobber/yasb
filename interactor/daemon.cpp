#include <iostream>
#include <WS2tcpip.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <chrono>
#include <vector>
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "27015"
#define DEFAULT_BUFLEN 512

#include "subscriber.h"

std::vector<subscriber*> subscribers;
std::wstring s2ws(const std::string& str);
HANDLE connectToNamePipe(const std::wstring& pipeName);

std::atomic<bool> exitSignal(false);
std::mutex m;
std::condition_variable cv;

void listen() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << "\n";
        return;
    }

    struct addrinfo* resultAddr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultAddr);
    if (result != 0) {
        std::cerr << "getaddrinfo failed: " << result << "\n";
        WSACleanup();
        return;
    }

    SOCKET listenSocket = socket(resultAddr->ai_family, resultAddr->ai_socktype, resultAddr->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "Error at socket(): " << WSAGetLastError() << "\n";
        freeaddrinfo(resultAddr);
        WSACleanup();
        return;
    }

    result = bind(listenSocket, resultAddr->ai_addr, (int)resultAddr->ai_addrlen);
    if (result == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << "\n";
        freeaddrinfo(resultAddr);
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    freeaddrinfo(resultAddr);

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    while (!exitSignal) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
            closesocket(listenSocket);
            WSACleanup();
            return;
        }

        char recvbuf[DEFAULT_BUFLEN];
        int recvbuflen = DEFAULT_BUFLEN;

        int result = recv(clientSocket, recvbuf, recvbuflen, 0);
        if (result > 0) {
            recvbuf[result] = '\0';
            std::string name(recvbuf);
            std::wstring pipeName = L"\\\\.\\pipe\\" + s2ws(name);

            HANDLE pipe = connectToNamedPipe(pipeName);
            if (pipe == INVALID_HANDLE_VALUE){
                std::cout << "Failed to connect to the named pipe " << name << std::endl;
                continue;
            }
            std::cout << "Subscribed to " << name << std::endl;
            subscriber* sub = new subscriber();
            sub->name = name;
            sub->pipe = pipe;
            subscribers.push_back(sub);
            std::cout << "Current Subscribers: ";
            for(auto i: subscribers) std::cout << i->name << " ";
            std::cout << std::endl;
        } else if (result == 0) {
            std::cerr << "Connection closed\n";
        } else {
            std::cerr << "Receive failed: " << WSAGetLastError() << "\n";
        }

        closesocket(clientSocket);
    }

    closesocket(listenSocket);
    WSACleanup();
}

void actualWork(){
    // keep in mind cout will be available to both concurrently, hence making the output kinda illegible
    // do the actual work on parsing commands and switching virtual desktops here
    // are there locks on cout?
    // is there a need of a worker thread seperate from main thread?
    for(int i=0; ; i++) std::cout << i << std::endl;
}

int main() {
    // std::thread listenThread(listen);
    // subscriber vector is available to acutal work too, hence we have successfully completed our task 
    // of non blocking listening with VDA
    std::thread listenThread([]() { listen(); });

    // actualWork();

    // Wait for exit signal
    {
        std::unique_lock<std::mutex> lock(m);
    	cv.wait(lock, [&exitSignal] { return exitSignal.load(); });
    }

    // Clean up and exit
    exitSignal = true;
    listenThread.join();

    return 0;
}
