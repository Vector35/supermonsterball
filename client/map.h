#pragma once

#include "player.h"
#include "terminal.h"

class MapRenderer
{
	Player* m_player;

	int32_t m_centerX, m_centerY;
	std::vector<MonsterSighting> m_sightings;

public:
	MapRenderer(Player* player);

	void Paint();

	void SetCenterLocation(int32_t x, int32_t y);
	void EnsurePlayerVisible();
	void SetMonsters(const std::vector<MonsterSighting>& sightings);
};
