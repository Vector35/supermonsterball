#pragma once

#include "player.h"
#include "monster.h"
#include "database.h"

class PitBattle
{
	Database* m_db;
	std::vector<std::shared_ptr<Monster>> m_attackers;
	std::vector<std::shared_ptr<Monster>> m_defenders;
	std::shared_ptr<Monster> m_curAttacker;
	std::shared_ptr<Monster> m_curDefender;
	bool m_training;
	int32_t m_pitX, m_pitY;
	uint32_t m_reputationChange;
	PitBattleAction m_action;
	uint32_t m_attackerCooldown, m_defenderCooldown;
	uint32_t m_attackerCharge, m_defenderCharge;
	uint32_t m_largestAttackerCP;
	bool m_newDefender;

public:
	PitBattle(const std::vector<std::shared_ptr<Monster>>& attackers,
		const std::vector<std::shared_ptr<Monster>>& defenders, bool training, int32_t x, int32_t y,
		Database* db);

	bool IsTraining() const { return m_training; }
	int32_t GetPitX() const { return m_pitX; }
	int32_t GetPitY() const { return m_pitY; }
	uint32_t GetReputationChange() const { return m_reputationChange; }

	std::vector<std::shared_ptr<Monster>> GetDefenders() { return m_defenders; }

	void SetAttacker(std::shared_ptr<Monster> monster);
	void SetAction(PitBattleAction action) { m_action = action; }

	PitBattleStatus Step();
};
