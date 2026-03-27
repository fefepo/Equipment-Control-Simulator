#pragma once

#include <atomic>
#include <thread>

#include "EquipmentController.hpp"
#include "Logger.hpp"

// TCP 통신으로 외부 명령을 받는 서버 클래스
class TcpServer {
private:
    EquipmentController& controller; // 장비 제어 객체
    Logger& logger; // 로그 기록 객체
    std::atomic<bool> running{ false }; // 서버 실행 여부
    std::thread serverThread; // 서버 동작 스레드
    int serverSocket = -1; // 서버 소켓
    int port = 5000; // 서버 포트 번호

    void serverLoop(); // 클라이언트 접속/명령 처리 루프

public:
    TcpServer(EquipmentController& ctrl, Logger& logRef, int listenPort = 5000);
    ~TcpServer();

    void start(); // 서버 시작
    void stop(); // 서버 종료
};
