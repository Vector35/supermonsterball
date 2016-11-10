#include <set>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "world.h"
#include "perlin2d.h"
#include "database.h"

using namespace std;


World* World::m_world = nullptr;


static uint32_t CoordToGridNumber(int32_t x, int32_t y)
{
	x += (MAP_SIZE / 2);
	y += (MAP_SIZE / 2);
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x >= MAP_SIZE)
		x = MAP_SIZE - 1;
	if (y >= MAP_SIZE)
		y = MAP_SIZE - 1;

	x /= GRID_SIZE;
	y /= GRID_SIZE;
	return (x << 16) | y;
}


World::World(Database* db)
{
	m_db = db;
	m_mapData = new uint8_t[MAP_SIZE * MAP_SIZE];
	memset(m_mapData, TILE_GRASS, MAP_SIZE * MAP_SIZE);

	Perlin2D water(0xbaadcafec0debabeLL);
	Perlin2D population(0xdeadbeeffeedfaceLL);
	Perlin2D temp(0xc01dc01ddeaddeadLL);

	uint64_t seed = 0xbaadbaaddeadc0deLL;
	bool afterSpawn = false;
	bool afterChurch = false;
	for (size_t y = 0; y < MAP_SIZE; y++)
	{
		for (size_t x = 0; x < MAP_SIZE; x++)
		{
			uint8_t tile = TILE_GRASS;
			float wet = water.GetValue(x / 32.0f, y / 32.0f);
			float dense = population.GetValue(x / 64.0f, y / 64.0f);
			float cold = temp.GetValue(x / 64.0f, y / 64.0f);

			int32_t fromCenterX = (int32_t)x - (MAP_SIZE / 2);
			int32_t fromCenterY = (int32_t)y - (MAP_SIZE / 2);
			if (fromCenterX < 0)
				fromCenterX = -fromCenterX;
			if (fromCenterY < 0)
				fromCenterY = -fromCenterY;
			float fromCenter = sqrt((fromCenterX * fromCenterX * 0.8f) + (fromCenterY * fromCenterY));
			if (fromCenter < 50)
				dense += 0.5f;
			else if (fromCenter < 120)
				dense += 0.5f * (1.0f - ((fromCenter - 50) / 70.0f));

			uint32_t spawnValue = (seed >> 16) & 0xffff;
			seed = (seed * 25214903917LL) + 11LL;
			uint32_t requiredSpawnValue = (uint32_t)(0x40 + (0x200 * (dense / 0.4f)));
			if (requiredSpawnValue > 0x400)
				requiredSpawnValue = 0x400;
			bool spawnHere = spawnValue < requiredSpawnValue;
			const char* spawnType = "grass";

			if (wet > 0.35f)
			{
				tile = TILE_WATER;
				spawnHere = false;
			}
			else if (dense > 0.4f)
			{
				tile = TILE_CITY;
				spawnType = "city";
				if ((!afterSpawn) && (!spawnHere) && ((x % 4) != 0) && ((y % 4) != 0))
				{
					uint32_t decorationValue = (seed >> 16) & 0xffff;
					seed = (seed * 25214903917LL) + 11LL;
					if (decorationValue < 0x4000)
						tile = TILE_CITY_BUILDING_1;
					else if (decorationValue < 0x4a00)
						tile = TILE_CITY_BUILDING_2;
					else if (decorationValue < 0x4c00)
						tile = TILE_CITY_BUILDING_3;
				}
			}
			else if (wet < -0.3f)
			{
				tile = TILE_DESERT;
				spawnType = "desert";
				if ((!afterSpawn) && (!spawnHere))
				{
					uint32_t decorationValue = (seed >> 16) & 0xffff;
					seed = (seed * 25214903917LL) + 11LL;
					if ((dense > 0.4f) && (decorationValue < 0x2000))
						tile = TILE_DESERT_HOUSE;
					else if (decorationValue < 0x200)
						tile = TILE_DESERT_CACTUS;
				}
			}
			else
			{
				tile = TILE_GRASS;
				if ((!afterSpawn) && (!spawnHere))
				{
					uint32_t decorationValue = (seed >> 16) & 0xffff;
					seed = (seed * 25214903917LL) + 11LL;
					if ((dense > 0.3f) && (decorationValue < 0x800))
						tile = TILE_SUBURB_HOUSE;
					else if ((dense > 0.3f) && (decorationValue < 0x900))
						tile = TILE_SUBURB_CHURCH;
					else if ((wet > 0.3f) && (decorationValue < 0x200))
						tile = TILE_GRASS_PALM_TREE;
					else if (decorationValue < 0x100)
						tile = TILE_GRASS_TREE_1;
					else if (decorationValue < 0x200)
						tile = TILE_GRASS_TREE_2;
				}

				if (wet > 0.3f)
					spawnType = "water";
				else if (cold > 0.25f)
					spawnType = "mountain";
			}

			if (tile != TILE_WATER)
			{
				uint32_t stopValue = (seed >> 16) & 0xffff;
				seed = (seed * 25214903917LL) + 11LL;
				uint32_t requiredStopValue = (uint32_t)(0x8 + (0x100 * (dense / 0.4f)));
				if (dense < 0.25f)
					requiredStopValue = 0x8;
				if (requiredStopValue > 0x140)
					requiredStopValue = 0x140;
				if (afterChurch)
					requiredStopValue = 0x6000;
				if (stopValue < requiredStopValue)
				{
					if (stopValue < (requiredStopValue / 4))
						tile = TILE_PIT;
					else
						tile = TILE_STOP;
					spawnHere = false;
				}
			}

			if ((x >= ((MAP_SIZE / 2) + PIT_OF_DOOM_X - 1)) && (x <= ((MAP_SIZE / 2) + PIT_OF_DOOM_X + 1)) &&
				(y >= ((MAP_SIZE / 2) + PIT_OF_DOOM_Y - 1)) && (y <= ((MAP_SIZE / 2) + PIT_OF_DOOM_Y + 1)))
			{
				tile = TILE_CITY;
				if ((x == ((MAP_SIZE / 2) + PIT_OF_DOOM_X)) && (y == ((MAP_SIZE / 2) + PIT_OF_DOOM_Y)))
					tile = TILE_PIT;
			}
			else if ((x > ((MAP_SIZE / 2) + PIT_OF_DOOM_X)) && (x <= ((MAP_SIZE / 2) + PIT_OF_DOOM_X + 8)) &&
				(y == ((MAP_SIZE / 2) + PIT_OF_DOOM_Y)))
			{
				tile = TILE_CITY;
			}

			m_mapData[(y * MAP_SIZE) + x] = tile;

			afterChurch = (tile == TILE_SUBURB_CHURCH);

			afterSpawn = false;
			if (spawnHere)
			{
				uint32_t timeValue = (seed >> 16) & 0xffff;
				seed = (seed * 25214903917LL) + 11LL;

				SpawnPoint s;
				s.x = x - (MAP_SIZE / 2);
				s.y = y - (MAP_SIZE / 2);
				s.timeOffset = timeValue % MONSTER_SPAWN_RESET_INTERVAL;
				s.timeActive = DEFAULT_SPAWN_TIME;
				s.biome = Biome::GetByName(spawnType);
				AddSpawnPoint(s);
				afterSpawn = true;
			}
		}
	}

	m_pitOfDoom.x = PIT_OF_DOOM_X;
	m_pitOfDoom.y = PIT_OF_DOOM_Y;
	m_pitOfDoom.team = TEAM_UNASSIGNED;
	m_pitOfDoom.reputation = 100000;

	shared_ptr<Monster> monster(new Monster(MonsterSpecies::GetByIndex(12), 0, 0, 0)); // Beezer
	monster->SetIV(8, 8, 8);
	monster->SetLevel(30);
	monster->SetSize(31);
	monster->SetMoves(Move::GetByName("Bug Bite"), Move::GetByName("Infestation"));
	monster->ResetHP();
	monster->SetID((uint64_t)-1);
	monster->SetOwner((uint64_t)-1, "Prof. Vick");
	m_pitOfDoom.defenders.push_back(monster);

	monster = shared_ptr<Monster>(new Monster(MonsterSpecies::GetByIndex(72), 0, 0, 0)); // Bonedread
	monster->SetIV(8, 8, 8);
	monster->SetLevel(30);
	monster->SetSize(31);
	monster->SetMoves(Move::GetByName("Ghost Blade"), Move::GetByName("Shadow Claw"));
	monster->ResetHP();
	monster->SetID((uint64_t)-2);
	monster->SetOwner((uint64_t)-1, "Prof. Vick");
	m_pitOfDoom.defenders.push_back(monster);

	monster = shared_ptr<Monster>(new Monster(MonsterSpecies::GetByIndex(76), 0, 0, 0)); // Ogreat
	monster->SetIV(8, 8, 8);
	monster->SetLevel(30);
	monster->SetSize(31);
	monster->SetMoves(Move::GetByName("Pound"), Move::GetByName("Juggernaut"));
	monster->ResetHP();
	monster->SetID((uint64_t)-3);
	monster->SetOwner((uint64_t)-1, "Prof. Vick");
	m_pitOfDoom.defenders.push_back(monster);

	monster = shared_ptr<Monster>(new Monster(MonsterSpecies::GetByIndex(49), 0, 0, 0)); // Krabber
	monster->SetIV(8, 8, 8);
	monster->SetLevel(30);
	monster->SetSize(31);
	monster->SetMoves(Move::GetByName("Water Blast"), Move::GetByName("Heavy Rain"));
	monster->ResetHP();
	monster->SetID((uint64_t)-4);
	monster->SetOwner((uint64_t)-1, "Prof. Vick");
	m_pitOfDoom.defenders.push_back(monster);

	monster = shared_ptr<Monster>(new Monster(MonsterSpecies::GetByIndex(101), 0, 0, 0)); // Burninator
	monster->SetIV(8, 8, 8);
	monster->SetLevel(30);
	monster->SetSize(31);
	monster->SetMoves(Move::GetByName("Fireball"), Move::GetByName("Burninate"));
	monster->ResetHP();
	monster->SetID((uint64_t)-5);
	monster->SetOwner((uint64_t)-1, "Prof. Vick");
	m_pitOfDoom.defenders.push_back(monster);

	monster = shared_ptr<Monster>(new Monster(MonsterSpecies::GetByIndex(103), 0, 0, 0)); // Ehkaybear
	monster->SetIV(8, 8, 8);
	monster->SetLevel(30);
	monster->SetSize(31);
	monster->SetMoves(Move::GetByName("Burst Fire"), Move::GetByName("Aimed Shot"));
	monster->ResetHP();
	monster->SetID((uint64_t)-6);
	monster->SetOwner((uint64_t)-1, "Prof. Vick");
	m_pitOfDoom.defenders.push_back(monster);
}


void World::Init(Database* db)
{
	m_world = new World(db);
}


World* World::GetWorld()
{
	return m_world;
}


void World::AddSpawnPoint(const SpawnPoint& spawn)
{
	uint32_t grid = CoordToGridNumber(spawn.x, spawn.y);
	m_data[grid].spawns.push_back(spawn);
}


PitStatus World::LoadPit(int32_t x, int32_t y)
{
	if (!m_db)
	{
		PitStatus status;
		status.x = x;
		status.y = y;
		status.team = TEAM_UNASSIGNED;
		status.reputation = 0;
		return status;
	}

	return m_db->GetPitStatus(x, y);
}


PitStatus* World::GetPit(int32_t x, int32_t y)
{
	if ((x == PIT_OF_DOOM_X) && (y == PIT_OF_DOOM_Y))
		return &m_pitOfDoom;

	if (GetMapTile(x, y) != TILE_PIT)
		return nullptr;

	uint32_t grid = CoordToGridNumber(x, y);
	auto i = m_data.find(grid);
	if (i == m_data.end())
	{
		PitStatus status = LoadPit(x, y);
		m_data[grid].pits.push_back(status);
		return &(*(m_data[grid].pits.end() - 1));
	}

	for (auto& j : i->second.pits)
	{
		if ((j.x != x) || (j.y != y))
			continue;
		return &j;
	}

	PitStatus status = LoadPit(x, y);
	i->second.pits.push_back(status);
	return &(*(m_data[grid].pits.end() - 1));
}


vector<SpawnPoint> World::GetSpawnPointsInRange(int32_t x, int32_t y)
{
	vector<SpawnPoint> result;
	uint32_t topLeft = CoordToGridNumber(x - SCANNER_RADIUS, y - SCANNER_RADIUS);
	uint32_t topCenter = CoordToGridNumber(x, y - SCANNER_RADIUS);
	uint32_t topRight = CoordToGridNumber(x + SCANNER_RADIUS, y - SCANNER_RADIUS);
	uint32_t centerLeft = CoordToGridNumber(x - SCANNER_RADIUS, y);
	uint32_t center = CoordToGridNumber(x, y);
	uint32_t centerRight = CoordToGridNumber(x + SCANNER_RADIUS, y);
	uint32_t botLeft = CoordToGridNumber(x - SCANNER_RADIUS, y + SCANNER_RADIUS);
	uint32_t botCenter = CoordToGridNumber(x, y + SCANNER_RADIUS);
	uint32_t botRight = CoordToGridNumber(x + SCANNER_RADIUS, y + SCANNER_RADIUS);
	std::set<uint32_t> grids;
	grids.insert(topLeft);
	grids.insert(topCenter);
	grids.insert(topRight);
	grids.insert(centerLeft);
	grids.insert(center);
	grids.insert(centerRight);
	grids.insert(botLeft);
	grids.insert(botCenter);
	grids.insert(botRight);

	for (auto grid : grids)
	{
		auto i = m_data.find(grid);
		if (i == m_data.end())
			continue;

		for (auto& spawn : i->second.spawns)
		{
			uint32_t distSq = ((x - spawn.x) * (x - spawn.x)) + (4 * ((y - spawn.y) * (y - spawn.y)));
			if (distSq <= (SCANNER_RADIUS * SCANNER_RADIUS))
				result.push_back(spawn);
		}
	}

	return result;
}


bool World::GetSpawnPointAt(int32_t x, int32_t y, SpawnPoint& spawn)
{
	uint32_t grid = CoordToGridNumber(x, y);
	auto i = m_data.find(grid);
	if (i == m_data.end())
		return false;

	for (auto& j : i->second.spawns)
	{
		if ((j.x == x) && (j.y == y))
		{
			spawn = j;
			return true;
		}
	}
	return false;
}


shared_ptr<Monster> World::GetMonsterAt(int32_t x, int32_t y, uint32_t trainerLevel)
{
	// Is there a spawn point actually there?
	SpawnPoint spawn;
	if (!GetSpawnPointAt(x, y, spawn))
		return shared_ptr<Monster>();

	// Is spawn point currently active?
	time_t t = time(NULL);
	uint32_t hour = (uint32_t)(t / MONSTER_SPAWN_RESET_INTERVAL);
	uint32_t curTimeOffset = (uint32_t)(t % MONSTER_SPAWN_RESET_INTERVAL);
	uint32_t spawnTimeOffset = spawn.timeOffset;
	if (curTimeOffset < spawnTimeOffset)
	{
		curTimeOffset += MONSTER_SPAWN_RESET_INTERVAL;
		hour--;
	}

	if (curTimeOffset > (spawnTimeOffset + spawn.timeActive))
		return shared_ptr<Monster>();

	// Get total weighting of biome spawns
	uint32_t total = 0;
	for (auto& i : spawn.biome->spawns)
		total += i.weight;

	// Pick spawn parameters based on coordinates and current time
	uint64_t seed = (((uint64_t)x * 694847539LL) + ((uint64_t)y * 91939LL) + ((uint64_t)hour * 349LL)) + 92893LL;
	uint64_t n = ((uint64_t)seed * 25214903917LL) + 11LL;
	uint64_t ivBase = ((uint64_t)n * 25214903917LL) + 11LL;
	uint64_t levelBase = ((uint64_t)(ivBase + ((uint64_t)trainerLevel * 277LL)) * 25214903917LL) + 11LL;
	uint64_t sizeBase = ((uint64_t)levelBase * 25214903917LL) + 11LL;
	uint64_t quickMoveBase = ((uint64_t)sizeBase * 25214903917LL) + 11LL;
	uint64_t chargeMoveBase = ((uint64_t)chargeMoveBase * 25214903917LL) + 11LL;
	uint32_t pick = (uint32_t)((n >> 16) % (uint64_t)total);
	uint32_t iv = (uint32_t)(ivBase >> 16) & 0xfff;
	uint32_t level = (uint32_t)((levelBase >> 16) % trainerLevel) + 1;
	uint32_t size = sizeBase % 32;
	uint32_t quickMoveChoice = (uint32_t)(quickMoveBase >> 16);
	uint32_t chargeMoveChoice = (uint32_t)(chargeMoveBase >> 16);
	if (level > 30)
		level = 30;

	// Find the chosen spawn out of the possible spawns and return result
	uint32_t cur = 0;
	for (auto& i : spawn.biome->spawns)
	{
		if (pick < (cur + i.weight))
		{
			std::shared_ptr<Monster> monster(new Monster(i.species, x, y, hour));
			monster->SetIV((iv >> 8) & 0xf, (iv >> 4) & 0xf, iv & 0xf);
			monster->SetSize(size);
			monster->SetLevel(level);

			Move* quickMove = monster->GetQuickMove();
			Move* chargeMove = monster->GetChargeMove();
			if (i.species->GetQuickMoves().size() > 0)
				quickMove = i.species->GetQuickMoves()[quickMoveChoice % i.species->GetQuickMoves().size()];
			if (i.species->GetChargeMoves().size() > 0)
				chargeMove = i.species->GetChargeMoves()[chargeMoveChoice % i.species->GetChargeMoves().size()];
			monster->SetMoves(quickMove, chargeMove);

			monster->ResetHP();
			return monster;
		}
		cur += i.weight;
	}
	return shared_ptr<Monster>();
}


MonsterSpecies* World::GetSpeciesForSpawnPoint(const SpawnPoint& spawn, uint32_t& spawnTime)
{
	// Is spawn point currently active?
	time_t t = time(NULL);
	uint32_t hour = (uint32_t)(t / MONSTER_SPAWN_RESET_INTERVAL);
	uint32_t curTimeOffset = (uint32_t)(t % MONSTER_SPAWN_RESET_INTERVAL);
	uint32_t spawnTimeOffset = spawn.timeOffset;
	if (curTimeOffset < spawnTimeOffset)
	{
		curTimeOffset += MONSTER_SPAWN_RESET_INTERVAL;
		hour--;
	}

	if (curTimeOffset > (spawnTimeOffset + spawn.timeActive))
		return nullptr;

	// Get total weighting of biome spawns
	uint32_t total = 0;
	for (auto& i : spawn.biome->spawns)
		total += i.weight;

	// Pick spawn parameters based on coordinates and current time
	uint64_t seed = (((uint64_t)spawn.x * 694847539LL) + ((uint64_t)spawn.y * 91939LL) +
		((uint64_t)hour * 349LL)) + 92893LL;
	uint64_t n = ((uint64_t)seed * 25214903917LL) + 11LL;
	uint32_t pick = (uint32_t)((n >> 16) % (uint64_t)total);

	// Find the chosen spawn out of the possible spawns and return result
	uint32_t cur = 0;
	for (auto& i : spawn.biome->spawns)
	{
		if (pick < (cur + i.weight))
		{
			spawnTime = hour;
			return i.species;
		}
		cur += i.weight;
	}
	return nullptr;
}


uint8_t World::GetMapTile(int32_t x, int32_t y)
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
	return m_mapData[(y * MAP_SIZE) + x];
}


Team World::GetPitTeam(int32_t x, int32_t y)
{
	PitStatus* pit = GetPit(x, y);
	if (!pit)
		return TEAM_UNASSIGNED;
	return pit->team;
}


uint32_t World::GetPitReputation(int32_t x, int32_t y)
{
	PitStatus* pit = GetPit(x, y);
	if (!pit)
		return 0;
	return pit->reputation;
}


vector<shared_ptr<Monster>> World::GetPitDefenders(int32_t x, int32_t y)
{
	PitStatus* pit = GetPit(x, y);
	if (!pit)
		return vector<shared_ptr<Monster>>();
	return pit->defenders;
}


bool World::AssignPitDefender(int32_t x, int32_t y, Team team, shared_ptr<Monster> monster)
{
	PitStatus* pit = GetPit(x, y);
	if (!pit)
		return false;
	if (team == TEAM_UNASSIGNED)
		return false;
	if (monster->IsDefending())
		return false;
	if ((x == PIT_OF_DOOM_X) && (y == PIT_OF_DOOM_Y))
		return false;

	if (pit->team == TEAM_UNASSIGNED)
	{
		pit->team = team;
		pit->reputation = DEFAULT_PIT_REPUTATION;
		pit->defenders.clear();
		pit->defenders.push_back(monster);

		if (m_db)
			m_db->SetPitStatus(*pit);

		monster->SetDefending(true);
		if (m_db)
			m_db->UpdateMonster(monster->GetOwnerID(), monster);
		return true;
	}

	if (pit->team != team)
		return false;

	if (Player::GetPitLevelByReputation(pit->reputation) >= 10)
		return false;
	if (pit->defenders.size() >= Player::GetPitLevelByReputation(pit->reputation))
		return false;

	for (auto i = pit->defenders.begin(); i != pit->defenders.end(); ++i)
	{
		if ((*i)->GetOwnerID() == monster->GetOwnerID())
		{
			// Already has a defender
			return false;
		}
	}

	for (auto i = pit->defenders.begin(); i != pit->defenders.end(); ++i)
	{
		if ((*i)->GetCP() > monster->GetCP())
		{
			pit->defenders.insert(i, monster);
			if (m_db)
				m_db->SetPitStatus(*pit);

			monster->SetDefending(true);
			if (m_db)
				m_db->UpdateMonster(monster->GetOwnerID(), monster);
			return true;
		}
	}

	pit->defenders.push_back(monster);
	if (m_db)
		m_db->SetPitStatus(*pit);

	monster->SetDefending(true);
	if (m_db)
		m_db->UpdateMonster(monster->GetOwnerID(), monster);
	return true;
}


uint32_t World::AddPitReputation(int32_t x, int32_t y, uint32_t reputation)
{
	PitStatus* pit = GetPit(x, y);
	if (!pit)
		return 0;
	if (pit->team == TEAM_UNASSIGNED)
		return 0;
	if ((x == PIT_OF_DOOM_X) && (y == PIT_OF_DOOM_Y))
		return 0;

	pit->reputation += reputation;
	if (pit->reputation > MAX_PIT_REPUTATION)
	{
		reputation -= pit->reputation - MAX_PIT_REPUTATION;
		pit->reputation = MAX_PIT_REPUTATION;
	}
	if (m_db)
		m_db->SetPitStatus(*pit);

	return reputation;
}


uint32_t World::RemovePitReputation(int32_t x, int32_t y, uint32_t reputation)
{
	PitStatus* pit = GetPit(x, y);
	if (!pit)
		return 0;
	if (pit->team == TEAM_UNASSIGNED)
		return 0;
	if ((x == PIT_OF_DOOM_X) && (y == PIT_OF_DOOM_Y))
		return 0;

	if (reputation >= pit->reputation)
	{
		reputation = pit->reputation;
		pit->team = TEAM_UNASSIGNED;
		pit->reputation = 0;
		for (auto& i : pit->defenders)
		{
			i->SetDefending(false);
			i->SetHP(0);
			if (m_db)
				m_db->UpdateMonster(i->GetOwnerID(), i);
		}
		if (m_db)
			m_db->SetPitStatus(*pit);
		return reputation;
	}

	pit->reputation -= reputation;
	while ((pit->defenders.size() > 0) && (pit->defenders.size() > Player::GetPitLevelByReputation(pit->reputation)))
	{
		pit->defenders[0]->SetDefending(false);
		pit->defenders[0]->SetHP(0);
		if (m_db)
			m_db->UpdateMonster(pit->defenders[0]->GetOwnerID(), pit->defenders[0]);
		pit->defenders.erase(pit->defenders.begin());
	}
	if (m_db)
		m_db->SetPitStatus(*pit);
	return reputation;
}
