#include "EquipmentController.hpp"

#include <iostream>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>
#include <cmath>   // std::abs

// ============================
// enum -> string 변환
// ============================
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

std::string toString(OperationMode mode) {
    switch (mode) {
    case OperationMode::MANUAL: return "MANUAL";
    case OperationMode::AUTO: return "AUTO";
    default: return "UNKNOWN";
    }
}

// ============================
// 내부 유틸 함수
// ============================
namespace {
    // 공백 제거 + 대문자 변환
    std::string normalizeCommand(const std::string& input) {
        std::string result;

        for (char ch : input) {
            if (!std::isspace(static_cast<unsigned char>(ch))) {
                result += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            }
        }

        return result;
    }

    // 문자열 전체를 대문자로 복사
    std::string toUpperCopy(const std::string& input) {
        std::string result = input;
        for (char& ch : result) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        return result;
    }

    // 공백 기준으로 명령어 토큰 분리
    std::vector<std::string> splitTokens(const std::string& input) {
        std::vector<std::string> tokens;
        std::istringstream iss(input);
        std::string token;

        while (iss >> token) {
            tokens.push_back(token);
        }

        return tokens;
    }

    // 문자열을 double로 안전하게 변환
    bool tryParseDouble(const std::string& text, double& value) {
        try {
            size_t idx = 0;
            value = std::stod(text, &idx);
            return idx == text.size();
        }
        catch (...) {
            return false;
        }
    }
}

// ============================
// 생성자
// ============================
EquipmentController::EquipmentController(Logger& logRef, const Config& cfg)
    : logger(logRef), config(cfg) {
}

// ============================
// 상태 변경 함수
// ============================
void EquipmentController::changeState(EquipmentState newState) {
    if (state != newState) {
        std::string oldState = toString(state);
        std::string nextState = toString(newState);
        state = newState;
        logger.log("STATE " + oldState + " -> " + nextState);
    }
}

void EquipmentController::changeStep(ProcessStep newStep) {
    if (currentStep != newStep) {
        std::string oldStep = toString(currentStep);
        std::string nextStep = toString(newStep);
        currentStep = newStep;
        stepTick = 0;
        logger.log("PROCESS STEP " + oldStep + " -> " + nextStep);
    }
}

// ============================
// 알람 처리
// ============================
void EquipmentController::setAlarm(const std::string& code, const std::string& message) {
    activeAlarmCode = code;
    activeAlarmMessage = message;

    std::string historyEntry = "[" + activeAlarmCode + "] " + activeAlarmMessage;
    alarmHistory.push_back(historyEntry);

    logger.log("ALARM " + historyEntry);
    logger.logAlarmToCsv(activeAlarmCode, activeAlarmMessage);
}

void EquipmentController::clearAlarm() {
    activeAlarmCode = "NONE";
    activeAlarmMessage = "No active alarm";
    logger.log("ALARM CLEARED");
}

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

// ============================
// START 조건 확인 함수
// X=50, Y=50, Z=100 위치인지 검사
// ============================
bool EquipmentController::isAxisInPosition(const Axis& axis, double target, double tolerance) const {
    return std::abs(axis.currentPosition - target) <= tolerance;
}

bool EquipmentController::canStartProcess(std::string& reason) const {
    Axis x = motionController.getAxisData("X");
    Axis y = motionController.getAxisData("Y");
    Axis z = motionController.getAxisData("Z");


    // 모든 축 Servo ON 상태인지 확인
    if (!x.servoOn || !y.servoOn || !z.servoOn) {
        reason = "All axes must be SERVO ON";
        return false;
    }

    // 모든 축 Home 완료 여부 확인
    if (!x.homed || !y.homed || !z.homed) {
        reason = "All axes must be HOMED";
        return false;
    }

    // 시작 좌표 조건 확인
    if (!isAxisInPosition(x, 50.0)) {
        reason = "X axis must be at position 50";
        return false;
    }

    if (!isAxisInPosition(y, 50.0)) {
        reason = "Y axis must be at position 50";
        return false;
    }

    if (!isAxisInPosition(z, 100.0)) {
        reason = "Z axis must be at position 100";
        return false;
    }



    if (std::abs(x.currentPosition - x.requiredPosition) > 0.5) {
        reason = "X axis not in required position";
        return false;
    }

    if (std::abs(y.currentPosition - y.requiredPosition) > 0.5) {
        reason = "Y axis not in required position";
        return false;
    }

    if (std::abs(z.currentPosition - z.requiredPosition) > 0.5) {
        reason = "Z axis not in required position";
        return false;
    }

    return true;
}

// ============================
// 기본 장비 명령
// ============================
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

void EquipmentController::start() {
    std::lock_guard<std::mutex> lock(mtx);
    logger.log("CMD START");

    if (state == EquipmentState::READY) {
        // START 전 모션 조건 검사
        std::string reason;
        if (!canStartProcess(reason)) {
            logger.log("WARN Cannot START: " + reason);
            return;
        }

        motorSpeed = 800;
        stepTick = 0;
        changeStep(ProcessStep::LOAD);
        changeState(EquipmentState::RUNNING);
    }
    else {
        logger.log("WARN Cannot START in current state");
    }
}

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

// ============================
// 주기 업데이트 함수
// ============================
void EquipmentController::update() {
    std::lock_guard<std::mutex> lock(mtx);

    // 모션 축 상태도 주기적으로 갱신
    motionController.update();

    // 축 알람 발생 시 장비 전체를 ERROR로 전환
    if (motionController.hasAnyAlarm() && state != EquipmentState::ERROR) {
        motorSpeed = 0;
        setAlarm("E200", "Motion axis alarm detected");
        changeStep(ProcessStep::NONE);
        changeState(EquipmentState::ERROR);
        return;
    }

    switch (state) {
    case EquipmentState::INITIALIZING:
        temperature += 0.3;
        if (temperature >= 30.0) {
            changeState(EquipmentState::READY);
        }
        break;

    case EquipmentState::RUNNING:
        stepTick++;

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

        // 기존 장비 인터락
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

// ============================
// 상태 출력
// ============================
std::string EquipmentController::getStatusString() const {
    std::lock_guard<std::mutex> lock(mtx);

    return "State=" + toString(state) +
        ", Step=" + toString(currentStep) +
        ", Mode=" + toString(mode) +
        ", Temp=" + std::to_string(temperature) +
        ", Pressure=" + std::to_string(pressure) +
        ", MotorSpeed=" + std::to_string(motorSpeed) +
        ", AlarmCode=" + activeAlarmCode +
        ", AlarmMessage=" + activeAlarmMessage +
        "\nMotion:\n" + motionController.getAllAxisStatus();
}

void EquipmentController::printStatus() const {
    std::cout << "[STATUS] " << getStatusString() << "\n";
}

// ============================
// 모션 명령 처리
// ============================
std::string EquipmentController::executeMotionCommand(const std::string& rawCommand) {
    std::vector<std::string> tokens = splitTokens(rawCommand);
    if (tokens.empty()) {
        return "ERROR: Empty command";
    }

    for (std::string& token : tokens) {
        token = toUpperCopy(token);
    }

    // 수동 모션 제어는 READY 상태에서만 허용
    if (state != EquipmentState::READY) {
        return "ERROR: Motion command allowed only in READY state";
    }

    MotionResult result{ false, "E999", "Unhandled motion command" };

    if (tokens[0] == "SERVO_ON" && tokens.size() == 2) {
        result = motionController.servoOn(tokens[1]);
    }
    else if (tokens[0] == "SERVO_OFF" && tokens.size() == 2) {
        result = motionController.servoOff(tokens[1]);
    }
    else if (tokens[0] == "HOME" && tokens.size() == 2) {
        result = motionController.home(tokens[1]);
    }
    else if (tokens[0] == "MOVE_ABS" && tokens.size() == 3) {
        double pos = 0.0;
        if (!tryParseDouble(tokens[2], pos)) {
            return "ERROR: Invalid numeric parameter";
        }
        result = motionController.moveAbsolute(tokens[1], pos);
    }
    else if (tokens[0] == "MOVE_REL" && tokens.size() == 3) {
        double delta = 0.0;
        if (!tryParseDouble(tokens[2], delta)) {
            return "ERROR: Invalid numeric parameter";
        }
        result = motionController.moveRelative(tokens[1], delta);
    }
    else if (tokens[0] == "AXIS_STOP" && tokens.size() == 2) {
        result = motionController.stop(tokens[1]);
    }
    else if (tokens[0] == "RESET_ALARM" && tokens.size() == 2) {
        result = motionController.resetAlarm(tokens[1]);
    }
    else if (tokens[0] == "AXIS_STATUS" && tokens.size() == 2) {
        return "OK: " + motionController.getAxisStatus(tokens[1]);
    }
    else if (tokens[0] == "MOTION_STATUS" && tokens.size() == 1) {
        return "OK:\n" + motionController.getAllAxisStatus();
    }
    else {
        return "ERROR: Unknown motion command";
    }

    if (result.success) {
        logger.log("MOTION CMD OK: " + rawCommand + " -> " + result.message);
        return result.code + ": " + result.message;
    }

    logger.log("MOTION CMD FAIL: " + rawCommand + " -> " + result.code + " " + result.message);
    logger.logAlarmToCsv(result.code, result.message);
    return result.code + ": " + result.message;
}

// ============================
// 전체 명령 처리
// ============================
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
            // START 전 위치/서보/Home 조건 검사
            std::string reason;
            if (!canStartProcess(reason)) {
                return "ERROR: Cannot START - " + reason;
            }

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

    // 모션 제어 명령 분기
    {
        std::vector<std::string> tokens = splitTokens(rawCommand);
        if (!tokens.empty()) {
            std::string first = toUpperCopy(tokens[0]);

            if (first == "SERVO_ON" ||
                first == "SERVO_OFF" ||
                first == "HOME" ||
                first == "MOVE_ABS" ||
                first == "MOVE_REL" ||
                first == "AXIS_STOP" ||
                first == "RESET_ALARM" ||
                first == "AXIS_STATUS" ||
                first == "MOTION_STATUS") {

                std::lock_guard<std::mutex> lock(mtx);
                logger.log("CMD MOTION " + rawCommand);
                return executeMotionCommand(rawCommand);
            }
        }
    }

    logger.log("WARN Unknown command: " + rawCommand);
    return "ERROR: Unknown command";
}