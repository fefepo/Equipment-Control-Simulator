#include "Logger.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <fstream>

std::string Logger::getCurrentTimestamp() {
    auto now = std::time(nullptr);
    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool Logger::isFileEmpty(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    return !file.is_open() || file.tellg() == 0;
}

Logger::Logger(const std::string& filename) {
    logFile.open(filename, std::ios::app);

    const std::string alarmCsvFilename = "alarm_history.csv";
    bool writeHeader = isFileEmpty(alarmCsvFilename);

    alarmCsvFile.open(alarmCsvFilename, std::ios::app);

    if (alarmCsvFile.is_open() && writeHeader) {
        alarmCsvFile << "Timestamp,AlarmCode,AlarmMessage\n";
        alarmCsvFile.flush();
    }
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }

    if (alarmCsvFile.is_open()) {
        alarmCsvFile.close();
    }
}

void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::string line = "[" + getCurrentTimestamp() + "] " + message;

    std::cout << line << "\n";

    if (logFile.is_open()) {
        logFile << line << "\n";
        logFile.flush();
    }
}

void Logger::logAlarmToCsv(const std::string& alarmCode, const std::string& alarmMessage) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (!alarmCsvFile.is_open()) {
        return;
    }

    std::string sanitizedMessage = alarmMessage;
    for (char& ch : sanitizedMessage) {
        if (ch == ',') {
            ch = ';';
        }
    }

    alarmCsvFile << getCurrentTimestamp() << ","
        << alarmCode << ","
        << sanitizedMessage << "\n";

    alarmCsvFile.flush();
}