#include "game.h"
#include "stm32f10x.h"
#include "esdl/uart.h"
#include "esdl/sensor.h"
#include "esdl/time.h"
#include "esdl/protocol.h"
#include <stdio.h>
#include <string.h>

// === 설정값 ===
#define FLASH_DURATION_MS   120   // PC 화면 플래시 지속 시간
#define HIT_THRESHOLD       200   // 이 값보다 ADC가 낮으면 명중으로 판정

// === 상태 머신 ===
typedef enum {
    STATE_IDLE,
    STATE_WAIT_FLASH
} GameState;

static GameState currentState = STATE_IDLE;
static uint32_t triggerTime = 0;
extern volatile uint32_t seq; // protocol.c (또는 외부) 변수

// UART 수신 버퍼
#define RX_BUF_SIZE 64
static char rxBuffer[RX_BUF_SIZE];
static uint8_t rxIndex = 0;

// 초기화 여부 플래그
static int isGameInitialized = 0;

// 내부 함수: 명중 여부 판별
static int IsHit(uint16_t peakValue) {
    return (peakValue < HIT_THRESHOLD) ? 1 : 0;
}

// 내부 함수: 수신된 문자열 처리
static void ProcessReceivedData(void) {
    if (strstr(rxBuffer, "ACK") != NULL) {
        // ACK 수신 시 타이머 시작
        triggerTime = millis();
        currentState = STATE_WAIT_FLASH;

        // 버퍼 초기화
        memset(rxBuffer, 0, RX_BUF_SIZE);
        rxIndex = 0;
    }
}

void Game_Loop(void) {
    // [중요] 1. 초기화: uart.c가 켜놓은 인터럽트를 강제로 끈다.
    // 인터럽트가 켜져 있으면 uart.c의 핸들러가 데이터를 가져가버려서
    // 여기서 아무리 기다려도 데이터를 받을 수 없음.
    if (!isGameInitialized) {
        USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
        USART_ITConfig(USART2, USART_IT_RXNE, DISABLE); // 안전하게 둘 다 끔
        isGameInitialized = 1;
    }

    // ============================================================
    // 2-1. PC(USART1) 데이터 처리 -> 앱으로 전달 & 게임 로직
    // ============================================================
    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET) {
        // 데이터 읽기
        uint8_t ch = (uint8_t)USART_ReceiveData(USART1);

        // [기능 복구] 인터럽트 핸들러가 하던 일(Bridge & Echo)을 여기서 대신 수행
        // 1) Bluetooth(USART2)로 포워딩 (앱 화면 갱신용)
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, ch);

        // 2) PC(USART1)로 에코 (필요 시)
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, ch);

        // 3) 게임 로직을 위한 버퍼링
        if (rxIndex < RX_BUF_SIZE - 1) {
            rxBuffer[rxIndex++] = (char)ch;
            rxBuffer[rxIndex] = '\0';
            ProcessReceivedData();
        }
        else {
            // 버퍼 오버플로우 방지
            rxIndex = 0;
            rxBuffer[0] = '\0';
        }
    }

    // ============================================================
    // 2-2. 앱(USART2) 데이터 처리 -> PC로 전달 (START 버튼용)
    // ============================================================
    if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET) {
        // 1. 블루투스(App)에서 날아온 데이터 읽기
        uint8_t ch2 = (uint8_t)USART_ReceiveData(USART2);

        // 2. PC(Unity)로 그대로 토스해주기
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, ch2);
    }

    // ============================================================
    // 3. 게임 상태 머신 처리 (플래시 감지)
    // ============================================================
    switch (currentState) {
    case STATE_IDLE:
        break;

    case STATE_WAIT_FLASH:
        // ACK 수신 후 120ms 대기
        if (millis() - triggerTime >= FLASH_DURATION_MS) {

            // 120ms 전부터 현재까지(약 240샘플) 중 최솟값(가장 밝은 값) 탐색
            uint16_t len = 240;
            uint16_t currentIdx = ADC_GetWriteIndex();
            uint16_t startIdx = (currentIdx + ADC_BUF_LEN - len) % ADC_BUF_LEN;

            uint16_t peak = ReadPeak(startIdx, len);
            int hit = IsHit(peak);

            char resultMsg[64];
            snprintf(resultMsg, sizeof(resultMsg),
                "RESULT,seq=%lu,peak=%u,hit=%d\r\n",
                seq, peak, hit);

            USART1_SendString(resultMsg);

            // 상태 초기화
            currentState = STATE_IDLE;
        }
        break;
    }
}