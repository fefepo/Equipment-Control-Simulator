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

class EquipmentController {
private:
    EquipmentState state = EquipmentState::IDLE;
    double temperature = 25.0;
    double pressure = 1.0;
    int motorSpeed = 0;
    mutable std::mutex mtx;

public:
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

    std::thread updateThread([&]() {
        while (running) {
            controller.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        });

    std::cout << "=== Equipment Control Simulator ===\n";

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