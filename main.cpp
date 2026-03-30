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

    std::cout << "Current working directory: "
        << std::filesystem::current_path().string() << "\n";

    Config config;

    if (!config.load("config.json")) {
        std::cout << "Failed to load config.json, using defaults\n";
    }

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

    tcpServer.start();

    std::thread updateThread([&]() {
        while (running) {
            controller.update();
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config.updateIntervalMs)
            );
        }
        });

    std::cout << "=== Equipment Control Simulator ===\n";
    std::cout << "TCP server listening on port " << config.port << "\n";
    std::cout << "Temp threshold: " << config.tempThreshold << "\n";
    std::cout << "Pressure threshold: " << config.pressureThreshold << "\n";
    std::cout << "Update interval(ms): " << config.updateIntervalMs << "\n";

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

        if (upper == "EXIT") {
            logger.log("CMD EXIT");
            running = false;
            break;
        }

        std::string response = controller.executeCommand(command);
        std::cout << response << "\n";
    }

    if (updateThread.joinable()) {
        updateThread.join();
    }

    tcpServer.stop();

    logger.log("SYSTEM STOP");
    return 0;
}