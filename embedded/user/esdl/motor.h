#ifndef ESDL_MOTOR_H
#define ESDL_MOTOR_H

#include <stdint.h>

void Motor_Init(void);
void Motor_On(void);
void Motor_Off(void);
void Motor_Run(uint32_t ms);
void Motor_Update(void);

#endif
