#include <openssl/ssl.h>
#include "database.h"

using namespace std;


Database* Database::m_dbInstance = nullptr;


Database::Database()
{
	if (sqlite3_open_v2("db.sqlite", &m_db, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK)
	{
		// No existing database, create a new one
		if (sqlite3_open_v2("db.sqlite", &m_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));

		// Ensure foreign key support is on
		char* err;
		if (sqlite3_exec(m_db, "PRAGMA foreign_keys = ON", NULL, NULL, &err) != SQLITE_OK)
			throw DatabaseException(err);

		if (sqlite3_exec(m_db,
			"CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT NOT NULL, password BLOB NOT NULL,\n"
			"                    flagged BOOLEAN NOT NULL, banned BOOLEAN NOT NULL, level INTEGER NOT NULL,\n"
			"                    xp INTEGER NOT NULL, powder INTEGER NOT NULL, x INTEGER, y INTEGER);\n"
			"CREATE TABLE monsters (id INTEGER PRIMARY KEY, user INTEGER NOT NULL, species INTEGER NOT NULL,\n"
			"                       name TEXT, hp INTEGER, attack INTEGER, defense INTEGER, stamina INTEGER,\n"
			"                       size INTEGER, level INTEGER NOT NULL, x INTEGER, y INTEGER, spawn_time INTEGER,\n"
			"                       ball INTEGER, quick_move INTEGER NOT NULL, charge_move INTEGER NOT NULL,\n"
			"                       FOREIGN KEY (user) REFERENCES users(id));\n"
			"CREATE TABLE inventory (user INTEGER NOT NULL, item INTEGER NOT NULL, count INTEGER NOT NULL,\n"
			"                        FOREIGN KEY (user) REFERENCES users(id));\n"
			"CREATE TABLE seen (user INTEGER NOT NULL, species INTEGER NOT NULL, count INTEGER NOT NULL,\n"
			"                   FOREIGN KEY (user) REFERENCES users(id));\n"
			"CREATE TABLE captured (user INTEGER NOT NULL, species INTEGER NOT NULL, count INTEGER NOT NULL,\n"
			"                       FOREIGN KEY (user) REFERENCES users(id));\n"
			"CREATE TABLE treats (user INTEGER NOT NULL, species INTEGER NOT NULL, count INTEGER NOT NULL,\n"
			"                     FOREIGN KEY (user) REFERENCES users(id));\n"
			"CREATE UNIQUE INDEX idx_users_by_name ON users (name);\n"
			"CREATE INDEX idx_monsters_by_user ON monsters (user);\n"
			"CREATE INDEX idx_inventory_by_user ON inventory (user);\n"
			"CREATE INDEX idx_seen_by_user ON seen (user);\n"
			"CREATE INDEX idx_captured_by_user ON captured (user);\n"
			"CREATE INDEX idx_treats_by_user ON treats (user);\n",
			NULL, NULL, &err))
			throw DatabaseException(err);
	}
	else
	{
		// Ensure foreign key support is on
		char* err;
		if (sqlite3_exec(m_db, "PRAGMA foreign_keys = ON", NULL, NULL, &err) != SQLITE_OK)
			throw DatabaseException(err);
	}

	// Initialize prepared statements
	if (!InitStatements())
		throw DatabaseException(sqlite3_errmsg(m_db));
}


Database::~Database()
{
	sqlite3_finalize(m_beginQuery);
	sqlite3_finalize(m_commitQuery);
	sqlite3_finalize(m_rollbackQuery);

	sqlite3_finalize(m_loginQuery);
	sqlite3_finalize(m_registerQuery);
	sqlite3_finalize(m_checkUsernameQuery);

	sqlite3_finalize(m_readInventoryItemQuery);
	sqlite3_finalize(m_writeInventoryItemQuery);
	sqlite3_finalize(m_insertInventoryItemQuery);

	sqlite3_close(m_db);
}


void Database::Init()
{
	m_dbInstance = new Database();
}


bool Database::InitStatements()
{
	// Create prepared statements for the transactions
	if (sqlite3_prepare_v2(m_db, "BEGIN TRANSACTION", -1, &m_beginQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "COMMIT TRANSACTION", -1, &m_commitQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "ROLLBACK TRANSACTION", -1, &m_rollbackQuery, NULL) != SQLITE_OK)
		return false;

	if (sqlite3_prepare_v2(m_db, "SELECT id, flagged, banned, level, xp, powder, x, y FROM users WHERE name=? "
		"AND password=?", -1, &m_loginQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "INSERT INTO users (name, password, flagged, banned, level, xp, powder, x, y) "
		"VALUES (?, ?, 0, 0, 1, 0, 0, 0, 0)", -1, &m_registerQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "SELECT id FROM users WHERE name=?", -1, &m_checkUsernameQuery, NULL) != SQLITE_OK)
		return false;

	if (sqlite3_prepare_v2(m_db, "SELECT count FROM inventory WHERE user=? AND item=?",
		-1, &m_readInventoryItemQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "UPDATE inventory SET count=? WHERE user=? AND item=?",
		-1, &m_writeInventoryItemQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "INSERT INTO inventory (user, item, count) VALUES (?, ?, ?)",
		-1, &m_insertInventoryItemQuery, NULL) != SQLITE_OK)
		return false;

	return true;
}


void Database::FinishStatement(sqlite3_stmt* stmt)
{
	while (true)
	{
		int result = sqlite3_step(stmt);
		if (result == SQLITE_ROW)
			continue;
		if (result == SQLITE_DONE)
			break;
		throw DatabaseException(sqlite3_errmsg(m_db));
	}
}


void Database::PerformTransaction(const function<void()>& func)
{
	if (m_inTransaction)
	{
		// Already in a transaction, execute the action(s) inside the existing transaction
		func();
		return;
	}

	// Start the transaction
	if (sqlite3_reset(m_beginQuery) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_step(m_beginQuery) != SQLITE_DONE)
		throw DatabaseException(sqlite3_errmsg(m_db));

	try
	{
		// Perform action(s) in the transaction
		m_inTransaction = true;
		func();

		// Commit the transaction
		m_inTransaction = false;
		if (sqlite3_reset(m_commitQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_step(m_commitQuery) != SQLITE_DONE)
			throw DatabaseException(sqlite3_errmsg(m_db));
	}
	catch (DatabaseException& e)
	{
		// Error during transaction, issue a rollback
		m_inTransaction = false;
		if (sqlite3_reset(m_rollbackQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_step(m_rollbackQuery) != SQLITE_DONE)
			throw DatabaseException(sqlite3_errmsg(m_db));
		throw e;
	}
	catch (exception& e)
	{
		// Error during transaction, issue a rollback
		m_inTransaction = false;
		if (sqlite3_reset(m_rollbackQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_step(m_rollbackQuery) != SQLITE_DONE)
			throw DatabaseException(sqlite3_errmsg(m_db));
		throw e;
	}
}


DatabaseLoginResult Database::Login(const string& name, const string& password)
{
	DatabaseLoginResult result;
	result.valid = false;

	SHA256_CTX ctx;
	uint8_t passwordHash[SHA256_DIGEST_LENGTH];
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, (const uint8_t*)password.c_str(), password.size());
	SHA256_Final(passwordHash, &ctx);

	if (sqlite3_reset(m_loginQuery) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_text(m_loginQuery, 1, name.c_str(), (int)name.size(), SQLITE_TRANSIENT) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_blob(m_loginQuery, 2, passwordHash, SHA256_DIGEST_LENGTH, SQLITE_TRANSIENT) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));

	if (sqlite3_step(m_loginQuery) == SQLITE_ROW)
	{
		result.id = sqlite3_column_int64(m_loginQuery, 0);
		result.flagged = sqlite3_column_int(m_loginQuery, 1) != 0;
		result.banned = sqlite3_column_int(m_loginQuery, 2) != 0;
		result.level = sqlite3_column_int(m_loginQuery, 3);
		result.xp = sqlite3_column_int(m_loginQuery, 4);
		result.powder = sqlite3_column_int(m_loginQuery, 5);
		result.x = sqlite3_column_int(m_loginQuery, 6);
		result.y = sqlite3_column_int(m_loginQuery, 7);
		FinishStatement(m_loginQuery);
		result.valid = true;
	}

	return result;
}


DatabaseRegisterResult Database::Register(const string& name, const string& password)
{
	DatabaseRegisterResult result;
	result.valid = false;

	SHA256_CTX ctx;
	uint8_t passwordHash[SHA256_DIGEST_LENGTH];
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, (const uint8_t*)password.c_str(), password.size());
	SHA256_Final(passwordHash, &ctx);

	PerformTransaction([&]() {
		if (sqlite3_reset(m_checkUsernameQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_text(m_checkUsernameQuery, 1, name.c_str(), (int)name.size(), SQLITE_TRANSIENT) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		int usernameResult = sqlite3_step(m_checkUsernameQuery);
		if (usernameResult == SQLITE_ROW)
		{
			FinishStatement(m_checkUsernameQuery);
			result.valid = false;
		}
		else if (usernameResult == SQLITE_DONE)
		{
			if (sqlite3_reset(m_registerQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_text(m_registerQuery, 1, name.c_str(), (int)name.size(), SQLITE_TRANSIENT) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_blob(m_registerQuery, 2, passwordHash, SHA256_DIGEST_LENGTH, SQLITE_TRANSIENT) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));

			if (sqlite3_step(m_registerQuery) != SQLITE_DONE)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_clear_bindings(m_registerQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			result.id = sqlite3_last_insert_rowid(m_db);
			result.valid = true;
		}
		else
		{
			throw DatabaseException(sqlite3_errmsg(m_db));
		}

		if (sqlite3_clear_bindings(m_checkUsernameQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
	});
	return result;
}


void Database::SetInventory(uint64_t user, uint32_t item, uint32_t count)
{
	PerformTransaction([&]() {
		if (sqlite3_reset(m_readInventoryItemQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int64(m_readInventoryItemQuery, 1, user))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_readInventoryItemQuery, 2, item))
			throw DatabaseException(sqlite3_errmsg(m_db));

		int result = sqlite3_step(m_readInventoryItemQuery);
		if ((result != SQLITE_ROW) && (result != SQLITE_DONE))
			throw DatabaseException(sqlite3_errmsg(m_db));

		if (result == SQLITE_ROW)
		{
			// Item already exists, update count
			FinishStatement(m_readInventoryItemQuery);

			if (sqlite3_reset(m_writeInventoryItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_writeInventoryItemQuery, 1, count))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int64(m_writeInventoryItemQuery, 2, user))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_writeInventoryItemQuery, 3, item))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_step(m_writeInventoryItemQuery) != SQLITE_DONE)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_clear_bindings(m_writeInventoryItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
		}
		else
		{
			if (sqlite3_reset(m_insertInventoryItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int64(m_insertInventoryItemQuery, 1, user))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_insertInventoryItemQuery, 2, item))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_insertInventoryItemQuery, 3, count))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_step(m_insertInventoryItemQuery) != SQLITE_DONE)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_clear_bindings(m_insertInventoryItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
		}
	});
}
