// [game.h]
#ifndef __GAME_H
#define __GAME_H

#include <stdint.h>

void Game_UpdateSensor(uint16_t value);
uint32_t Game_GetRecentPeak(void);

#endif