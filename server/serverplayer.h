#pragma once

#include <time.h>
#include "player.h"
#include "database.h"
#include "battle.h"

class ServerPlayer: public Player
{
	uint64_t m_id;
	std::string m_name;
	Team m_team;
	uint32_t m_level, m_xp, m_powder;
	std::map<uint64_t, std::shared_ptr<Monster>> m_monsters;
	std::map<ItemType, uint32_t> m_inventory;
	std::map<uint32_t, uint32_t> m_seen, m_captured;
	std::map<uint32_t, uint32_t> m_treats;
	std::map<uint32_t, std::vector<std::shared_ptr<Monster>>> m_recentEncounters;
	std::vector<RecentStopVisit> m_recentStopsVisited;
	int32_t m_x, m_y;
	std::shared_ptr<Monster> m_encounter;
	bool m_seedGiven;
	std::shared_ptr<PitBattle> m_battle;
	bool m_flaggedForBan, m_banned;
	std::string m_banReason;
	time_t m_lastSavedLocation;
	bool m_hasValidChallengeResponse;

	void EndEncounter(bool caught, ItemType ball = ITEM_STANDARD_BALL);
	void EarnExperience(uint32_t xp);

public:
	ServerPlayer(const std::string& name, uint64_t id);
	ServerPlayer(const std::string& name, const DatabaseLoginResult& login);
	virtual ~ServerPlayer() {}

	virtual uint64_t GetID() override { return m_id; }
	virtual std::string GetName() override { return m_name; }
	virtual Team GetTeam() override { return m_team; }
	virtual uint32_t GetLevel() override { return m_level; }
	virtual uint32_t GetTotalExperience() override { return m_xp; }
	virtual uint32_t GetPowder() override { return m_powder; }

	virtual std::vector<std::shared_ptr<Monster>> GetMonsters() override;
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

	virtual void SetTeam(Team team) override;

	virtual void ForcePitRefresh() override {}
	virtual Team GetPitTeam(int32_t x, int32_t y) override;
	virtual uint32_t GetPitReputation(int32_t x, int32_t y) override;
	virtual std::vector<std::shared_ptr<Monster>> GetPitDefenders(int32_t x, int32_t y) override;
	virtual bool AssignPitDefender(int32_t x, int32_t y, std::shared_ptr<Monster> monster) override;
	virtual bool StartPitBattle(int32_t x, int32_t y, std::vector<std::shared_ptr<Monster>> monsters) override;
	virtual std::vector<std::shared_ptr<Monster>> GetPitBattleDefenders() override;
	virtual void SetAttacker(std::shared_ptr<Monster> monster) override;
	virtual PitBattleStatus StepPitBattle() override;
	virtual void SetPitBattleAction(PitBattleAction action) override;
	virtual uint32_t EndPitBattle() override;

	virtual void HealMonster(std::shared_ptr<Monster> monster, ItemType type) override;

	virtual void TravelToPitOfDoom() override;

	virtual std::string GetLevel40Flag() override;
	virtual std::string GetCatchEmAllFlag() override;

	void FlagForBan(const std::string& reason);
	void BanWave();
	bool IsBanned() const { return m_banned; }
	void MarkValidChallengeResponse() { m_hasValidChallengeResponse = true; }
};
