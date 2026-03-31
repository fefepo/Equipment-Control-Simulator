#pragma once

#include <string>
#include <mutex>
#include <vector>

#include "Logger.hpp"
#include "Config.hpp"
#include "MotionController.hpp"


// 장비 전체 상태 (상위 상태 머신)
enum class EquipmentState {
    IDLE, // 대기 상태
    INITIALIZING, // 초기화 중
    READY, // 준비 완료
    RUNNING, // 공정 실행 중
    STOPPING, // 정지 처리 중
    ERROR // 에러 상태
};

// 공정 내부 단계 (시퀀스 흐름)
enum class ProcessStep {
    NONE, // 아무 단계 없음
    LOAD, // 로딩
    HEAT, // 가열
    PROCESS, // 공정 처리
    COOLING,  // 냉각
    UNLOAD // 언로딩

};


// 운전 모드 (수동/자동)
enum class OperationMode {
    MANUAL, // 수동 제어
    AUTO // 자동 공정
};

// enum → 문자열 변환 함수 (로그/출력용)
std::string toString(EquipmentState state);
std::string toString(ProcessStep step);
std::string toString(OperationMode mode);

class EquipmentController {
private:

    // ===== 상태 관리 =====
    EquipmentState state = EquipmentState::IDLE; // 현재 장비 상태
    ProcessStep currentStep = ProcessStep::NONE; // 현재 공정 단계
    OperationMode mode = OperationMode::MANUAL; // 현재 모드

    // ===== 공정 변수 (시뮬레이션 값) =====
    double temperature = 25.0; // 온도
    double pressure = 1.0; // 압력
    int motorSpeed = 0;  // 모터 속도


    // ===== 알람 관리 =====
    std::string activeAlarmCode = "NONE"; // 현재 알람 코드
    std::string activeAlarmMessage = "No active alarm"; // 알람 메시지
    std::vector<std::string> alarmHistory; // 알람 이력


    // 공정 단계 시간 카운터 (step 진행 제어용)
    int stepTick = 0;

    // ===== 동기화 =====
    mutable std::mutex mtx; // 멀티스레드 접근 보호 (update / command 동시 접근 방지)

    // ===== 외부 의존 객체 =====
    Logger& logger; // 로그 시스템 (참조)
    const Config& config; // 설정값 (읽기 전용 참조)

    // ===== 모션 제어 =====
    MotionController motionController; // X/Y/Z 축 등 제어 담당

    // ===== 내부 상태 변경 함수 =====
    void changeState(EquipmentState newState); // 상태 변경 + 로그 처리
    void changeStep(ProcessStep newStep); // 공정 단계 변경
    void setAlarm(const std::string& code, const std::string& message); // 알람 발생
    void clearAlarm(); // 알람 해제

    // 알람 이력을 문자열로 변환 (출력용)
    std::string getAlarmHistoryString() const;

    // 모션 관련 명령 처리 (MOVE, HOME 등)
    std::string executeMotionCommand(const std::string& rawCommand);

    // 특정 축이 목표 위치에 도달했는지 확인
    bool isAxisInPosition(const Axis& axis, double target, double tolerance = 0.5) const;

    // 공정 시작 가능 여부 체크 (인터락 로직)
    bool canStartProcess(std::string& reason) const;


public:
    // 생성자 (Logger, Config 의존성 주입)
    EquipmentController(Logger& logRef, const Config& cfg);

    // ===== 외부 제어 함수 =====
    void init(); // 초기화
    void start(); // 공정 시작
    void stop(); // 정지
    void reset(); // 에러/상태 초기화
    void update();  // 주기적으로 호출되는 업데이트 (온도/압력/공정 진행)
    void printStatus() const; // 콘솔 출력용 상태 표시

    std::string getStatusString() const; // 상태 문자열 반환 (TCP 응답용)
    std::string executeCommand(const std::string& rawCommand); // 외부 명령 처리 (콘솔 / TCP 공통 진입점)

};