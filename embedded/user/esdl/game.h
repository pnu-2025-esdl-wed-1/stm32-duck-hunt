#ifndef __GAME_H
#define __GAME_H

#include <stdint.h>

void Game_Init(void);
void Game_Loop(void);          // main.c에서 사용
uint32_t Game_GetRecentPeak(void); // trigger.c에서 사용

#endif