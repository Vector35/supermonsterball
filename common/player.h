#pragma once

#include <map>
#include "monster.h"
#include "item.h"

#define MIN_MOVEMENT_INTERVAL 100

enum BallThrowResult
{
	THROW_RESULT_BREAK_OUT_AFTER_ONE = 1,
	THROW_RESULT_BREAK_OUT_AFTER_TWO = 2,
	THROW_RESULT_BREAK_OUT_AFTER_THREE = 3,
	THROW_RESULT_CATCH = 4,
	THROW_RESULT_RUN_AWAY_AFTER_ONE = 5,
	THROW_RESULT_RUN_AWAY_AFTER_TWO = 6,
	THROW_RESULT_RUN_AWAY_AFTER_THREE = 7,
};

class Player
{
public:
	virtual std::string GetName() = 0;
	virtual uint32_t GetLevel() = 0;
	virtual uint32_t GetTotalExperience() = 0;
	uint32_t GetTotalExperienceNeededForCurrentLevel();
	uint32_t GetTotalExperienceNeededForNextLevel();
	virtual uint32_t GetPowder() = 0;

	virtual std::vector<std::shared_ptr<Monster>> GetMonsters() = 0;
	virtual uint32_t GetNumberCaptured(MonsterSpecies* species) = 0;
	virtual uint32_t GetNumberSeen(MonsterSpecies* species) = 0;
	virtual uint32_t GetTreatsForSpecies(MonsterSpecies* species) = 0;

	virtual std::map<ItemType, uint32_t> GetInventory() = 0;
	virtual uint32_t GetItemCount(ItemType type) = 0;
	virtual bool UseItem(ItemType type) = 0;

	virtual int32_t GetLastLocationX() = 0;
	virtual int32_t GetLastLocationY() = 0;
	virtual void ReportLocation(int32_t x, int32_t y) = 0;
	virtual std::vector<MonsterSighting> GetMonstersInRange() = 0;

	virtual std::shared_ptr<Monster> StartWildEncounter(int32_t x, int32_t y) = 0;
	virtual bool GiveSeed() = 0;
	virtual BallThrowResult ThrowBall(ItemType type) = 0;
	virtual void RunFromEncounter() = 0;
};
