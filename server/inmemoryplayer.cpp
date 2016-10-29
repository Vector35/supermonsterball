#include <cstdlib>
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


void InMemoryPlayer::GiveSeed()
{
	if (!m_encounter)
		return;
	if (!UseItem(ITEM_MEGA_SEED))
		return;
	m_seedGiven = true;
}


BallThrowResult InMemoryPlayer::ThrowBall(ItemType type)
{
	if (!m_encounter)
		return THROW_RESULT_RUN_AWAY;
	if ((type != ITEM_STANDARD_BALL) && (type != ITEM_SUPER_BALL) && (type != ITEM_UBER_BALL))
		return THROW_RESULT_RUN_AWAY;

	if (!UseItem(type))
		return THROW_RESULT_RUN_AWAY;

	return THROW_RESULT_CATCH;
}
