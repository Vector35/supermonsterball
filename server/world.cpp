#include <set>
#include <time.h>
#include "world.h"

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
	SpawnPoint s;
	s.x = -5;
	s.y = -5;
	s.timeOffset = 0;
	s.timeActive = DEFAULT_SPAWN_TIME;
	s.biome = Biome::GetByName("grass");
	AddSpawnPoint(s);

	s.x = 5;
	s.timeOffset = 12 * 60;
	s.biome = Biome::GetByName("water");
	AddSpawnPoint(s);

	s.x = 0;
	s.y = -5;
	s.timeOffset = 24 * 60;
	s.biome = Biome::GetByName("city");
	AddSpawnPoint(s);

	s.x = 2;
	s.timeOffset = 36 * 60;
	s.biome = Biome::GetByName("city");
	AddSpawnPoint(s);

	s.x = 0;
	s.y = 5;
	s.timeOffset = 48 * 60;
	s.biome = Biome::GetByName("grass");
	AddSpawnPoint(s);
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
	uint32_t pick = (uint32_t)((n >> 16) % (uint64_t)total);
	uint32_t iv = (uint32_t)(ivBase >> 16) & 0xfff;
	uint32_t level = (uint32_t)((levelBase >> 16) % trainerLevel) + 1;
	if (level > 30)
		level = 30;

	// Find the chosen spawn out of the possible spawns and return result
	uint32_t cur = 0;
	for (auto& i : spawn.biome->spawns)
	{
		if (pick < (cur + i.weight))
		{
			std::shared_ptr<Monster> monster(new Monster(i.species));
			monster->SetIV((iv >> 8) & 0xf, (iv >> 4) & 0xf, iv & 0xf);
			monster->SetLevel(level);
			monster->ResetHP();
			return monster;
		}
		cur += i.weight;
	}
	return shared_ptr<Monster>();
}
