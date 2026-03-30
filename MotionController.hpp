#pragma once
#include <string>
#include <mutex>

// 각 축의 상태 정의
enum class AxisState {
    SERVO_OFF,   // 서보 OFF
    READY,       // 동작 가능
    HOMING,      // Home 동작 중
    MOVING,      // 이동 중
    INPOS,       // 목표 위치 도착
    ALARM        // 알람 상태
};

// 명령 실행 결과 구조체
struct MotionResult {
    bool success;            // 성공 여부
    std::string code;        // 에러/성공 코드
    std::string message;     // 설명 메시지
};

// 단일 축 정보
struct Axis {
    std::string name;            // 축 이름 (X, Y, Z)

    double currentPosition;      // 현재 위치
    double targetPosition;       // 마지막 이동 명령의 목표 위치
    double requiredPosition = 0.0;   // 시작 조건용 고정 목표 위치

    double speed;                // 1회 update당 이동량
    double minLimit;             // 최소 허용 위치
    double maxLimit;             // 최대 허용 위치

    bool servoOn;                // 서보 ON 여부
    bool homed;                  // Home 완료 여부
    bool moving;                 // 이동 중 여부

    AxisState state;             // 축 상태

    Axis()
        : currentPosition(0.0),
        targetPosition(0.0),
        speed(5.0),
        minLimit(0.0),
        maxLimit(1000.0),
        servoOn(false),
        homed(false),
        moving(false),
        state(AxisState::SERVO_OFF) {
    }
};

class MotionController {
private:
    Axis xAxis;
    Axis yAxis;
    Axis zAxis;

    mutable std::mutex mtx;

private:
    // 축 이름으로 해당 Axis 반환
    Axis* findAxis(const std::string& axisName);
    const Axis* findAxisConst(const std::string& axisName) const;

    // 위치가 리미트 안에 있는지 검사
    bool isWithinLimit(const Axis& axis, double pos) const;

    // 단일 축 업데이트
    void updateAxis(Axis& axis);

    // 공통 결과 생성 함수
    MotionResult makeResult(bool success, const std::string& code, const std::string& message) const;

    // 상태 문자열 변환
    std::string axisStateToString(AxisState state) const;

public:
    MotionController();

    // 주기적으로 호출하여 축 위치를 갱신
    void update();

    // 기본 모션 명령
    MotionResult servoOn(const std::string& axisName);
    MotionResult servoOff(const std::string& axisName);
    MotionResult home(const std::string& axisName);
    MotionResult moveAbsolute(const std::string& axisName, double position);
    MotionResult moveRelative(const std::string& axisName, double delta);
    MotionResult stop(const std::string& axisName);
    MotionResult resetAlarm(const std::string& axisName);

    // 상태 조회
    std::string getAxisStatus(const std::string& axisName) const;
    std::string getAllAxisStatus() const;
    bool hasAnyAlarm() const;


    Axis getAxisData(const std::string& axisName) const;
};