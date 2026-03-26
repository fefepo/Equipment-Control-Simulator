#pragma once

#include <atomic>
#include <thread>

#include "EquipmentController.hpp"
#include "Logger.hpp"

class TcpServer {
private:
    EquipmentController& controller;
    Logger& logger;
    std::atomic<bool> running{ false };
    std::thread serverThread;
    int serverSocket = -1;
    int port = 5000;

    void serverLoop();

public:
    TcpServer(EquipmentController& ctrl, Logger& logRef, int listenPort = 5000);
    ~TcpServer();

    void start();
    void stop();
};