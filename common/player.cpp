#include "player.h"


static uint32_t g_levelExperience[39] = {
	1000, 2000, 3000, 4000, 5000,
	6000, 7000, 8000, 10000, 12000,
	14000, 16000, 18000, 20000, 23000,
	26000, 30000, 35000, 40000, 45000,
	50000, 60000, 70000, 80000, 115000,
	150000, 200000, 250000, 300000, 350000,
	400000, 500000, 650000, 1000000, 1500000,
	2000000, 3000000, 4000000, 5000000
};


uint32_t Player::GetTotalExperienceNeededForCurrentLevel()
{
	uint32_t level = GetLevel();
	if (level < 1)
		level = 1;
	if (level >= 40)
		level = 40;
	uint32_t total = 0;
	for (uint32_t i = 0; i < (level - 1); i++)
		total += g_levelExperience[i];
	return total;
}


uint32_t Player::GetTotalExperienceNeededForNextLevel()
{
	uint32_t level = GetLevel();
	if (level < 1)
		level = 1;
	if (level >= 40)
		return GetTotalExperienceNeededForCurrentLevel();
	uint32_t total = 0;
	for (uint32_t i = 0; i < level; i++)
		total += g_levelExperience[i];
	return total;
}
