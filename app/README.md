# 블루투스 앱

### `BluetoothManager.kt`
*   **역할**: 블루투스 연결 관리 및 데이터 송수신
*   **핵심 기능**:
    *   `connect()`: HC-06 등 블루투스 모듈과 페어링 및 소켓 연결.
    *   `startListening()`: 수신된 데이터를 버퍼링하여 **줄 단위(`\n` 기준)**로 완전한 메시지를 만듦. (데이터 깨짐 방지)
    *   `sendMessage()`: 앱에서 STM32로 명령어(`CMD,START` 등)를 전송하는 함수. (Suspend 함수로 구현되어 코루틴 필요)

### `MessageParser.kt`
*   **역할**: 수신 데이터 파싱
*   **동작**:
    *   `"STATUS,score=1500,time=45"` 형식의 문자열을 입력받음.
    *   `score` (점수)와 `time` (남은 시간)을 정수형으로 추출하여 `GameStatus` 객체로 반환.

### `MainActivity.kt` (UI Layer)
*   **역할**: 사용자 인터페이스(UI) 구성
*   **특징**:
    *   **Dashboard**: 현재 점수를 아주 크게, 남은 시간을 적당히 크게 표시.
    *   **Visual Warning**: 남은 시간이 **5초 이하**일 때 글씨가 **빨간색**으로 변함.
    *   **Control Buttons**: 게임 시작/재시작 버튼 포함.

### 수신 (App <- STM32)
*   **형식**: `STATUS,score={점수},time={시간}`
*   **예시**: `STATUS,score=100,time=29`
*   **설명**: PC 게임 -> STM32 -> 앱 순서로 전달되며, 앱은 이를 화면에 갱신합니다.

### 송신 (App -> STM32)
버튼을 누르면 앱이 다음 명령어를 전송합니다. PC 게임은 이 신호를 감지해야 합니다.

| 버튼 이름 | 전송되는 명령어 | 동작 설명 |
| :--- | :--- | :--- |
| **START GAME** | `CMD,START\n` | 게임을 시작하라는 신호 |
| **RESET GAME** | `CMD,RESET\n` | 게임을 초기화(리셋)하라는 신호 |


### 실행화면

![App Screenshot](app.png)

