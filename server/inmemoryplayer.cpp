#include <cstdlib>
#include <set>
#include "inmemoryplayer.h"
#include "world.h"

using namespace std;


InMemoryPlayer::InMemoryPlayer(const string& name): m_name(name)
{
	m_level = 1;
	m_xp = 0;
	m_powder = 0;
	m_x = 0;
	m_y = 0;

	m_inventory[ITEM_STANDARD_BALL] = 30;
	m_inventory[ITEM_SUPER_BALL] = 5;
	m_inventory[ITEM_UBER_BALL] = 3;
	m_inventory[ITEM_MEGA_SEED] = 10;
}


uint32_t InMemoryPlayer::GetNumberCaptured(MonsterSpecies* species)
{
	auto i = m_captured.find(species->GetIndex());
	if (i == m_captured.end())
		return 0;
	return i->second;
}


uint32_t InMemoryPlayer::GetNumberSeen(MonsterSpecies* species)
{
	auto i = m_seen.find(species->GetIndex());
	if (i == m_seen.end())
		return 0;
	return i->second;
}


uint32_t InMemoryPlayer::GetTreatsForSpecies(MonsterSpecies* species)
{
	auto i = m_treats.find(species->GetIndex());
	if (i == m_treats.end())
		return 0;
	return i->second;
}


uint32_t InMemoryPlayer::GetItemCount(ItemType type)
{
	auto i = m_inventory.find(type);
	if (i == m_inventory.end())
		return 0;
	return i->second;
}


bool InMemoryPlayer::UseItem(ItemType type)
{
	auto i = m_inventory.find(type);
	if (i == m_inventory.end())
		return false;
	if (i->second == 0)
		return false;
	i->second--;
	return true;
}


void InMemoryPlayer::ReportLocation(int32_t x, int32_t y)
{
	m_x = x;
	m_y = y;
}


vector<MonsterSighting> InMemoryPlayer::GetMonstersInRange()
{
	vector<SpawnPoint> spawns = World::GetWorld()->GetSpawnPointsInRange(m_x, m_y);
	vector<MonsterSighting> result;
	for (auto& i : spawns)
	{
		shared_ptr<Monster> monster = World::GetWorld()->GetMonsterAt(i.x, i.y, m_level);
		if (monster)
		{
			// Ensure the monster hasn't already been encountered
			bool valid = true;
			auto j = m_recentEncounters.find(monster->GetSpawnTime());
			if (j != m_recentEncounters.end())
			{
				for (auto& k : j->second)
				{
					if ((k->GetSpawnX() == i.x) && (k->GetSpawnY() == i.y))
					{
						// Already finished encounter with this one
						valid = false;
						break;
					}
				}
			}

			if (!valid)
				continue;

			MonsterSighting sighting;
			sighting.species = monster->GetSpecies();
			sighting.x = i.x;
			sighting.y = i.y;
			result.push_back(sighting);
		}
	}
	return result;
}


shared_ptr<Monster> InMemoryPlayer::StartWildEncounter(int32_t x, int32_t y)
{
	uint32_t dist = abs(x - m_x) + abs(y - m_y);
	if (dist > CAPTURE_RADIUS)
		return nullptr;

	m_encounter = World::GetWorld()->GetMonsterAt(x, y, m_level);
	m_seedGiven = false;
	return m_encounter;
}


bool InMemoryPlayer::GiveSeed()
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


void InMemoryPlayer::EndEncounter(bool caught, ItemType ball)
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


void InMemoryPlayer::EarnExperience(uint32_t xp)
{
	m_xp += xp;
	while ((m_level < 40) && (m_xp >= GetTotalExperienceNeededForNextLevel()))
	{
		for (auto& i : GetItemsOnLevelUp(m_level))
			m_inventory[i.type] += i.count;
		m_level++;
	}
}


BallThrowResult InMemoryPlayer::ThrowBall(ItemType type)
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
		m_treats[m_encounter->GetSpecies()->GetIndex()] += 3;
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


void InMemoryPlayer::RunFromEncounter()
{
	EndEncounter(false);
}


bool InMemoryPlayer::PowerUpMonster(std::shared_ptr<Monster> monster)
{
	if (monster->GetLevel() >= GetLevel())
		return false;
	if (monster->GetLevel() >= 40)
		return false;

	if (GetTreatsForSpecies(monster->GetSpecies()) < GetPowerUpCost(monster->GetLevel()).treats)
		return false;
	if (GetPowder() < GetPowerUpCost(monster->GetLevel()).powder)
		return false;

	m_treats[monster->GetSpecies()->GetIndex()] -= GetPowerUpCost(monster->GetLevel()).treats;
	m_powder -= GetPowerUpCost(monster->GetLevel()).powder;
	monster->SetLevel(monster->GetLevel() + 1);
	return true;
}


bool InMemoryPlayer::EvolveMonster(std::shared_ptr<Monster> monster)
{
	if (monster->GetSpecies()->GetEvolutions().size() == 0)
		return false;
	if (GetTreatsForSpecies(monster->GetSpecies()) < monster->GetSpecies()->GetEvolutionCost())
		return false;

	m_treats[monster->GetSpecies()->GetIndex()] -= monster->GetSpecies()->GetEvolutionCost();
	monster->Evolve();
	return true;
}
