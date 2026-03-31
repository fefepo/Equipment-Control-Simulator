#include "Logger.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <fstream>

// 현재 시간을 "YYYY-MM-DD HH:MM:SS" 형식으로 반환
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

// 파일이 비어있는지 확인 (헤더 작성 여부 판단용)
bool Logger::isFileEmpty(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    return !file.is_open() || file.tellg() == 0;
}

// 로그 파일 + 알람 CSV 파일 초기화
Logger::Logger(const std::string& filename) {
    logFile.open(filename, std::ios::app);

    const std::string alarmCsvFilename = "alarm_history.csv";
    bool writeHeader = isFileEmpty(alarmCsvFilename);

    alarmCsvFile.open(alarmCsvFilename, std::ios::app);
    
    // CSV 파일이 비어있으면 헤더 작성
    if (alarmCsvFile.is_open() && writeHeader) {
        alarmCsvFile << "Timestamp,AlarmCode,AlarmMessage\n";
        alarmCsvFile.flush();
    }
}

// 파일 종료 처리
Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }

    if (alarmCsvFile.is_open()) {
        alarmCsvFile.close();
    }
}

// 일반 로그 출력 (콘솔 + 파일)
void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::string line = "[" + getCurrentTimestamp() + "] " + message;

    std::cout << line << "\n";

    if (logFile.is_open()) {
        logFile << line << "\n";
        logFile.flush();
    }
}

// 알람 로그를 CSV 파일에 기록
void Logger::logAlarmToCsv(const std::string& alarmCode, const std::string& alarmMessage) {
    std::lock_guard<std::mutex> lock(logMutex);

    if (!alarmCsvFile.is_open()) {
        return;
    }
    
    // CSV 깨짐 방지 ',' → ';' 치환
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
