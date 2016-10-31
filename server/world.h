#pragma once

#include <map>
#include "monster.h"

#define GRID_SIZE 128
#define MAP_SIZE 4096

#define SCANNER_RADIUS 100
#define CAPTURE_RADIUS 15

#define DEFAULT_SPAWN_TIME (15 * 60)

enum MapTile
{
	TILE_GRASS = 0,
	TILE_GRASS_TREE_1 = 1,
	TILE_GRASS_TREE_2 = 2,
	TILE_GRASS_PALM_TREE = 3,
	TILE_SUBURB_HOUSE = 4,
	TILE_SUBURB_CHURCH = 5,
	TILE_WATER = 6,
	TILE_CITY = 7,
	TILE_CITY_BUILDING_1 = 8,
	TILE_CITY_BUILDING_2 = 9,
	TILE_CITY_BUILDING_3 = 10,
	TILE_CITY_BUILDING_4 = 11,
	TILE_CITY_BUILDING_5 = 12,
	TILE_DESERT = 13,
	TILE_DESERT_CACTUS = 14,
	TILE_DESERT_HOUSE = 15
};

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
	uint8_t* m_mapData;

	void AddSpawnPoint(const SpawnPoint& data);

public:
	World();

	static void Init();
	static World* GetWorld();

	std::vector<SpawnPoint> GetSpawnPointsInRange(int32_t x, int32_t y);
	bool GetSpawnPointAt(int32_t x, int32_t y, SpawnPoint& spawn);
	std::shared_ptr<Monster> GetMonsterAt(int32_t x, int32_t y, uint32_t trainerLevel);

	uint8_t GetMapTile(int32_t x, int32_t y);
};
