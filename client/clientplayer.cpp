#include <cstdlib>
#include <set>
#include <string.h>
#include "clientplayer.h"
#include "world.h"
#include "clientrequest.h"

using namespace std;
using namespace request;


ClientPlayer::ClientPlayer(const string& name): m_name(name)
{
	GetPlayerDetailsResponse details = ClientRequest::GetClient()->GetPlayerDetails();
	m_level = details.level();
	m_xp = details.xp();
	m_powder = details.powder();
	m_x = details.x();
	m_y = details.y();

	m_monsters = ClientRequest::GetClient()->GetMonsterList();
	m_inventory = ClientRequest::GetClient()->GetInventory();

	GetMonstersSeenAndCapturedResponse seenAndCaptured = ClientRequest::GetClient()->GetMonstersSeenAndCaptured();
	for (int i = 0; i < seenAndCaptured.seen_size(); i++)
		m_seen[seenAndCaptured.seen(i).species()] = seenAndCaptured.seen(i).count();
	for (int i = 0; i < seenAndCaptured.captured_size(); i++)
		m_captured[seenAndCaptured.captured(i).species()] = seenAndCaptured.captured(i).count();

	m_treats = ClientRequest::GetClient()->GetTreats();
	m_recentStopsVisited = ClientRequest::GetClient()->GetRecentStops();

	m_lastSightingRequest = 0;

	m_mapData = new uint8_t[MAP_SIZE * MAP_SIZE];
	memset(m_mapData, TILE_NOT_LOADED, MAP_SIZE * MAP_SIZE);
}


shared_ptr<Monster> ClientPlayer::GetMonsterByID(uint64_t id)
{
	for (auto& i : m_monsters)
	{
		if (i->GetID() == id)
			return i;
	}
	return nullptr;
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
	if ((time(NULL) - m_lastSightingRequest) >= 5)
	{
		m_recentSightings = ClientRequest::GetClient()->GetMonstersInRange(m_x, m_y);
		m_lastSightingRequest = time(NULL);
	}
	return m_recentSightings;
}


shared_ptr<Monster> ClientPlayer::StartWildEncounter(int32_t x, int32_t y)
{
	m_encounter = ClientRequest::GetClient()->StartEncounter(x, y);
	return m_encounter;
}


bool ClientPlayer::GiveSeed()
{
	if (!ClientRequest::GetClient()->GiveSeed())
		return false;

	m_inventory = ClientRequest::GetClient()->GetInventory();
	return true;
}


BallThrowResult ClientPlayer::ThrowBall(ItemType type)
{
	if (!m_encounter)
		return THROW_RESULT_RUN_AWAY_AFTER_ONE;

	BallThrowResult result = ClientRequest::GetClient()->ThrowBall(type, m_encounter);
	if (result == THROW_RESULT_CATCH)
	{
		// Caught it, grab new player data
		GetPlayerDetailsResponse details = ClientRequest::GetClient()->GetPlayerDetails();
		m_level = details.level();
		m_xp = details.xp();
		m_powder = details.powder();

		m_monsters = ClientRequest::GetClient()->GetMonsterList();
		m_inventory = ClientRequest::GetClient()->GetInventory();

		GetMonstersSeenAndCapturedResponse seenAndCaptured = ClientRequest::GetClient()->GetMonstersSeenAndCaptured();
		for (int i = 0; i < seenAndCaptured.seen_size(); i++)
			m_seen[seenAndCaptured.seen(i).species()] = seenAndCaptured.seen(i).count();
		for (int i = 0; i < seenAndCaptured.captured_size(); i++)
			m_captured[seenAndCaptured.captured(i).species()] = seenAndCaptured.captured(i).count();

		m_treats = ClientRequest::GetClient()->GetTreats();
	}
	else
	{
		// Did not catch it, grab new inventory counts
		m_inventory = ClientRequest::GetClient()->GetInventory();
	}
	return result;
}


void ClientPlayer::RunFromEncounter()
{
	ClientRequest::GetClient()->RunFromEncounter();
	m_encounter.reset();
}


bool ClientPlayer::PowerUpMonster(std::shared_ptr<Monster> monster)
{
	if (!ClientRequest::GetClient()->PowerUpMonster(monster))
		return false;

	GetPlayerDetailsResponse details = ClientRequest::GetClient()->GetPlayerDetails();
	m_level = details.level();
	m_xp = details.xp();
	m_powder = details.powder();
	m_treats = ClientRequest::GetClient()->GetTreats();
	return true;
}


bool ClientPlayer::EvolveMonster(std::shared_ptr<Monster> monster)
{
	if (!ClientRequest::GetClient()->EvolveMonster(monster))
		return false;

	GetPlayerDetailsResponse details = ClientRequest::GetClient()->GetPlayerDetails();
	m_level = details.level();
	m_xp = details.xp();
	m_powder = details.powder();

	GetMonstersSeenAndCapturedResponse seenAndCaptured = ClientRequest::GetClient()->GetMonstersSeenAndCaptured();
	for (int i = 0; i < seenAndCaptured.seen_size(); i++)
		m_seen[seenAndCaptured.seen(i).species()] = seenAndCaptured.seen(i).count();
	for (int i = 0; i < seenAndCaptured.captured_size(); i++)
		m_captured[seenAndCaptured.captured(i).species()] = seenAndCaptured.captured(i).count();

	m_treats = ClientRequest::GetClient()->GetTreats();
	return true;
}


void ClientPlayer::TransferMonster(std::shared_ptr<Monster> monster)
{
	ClientRequest::GetClient()->TransferMonster(monster);
	m_treats = ClientRequest::GetClient()->GetTreats();

	for (auto i = m_monsters.begin(); i != m_monsters.end(); ++i)
	{
		if ((*i)->GetID() == monster->GetID())
		{
			m_monsters.erase(i);
			break;
		}
	}
}


void ClientPlayer::SetMonsterName(std::shared_ptr<Monster> monster, const string& name)
{
	ClientRequest::GetClient()->SetMonsterName(monster, name);
	monster->SetName(name);
}


uint8_t ClientPlayer::GetMapTile(int32_t x, int32_t y)
{
	x += MAP_SIZE / 2;
	y += MAP_SIZE / 2;
	if (x < 0)
		return TILE_NOT_LOADED;
	if (y < 0)
		return TILE_NOT_LOADED;
	if (x >= MAP_SIZE)
		return TILE_NOT_LOADED;
	if (y >= MAP_SIZE)
		return TILE_NOT_LOADED;

	if (m_mapData[(y * MAP_SIZE) + x] == TILE_NOT_LOADED)
	{
		int32_t baseX = x & ~(GRID_SIZE - 1);
		int32_t baseY = y & ~(GRID_SIZE - 1);
		uint8_t data[GRID_SIZE * GRID_SIZE];
		ClientRequest::GetClient()->GetMapTiles(baseX - (MAP_SIZE / 2), baseY - (MAP_SIZE / 2), data);
		for (size_t dy = 0; dy < GRID_SIZE; dy++)
			for (size_t dx = 0; dx < GRID_SIZE; dx++)
				m_mapData[((baseY + dy) * MAP_SIZE) + (baseX + dx)] = data[(dy * GRID_SIZE) + dx];
	}

	return m_mapData[(y * MAP_SIZE) + x];
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

	map<ItemType, uint32_t> result = ClientRequest::GetClient()->GetItemsFromStop(x, y);
	m_inventory = ClientRequest::GetClient()->GetInventory();

	// Add this visit to the recent list so that it can't be used until the cooldown expires
	RecentStopVisit visit;
	visit.x = x;
	visit.y = y;
	visit.visitTime = time(NULL);
	m_recentStopsVisited.push_back(visit);
	return result;
}
