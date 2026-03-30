#include "MotionController.hpp"
#include <sstream>
#include <cmath>

// 생성자: X/Y/Z 축 기본값 초기화
// requiredPosition은 고정 기준 위치
// targetPosition은 초기에는 requiredPosition과 동일하게 시작
MotionController::MotionController() {
    xAxis.name = "X";
    xAxis.minLimit = 0.0;
    xAxis.maxLimit = 500.0;
    xAxis.speed = 5.0;
    xAxis.requiredPosition = 50.0; // 조건 고정 위치
    // xAxis.targetPosition = 50.0;

    yAxis.name = "Y";
    yAxis.minLimit = 0.0;
    yAxis.maxLimit = 500.0;
    yAxis.speed = 5.0;
    yAxis.requiredPosition = 50.0; // 조건 고정 위치
    // yAxis.targetPosition = 50.0;

    zAxis.name = "Z";
    zAxis.minLimit = 0.0;
    zAxis.maxLimit = 300.0;
    zAxis.speed = 3.0;
    zAxis.requiredPosition = 100.0; // 조건 고정 위치
    // zAxis.targetPosition = 100.0; 
}

MotionResult MotionController::makeResult(bool success, const std::string& code, const std::string& message) const {
    return MotionResult{ success, code, message };
}

std::string MotionController::axisStateToString(AxisState state) const {
    switch (state) {
    case AxisState::SERVO_OFF: return "SERVO_OFF";
    case AxisState::READY:     return "READY";
    case AxisState::HOMING:    return "HOMING";
    case AxisState::MOVING:    return "MOVING";
    case AxisState::INPOS:     return "INPOS";
    case AxisState::ALARM:     return "ALARM";
    default:                   return "UNKNOWN";
    }
}

// 축 이름으로 Axis 찾기
Axis* MotionController::findAxis(const std::string& axisName) {
    if (axisName == "X") return &xAxis;
    if (axisName == "Y") return &yAxis;
    if (axisName == "Z") return &zAxis;
    return nullptr;
}

const Axis* MotionController::findAxisConst(const std::string& axisName) const {
    if (axisName == "X") return &xAxis;
    if (axisName == "Y") return &yAxis;
    if (axisName == "Z") return &zAxis;
    return nullptr;
}
// 리미트 검사
bool MotionController::isWithinLimit(const Axis& axis, double pos) const {
    return pos >= axis.minLimit && pos <= axis.maxLimit;
}

// 주기적으로 축 상태를 갱신
void MotionController::update() {
    std::lock_guard<std::mutex> lock(mtx);
}

// 서보 ON
MotionResult MotionController::servoOn(const std::string& axisName) {
    std::lock_guard<std::mutex> lock(mtx);

    Axis* axis = findAxis(axisName);
    if (!axis) {
        return makeResult(false, "E204", "Invalid axis name");
    }

    if (axis->state == AxisState::ALARM) {
        return makeResult(false, "E206", axisName + " axis is in ALARM state");
    }

    axis->servoOn = true;
    axis->state = AxisState::READY;

    return makeResult(true, "OK", axisName + " servo ON");
}

// 서보 OFF
MotionResult MotionController::servoOff(const std::string& axisName) {
    std::lock_guard<std::mutex> lock(mtx);

    Axis* axis = findAxis(axisName);
    if (!axis) {
        return makeResult(false, "E204", "Invalid axis name");
    }

    axis->servoOn = false;
    axis->moving = false;
    axis->state = AxisState::SERVO_OFF;

    return makeResult(true, "OK", axisName + " servo OFF");
}


// Home 동작
// requiredPosition은 고정값이므로 바꾸지 않음
MotionResult MotionController::home(const std::string& axisName) {
    std::lock_guard<std::mutex> lock(mtx);

    Axis* axis = findAxis(axisName);
    if (!axis) {
        return makeResult(false, "E204", "Invalid axis name");
    }

    if (axis->state == AxisState::ALARM) {
        return makeResult(false, "E206", axisName + " axis is in ALARM state");
    }

    if (!axis->servoOn) {
        return makeResult(false, "E201", axisName + " servo is OFF");
    }

    axis->targetPosition = 0.0;
    axis->currentPosition = 0.0;
    axis->moving = true;   // 한 번이라도 이동했으면 YES 유지
    axis->homed = true;
    axis->state = AxisState::INPOS;

    return makeResult(true, "OK", axisName + " homing completed");
}

// 절대 이동
// currentPosition과 targetPosition을 같은 값으로 즉시 반영
// requiredPosition은 유지
MotionResult MotionController::moveAbsolute(const std::string& axisName, double position) {
    std::lock_guard<std::mutex> lock(mtx);

    Axis* axis = findAxis(axisName);
    if (!axis) {
        return makeResult(false, "E204", "Invalid axis name");
    }

    if (axis->state == AxisState::ALARM) {
        return makeResult(false, "E206", axisName + " axis is in ALARM state");
    }

    if (!axis->servoOn) {
        return makeResult(false, "E201", axisName + " servo is OFF");
    }

    if (!axis->homed) {
        return makeResult(false, "E202", axisName + " axis home is required");
    }

    if (!isWithinLimit(*axis, position)) {
        axis->state = AxisState::ALARM;
        return makeResult(false, "E203", axisName + " axis position limit exceeded");
    }

    axis->targetPosition = position;
    axis->currentPosition = position;
    axis->moving = true;
    axis->state = AxisState::INPOS;

    std::ostringstream oss;
    oss << axisName << " move absolute completed -> " << position;
    return makeResult(true, "OK", oss.str());
}

// 상대 이동
// requiredPosition은 유지
MotionResult MotionController::moveRelative(const std::string& axisName, double delta) {
    std::lock_guard<std::mutex> lock(mtx);

    Axis* axis = findAxis(axisName);
    if (!axis) {
        return makeResult(false, "E204", "Invalid axis name");
    }

    if (axis->state == AxisState::ALARM) {
        return makeResult(false, "E206", axisName + " axis is in ALARM state");
    }

    if (!axis->servoOn) {
        return makeResult(false, "E201", axisName + " servo is OFF");
    }

    if (!axis->homed) {
        return makeResult(false, "E202", axisName + " axis home is required");
    }

    double newTarget = axis->currentPosition + delta;

    if (!isWithinLimit(*axis, newTarget)) {
        axis->state = AxisState::ALARM;
        return makeResult(false, "E203", axisName + " axis position limit exceeded");
    }

    axis->targetPosition = newTarget;
    axis->currentPosition = newTarget;
    axis->moving = true;
    axis->state = AxisState::INPOS;

    std::ostringstream oss;
    oss << axisName << " move relative completed -> " << delta;
    return makeResult(true, "OK", oss.str());
}

// 정지
MotionResult MotionController::stop(const std::string& axisName) {
    std::lock_guard<std::mutex> lock(mtx);

    Axis* axis = findAxis(axisName);
    if (!axis) {
        return makeResult(false, "E204", "Invalid axis name");
    }

    if (axis->state == AxisState::ALARM) {
        return makeResult(false, "E206", axisName + " axis is in ALARM state");
    }

    axis->moving = false;
    axis->state = axis->servoOn ? AxisState::READY : AxisState::SERVO_OFF;

    return makeResult(true, "OK", axisName + " axis stopped");
}

// 알람 리셋
MotionResult MotionController::resetAlarm(const std::string& axisName) {
    std::lock_guard<std::mutex> lock(mtx);

    Axis* axis = findAxis(axisName);
    if (!axis) {
        return makeResult(false, "E204", "Invalid axis name");
    }

    if (axis->state != AxisState::ALARM) {
        return makeResult(false, "E208", axisName + " axis is not in ALARM state");
    }

    // 알람만 해제, 다시 사용하려면 servoOn/home 절차 필요
    axis->moving = false;
    axis->servoOn = false;
    axis->homed = false;
    axis->state = AxisState::SERVO_OFF;

    return makeResult(true, "OK", axisName + " alarm reset");
}

// 단일 축 상태 조회
std::string MotionController::getAxisStatus(const std::string& axisName) const {
    std::lock_guard<std::mutex> lock(mtx);

    const Axis* axis = findAxisConst(axisName);
    if (!axis) {
        return "E204: Invalid axis name";
    }

    std::ostringstream oss;
    oss << "[" << axis->name << "] "
        << "State=" << axisStateToString(axis->state)
        << ", Current=" << axis->currentPosition
        << ", Target=" << axis->targetPosition
        << ", Required=" << axis->requiredPosition
        << ", Servo=" << (axis->servoOn ? "ON" : "OFF")
        << ", Homed=" << (axis->homed ? "YES" : "NO")
        << ", Moving=" << (axis->moving ? "YES" : "NO");

    return oss.str();
}

// 전체 축 상태 조회
std::string MotionController::getAllAxisStatus() const {
    std::lock_guard<std::mutex> lock(mtx);

    std::ostringstream oss;
    const Axis* axes[3] = { &xAxis, &yAxis, &zAxis };

    for (int i = 0; i < 3; ++i) {
        const Axis* axis = axes[i];
        oss << "[" << axis->name << "] "
            << "State=" << axisStateToString(axis->state)
            << ", Current=" << axis->currentPosition
            << ", Target=" << axis->targetPosition
            << ", Required=" << axis->requiredPosition
            << ", Servo=" << (axis->servoOn ? "ON" : "OFF")
            << ", Homed=" << (axis->homed ? "YES" : "NO")
            << ", Moving=" << (axis->moving ? "YES" : "NO");

        if (i < 2) {
            oss << "\n";
        }
    }

    return oss.str();
}

// 전체 축 중 하나라도 알람인지 확인
bool MotionController::hasAnyAlarm() const {
    std::lock_guard<std::mutex> lock(mtx);

    return xAxis.state == AxisState::ALARM ||
        yAxis.state == AxisState::ALARM ||
        zAxis.state == AxisState::ALARM;
}

Axis MotionController::getAxisData(const std::string& axisName) const {
    std::lock_guard<std::mutex> lock(mtx);

    const Axis* axis = findAxisConst(axisName);
    if (!axis) {
        return Axis{};
    }

    return *axis;
}