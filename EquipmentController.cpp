#include "EquipmentController.hpp"

#include <iostream>
#include <algorithm>
#include <cctype>
#include <sstream>

// enum → 문자열 변환 (로그/출력용)
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

// 공정 단계 문자열 변환
std::string toString(ProcessStep step) {
    switch (step) {
    case ProcessStep::NONE: return "NONE";
    case ProcessStep::LOAD: return "LOAD";
    case ProcessStep::HEAT: return "HEAT";
    case ProcessStep::PROCESS: return "PROCESS";
    case ProcessStep::COOLING: return "COOLING";
    case ProcessStep::UNLOAD: return "UNLOAD";
    default: return "UNKNOWN";
    }
}

// 동작 모드 문자열 변환
std::string toString(OperationMode mode) {
    switch (mode) {
    case OperationMode::MANUAL: return "MANUAL";
    case OperationMode::AUTO: return "AUTO";
    default: return "UNKNOWN";
    }
}

namespace {
    // 공백 제거 + 대문자 변환 → 명령어 표준화
    std::string normalizeCommand(const std::string& input) {
        std::string result;

        for (char ch : input) {
            if (!std::isspace(static_cast<unsigned char>(ch))) {
                result += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            }
        }

        return result;
    }
    // 문자열 전체 대문자 변환
    std::string toUpperCopy(const std::string& input) {
        std::string result = input;
        for (char& ch : result) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        return result;
    }
}

// 생성자 (Logger, Config 참조 저장)
EquipmentController::EquipmentController(Logger& logRef, const Config& cfg)
    : logger(logRef), config(cfg) {
}

// 상태 변경 + 로그 기록
void EquipmentController::changeState(EquipmentState newState) {
    if (state != newState) {
        std::string oldState = toString(state);
        std::string nextState = toString(newState);
        state = newState;
        logger.log("STATE " + oldState + " -> " + nextState);
    }
}

// 공정 단계 변경 + tick 초기화
void EquipmentController::changeStep(ProcessStep newStep) {
    if (currentStep != newStep) {
        std::string oldStep = toString(currentStep);
        std::string nextStep = toString(newStep);
        currentStep = newStep;
        stepTick = 0;
        logger.log("PROCESS STEP " + oldStep + " -> " + nextStep);
    }
}

// 알람 설정 + 기록 저장 + 로그/CSV 출력
void EquipmentController::setAlarm(const std::string& code, const std::string& message) {
    activeAlarmCode = code;
    activeAlarmMessage = message;

    std::string historyEntry = "[" + activeAlarmCode + "] " + activeAlarmMessage;
    alarmHistory.push_back(historyEntry);

    logger.log("ALARM " + historyEntry);
    logger.logAlarmToCsv(activeAlarmCode, activeAlarmMessage);
}

// 알람 초기화
void EquipmentController::clearAlarm() {
    activeAlarmCode = "NONE";
    activeAlarmMessage = "No active alarm";
    logger.log("ALARM CLEARED");
}

// 알람 히스토리 문자열 반환
std::string EquipmentController::getAlarmHistoryString() const {
    if (alarmHistory.empty()) {
        return "No alarm history";
    }

    std::ostringstream oss;
    for (size_t i = 0; i < alarmHistory.size(); ++i) {
        oss << i + 1 << ". " << alarmHistory[i];
        if (i + 1 < alarmHistory.size()) {
            oss << " | ";
        }
    }

    return oss.str();
}

// INIT 명령 → IDLE → INITIALIZING
void EquipmentController::init() {
    std::lock_guard<std::mutex> lock(mtx);
    logger.log("CMD INIT");

    if (state == EquipmentState::IDLE) {
        changeState(EquipmentState::INITIALIZING);
    }
    else {
        logger.log("WARN Cannot INIT in current state");
    }
}

// START 명령 → 공정 시작
void EquipmentController::start() {
    std::lock_guard<std::mutex> lock(mtx);
    logger.log("CMD START");

    if (state == EquipmentState::READY) {
        motorSpeed = 800;
        stepTick = 0;
        changeStep(ProcessStep::LOAD);
        changeState(EquipmentState::RUNNING);
    }
    else {
        logger.log("WARN Cannot START in current state");
    }
}

// STOP 명령 → 정지 단계 진입
void EquipmentController::stop() {
    std::lock_guard<std::mutex> lock(mtx);
    logger.log("CMD STOP");

    if (state == EquipmentState::RUNNING) {
        changeStep(ProcessStep::NONE);
        changeState(EquipmentState::STOPPING);
    }
    else {
        logger.log("WARN Cannot STOP in current state");
    }
}

// RESET → ERROR 상태에서 초기화
void EquipmentController::reset() {
    std::lock_guard<std::mutex> lock(mtx);
    logger.log("CMD RESET");

    if (state == EquipmentState::ERROR) {
        temperature = 25.0;
        pressure = 1.0;
        motorSpeed = 0;
        stepTick = 0;
        changeStep(ProcessStep::NONE);
        clearAlarm();
        changeState(EquipmentState::IDLE);
    }
    else {
        logger.log("WARN Cannot RESET in current state");
    }
}

// 시스템 주기 업데이트 (핵심 로직)
void EquipmentController::update() {
    std::lock_guard<std::mutex> lock(mtx);

    switch (state) {
    case EquipmentState::INITIALIZING:
        temperature += 0.3;
        if (temperature >= 30.0) {
            changeState(EquipmentState::READY);
        }
        break;

    case EquipmentState::RUNNING:
        stepTick++;
        
        // 공정 단계별 동작
        switch (currentStep) {
        case ProcessStep::LOAD:
            motorSpeed = 600;
            temperature += 0.2;
            pressure += 0.01;
            if (stepTick >= 10) {
                changeStep(ProcessStep::HEAT);
            }
            break;

        case ProcessStep::HEAT:
            motorSpeed = 900;
            temperature += 1.2;
            pressure += 0.02;
            if (stepTick >= 15) {
                changeStep(ProcessStep::PROCESS);
            }
            break;

        case ProcessStep::PROCESS:
            motorSpeed = 1200;
            temperature += 0.8;
            pressure += 0.06;
            if (stepTick >= 20) {
                changeStep(ProcessStep::COOLING);
            }
            break;

        case ProcessStep::COOLING:
            motorSpeed = 700;
            if (temperature > 25.0) temperature -= 1.0;
            if (pressure > 1.0) pressure -= 0.04;
            if (stepTick >= 15) {
                changeStep(ProcessStep::UNLOAD);
            }
            break;

        case ProcessStep::UNLOAD:
            motorSpeed = 400;
            if (temperature > 25.0) temperature -= 0.3;
            if (pressure > 1.0) pressure -= 0.02;
            if (stepTick >= 10) {
                motorSpeed = 0;

                if (mode == OperationMode::AUTO) {
                    logger.log("AUTO MODE: restarting next cycle");
                    changeStep(ProcessStep::LOAD);
                }
                else {
                    changeStep(ProcessStep::NONE);
                    changeState(EquipmentState::READY);
                }
            }
            break;

        case ProcessStep::NONE:
            motorSpeed = 0;
            break;
        }
        
        // 임계값 초과 시 알람 발생
        if (temperature > config.tempThreshold) {
            motorSpeed = 0;
            setAlarm("E001", "Overheat detected during step " + toString(currentStep));
            changeStep(ProcessStep::NONE);
            changeState(EquipmentState::ERROR);
            return;
        }

        if (pressure > config.pressureThreshold) {
            motorSpeed = 0;
            setAlarm("E002", "Overpressure detected during step " + toString(currentStep));
            changeStep(ProcessStep::NONE);
            changeState(EquipmentState::ERROR);
            return;
        }

        if (motorSpeed > config.speedThreshold) {
            motorSpeed = 0;
            setAlarm("E003", "Overspeed detected during step " + toString(currentStep));
            changeStep(ProcessStep::NONE);
            changeState(EquipmentState::ERROR);
            return;
        }

        break;

    case EquipmentState::STOPPING:
        if (motorSpeed > 0) motorSpeed -= 200;
        if (temperature > 25.0) temperature -= 0.4;
        if (pressure > 1.0) pressure -= 0.02;

        if (motorSpeed <= 0) {
            motorSpeed = 0;
            changeState(EquipmentState::READY);
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

// 현재 상태 문자열 반환
std::string EquipmentController::getStatusString() const {
    std::lock_guard<std::mutex> lock(mtx);

    return "State=" + toString(state) +
        ", Step=" + toString(currentStep) +
        ", Mode=" + toString(mode) +
        ", Temp=" + std::to_string(temperature) +
        ", Pressure=" + std::to_string(pressure) +
        ", MotorSpeed=" + std::to_string(motorSpeed) +
        ", AlarmCode=" + activeAlarmCode +
        ", AlarmMessage=" + activeAlarmMessage;
}

// 상태 출력
void EquipmentController::printStatus() const {
    std::cout << "[STATUS] " << getStatusString() << "\n";
}

// 문자열 명령어 실행 (핵심 인터페이스)
std::string EquipmentController::executeCommand(const std::string& rawCommand) {
    std::string upperRaw = toUpperCopy(rawCommand);
    std::string command = normalizeCommand(rawCommand);

    if (command == "INIT") {
        std::lock_guard<std::mutex> lock(mtx);
        logger.log("CMD INIT");

        if (state == EquipmentState::IDLE) {
            changeState(EquipmentState::INITIALIZING);
            return "OK: INIT";
        }
        return "ERROR: Cannot INIT in current state";
    }

    if (command == "START") {
        std::lock_guard<std::mutex> lock(mtx);
        logger.log("CMD START");

        if (state == EquipmentState::READY) {
            motorSpeed = 800;
            stepTick = 0;
            changeStep(ProcessStep::LOAD);
            changeState(EquipmentState::RUNNING);
            return "OK: START";
        }
        return "ERROR: Cannot START in current state";
    }

    if (command == "STOP") {
        std::lock_guard<std::mutex> lock(mtx);
        logger.log("CMD STOP");

        if (state == EquipmentState::RUNNING) {
            changeStep(ProcessStep::NONE);
            changeState(EquipmentState::STOPPING);
            return "OK: STOP";
        }
        return "ERROR: Cannot STOP in current state";
    }

    if (command == "RESET") {
        std::lock_guard<std::mutex> lock(mtx);
        logger.log("CMD RESET");

        if (state == EquipmentState::ERROR) {
            temperature = 25.0;
            pressure = 1.0;
            motorSpeed = 0;
            stepTick = 0;
            changeStep(ProcessStep::NONE);
            clearAlarm();
            changeState(EquipmentState::IDLE);
            return "OK: RESET";
        }
        return "ERROR: Cannot RESET in current state";
    }

    if (command == "STATUS") {
        return "OK: STATUS " + getStatusString();
    }

    if (command == "ALARMHISTORY") {
        std::lock_guard<std::mutex> lock(mtx);
        return "OK: ALARM_HISTORY " + getAlarmHistoryString();
    }

    if (upperRaw.find("SET MODE ") == 0 || upperRaw.find("SETMODE") == 0) {
        std::lock_guard<std::mutex> lock(mtx);

        std::string normalized = upperRaw;
        normalized.erase(
            std::remove_if(normalized.begin(), normalized.end(),
                [](unsigned char c) { return std::isspace(c); }),
            normalized.end()
        );

        if (normalized == "SETMODEAUTO") {
            mode = OperationMode::AUTO;
            logger.log("MODE CHANGED -> AUTO");
            return "OK: MODE AUTO";
        }

        if (normalized == "SETMODEMANUAL") {
            mode = OperationMode::MANUAL;
            logger.log("MODE CHANGED -> MANUAL");
            return "OK: MODE MANUAL";
        }

        return "ERROR: Invalid mode";
    }

    logger.log("WARN Unknown command: " + rawCommand);
    return "ERROR: Unknown command";
}
