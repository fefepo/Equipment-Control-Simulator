#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <cctype>
#include <filesystem>

#include "Logger.hpp"
#include "Config.hpp"
#include "EquipmentController.hpp"
#include "TcpServer.hpp"

int main() {
    
    // 현재 실행 경로 출력
    std::cout << "Current working directory: "
        << std::filesystem::current_path().string() << "\n";

    Config config;
    
    // 설정 파일 로드 실패 시 기본값 사용
    if (!config.load("config.json")) {
        std::cout << "Failed to load config.json, using defaults\n";
    }
    
    // 주요 객체 생성
    Logger logger("simulator_log.txt");
    logger.log("SYSTEM START");
    logger.log("CONFIG PORT=" + std::to_string(config.port) +
        ", TEMP_THRESHOLD=" + std::to_string(config.tempThreshold) +
        ", PRESSURE_THRESHOLD=" + std::to_string(config.pressureThreshold) +
        ", UPDATE_INTERVAL_MS=" + std::to_string(config.updateIntervalMs));

    EquipmentController controller(logger, config);
    TcpServer tcpServer(controller, logger, config.port);

    std::atomic<bool> running = true;
    std::string command;
    
    // TCP 서버 시작
    tcpServer.start();
    
    // 장비 상태 주기적 업데이트 스레드
    std::thread updateThread([&]() {
        while (running) {
            controller.update();
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config.updateIntervalMs)
            );
        }
        });
    
    // 초기 실행 정보 출력
    std::cout << "=== Equipment Control Simulator ===\n";
    std::cout << "TCP server listening on port " << config.port << "\n";
    std::cout << "Temp threshold: " << config.tempThreshold << "\n";
    std::cout << "Pressure threshold: " << config.pressureThreshold << "\n";
    std::cout << "Update interval(ms): " << config.updateIntervalMs << "\n";
    
    // 사용자 명령 입력 루프
    while (true) {
        std::cout << "\nEnter command (INIT, START, STOP, RESET, STATUS, EXIT): ";
        std::getline(std::cin, command);

        if (command.empty()) {
            continue;
            
        }
        std::string upper = command;
        for (char& ch : upper) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        
        // EXIT 입력 시 프로그램 종료
        if (upper == "EXIT") {
            logger.log("CMD EXIT");
            running = false;
            break;
        }
        
        // 명령 실행 후 결과 출력
        std::string response = controller.executeCommand(command);
        std::cout << response << "\n";
    }
    
    // 스레드 및 서버 종료
    if (updateThread.joinable()) {
        updateThread.join();
    }

    tcpServer.stop();

    logger.log("SYSTEM STOP");
    return 0;
}
