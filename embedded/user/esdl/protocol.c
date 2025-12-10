#include "protocol.h"
#include <stdio.h>

volatile uint32_t seq = 0;

char *Protocol_BuildTriggerMessage(uint32_t seq, uint32_t ambient)
{
    static char buffer[64];

    // 메시지 타입, seq, ambient, distance
    snprintf(buffer, sizeof(buffer),
             "TYPE=%u;SEQ=%lu;AMB=%lu",
             TYPE_TRIGGER, seq, ambient);

    return buffer;
}
