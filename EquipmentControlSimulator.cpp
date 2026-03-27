#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>

enum class EquipmentState {
    IDLE,
    INITIALIZING,
    READY,
    RUNNING,
    STOPPING,
    ERROR
};

// 장비 상태를 문자열로 변환
std::string toString(EquipmentState state) {
    switch (state) {
    case EquipmentState::IDLE: return "IDLE";
    case EquipmentState::INITIALIZING: return "INITIALIZING";
    case EquipmentState::READY: return "READY";
    case EquipmentState::RUNNING: return "RUNNING";
    case EquipmentState::STOPPING: return "STOPPING";
    case EquipmentState::ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

// 장비 상태와 센서값을 제어하는 클래스
class EquipmentController {
private:
    EquipmentState state = EquipmentState::IDLE; // 현재 장비 상태
    double temperature = 25.0; // 현재 온도
    double pressure = 1.0; // 현재 압력
    int motorSpeed = 0; // 현재 모터 속도
    mutable std::mutex mtx; // 동시 접근 보호

public:

    // 초기화 시작
    void init() {
        std::lock_guard<std::mutex> lock(mtx);
        if (state == EquipmentState::IDLE) {
            state = EquipmentState::INITIALIZING;
            std::cout << "INIT OK\n";
        }
        else {
            std::cout << "Cannot INIT in current state\n";
        }
    }

    // 장비 동작 시작
    void start() {
        std::lock_guard<std::mutex> lock(mtx);
        if (state == EquipmentState::READY) {
            state = EquipmentState::RUNNING;
            motorSpeed = 1000;
            std::cout << "START OK\n";
        }
        else {
            std::cout << "Cannot START in current state\n";
        }
    }

    // 장비 정지 시작
    void stop() {
        std::lock_guard<std::mutex> lock(mtx);
        if (state == EquipmentState::RUNNING) {
            state = EquipmentState::STOPPING;
            std::cout << "STOP OK\n";
        }
        else {
            std::cout << "Cannot STOP in current state\n";
        }
    }

    // 에러 상태 초기화
    void reset() {
        std::lock_guard<std::mutex> lock(mtx);
        if (state == EquipmentState::ERROR) {
            state = EquipmentState::IDLE;
            temperature = 25.0;
            pressure = 1.0;
            motorSpeed = 0;
            std::cout << "RESET OK\n";
        }
        else {
            std::cout << "Cannot RESET in current state\n";
        }
    }

    // 주기적으로 상태를 갱신하는 핵심 로직
    void update() {
        std::lock_guard<std::mutex> lock(mtx);

        switch (state) {
        case EquipmentState::INITIALIZING:
            temperature += 0.3;
            if (temperature >= 30.0) {
                state = EquipmentState::READY;
                std::cout << "[AUTO] INITIALIZING -> READY\n";
            }
            break;

        case EquipmentState::RUNNING:
            temperature += 0.5;
            pressure += 0.03;
            motorSpeed = 1200;
            
            // 온도 초과 시 에러 상태 전환
            if (temperature > 80.0) {
                state = EquipmentState::ERROR;
                motorSpeed = 0;
                std::cout << "!!! ERROR: OVERHEAT !!!\n";
            }
            break;

        case EquipmentState::STOPPING:
            if (motorSpeed > 0) motorSpeed -= 200;
            if (temperature > 25.0) temperature -= 0.4;
            if (pressure > 1.0) pressure -= 0.02;

            if (motorSpeed <= 0) {
                motorSpeed = 0;
                state = EquipmentState::READY;
                std::cout << "[AUTO] STOPPING -> READY\n";
            }
            break;

        case EquipmentState::READY:
            if (temperature > 28.0) temperature -= 0.1;
            if (pressure > 1.0) pressure -= 0.01;
            break;

        case EquipmentState::ERROR:
            motorSpeed = 0;
            break;

        case EquipmentState::IDLE:
            break;
        }
    }

    // 현재 상태 출력
    void printStatus() const {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "[STATUS] "
            << "State=" << toString(state)
            << ", Temp=" << temperature
            << ", Pressure=" << pressure
            << ", MotorSpeed=" << motorSpeed
            << "\n";
    }
};

int main() {
    EquipmentController controller;
    std::atomic<bool> running = true;
    std::string command;
    
    // 장비 상태를 주기적으로 업데이트하는 스레드
    std::thread updateThread([&]() {
        while (running) {
            controller.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        });

    std::cout << "=== Equipment Control Simulator ===\n";
    
    // 사용자 명령 입력 루프
    while (true) {
        std::cout << "\nEnter command (INIT, START, STOP, RESET, STATUS, EXIT): ";
        std::cin >> command;

        if (command == "INIT") controller.init();
        else if (command == "START") controller.start();
        else if (command == "STOP") controller.stop();
        else if (command == "RESET") controller.reset();
        else if (command == "STATUS") controller.printStatus();
        else if (command == "EXIT") {
            running = false;
            break;
        }
        else {
            std::cout << "Unknown command\n";
        }
    }

    if (updateThread.joinable()) {
        updateThread.join();
    }

    return 0;
}
