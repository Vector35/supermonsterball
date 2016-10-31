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

			if (wet > 0.35f)
				tile = TILE_WATER;
			else if (dense > 0.4f)
			{
				tile = TILE_CITY;
				if (((x % 4) != 0) && ((y % 4) != 0))
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
					else if (decorationValue < 0x4e00)
						tile = TILE_CITY_BUILDING_5;
				}
			}
			else if (wet < -0.3f)
			{
				tile = TILE_DESERT;
				uint32_t decorationValue = (seed >> 16) & 0xffff;
				seed = (seed * 25214903917LL) + 11LL;
				if ((dense > 0.4f) && (decorationValue < 0x2000))
					tile = TILE_DESERT_HOUSE;
				else if (decorationValue < 0x200)
					tile = TILE_DESERT_CACTUS;
			}
			else
			{
				tile = TILE_GRASS;
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
			m_mapData[(y * MAP_SIZE) + x] = tile;
		}
	}

	for (size_t i = 0; i < 100000; i++)
	{
		SpawnPoint s;
		s.x = (rand() % MAP_SIZE) - (MAP_SIZE / 2);
		s.y = (rand() % MAP_SIZE) - (MAP_SIZE / 2);
		s.timeOffset = rand() % 3600;
		s.timeActive = DEFAULT_SPAWN_TIME;
		switch (rand() % 5)
		{
		case 0:
			s.biome = Biome::GetByName("grass");
			break;
		case 1:
			s.biome = Biome::GetByName("water");
			break;
		case 2:
			s.biome = Biome::GetByName("mountain");
			break;
		case 3:
			s.biome = Biome::GetByName("desert");
			break;
		default:
			s.biome = Biome::GetByName("city");
			break;
		}
		AddSpawnPoint(s);
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
	uint32_t pick = (uint32_t)((n >> 16) % (uint64_t)total);
	uint32_t iv = (uint32_t)(ivBase >> 16) & 0xfff;
	uint32_t level = (uint32_t)((levelBase >> 16) % trainerLevel) + 1;
	uint32_t size = sizeBase % 32;
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
		return TILE_GRASS;
	if (y < 0)
		return TILE_GRASS;
	if (x >= MAP_SIZE)
		return TILE_GRASS;
	if (y >= MAP_SIZE)
		return TILE_GRASS;
	return m_mapData[(y * MAP_SIZE) + x];
}
