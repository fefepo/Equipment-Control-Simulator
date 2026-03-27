#pragma once

#include <string>

class Config {
public:
    int port = 5000; // 서버 포트
    double tempThreshold = 80.0; // 온도 임계값
    double pressureThreshold = 3.5; // 압력 임계값
    double speedThreshold = 1300; // 속도 임계값
    int updateIntervalMs = 100; // 업데이트 주기(ms)

    bool load(const std::string& filename); // 설정 파일 로드 함수
};
