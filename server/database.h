#pragma once

#include "player.h"
#include "monster.h"
#include "sqlite3.h"

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
	sqlite3_stmt* m_registerQuery;
	sqlite3_stmt* m_checkUsernameQuery;

	sqlite3_stmt* m_readInventoryItemQuery;
	sqlite3_stmt* m_writeInventoryItemQuery;
	sqlite3_stmt* m_insertInventoryItemQuery;

	bool InitStatements();
	void FinishStatement(sqlite3_stmt* stmt);

public:
	Database();
	~Database();

	static void Init();
	static Database* GetDatabase() { return m_dbInstance; }

	void PerformTransaction(const std::function<void()>& func);

	DatabaseLoginResult Login(const std::string& name, const std::string& password);
	DatabaseRegisterResult Register(const std::string& name, const std::string& password);

	void SetInventory(uint64_t user, uint32_t item, uint32_t count);
};
