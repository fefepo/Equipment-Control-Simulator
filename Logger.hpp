#pragma once

#include <string>
#include <fstream>
#include <mutex>

class Logger {
private:
    std::ofstream logFile; // 일반 로그 파일
    std::ofstream alarmCsvFile;  // 알람 CSV 파일
    std::mutex logMutex;  // 멀티스레드 보호

    std::string getCurrentTimestamp(); // 현재 시간 문자열 생성
    bool isFileEmpty(const std::string& filename); // 파일 비어있는지 확인

public:
    Logger(const std::string& filename); // 로그 파일 초기화
    ~Logger(); // 파일 종료

    void log(const std::string& message); // 콘솔 + 파일 로그 출력
    void logAlarmToCsv(const std::string& alarmCode, const std::string& alarmMessage); // 알람(ERROR) CSV 기록
};
