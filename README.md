# 장비 제어 시뮬레이터 ⚙️
 
## 📊 미리보기


## 🧾 프로젝트 개요
장비 제어 소프트웨어의 구조를 이해하기 위해  
상태 머신과 모션 제어 기반 장비의 동작을 모델링하고,  
공정 시퀀스 및 인터락 로직을 포함한 **장비 제어 시뮬레이터**를 구현하였습니다.

## 📅 개발 기간
2026.03 ~ 2026.03

## ⚙️ 주요 기능

- 3축 모션 제어 (X/Y/Z) 및 명령 기반 위치 제어
- 위치 조건 기반 START 제어 (지정 좌표 만족 시 공정 시작)
- 모션 상태 관리 및 명령 인터락 (Servo/Home 상태 검증)
- 상태 머신 기반 장비 제어
  ㄴ IDLE → INITIALIZING → READY → RUNNING → STOPPING / ERROR → READY / IDLE
- 공정 시퀀스 제어
  ㄴ LOAD → HEAT → PROCESS → COOLING → UNLOAD
- 인터락(안전 조건) 감시
  ㄴ 온도 / 압력 / 모터 속도 초과 시 즉시 ERROR 전환
- AUTO / MANUAL 모드 지원
- TCP 통신 기반 외부 제어
- 알람 코드 및 알람 히스토리 관리
- CSV 파일 기반 알람 로그 저장
- JSON 설정 파일을 통한 파라미터 관리

## 🧩 시스템 구성
- **Motion Controller**
  - 3축 모션 제어 및 위치 상태 관리

- **Equipment Controller**
  - 상태 머신 및 공정 시퀀스 관리
  - 온도, 압력, 모터 상태 제어

- **TCP Server**
  - 외부 클라이언트 명령 수신
  - 장비 제어 명령 처리

- **Logger**
  - 상태 변화 및 이벤트 로그 기록
  - 알람 발생 시 CSV 파일 저장

- **Config (JSON)**
  - 포트, 임계값, 업데이트 주기 설정

## 🛠️ 사용 기술
- C++
- 멀티스레딩 (std::thread, std::mutex)
- TCP Socket 통신
- JSON 설정 파일 기반 파라미터 관리
- 파일 입출력 (CSV 로그 파일)

## 🔄 동작 로직
- 모션 준비 후 위치 조건 만족 시 공정 실행
- START 시 공정 시퀀스 실행
  ㄴ LOAD → HEAT → PROCESS → COOLING → UNLOAD
- 각 단계 수행 중 인터락 검사
  - 온도 > Threshold → ERROR (E001)
  - 압력 > Threshold → ERROR (E002)
  - 속도 > Threshold → ERROR (E003)
- ERROR 발생 시 장비 정지 및 알람 기록
- RESET 시 IDLE 상태로 복귀
- AUTO 모드 시 공정 반복 수행

## 👨‍💻 역할
- 전체 시스템 구조 설계
- 상태 머신 및 시퀀스 로직 구현
- TCP 통신 기능 구현
- 알람 및 로그 시스템 개발
- JSON 설정 파일 구조 설계

## ✨ 특징
- 모션 제어와 상태 머신을 결합한 **장비 제어 구조**
- 공정 수행 중에도 이상 상태를 감지하는 **인터락 로직 적용**
- TCP 기반 **외부 제어 가능한 구조**
- 알람 이력을 CSV로 저장하는 **로그 관리 시스템**
- JSON 기반 **설정 외부화로 확장성 확보**

  
## 🚀 실행 방법

```bash

# 상태 머신 명령어
INIT
→ 장비 초기화 (IDLE → INITIALIZING → READY)
START
→ 위치 조건 만족 시 공정 시작
STOP
→ 공정 중지 (RUNNING → STOPPING → READY)
RESET
→ ERROR 상태 해제 후 초기 상태 복귀
STATUS
→ 현재 장비 상태 출력

# 모션 제어 명령어
SERVO_ON X/Y/Z
→ 해당 축 서보 ON (동작 가능 상태)
SERVO_OFF X/Y/Z
→ 해당 축 서보 OFF (동작 불가)
HOME X/Y/Z
→ 원점 복귀 (좌표 기준 설정)
MOVE_ABS X/Y/Z [값]
→ 지정 좌표로 이동
MOVE_REL X/Y/Z [값]
→ 현재 위치 기준 상대 이동
MOTION_STATUS
→ 전체 축 상태 출력


