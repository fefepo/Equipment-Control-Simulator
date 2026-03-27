#pragma once

#include <string>
#include <mutex>
#include <vector>

#include "Logger.hpp"
#include "Config.hpp"

// 장비 전체 동작 상태
enum class EquipmentState {
    IDLE,
    INITIALIZING,
    READY,
    RUNNING,
    STOPPING,
    ERROR
};

// 공정 진행 단계
enum class ProcessStep {
    NONE,
    LOAD,
    HEAT,
    PROCESS,
    COOLING,
    UNLOAD
};

// 운전 모드
enum class OperationMode {
    MANUAL,
    AUTO
};

std::string toString(EquipmentState state);
std::string toString(ProcessStep step);
std::string toString(OperationMode mode);

// 장비 상태, 센서값, 명령 제어를 담당하는 클래스
class EquipmentController {
private:
    EquipmentState state = EquipmentState::IDLE; // 현재 장비 상태
    ProcessStep currentStep = ProcessStep::NONE; // 현재 공정 단계
    OperationMode mode = OperationMode::MANUAL; // 현재 운전 모드

    double temperature = 25.0; // 초기 온도
    double pressure = 1.0; // 초기 압력
    int motorSpeed = 0; // 초기 모터 속도

    std::string activeAlarmCode = "NONE"; // 기본 알람 코드
    std::string activeAlarmMessage = "No active alarm"; // 기본 알람 메시지
    std::vector<std::string> alarmHistory; // 알람 이력 저장

    int stepTick = 0; // 공정 단계 진행 시간 카운트

    mutable std::mutex mtx; // 동시 접근 보호
    Logger& logger; // 로그 기록 객체
    const Config& config; // 설정값 참조

    void changeState(EquipmentState newState); // 상태 변경
    void changeStep(ProcessStep newStep); // 공정 단계 변경
    void setAlarm(const std::string& code, const std::string& message); // 알람 발생
    void clearAlarm(); // 알람 초기화
    std::string getAlarmHistoryString() const; // 알람 이력 반환

public:
    EquipmentController(Logger& logRef, const Config& cfg);

    void init(); // 초기화 시작
    void start(); // 장비 동작 시작
    void stop(); // 장비 정지
    void reset(); // 에러 초기화
    void update(); // 상태 주기적 갱신
    void printStatus() const; // 상태 출력

    std::string getStatusString() const; // 상태 문자열 반환
    std::string executeCommand(const std::string& rawCommand); // 명령어 실행
};
