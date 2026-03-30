#pragma once

#include <string>
#include <mutex>
#include <vector>

#include "Logger.hpp"
#include "Config.hpp"
#include "MotionController.hpp"


enum class EquipmentState {
    IDLE,
    INITIALIZING,
    READY,
    RUNNING,
    STOPPING,
    ERROR
};

enum class ProcessStep {
    NONE,
    LOAD,
    HEAT,
    PROCESS,
    COOLING,
    UNLOAD
};

enum class OperationMode {
    MANUAL,
    AUTO
};

std::string toString(EquipmentState state);
std::string toString(ProcessStep step);
std::string toString(OperationMode mode);

class EquipmentController {
private:
    EquipmentState state = EquipmentState::IDLE;
    ProcessStep currentStep = ProcessStep::NONE;
    OperationMode mode = OperationMode::MANUAL;

    double temperature = 25.0;
    double pressure = 1.0;
    int motorSpeed = 0;

    std::string activeAlarmCode = "NONE";
    std::string activeAlarmMessage = "No active alarm";
    std::vector<std::string> alarmHistory;

    int stepTick = 0;

    mutable std::mutex mtx;
    Logger& logger;
    const Config& config;

    MotionController motionController;

    void changeState(EquipmentState newState);
    void changeStep(ProcessStep newStep);
    void setAlarm(const std::string& code, const std::string& message);
    void clearAlarm();
    std::string getAlarmHistoryString() const;
    std::string executeMotionCommand(const std::string& rawCommand);

    bool isAxisInPosition(const Axis& axis, double target, double tolerance = 0.5) const;
    bool canStartProcess(std::string& reason) const;


public:
    EquipmentController(Logger& logRef, const Config& cfg);

    void init();
    void start();
    void stop();
    void reset();
    void update();
    void printStatus() const;

    std::string getStatusString() const;
    std::string executeCommand(const std::string& rawCommand);

};