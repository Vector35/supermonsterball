#include "battle.h"

using namespace std;


PitBattle::PitBattle(const vector<shared_ptr<Monster>>& attackers, const vector<shared_ptr<Monster>>& defenders,
	bool training, int32_t x, int32_t y, Database* db)
{
	m_db = db;
	m_attackers = attackers;
	m_defenders = defenders;
	m_curAttacker = attackers[0];
	m_curDefender = defenders[0];
	m_curDefender->ResetHP();
	m_training = training;
	m_pitX = x;
	m_pitY = y;
	m_newDefender = true;

	m_attackerCooldown = 0;
	m_defenderCooldown = 1;

	m_attackerCharge = 0;
	m_defenderCharge = 0;

	m_reputationChange = 0;

	m_action = PIT_ACTION_NOT_CHOSEN;

	m_largestAttackerCP = 0;
	for (auto& i : m_attackers)
	{
		if (i->GetCP() > m_largestAttackerCP)
			m_largestAttackerCP = i->GetCP();
	}
}


void PitBattle::SetAttacker(shared_ptr<Monster> monster)
{
	for (auto& i : m_attackers)
	{
		if (i->GetID() == monster->GetID())
		{
			if (i->GetCurrentHP() == 0)
				return;
			if (i->GetID() != m_curAttacker->GetID())
			{
				if (m_db)
					m_db->UpdateMonster(m_curAttacker->GetOwnerID(), m_curAttacker);
				m_attackerCharge = 0;
				m_defenderCooldown = 1;
			}
			m_curAttacker = i;
			return;
		}
	}
}


PitBattleStatus PitBattle::Step()
{
	PitBattleStatus status;
	status.state = PIT_BATTLE_WAITING_FOR_ACTION;
	status.charge = m_attackerCharge;
	status.attackerHP = m_curAttacker ? m_curAttacker->GetCurrentHP() : 0;
	status.defenderHP = m_curDefender ? m_curDefender->GetCurrentHP() : 0;
	status.opponent = m_curDefender;

	if (!m_curDefender)
	{
		status.state = PIT_BATTLE_WIN;
		return status;
	}

	if (!m_curAttacker)
	{
		if (m_attackers.size() == 0)
			status.state = PIT_BATTLE_LOSE;
		return status;
	}

	if (m_newDefender)
	{
		status.state = PIT_BATTLE_NEW_OPPONENT;
		m_newDefender = false;
		return status;
	}

	if (m_curDefender->GetCurrentHP() == 0)
	{
		if (m_training)
		{
			if (m_largestAttackerCP <= (m_curDefender->GetCP() / 2))
				m_reputationChange += 1000;
			else if (m_largestAttackerCP <= m_curDefender->GetCP())
				m_reputationChange += 500;
			else
				m_reputationChange += 250;
		}

		if (m_db)
			m_db->UpdateMonster(m_curAttacker->GetOwnerID(), m_curAttacker);

		m_defenders.erase(m_defenders.begin());
		if (m_defenders.size() == 0)
		{
			status.state = PIT_BATTLE_DEFEND_FAINT;
			m_curDefender.reset();

			if (!m_training)
				m_reputationChange += 1000;
		}
		else
		{
			status.state = PIT_BATTLE_DEFEND_FAINT;
			m_curDefender = m_defenders[0];
			m_curDefender->ResetHP();
			m_newDefender = true;

			if (!m_training)
				m_reputationChange += 500;
		}
		m_defenderCharge = 0;
		m_defenderCooldown = 1;
		m_action = PIT_ACTION_NOT_CHOSEN;
		return status;
	}

	if (m_curAttacker->GetCurrentHP() == 0)
	{
		if (m_db)
			m_db->UpdateMonster(m_curAttacker->GetOwnerID(), m_curAttacker);

		for (auto i = m_attackers.begin(); i != m_attackers.end(); ++i)
		{
			if ((*i)->GetID() == m_curAttacker->GetID())
			{
				m_attackers.erase(i);
				break;
			}
		}

		status.state = PIT_BATTLE_ATTACK_FAINT;
		if (m_attackers.size() == 0)
			m_curAttacker.reset();
		else
			m_curAttacker = m_attackers[0];
		m_attackerCharge = 0;
		m_attackerCooldown = 0;
		m_defenderCooldown = 1;
		m_action = PIT_ACTION_NOT_CHOSEN;
		status.charge = 0;
		return status;
	}

	if ((m_attackerCooldown <= m_defenderCooldown) && (m_action != PIT_ACTION_DODGE))
	{
		uint32_t attackPower, attackCooldown;
		uint32_t neededCharge;
		Element attackElement;

		if (m_action == PIT_ACTION_NOT_CHOSEN)
			return status;

		switch (m_curAttacker->GetChargeMove()->GetType())
		{
		case TwoChargeMove:
			neededCharge = 50;
			break;
		case ThreeChargeMove:
			neededCharge = 33;
			break;
		case FourChargeMove:
			neededCharge = 25;
			break;
		default:
			neededCharge = 100;
			break;
		}

		if ((m_action == PIT_ACTION_ATTACK_QUICK_MOVE) || (m_attackerCharge < neededCharge))
		{
			attackPower = m_curAttacker->GetQuickMove()->GetPower();
			if (m_curAttacker->GetQuickMove()->GetDamagePerSecond() == 0)
				attackCooldown = 2000;
			else
				attackCooldown = (attackPower * 1000) / m_curAttacker->GetQuickMove()->GetDamagePerSecond();
			attackElement = m_curAttacker->GetQuickMove()->GetElement();
		}
		else
		{
			attackPower = m_curAttacker->GetChargeMove()->GetPower();
			if (m_curAttacker->GetChargeMove()->GetDamagePerSecond() == 0)
				attackCooldown = 2000;
			else
				attackCooldown = (attackPower * 1000) / m_curAttacker->GetChargeMove()->GetDamagePerSecond();
			attackElement = m_curAttacker->GetChargeMove()->GetElement();
		}
		if (attackCooldown < 50)
			attackCooldown = 50;

		attackPower = Move::GetDamageFromAttack(attackPower, m_curAttacker->GetLevel(), m_curAttacker->GetTotalAttack(),
			m_curDefender->GetTotalDefense());

		vector<Element> attackerElements = m_curAttacker->GetSpecies()->GetTypes();
		for (auto i : attackerElements)
		{
			if (i == attackElement)
			{
				// Same type attack bonus
				attackPower = (attackPower * 5) / 4;
				break;
			}
		}

		// Determine effectiveness
		vector<Element> defenderElements = m_curDefender->GetSpecies()->GetTypes();
		int32_t effectiveness = 0;
		for (auto i : defenderElements)
		{
			if (Move::IsSuperEffective(attackElement, i))
				effectiveness++;
			else if (Move::IsNotEffective(attackElement, i))
				effectiveness--;
		}

		// Super effective multiplier is 1.5
		switch (effectiveness)
		{
		case -2:
			attackPower = (attackPower * 4) / 9;
			break;
		case -1:
			attackPower = (attackPower * 2) / 3;
			break;
		case 1:
			attackPower = (attackPower * 3) / 2;
			break;
		case 2:
			attackPower = (attackPower * 9) / 4;
			break;
		default:
			break;
		}

		if ((m_action == PIT_ACTION_ATTACK_QUICK_MOVE) || (m_attackerCharge < neededCharge))
		{
			if (effectiveness < 0)
				status.state = PIT_BATTLE_ATTACK_QUICK_MOVE_NOT_EFFECTIVE;
			else if (effectiveness > 0)
				status.state = PIT_BATTLE_ATTACK_QUICK_MOVE_SUPER_EFFECTIVE;
			else
				status.state = PIT_BATTLE_ATTACK_QUICK_MOVE_EFFECTIVE;
			m_attackerCharge += attackCooldown / 40;
			if (m_attackerCharge > 100)
				m_attackerCharge = 100;
			status.charge = m_attackerCharge;
		}
		else
		{
			if (effectiveness < 0)
				status.state = PIT_BATTLE_ATTACK_CHARGE_MOVE_NOT_EFFECTIVE;
			else if (effectiveness > 0)
				status.state = PIT_BATTLE_ATTACK_CHARGE_MOVE_SUPER_EFFECTIVE;
			else
				status.state = PIT_BATTLE_ATTACK_CHARGE_MOVE_EFFECTIVE;
			m_attackerCharge -= neededCharge;
			status.charge = m_attackerCharge;
		}

		m_curDefender->Damage(attackPower);
		status.defenderHP = m_curDefender->GetCurrentHP();

		m_defenderCooldown -= m_attackerCooldown;
		m_attackerCooldown = attackCooldown;

		m_action = PIT_ACTION_NOT_CHOSEN;
		return status;
	}

	uint32_t attackPower, attackCooldown;
	uint32_t neededCharge;
	Element attackElement;

	switch (m_curDefender->GetChargeMove()->GetType())
	{
	case TwoChargeMove:
		neededCharge = 50;
		break;
	case ThreeChargeMove:
		neededCharge = 33;
		break;
	case FourChargeMove:
		neededCharge = 25;
		break;
	default:
		neededCharge = 100;
		break;
	}

	if (m_defenderCharge < neededCharge)
	{
		attackPower = m_curDefender->GetQuickMove()->GetPower();
		if (m_curDefender->GetQuickMove()->GetDamagePerSecond() == 0)
			attackCooldown = 2000;
		else
			attackCooldown = (attackPower * 1000) / m_curDefender->GetQuickMove()->GetDamagePerSecond();
		attackElement = m_curDefender->GetQuickMove()->GetElement();
	}
	else
	{
		attackPower = m_curDefender->GetChargeMove()->GetPower();
		if (m_curDefender->GetChargeMove()->GetDamagePerSecond() == 0)
			attackCooldown = 2000;
		else
			attackCooldown = (attackPower * 1000) / m_curDefender->GetChargeMove()->GetDamagePerSecond();
		attackElement = m_curDefender->GetChargeMove()->GetElement();
	}
	if (attackCooldown < 50)
		attackCooldown = 50;

	attackPower = Move::GetDamageFromAttack(attackPower, m_curDefender->GetLevel(), m_curDefender->GetTotalAttack(),
		m_curAttacker->GetTotalDefense());

	vector<Element> attackerElements = m_curDefender->GetSpecies()->GetTypes();
	for (auto i : attackerElements)
	{
		if (i == attackElement)
		{
			// Same type attack bonus
			attackPower = (attackPower * 5) / 4;
			break;
		}
	}

	// Determine effectiveness
	vector<Element> defenderElements = m_curAttacker->GetSpecies()->GetTypes();
	int32_t effectiveness = 0;
	for (auto i : defenderElements)
	{
		if (Move::IsSuperEffective(attackElement, i))
			effectiveness++;
		else if (Move::IsNotEffective(attackElement, i))
			effectiveness--;
	}

	// Super effective multiplier is 1.5
	switch (effectiveness)
	{
	case -2:
		attackPower = (attackPower * 4) / 9;
		break;
	case -1:
		attackPower = (attackPower * 2) / 3;
		break;
	case 1:
		attackPower = (attackPower * 3) / 2;
		break;
	case 2:
		attackPower = (attackPower * 9) / 4;
		break;
	default:
		break;
	}

	if ((m_attackerCooldown == 0) && (m_action == PIT_ACTION_DODGE))
	{
		// Dodging reduces damage to 1/4
		attackPower /= 4;
	}

	if (m_defenderCharge < neededCharge)
	{
		if ((m_attackerCooldown == 0) && (m_action == PIT_ACTION_DODGE))
			status.state = PIT_BATTLE_DEFEND_QUICK_MOVE_DODGE;
		else if (effectiveness < 0)
			status.state = PIT_BATTLE_DEFEND_QUICK_MOVE_NOT_EFFECTIVE;
		else if (effectiveness > 0)
			status.state = PIT_BATTLE_DEFEND_QUICK_MOVE_SUPER_EFFECTIVE;
		else
			status.state = PIT_BATTLE_DEFEND_QUICK_MOVE_EFFECTIVE;
		m_defenderCharge += attackCooldown / 40;
		if (m_defenderCharge > 100)
			m_defenderCharge = 100;
	}
	else
	{
		if ((m_attackerCooldown == 0) && (m_action == PIT_ACTION_DODGE))
			status.state = PIT_BATTLE_DEFEND_CHARGE_MOVE_DODGE;
		else if (effectiveness < 0)
			status.state = PIT_BATTLE_DEFEND_CHARGE_MOVE_NOT_EFFECTIVE;
		else if (effectiveness > 0)
			status.state = PIT_BATTLE_DEFEND_CHARGE_MOVE_SUPER_EFFECTIVE;
		else
			status.state = PIT_BATTLE_DEFEND_CHARGE_MOVE_EFFECTIVE;
		m_defenderCharge -= neededCharge;
	}

	if ((m_attackerCooldown == 0) && (m_action == PIT_ACTION_DODGE))
		m_action = PIT_ACTION_NOT_CHOSEN;

	m_curAttacker->Damage(attackPower);
	status.attackerHP = m_curAttacker->GetCurrentHP();

	if (m_defenderCooldown > m_attackerCooldown)
		m_attackerCooldown = 0;
	else
		m_attackerCooldown -= m_defenderCooldown;
	m_defenderCooldown = attackCooldown;
	return status;
}
