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
#define HIT_THRESHOLD       200   // 이 값보다 ADC가 낮으면 명중으로 판정 >>> 나중에 튜닝하기!!!!

// === 상태 머신 ===
typedef enum {
    STATE_IDLE,
    STATE_WAIT_FLASH
} GameState;

static GameState currentState = STATE_IDLE;
static uint32_t triggerTime = 0;
extern volatile uint32_t seq; // protocol.c에 있는 변수 사용

// UART 수신 버퍼 (Polling 방식용)
#define RX_BUF_SIZE 64
static char rxBuffer[RX_BUF_SIZE];
static uint8_t rxIndex = 0;

// 내부 함수: 명중 여부 판별
static int IsHit(uint16_t peakValue) {
    // ADC 값은 밝을수록 낮아짐 -> 임계값보다 낮으면 명중
    return (peakValue < HIT_THRESHOLD) ? 1 : 0;
}

// 내부 함수: UART 한 글자 읽기 (Non-blocking)
static int UART_ReadChar(uint8_t* c) {
    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET) {
        *c = USART_ReceiveData(USART1);
        return 1;
    }
    return 0;
}

// 내부 함수: 수신된 문자열 처리
static void ProcessReceivedData(void) {
    // "ACK" 문자열이 포함되어 있는지 확인
    if (strstr(rxBuffer, "ACK") != NULL) {
        // ACK를 받으면 즉시 타이머 시작
        triggerTime = millis();
        currentState = STATE_WAIT_FLASH;

        // 버퍼 초기화
        memset(rxBuffer, 0, RX_BUF_SIZE);
        rxIndex = 0;
    }
}

void Game_Loop(void) {
    // 1. UART 데이터 수신 처리 (Polling)
    uint8_t ch;
    if (UART_ReadChar(&ch)) {
        if (rxIndex < RX_BUF_SIZE - 1) {
            rxBuffer[rxIndex++] = ch;
            rxBuffer[rxIndex] = '\0';

            // 개행문자나 ACK 패턴 확인
            ProcessReceivedData();
        }
        else {
            // 버퍼 오버플로우 방지 (초기화)
            rxIndex = 0;
            rxBuffer[0] = '\0';
        }
    }

    // 2. 상태 머신 처리
    switch (currentState) {
    case STATE_IDLE:
        break;

    case STATE_WAIT_FLASH:
        // ACK 수신 후 120ms가 지났는지 확인
        if (millis() - triggerTime >= FLASH_DURATION_MS) {

            // 120ms 동안의 가장 밝았던 값(최소값) 계산
            // 현재 write index를 기준으로 120ms 전 데이터부터 탐색
            // 샘플링 속도(2kHz) -> 1ms당 2개 -> 120ms = 240개 샘플
            uint16_t len = 240;
            uint16_t currentIdx = ADC_GetWriteIndex();

            // 원형 버퍼 역계산 (현재 위치에서 len만큼 뒤로 이동)
            uint16_t startIdx = (currentIdx + ADC_BUF_LEN - len) % ADC_BUF_LEN;

            // 과거 시점부터 240개 데이터를 훑어서 제일 밝은 값(최소값) 가져오기
            uint16_t peak = ReadPeak(startIdx, len);

            // 명중 판별
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