#pragma once

#include <map>
#include <memory>
#include "monster.h"
#include "item.h"

#define SERVER_PORT 2525

#define MIN_MOVEMENT_INTERVAL 100

#define DEFAULT_PIT_REPUTATION 5000
#define MAX_PIT_REPUTATION 52000

enum Team
{
	TEAM_UNASSIGNED = 0,
	TEAM_RED = 1,
	TEAM_BLUE = 2,
	TEAM_YELLOW = 3
};

enum PitBattleState
{
	PIT_BATTLE_WAITING_FOR_ACTION = 0,
	PIT_BATTLE_ATTACK_QUICK_MOVE_NOT_EFFECTIVE = 1,
	PIT_BATTLE_ATTACK_QUICK_MOVE_EFFECTIVE = 2,
	PIT_BATTLE_ATTACK_QUICK_MOVE_SUPER_EFFECTIVE = 3,
	PIT_BATTLE_ATTACK_CHARGE_MOVE_NOT_EFFECTIVE = 4,
	PIT_BATTLE_ATTACK_CHARGE_MOVE_EFFECTIVE = 5,
	PIT_BATTLE_ATTACK_CHARGE_MOVE_SUPER_EFFECTIVE = 6,
	PIT_BATTLE_DEFEND_QUICK_MOVE_NOT_EFFECTIVE = 7,
	PIT_BATTLE_DEFEND_QUICK_MOVE_EFFECTIVE = 8,
	PIT_BATTLE_DEFEND_QUICK_MOVE_SUPER_EFFECTIVE = 9,
	PIT_BATTLE_DEFEND_QUICK_MOVE_DODGE = 10,
	PIT_BATTLE_DEFEND_CHARGE_MOVE_NOT_EFFECTIVE = 11,
	PIT_BATTLE_DEFEND_CHARGE_MOVE_EFFECTIVE = 12,
	PIT_BATTLE_DEFEND_CHARGE_MOVE_SUPER_EFFECTIVE = 13,
	PIT_BATTLE_DEFEND_CHARGE_MOVE_DODGE = 14,
	PIT_BATTLE_ATTACK_FAINT = 15,
	PIT_BATTLE_DEFEND_FAINT = 16,
	PIT_BATTLE_NEW_OPPONENT = 17,
	PIT_BATTLE_WIN = 18,
	PIT_BATTLE_LOSE = 19
};

enum PitBattleAction
{
	PIT_ACTION_NOT_CHOSEN = 0,
	PIT_ACTION_ATTACK_QUICK_MOVE = 1,
	PIT_ACTION_ATTACK_CHARGE_MOVE = 2,
	PIT_ACTION_DODGE = 3
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

struct RecentStopVisit
{
	int32_t x, y;
	time_t visitTime;
};

struct PitBattleStatus
{
	PitBattleState state;
	uint32_t charge;
	uint32_t attackerHP, defenderHP;
	std::shared_ptr<Monster> opponent;
};

class Player
{
public:
	virtual ~Player() {}

	virtual uint64_t GetID() = 0;
	virtual std::string GetName() = 0;
	virtual Team GetTeam() = 0;
	virtual uint32_t GetLevel() = 0;
	virtual uint32_t GetTotalExperience() = 0;
	uint32_t GetTotalExperienceNeededForCurrentLevel();
	uint32_t GetTotalExperienceNeededForNextLevel();
	std::vector<LevelUpItem> GetItemsOnLevelUp(uint32_t level);
	virtual uint32_t GetPowder() = 0;

	virtual std::vector<std::shared_ptr<Monster>> GetMonsters() = 0;
	virtual std::shared_ptr<Monster> GetMonsterByID(uint64_t id) = 0;
	virtual const std::map<uint32_t, uint32_t>& GetNumberCaptured() = 0;
	virtual uint32_t GetNumberCaptured(MonsterSpecies* species) = 0;
	virtual const std::map<uint32_t, uint32_t>& GetNumberSeen() = 0;
	virtual uint32_t GetNumberSeen(MonsterSpecies* species) = 0;
	virtual const std::map<uint32_t, uint32_t>& GetTreats() = 0;
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

	virtual std::vector<RecentStopVisit> GetRecentStops() = 0;
	virtual bool IsStopAvailable(int32_t x, int32_t y) = 0;
	virtual std::map<ItemType, uint32_t> GetItemsFromStop(int32_t x, int32_t y) = 0;

	virtual void SetTeam(Team team) = 0;

	virtual void ForcePitRefresh() = 0;
	virtual Team GetPitTeam(int32_t x, int32_t y) = 0;
	virtual uint32_t GetPitReputation(int32_t x, int32_t y) = 0;
	virtual std::vector<std::shared_ptr<Monster>> GetPitDefenders(int32_t x, int32_t y) = 0;
	virtual bool AssignPitDefender(int32_t x, int32_t y, std::shared_ptr<Monster> monster) = 0;
	virtual bool StartPitBattle(int32_t x, int32_t y, std::vector<std::shared_ptr<Monster>> monsters) = 0;
	virtual std::vector<std::shared_ptr<Monster>> GetPitBattleDefenders() = 0;
	virtual void SetAttacker(std::shared_ptr<Monster> monster) = 0;
	virtual PitBattleStatus StepPitBattle() = 0;
	virtual void SetPitBattleAction(PitBattleAction action) = 0;
	virtual uint32_t EndPitBattle() = 0;

	static uint32_t GetReputationRequirementForLevel(uint32_t level);
	static uint32_t GetPitLevelByReputation(uint32_t reputation);

	virtual void HealMonster(std::shared_ptr<Monster> monster, ItemType type) = 0;

	virtual void TravelToPitOfDoom() = 0;

	virtual std::string GetLevel40Flag() = 0;
	virtual std::string GetCatchEmAllFlag() = 0;
};
