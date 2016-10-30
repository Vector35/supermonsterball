#pragma once

#include "player.h"

#define TEXT_WAIT_TIME 3000

void GameLoop(Player* player);
std::shared_ptr<Monster> Encounter(Player* player, int32_t x, int32_t y);
void InterruptableWait(uint32_t ms);
void ShowMonsterDetails(Player* player, std::shared_ptr<Monster> monster);
