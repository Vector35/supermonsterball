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
	int32_t m_x, m_y;
	std::shared_ptr<Monster> m_encounter;
	bool m_seedGiven;

	void EndEncounter(bool caught);

public:
	InMemoryPlayer(const std::string& name);

	virtual std::string GetName() override { return m_name; }
	virtual uint32_t GetLevel() override { return m_level; }
	virtual uint32_t GetTotalExperience() override { return m_xp; }
	virtual uint32_t GetPowder() override { return m_powder; }

	virtual std::vector<std::shared_ptr<Monster>> GetMonsters() override { return m_monsters; }
	virtual uint32_t GetNumberCaptured(MonsterSpecies* species) override;
	virtual uint32_t GetNumberSeen(MonsterSpecies* species) override;
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
};
