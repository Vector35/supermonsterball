#pragma once

#include <map>
#include "monster.h"
#include "item.h"

#define SERVER_PORT 2525

#define MIN_MOVEMENT_INTERVAL 100
#define MAX_STOPS_WITHIN_COOLDOWN 100

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

struct LevelUpItem
{
	ItemType type;
	uint32_t count;
	LevelUpItem(ItemType t, uint32_t c): type(t), count(c) {}
};

struct PowerUpCost
{
	uint32_t treats;
	uint32_t powder;
	PowerUpCost(uint32_t t, uint32_t p): treats(t), powder(p) {}
};

class Player
{
public:
	virtual ~Player() {}

	virtual std::string GetName() = 0;
	virtual uint32_t GetLevel() = 0;
	virtual uint32_t GetTotalExperience() = 0;
	uint32_t GetTotalExperienceNeededForCurrentLevel();
	uint32_t GetTotalExperienceNeededForNextLevel();
	std::vector<LevelUpItem> GetItemsOnLevelUp(uint32_t level);
	virtual uint32_t GetPowder() = 0;

	virtual std::vector<std::shared_ptr<Monster>> GetMonsters() = 0;
	virtual uint32_t GetNumberCaptured(MonsterSpecies* species) = 0;
	virtual uint32_t GetNumberSeen(MonsterSpecies* species) = 0;
	virtual uint32_t GetTreatsForSpecies(MonsterSpecies* species) = 0;

	PowerUpCost GetPowerUpCost(uint32_t level);

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

	virtual bool PowerUpMonster(std::shared_ptr<Monster> monster) = 0;
	virtual bool EvolveMonster(std::shared_ptr<Monster> monster) = 0;
	virtual void TransferMonster(std::shared_ptr<Monster> monster) = 0;
	virtual void SetMonsterName(std::shared_ptr<Monster> monster, const std::string& name) = 0;

	virtual uint8_t GetMapTile(int32_t x, int32_t y) = 0;
	bool IsMapTileTraversable(int32_t x, int32_t y);

	virtual bool IsStopAvailable(int32_t x, int32_t y) = 0;
	virtual std::map<ItemType, uint32_t> GetItemsFromStop(int32_t x, int32_t y) = 0;
};
