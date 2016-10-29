#pragma once

#include <map>
#include "monster.h"

#define GRID_SIZE 128
#define MAP_SIZE 4096

#define SCANNER_RADIUS 100
#define CAPTURE_RADIUS 20

#define DEFAULT_SPAWN_TIME (15 * 60)

struct SpawnPoint
{
	int32_t x, y;
	uint32_t timeOffset, timeActive;
	Biome* biome;
};

struct GridData
{
	std::vector<SpawnPoint> spawns;
};

class World
{
	static World* m_world;
	std::map<uint32_t, GridData> m_data;

	void AddSpawnPoint(const SpawnPoint& data);

public:
	World();

	static void Init();
	static World* GetWorld();

	std::vector<SpawnPoint> GetSpawnPointsInRange(int32_t x, int32_t y);
	bool GetSpawnPointAt(int32_t x, int32_t y, SpawnPoint& spawn);
	std::shared_ptr<Monster> GetMonsterAt(int32_t x, int32_t y, uint32_t trainerLevel);
};
