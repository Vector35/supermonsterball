#include <set>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include "world.h"
#include "perlin2d.h"

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


World::World()
{
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
					else if (decorationValue < 0x4800)
						tile = TILE_CITY_BUILDING_2;
					else if (decorationValue < 0x4a00)
						tile = TILE_CITY_BUILDING_3;
					else if (decorationValue < 0x4c00)
						tile = TILE_CITY_BUILDING_4;
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
					tile = TILE_STOP;
					spawnHere = false;
				}
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
				s.timeOffset = timeValue % 3600;
				s.timeActive = DEFAULT_SPAWN_TIME;
				s.biome = Biome::GetByName(spawnType);
				AddSpawnPoint(s);
				afterSpawn = true;
			}
		}
	}
}


void World::Init()
{
	m_world = new World();
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
	uint32_t hour = (uint32_t)(t / 3600);
	uint32_t curTimeOffset = (uint32_t)(t % 3600);
	uint32_t spawnTimeOffset = spawn.timeOffset;
	if (curTimeOffset < spawnTimeOffset)
	{
		curTimeOffset += 3600;
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
