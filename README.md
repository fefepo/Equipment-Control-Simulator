# 장비 제어 시뮬레이터 ⚙️
 
## 📊 미리보기

<img width="1654" height="366" alt="tcp 통신" src="https://github.com/user-attachments/assets/1eb232ad-6618-414c-9311-1f572e361acb" />

### TCP 기반(Socket 통신) 클라이언트-서버 통신 테스트 // PowerShell을 TCP 클라이언트로 활용하여, 장비 서버와의 명령 송수신을 테스트하고 통신 동작을 검증


<p align="center">
<img width="45%" height="300" alt="그림2" src="https://github.com/user-attachments/assets/9f6dd651-2b73-42e3-83e4-ad888a05ec3c" />
<img width="45%" height="300" alt="image" src="https://github.com/user-attachments/assets/3dbc85d5-1ebe-4160-b1a8-7b2dbe4e1be5" />
</p>

### 좌표 위치 설정 / 모션 제어 축 위치 검사 로직(START 전에 X/Y/Z 축이 요구 위치에 있는지 확인하는 시작 조건 검사)



<img width="45%" height="405" alt="image" src="https://github.com/user-attachments/assets/0e6dcca9-fafc-43aa-aaf5-0003b4125d5f" />
<img width="386" height="213" alt="image" src="https://github.com/user-attachments/assets/7d9d9d3f-32ce-494f-9036-8272a6f27228" />

### 온도·압력·모터 속도 기반 설비 보호 인터락 / json 파일을 이용한 외부에서 설정 파일 관리(확성정 효과)



## 🧾 프로젝트 개요
장비 제어 소프트웨어의 구조를 이해하기 위해  
상태 머신 기반으로 장비의 동작을 모델링하고,  
공정 시퀀스 및 인터락 로직을 포함한 **장비 제어 시뮬레이터**를 구현하였습니다.

## 📅 개발 기간
2026.03 ~ 2026.03

## ⚙️ 주요 기능
- 상태 머신 기반 장비 제어
  - IDLE → INITIALIZING → READY → RUNNING → STOPPING / ERROR → READY / IDLE
- 공정 시퀀스 제어
  - LOAD → HEAT → PROCESS → COOLING → UNLOAD
- 인터락(안전 조건) 감시
  - 온도 / 압력 / 모터 속도 초과 시 즉시 ERROR 전환
- AUTO / MANUAL 모드 지원
- TCP 통신 기반 외부 제어
- 알람 코드 및 알람 히스토리 관리
- CSV 파일 기반 알람 로그 저장
- JSON 설정 파일을 통한 파라미터 관리

## 🧩 시스템 구성
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
- START 시 공정 시퀀스 실행
  - LOAD → HEAT → PROCESS → COOLING → UNLOAD
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
- 상태 머신 + 시퀀스 제어를 결합한 **장비 제어 구조 구현**
- 공정 수행 중에도 조건을 감시하는 **인터락 로직 적용**
- TCP 기반 **외부 제어 가능한 구조**
- 알람 이력을 CSV로 저장하는 **로그 관리 시스템**
- JSON 기반 **설정 외부화로 확장성 확보**
