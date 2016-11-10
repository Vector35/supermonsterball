#pragma once

#include "player.h"
#include "monster.h"
#include "sqlite3.h"
#include "world.h"

class DatabaseException: public std::exception
{
	std::string m_error;

public:
	DatabaseException(const std::string& error): std::exception(), m_error(error) {}
	virtual const char* what() const noexcept { return m_error.c_str(); }
};

struct DatabaseLoginResult
{
	bool valid;
	uint64_t id;
	bool flagged, banned;
	uint32_t level, xp, powder;
	int32_t x, y;
	Team team;
};

struct DatabaseRegisterResult
{
	bool valid;
	uint64_t id;
};

class Database
{
	static Database* m_dbInstance;

	sqlite3* m_db;
	bool m_inTransaction;

	sqlite3_stmt* m_beginQuery;
	sqlite3_stmt* m_commitQuery;
	sqlite3_stmt* m_rollbackQuery;

	sqlite3_stmt* m_loginQuery;
	sqlite3_stmt* m_readUserQuery;
	sqlite3_stmt* m_registerQuery;
	sqlite3_stmt* m_checkUsernameQuery;

	sqlite3_stmt* m_readInventoryQuery;
	sqlite3_stmt* m_readInventoryItemQuery;
	sqlite3_stmt* m_writeInventoryItemQuery;
	sqlite3_stmt* m_insertInventoryItemQuery;

	sqlite3_stmt* m_readSeenQuery;
	sqlite3_stmt* m_readSeenItemQuery;
	sqlite3_stmt* m_writeSeenItemQuery;
	sqlite3_stmt* m_insertSeenItemQuery;

	sqlite3_stmt* m_readCapturedQuery;
	sqlite3_stmt* m_readCapturedItemQuery;
	sqlite3_stmt* m_writeCapturedItemQuery;
	sqlite3_stmt* m_insertCapturedItemQuery;

	sqlite3_stmt* m_readTreatsQuery;
	sqlite3_stmt* m_readTreatsItemQuery;
	sqlite3_stmt* m_writeTreatsItemQuery;
	sqlite3_stmt* m_insertTreatsItemQuery;

	sqlite3_stmt* m_readMonstersQuery;
	sqlite3_stmt* m_writeMonsterQuery;
	sqlite3_stmt* m_insertMonsterQuery;
	sqlite3_stmt* m_removeMonsterQuery;

	sqlite3_stmt* m_setLocationQuery;
	sqlite3_stmt* m_setExperienceQuery;
	sqlite3_stmt* m_setPowderQuery;
	sqlite3_stmt* m_setTeamQuery;
	sqlite3_stmt* m_flagForBanQuery;
	sqlite3_stmt* m_banWaveQuery;

	sqlite3_stmt* m_readPitQuery;
	sqlite3_stmt* m_writePitQuery;
	sqlite3_stmt* m_insertPitQuery;
	sqlite3_stmt* m_readPitMonstersQuery;
	sqlite3_stmt* m_resetPitMonstersQuery;
	sqlite3_stmt* m_insertPitMonsterQuery;

	bool InitStatements();
	void FinishStatement(sqlite3_stmt* stmt);

public:
	Database();
	~Database();

	static void Init();
	static Database* GetDatabase() { return m_dbInstance; }

	void PerformTransaction(const std::function<void()>& func);

	DatabaseLoginResult Login(const std::string& name, const std::string& password);
	DatabaseLoginResult GetUserByID(uint64_t id, std::string& name);
	DatabaseRegisterResult Register(const std::string& name, const std::string& password);

	std::map<uint32_t, uint32_t> GetInventory(uint64_t user);
	void SetInventory(uint64_t user, uint32_t item, uint32_t count);

	std::map<uint32_t, uint32_t> GetMonstersSeen(uint64_t user);
	std::map<uint32_t, uint32_t> GetMonstersCaptured(uint64_t user);
	void SetMonsterSeenCount(uint64_t user, uint32_t species, uint32_t count);
	void SetMonsterCapturedCount(uint64_t user, uint32_t species, uint32_t count);

	std::map<uint32_t, uint32_t> GetTreats(uint64_t user);
	void SetTreats(uint64_t user, uint32_t species, uint32_t count);

	std::vector<std::shared_ptr<Monster>> GetMonsters(uint64_t user, const std::string& userName);
	uint64_t AddMonster(uint64_t user, std::shared_ptr<Monster> monster);
	void UpdateMonster(uint64_t user, std::shared_ptr<Monster> monster);
	void RemoveMonster(uint64_t user, std::shared_ptr<Monster> monster);

	void SetLocation(uint64_t user, int32_t x, int32_t y);
	void SetExperience(uint64_t user, uint32_t level, uint32_t xp);
	void SetPowder(uint64_t user, uint32_t powder);
	void SetTeam(uint64_t user, Team team);
	void FlagForBan(uint64_t user);
	void BanWave();

	PitStatus GetPitStatus(int32_t x, int32_t y);
	void SetPitStatus(const PitStatus& pit);
};
