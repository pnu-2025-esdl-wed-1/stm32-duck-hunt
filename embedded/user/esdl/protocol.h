#ifndef ESDL_PROTOCOL_H
#define ESDL_PROTOCOL_H

#include <stdint.h>

typedef enum
{
    TYPE_TRIGGER = 100
} ProtocolType;

extern volatile uint32_t seq;

char *Protocol_BuildTriggerMessage(uint32_t seq, uint32_t ambient, uint32_t peak);

#endif
