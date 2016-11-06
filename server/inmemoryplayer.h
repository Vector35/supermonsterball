#pragma once

#include "player.h"

class InMemoryPlayer: public Player
{
	std::string m_name;
	uint32_t m_level, m_xp, m_powder;
	std::vector<std::shared_ptr<Monster>> m_monsters;
	std::map<ItemType, uint32_t> m_inventory;
	std::map<uint32_t, uint32_t> m_seen, m_captured;
	std::map<uint32_t, uint32_t> m_treats;
	std::map<uint32_t, std::vector<std::shared_ptr<Monster>>> m_recentEncounters;
	std::vector<RecentStopVisit> m_recentStopsVisited;
	int32_t m_x, m_y;
	std::shared_ptr<Monster> m_encounter;
	bool m_seedGiven;
	uint64_t m_nextMonsterID;

	void EndEncounter(bool caught, ItemType ball = ITEM_STANDARD_BALL);
	void EarnExperience(uint32_t xp);

public:
	InMemoryPlayer(const std::string& name);

	virtual std::string GetName() override { return m_name; }
	virtual uint32_t GetLevel() override { return m_level; }
	virtual uint32_t GetTotalExperience() override { return m_xp; }
	virtual uint32_t GetPowder() override { return m_powder; }

	virtual std::vector<std::shared_ptr<Monster>> GetMonsters() override { return m_monsters; }
	virtual std::shared_ptr<Monster> GetMonsterByID(uint64_t id) override;
	virtual const std::map<uint32_t, uint32_t>& GetNumberCaptured() override { return m_captured; }
	virtual uint32_t GetNumberCaptured(MonsterSpecies* species) override;
	virtual const std::map<uint32_t, uint32_t>& GetNumberSeen() override { return m_seen; }
	virtual uint32_t GetNumberSeen(MonsterSpecies* species) override;
	virtual const std::map<uint32_t, uint32_t>& GetTreats() override { return m_treats; }
	virtual uint32_t GetTreatsForSpecies(MonsterSpecies* species) override;

	virtual std::map<ItemType, uint32_t> GetInventory() override { return m_inventory; }
	virtual uint32_t GetItemCount(ItemType type) override;
	virtual bool UseItem(ItemType type) override;

	virtual int32_t GetLastLocationX() override { return m_x; }
	virtual int32_t GetLastLocationY() override { return m_y; }
	virtual void ReportLocation(int32_t x, int32_t y) override;
	virtual std::vector<MonsterSighting> GetMonstersInRange() override;

	virtual std::shared_ptr<Monster> StartWildEncounter(int32_t x, int32_t y) override;
	virtual bool GiveSeed() override;
	virtual BallThrowResult ThrowBall(ItemType type) override;
	virtual void RunFromEncounter() override;

	virtual bool PowerUpMonster(std::shared_ptr<Monster> monster) override;
	virtual bool EvolveMonster(std::shared_ptr<Monster> monster) override;
	virtual void TransferMonster(std::shared_ptr<Monster> monster) override;
	virtual void SetMonsterName(std::shared_ptr<Monster> monster, const std::string& name) override;

	virtual uint8_t GetMapTile(int32_t x, int32_t y) override;

	virtual std::vector<RecentStopVisit> GetRecentStops() override { return m_recentStopsVisited; }
	virtual bool IsStopAvailable(int32_t x, int32_t y) override;
	virtual std::map<ItemType, uint32_t> GetItemsFromStop(int32_t x, int32_t y) override;
};
