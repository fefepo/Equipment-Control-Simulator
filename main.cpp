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

    // 현재 실행 경로 출력 (config.json 위치 확인용)
    std::cout << "Current working directory: "
        << std::filesystem::current_path().string() << "\n";
                
    // 설정 객체 생성
    Config config;
    
    // 설정 파일 로드 (실패 시 기본값 사용)
    if (!config.load("config.json")) {
        std::cout << "Failed to load config.json, using defaults\n";
    }

    // 로그 파일 생성 및 초기 로그 기록
    Logger logger("simulator_log.txt");
    logger.log("SYSTEM START");
        
    // 설정 값 로그 기록
    logger.log("CONFIG PORT=" + std::to_string(config.port) +
        ", TEMP_THRESHOLD=" + std::to_string(config.tempThreshold) +
        ", PRESSURE_THRESHOLD=" + std::to_string(config.pressureThreshold) +
        ", UPDATE_INTERVAL_MS=" + std::to_string(config.updateIntervalMs));

    // 장비 제어 객체 생성 (로그 + 설정 전달)
    EquipmentController controller(logger, config);

    // TCP 서버 생성 (외부에서 명령 받을 수 있음)
    TcpServer tcpServer(controller, logger, config.port);

    // 프로그램 실행 상태 플래그 (멀티스레드 공유)
    std::atomic<bool> running = true;
    std::string command;

    // TCP 서버 시작
    tcpServer.start();

    // 장비 상태를 주기적으로 업데이트하는 스레드
    std::thread updateThread([&]() {
        while (running) {
            controller.update(); // 온도, 압력 등 시뮬레이션 갱신
            
            // 설정된 주기만큼 대기
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config.updateIntervalMs)
            );
        }
        });

    // 초기 상태 출력
    std::cout << "=== Equipment Control Simulator ===\n";
    std::cout << "TCP server listening on port " << config.port << "\n";
    std::cout << "Temp threshold: " << config.tempThreshold << "\n";
    std::cout << "Pressure threshold: " << config.pressureThreshold << "\n";
    std::cout << "Update interval(ms): " << config.updateIntervalMs << "\n";

    // 사용자 명령 입력 루프
    while (true) {
        std::cout << "\nEnter command (INIT, START, STOP, RESET, STATUS, EXIT): ";
        std::getline(std::cin, command);

        // 빈 입력 무시
        if (command.empty()) {
            continue;
        }

        // 입력값을 대문자로 변환 (명령어 비교용)
        std::string upper = command;
        for (char& ch : upper) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }

        // 종료 명령 처리
        if (upper == "EXIT") {
            logger.log("CMD EXIT");
            running = false;
            break;
        }

        // 장비 컨트롤러에 명령 전달
        std::string response = controller.executeCommand(command);

        // 결과 출력
        std::cout << response << "\n";
    }

    // 스레드 종료 대기 (안전 종료)
    if (updateThread.joinable()) {
        updateThread.join();
    }

    // TCP 서버 종료
    tcpServer.stop();

    // 종료 로그 기록
    logger.log("SYSTEM STOP");
    return 0;
}