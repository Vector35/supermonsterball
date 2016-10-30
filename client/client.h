#pragma once

#include "player.h"

#define TEXT_WAIT_TIME 3000

void GameLoop(Player* player);
void Encounter(Player* player, int32_t x, int32_t y);
void InterruptableWait(uint32_t ms);
