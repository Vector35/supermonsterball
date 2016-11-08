#pragma once

#include <map>
#include "monster.h"
#include "player.h"

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
	TILE_DESERT = 11,
	TILE_DESERT_CACTUS = 12,
	TILE_DESERT_HOUSE = 13,
	TILE_PIT = 14,
	TILE_STOP = 15,
	TILE_NOT_LOADED = 255
};

struct SpawnPoint
{
	int32_t x, y;
	uint32_t timeOffset, timeActive;
	Biome* biome;
};

struct PitStatus
{
	int32_t x, y;
	Team team;
	uint32_t reputation;
	std::vector<std::shared_ptr<Monster>> defenders;
};

struct GridData
{
	std::vector<SpawnPoint> spawns;
	std::vector<PitStatus> pits;
};

class Database;

class World
{
	static World* m_world;
	std::map<uint32_t, GridData> m_data;
	uint8_t* m_mapData;
	Database* m_db;

	void AddSpawnPoint(const SpawnPoint& data);
	PitStatus LoadPit(int32_t x, int32_t y);
	PitStatus* GetPit(int32_t x, int32_t y);

public:
	World(Database* db);

	static void Init(Database* db);
	static World* GetWorld();

	std::vector<SpawnPoint> GetSpawnPointsInRange(int32_t x, int32_t y);
	bool GetSpawnPointAt(int32_t x, int32_t y, SpawnPoint& spawn);
	std::shared_ptr<Monster> GetMonsterAt(int32_t x, int32_t y, uint32_t trainerLevel);

	uint8_t GetMapTile(int32_t x, int32_t y);

	Team GetPitTeam(int32_t x, int32_t y);
	uint32_t GetPitReputation(int32_t x, int32_t y);
	std::vector<std::shared_ptr<Monster>> GetPitDefenders(int32_t x, int32_t y);
	bool AssignPitDefender(int32_t x, int32_t y, Team team, std::shared_ptr<Monster> monster);
	uint32_t AddPitReputation(int32_t x, int32_t y, uint32_t reputation);
	uint32_t RemovePitReputation(int32_t x, int32_t y, uint32_t reputation);
};
