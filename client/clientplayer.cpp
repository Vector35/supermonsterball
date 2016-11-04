#include <cstdlib>
#include <set>
#include "clientplayer.h"
#include "world.h"

using namespace std;


ClientPlayer::ClientPlayer(const string& name): m_name(name)
{
	m_level = 1;
	m_xp = 0;
	m_powder = 0;
	m_x = 0;
	m_y = 0;
	m_nextMonsterID = 1;

	m_inventory[ITEM_STANDARD_BALL] = 20;
	m_inventory[ITEM_MEGA_SEED] = 5;
}


uint32_t ClientPlayer::GetNumberCaptured(MonsterSpecies* species)
{
	auto i = m_captured.find(species->GetIndex());
	if (i == m_captured.end())
		return 0;
	return i->second;
}


uint32_t ClientPlayer::GetNumberSeen(MonsterSpecies* species)
{
	auto i = m_seen.find(species->GetIndex());
	if (i == m_seen.end())
		return 0;
	return i->second;
}


uint32_t ClientPlayer::GetTreatsForSpecies(MonsterSpecies* species)
{
	auto i = m_treats.find(species->GetBaseForm()->GetIndex());
	if (i == m_treats.end())
		return 0;
	return i->second;
}


uint32_t ClientPlayer::GetItemCount(ItemType type)
{
	auto i = m_inventory.find(type);
	if (i == m_inventory.end())
		return 0;
	return i->second;
}


bool ClientPlayer::UseItem(ItemType type)
{
	auto i = m_inventory.find(type);
	if (i == m_inventory.end())
		return false;
	if (i->second == 0)
		return false;
	i->second--;
	return true;
}


void ClientPlayer::ReportLocation(int32_t x, int32_t y)
{
	m_x = x;
	m_y = y;
}


vector<MonsterSighting> ClientPlayer::GetMonstersInRange()
{
	return vector<MonsterSighting>();
}


shared_ptr<Monster> ClientPlayer::StartWildEncounter(int32_t x, int32_t y)
{
	return nullptr;
}


bool ClientPlayer::GiveSeed()
{
	if (!m_encounter)
		return false;
	if (m_seedGiven)
		return false;
	if (!UseItem(ITEM_MEGA_SEED))
		return false;
	m_seedGiven = true;
	return true;
}


void ClientPlayer::EndEncounter(bool caught, ItemType ball)
{
	if (!m_encounter)
		return;

	m_encounter->SetCapture(caught, ball);
	m_recentEncounters[m_encounter->GetSpawnTime()].push_back(m_encounter);

	// Clear out old encounters
	set<uint32_t> toDelete;
	for (auto& i : m_recentEncounters)
	{
		if (i.first < (m_encounter->GetSpawnTime() - 2))
			toDelete.insert(i.first);
	}
	for (auto i : toDelete)
	{
		m_recentEncounters.erase(i);
	}

	m_encounter.reset();
}


void ClientPlayer::EarnExperience(uint32_t xp)
{
	m_xp += xp;
	while ((m_level < 40) && (m_xp >= GetTotalExperienceNeededForNextLevel()))
	{
		for (auto& i : GetItemsOnLevelUp(m_level))
			m_inventory[i.type] += i.count;
		m_level++;
	}
}


BallThrowResult ClientPlayer::ThrowBall(ItemType type)
{
	if (!m_encounter)
		return THROW_RESULT_RUN_AWAY_AFTER_ONE;
	if ((type != ITEM_STANDARD_BALL) && (type != ITEM_SUPER_BALL) && (type != ITEM_UBER_BALL))
	{
		EndEncounter(false);
		return THROW_RESULT_RUN_AWAY_AFTER_ONE;
	}

	if (!UseItem(type))
	{
		EndEncounter(false);
		return THROW_RESULT_RUN_AWAY_AFTER_ONE;
	}

	switch (rand() % 15)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		if (GetNumberCaptured(m_encounter->GetSpecies()) == 0)
			EarnExperience(600);
		else
			EarnExperience(100);
		m_seen[m_encounter->GetSpecies()->GetIndex()]++;
		m_captured[m_encounter->GetSpecies()->GetIndex()]++;
		m_treats[m_encounter->GetSpecies()->GetBaseForm()->GetIndex()] += 3;
		m_powder += 100;
		m_monsters.push_back(m_encounter);
		EndEncounter(true, type);
		return THROW_RESULT_CATCH;
	case 6:
	case 7:
		return THROW_RESULT_BREAK_OUT_AFTER_ONE;
	case 8:
	case 9:
		return THROW_RESULT_BREAK_OUT_AFTER_TWO;
	case 10:
	case 11:
		return THROW_RESULT_BREAK_OUT_AFTER_THREE;
	case 12:
		m_seen[m_encounter->GetSpecies()->GetIndex()]++;
		EndEncounter(false, type);
		return THROW_RESULT_RUN_AWAY_AFTER_ONE;
	case 13:
		m_seen[m_encounter->GetSpecies()->GetIndex()]++;
		EndEncounter(false, type);
		return THROW_RESULT_RUN_AWAY_AFTER_TWO;
	default:
		m_seen[m_encounter->GetSpecies()->GetIndex()]++;
		EndEncounter(false, type);
		return THROW_RESULT_RUN_AWAY_AFTER_THREE;
	}
}


void ClientPlayer::RunFromEncounter()
{
	EndEncounter(false);
}


bool ClientPlayer::PowerUpMonster(std::shared_ptr<Monster> monster)
{
	if (monster->GetLevel() >= GetLevel())
		return false;
	if (monster->GetLevel() >= 40)
		return false;

	if (GetTreatsForSpecies(monster->GetSpecies()) < GetPowerUpCost(monster->GetLevel()).treats)
		return false;
	if (GetPowder() < GetPowerUpCost(monster->GetLevel()).powder)
		return false;

	m_treats[monster->GetSpecies()->GetBaseForm()->GetIndex()] -= GetPowerUpCost(monster->GetLevel()).treats;
	m_powder -= GetPowerUpCost(monster->GetLevel()).powder;
	monster->SetLevel(monster->GetLevel() + 1);
	return true;
}


bool ClientPlayer::EvolveMonster(std::shared_ptr<Monster> monster)
{
	if (monster->GetSpecies()->GetEvolutions().size() == 0)
		return false;
	if (GetTreatsForSpecies(monster->GetSpecies()) < monster->GetSpecies()->GetEvolutionCost())
		return false;

	m_treats[monster->GetSpecies()->GetBaseForm()->GetIndex()] -= monster->GetSpecies()->GetEvolutionCost();
	monster->Evolve();

	if (GetNumberCaptured(monster->GetSpecies()) == 0)
		EarnExperience(1000);
	else
		EarnExperience(500);
	m_seen[monster->GetSpecies()->GetIndex()]++;
	m_captured[monster->GetSpecies()->GetIndex()]++;
	m_treats[monster->GetSpecies()->GetBaseForm()->GetIndex()]++;
	return true;
}


void ClientPlayer::TransferMonster(std::shared_ptr<Monster> monster)
{
	for (auto i = m_monsters.begin(); i != m_monsters.end(); ++i)
	{
		if ((*i)->GetID() == monster->GetID())
		{
			m_treats[monster->GetSpecies()->GetBaseForm()->GetIndex()]++;
			m_monsters.erase(i);
			break;
		}
	}
}


void ClientPlayer::SetMonsterName(std::shared_ptr<Monster> monster, const string& name)
{
	monster->SetName(name);
}


uint8_t ClientPlayer::GetMapTile(int32_t x, int32_t y)
{
	return TILE_NOT_LOADED;
}


bool ClientPlayer::IsStopAvailable(int32_t x, int32_t y)
{
	if (GetMapTile(x, y) != TILE_STOP)
		return false;
	for (auto& i : m_recentStopsVisited)
	{
		if ((i.x == x) && (i.y == y) && ((time(NULL) - i.visitTime) < 300))
			return false;
	}
	return true;
}


map<ItemType, uint32_t> ClientPlayer::GetItemsFromStop(int32_t x, int32_t y)
{
	if (!IsStopAvailable(x, y))
		return map<ItemType, uint32_t>();

	// Clear out old visits from recent list
	for (size_t i = 0; i < m_recentStopsVisited.size(); )
	{
		if ((m_recentStopsVisited[i].visitTime - time(NULL)) > 300)
		{
			m_recentStopsVisited.erase(m_recentStopsVisited.begin() + i);
			continue;
		}
		i++;
	}

	if (m_recentStopsVisited.size() > MAX_STOPS_WITHIN_COOLDOWN)
		return map<ItemType, uint32_t>();

	map<ItemType, uint32_t> itemWeights;
	itemWeights[ITEM_STANDARD_BALL] = 20;
	itemWeights[ITEM_MEGA_SEED] = 3;
	if (GetLevel() >= 5)
		itemWeights[ITEM_STANDARD_HEAL] = 8;
	if (GetLevel() >= 10)
		itemWeights[ITEM_SUPER_BALL] = 7;
	if (GetLevel() >= 15)
		itemWeights[ITEM_SUPER_HEAL] = 3;
	if (GetLevel() >= 20)
		itemWeights[ITEM_UBER_BALL] = 3;
	if (GetLevel() >= 25)
		itemWeights[ITEM_KEG_OF_HEALTH] = 1;
	uint32_t totalWeight = 0;
	for (auto& i : itemWeights)
		totalWeight += i.second;

	map<ItemType, uint32_t> result;
	for (size_t count = 0; ; count++)
	{
		if (count >= 3)
		{
			if ((rand() % 10) > 3)
				break;
		}

		uint32_t value = rand() % totalWeight;
		uint32_t cur = 0;
		for (auto& i : itemWeights)
		{
			if (value < (cur + i.second))
			{
				result[i.first]++;
				break;
			}
			cur += i.second;
		}
	}

	for (auto& i : result)
		m_inventory[i.first] += i.second;

	// Add this visit to the recent list so that it can't be used until the cooldown expires
	RecentStopVisit visit;
	visit.x = x;
	visit.y = y;
	visit.visitTime = time(NULL);
	m_recentStopsVisited.push_back(visit);
	return result;
}
