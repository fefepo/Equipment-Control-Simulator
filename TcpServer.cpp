#include "TcpServer.hpp"

#include <iostream>
#include <string>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

// TCP 서버 생성자
TcpServer::TcpServer(EquipmentController& ctrl, Logger& logRef, int listenPort)
    : controller(ctrl), logger(logRef), port(listenPort) {
}

// 객체 소멸 시 서버 종료
TcpServer::~TcpServer() {
    stop();
}

// 서버 스레드 시작
void TcpServer::start() {
    if (running) {
        return;
    }

    running = true;
    serverThread = std::thread(&TcpServer::serverLoop, this);
}

// 서버 종료 및 소켓 정리
void TcpServer::stop() {
    if (!running) {
        return;
    }

    running = false;

#ifdef _WIN32
    if (serverSocket != -1) {
        closesocket(serverSocket);
        serverSocket = -1;
    }
#else
    if (serverSocket != -1) {
        close(serverSocket);
        serverSocket = -1;
    }
#endif

    if (serverThread.joinable()) {
        serverThread.join();
    }
}

// TCP 서버 메인 루프
void TcpServer::serverLoop() {
#ifdef _WIN32
    
    // 윈도우 소켓 초기화
    WSADATA wsaData{};
    int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaResult != 0) {
        logger.log("TCP ERROR WSAStartup failed");
        return;
    }
#endif
    
    // 서버 소켓 생성
    serverSocket = static_cast<int>(socket(AF_INET, SOCK_STREAM, 0));
    if (serverSocket < 0) {
        logger.log("TCP ERROR socket creation failed");
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(static_cast<u_short>(port));
    
    // 포트 바인딩
    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        logger.log("TCP ERROR bind failed");
#ifdef _WIN32
        closesocket(serverSocket);
        WSACleanup();
#else
        close(serverSocket);
#endif
        serverSocket = -1;
        return;
    }
    
    // 클라이언트 접속 대기
    if (listen(serverSocket, 5) < 0) {
        logger.log("TCP ERROR listen failed");
#ifdef _WIN32
        closesocket(serverSocket);
        WSACleanup();
#else
        close(serverSocket);
#endif
        serverSocket = -1;
        return;
    }

    logger.log("TCP SERVER STARTED on port " + std::to_string(port));

    while (running) {
        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);

#ifdef _WIN32
        SOCKET clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSocket == INVALID_SOCKET) {
            if (running) {
                logger.log("TCP WARN accept failed");
            }
            continue;
        }
#else
        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), reinterpret_cast<socklen_t*>(&clientLen));
        if (clientSocket < 0) {
            if (running) {
                logger.log("TCP WARN accept failed");
            }
            continue;
        }
#endif
        
        // 접속한 클라이언트 IP 확인
        char clientIp[INET_ADDRSTRLEN] = { 0 };
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
        std::string clientIpStr = clientIp;

        logger.log("TCP CLIENT CONNECTED " + clientIpStr);

        char buffer[1024] = { 0 };
#ifdef _WIN32
        int received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
#else
        int received = static_cast<int>(recv(clientSocket, buffer, sizeof(buffer) - 1, 0));
#endif

        if (received > 0) {
            std::string command(buffer, received);
            
            // 줄바꿈/공백 제거
            while (!command.empty() &&
                (command.back() == '\n' || command.back() == '\r' || command.back() == ' ' || command.back() == '\t')) {
                command.pop_back();
            }

            logger.log("TCP CMD from " + clientIpStr + ": " + command);
            
            // 명령 실행 후 응답 전송
            std::string response = controller.executeCommand(command);
            logger.log("TCP RESP to " + clientIpStr + ": " + response);

            response += "\n";

#ifdef _WIN32
            send(clientSocket, response.c_str(), static_cast<int>(response.size()), 0);
#else
            send(clientSocket, response.c_str(), response.size(), 0);
#endif
        }
        else {
            logger.log("TCP WARN No data received from " + clientIpStr);
        }
        
        // 클라이언트 연결 종료
#ifdef _WIN32
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif

        logger.log("TCP CLIENT DISCONNECTED " + clientIpStr);
    }

    logger.log("TCP SERVER STOPPED");

#ifdef _WIN32
    if (serverSocket != -1) {
        closesocket(serverSocket);
        serverSocket = -1;
    }
    WSACleanup();
#else
    if (serverSocket != -1) {
        close(serverSocket);
        serverSocket = -1;
    }
#endif
}
