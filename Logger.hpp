#pragma once

#include <string>
#include <fstream>
#include <mutex>

class Logger {
private:
    std::ofstream logFile;
    std::ofstream alarmCsvFile;
    std::mutex logMutex;

    std::string getCurrentTimestamp();
    bool isFileEmpty(const std::string& filename);

public:
    Logger(const std::string& filename);
    ~Logger();

    void log(const std::string& message);
    void logAlarmToCsv(const std::string& alarmCode, const std::string& alarmMessage);
};