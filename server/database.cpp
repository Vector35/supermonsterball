#include <openssl/ssl.h>
#include "database.h"
#include "clienthandler.h"

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
			"                    xp INTEGER NOT NULL, powder INTEGER NOT NULL, x INTEGER, y INTEGER, team INTEGER);\n"
			"CREATE TABLE monsters (id INTEGER PRIMARY KEY, user INTEGER NOT NULL, species INTEGER NOT NULL,\n"
			"                       name TEXT, hp INTEGER, attack INTEGER, defense INTEGER, stamina INTEGER,\n"
			"                       size INTEGER, level INTEGER NOT NULL, x INTEGER, y INTEGER, spawn_time INTEGER,\n"
			"                       ball INTEGER, quick_move INTEGER NOT NULL, charge_move INTEGER NOT NULL,\n"
			"                       defending BOOLEAN NOT NULL, FOREIGN KEY (user) REFERENCES users(id));\n"
			"CREATE TABLE inventory (user INTEGER NOT NULL, item INTEGER NOT NULL, count INTEGER NOT NULL,\n"
			"                        FOREIGN KEY (user) REFERENCES users(id));\n"
			"CREATE TABLE seen (user INTEGER NOT NULL, species INTEGER NOT NULL, count INTEGER NOT NULL,\n"
			"                   FOREIGN KEY (user) REFERENCES users(id));\n"
			"CREATE TABLE captured (user INTEGER NOT NULL, species INTEGER NOT NULL, count INTEGER NOT NULL,\n"
			"                       FOREIGN KEY (user) REFERENCES users(id));\n"
			"CREATE TABLE treats (user INTEGER NOT NULL, species INTEGER NOT NULL, count INTEGER NOT NULL,\n"
			"                     FOREIGN KEY (user) REFERENCES users(id));\n"
			"CREATE TABLE pits (id INTEGER PRIMARY KEY, x INTEGER, y INTEGER, team INTEGER, reputation INTEGER);\n"
			"CREATE TABLE pit_monsters (pit INTEGER NOT NULL, monster INTEGER NOT NULL, ordering INTEGER,\n"
			"                           FOREIGN KEY (pit) REFERENCES pits(id),\n"
			"                           FOREIGN KEY (monster) REFERENCES monsters(id));\n"
			"CREATE UNIQUE INDEX idx_users_by_name ON users (name);\n"
			"CREATE INDEX idx_monsters_by_user ON monsters (user);\n"
			"CREATE INDEX idx_inventory_by_user ON inventory (user);\n"
			"CREATE INDEX idx_seen_by_user ON seen (user);\n"
			"CREATE INDEX idx_captured_by_user ON captured (user);\n"
			"CREATE INDEX idx_treats_by_user ON treats (user);\n"
			"CREATE UNIQUE INDEX idx_pit_pos ON pits (x, y);\n"
			"CREATE INDEX idx_pit_monsters ON pit_monsters (pit);\n",
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
	sqlite3_finalize(m_readUserQuery);
	sqlite3_finalize(m_registerQuery);
	sqlite3_finalize(m_checkUsernameQuery);

	sqlite3_finalize(m_readInventoryQuery);
	sqlite3_finalize(m_readInventoryItemQuery);
	sqlite3_finalize(m_writeInventoryItemQuery);
	sqlite3_finalize(m_insertInventoryItemQuery);

	sqlite3_finalize(m_readSeenQuery);
	sqlite3_finalize(m_readSeenItemQuery);
	sqlite3_finalize(m_writeSeenItemQuery);
	sqlite3_finalize(m_insertSeenItemQuery);

	sqlite3_finalize(m_readCapturedQuery);
	sqlite3_finalize(m_readCapturedItemQuery);
	sqlite3_finalize(m_writeCapturedItemQuery);
	sqlite3_finalize(m_insertCapturedItemQuery);

	sqlite3_finalize(m_readTreatsQuery);
	sqlite3_finalize(m_readTreatsItemQuery);
	sqlite3_finalize(m_writeTreatsItemQuery);
	sqlite3_finalize(m_insertTreatsItemQuery);

	sqlite3_finalize(m_readMonstersQuery);
	sqlite3_finalize(m_writeMonsterQuery);
	sqlite3_finalize(m_insertMonsterQuery);
	sqlite3_finalize(m_removeMonsterQuery);

	sqlite3_finalize(m_setLocationQuery);
	sqlite3_finalize(m_setExperienceQuery);
	sqlite3_finalize(m_setPowderQuery);
	sqlite3_finalize(m_setTeamQuery);

	sqlite3_finalize(m_readPitQuery);
	sqlite3_finalize(m_writePitQuery);
	sqlite3_finalize(m_insertPitQuery);
	sqlite3_finalize(m_readPitMonstersQuery);
	sqlite3_finalize(m_resetPitMonstersQuery);
	sqlite3_finalize(m_insertPitMonsterQuery);

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

	if (sqlite3_prepare_v2(m_db, "SELECT id, flagged, banned, level, xp, powder, x, y, team FROM users WHERE name=? "
		"AND password=?", -1, &m_loginQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "SELECT name, flagged, banned, level, xp, powder, x, y, team FROM users WHERE id=?",
		-1, &m_readUserQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "INSERT INTO users (name, password, flagged, banned, level, xp, powder, x, y, team) "
		"VALUES (?, ?, 0, 0, 1, 0, 0, 0, 0, 0)", -1, &m_registerQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "SELECT id FROM users WHERE name=?", -1, &m_checkUsernameQuery, NULL) != SQLITE_OK)
		return false;

	if (sqlite3_prepare_v2(m_db, "SELECT item, count FROM inventory WHERE user=?",
		-1, &m_readInventoryQuery, NULL) != SQLITE_OK)
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

	if (sqlite3_prepare_v2(m_db, "SELECT species, count FROM seen WHERE user=?",
		-1, &m_readSeenQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "SELECT count FROM seen WHERE user=? AND species=?",
		-1, &m_readSeenItemQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "UPDATE seen SET count=? WHERE user=? AND species=?",
		-1, &m_writeSeenItemQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "INSERT INTO seen (user, species, count) VALUES (?, ?, ?)",
		-1, &m_insertSeenItemQuery, NULL) != SQLITE_OK)
		return false;

	if (sqlite3_prepare_v2(m_db, "SELECT species, count FROM captured WHERE user=?",
		-1, &m_readCapturedQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "SELECT count FROM captured WHERE user=? AND species=?",
		-1, &m_readCapturedItemQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "UPDATE captured SET count=? WHERE user=? AND species=?",
		-1, &m_writeCapturedItemQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "INSERT INTO captured (user, species, count) VALUES (?, ?, ?)",
		-1, &m_insertCapturedItemQuery, NULL) != SQLITE_OK)
		return false;

	if (sqlite3_prepare_v2(m_db, "SELECT species, count FROM treats WHERE user=?",
		-1, &m_readTreatsQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "SELECT count FROM treats WHERE user=? AND species=?",
		-1, &m_readTreatsItemQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "UPDATE treats SET count=? WHERE user=? AND species=?",
		-1, &m_writeTreatsItemQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "INSERT INTO treats (user, species, count) VALUES (?, ?, ?)",
		-1, &m_insertTreatsItemQuery, NULL) != SQLITE_OK)
		return false;

	if (sqlite3_prepare_v2(m_db, "SELECT id, species, name, hp, attack, defense, stamina, size, level, x, y, "
		"spawn_time, ball, quick_move, charge_move, defending FROM monsters WHERE user=?",
		-1, &m_readMonstersQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "UPDATE monsters SET species=?, name=?, hp=?, level=?, quick_move=?, charge_move=?, "
		"defending=? WHERE user=? AND id=?", -1, &m_writeMonsterQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "INSERT INTO monsters (user, species, name, hp, attack, defense, stamina, size, "
		"level, x, y, spawn_time, ball, quick_move, charge_move, defending) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
		"?, ?, ?, ?, ?)",
		-1, &m_insertMonsterQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "DELETE FROM monsters WHERE user=? AND id=?",
		-1, &m_removeMonsterQuery, NULL) != SQLITE_OK)
		return false;

	if (sqlite3_prepare_v2(m_db, "UPDATE users SET x=?, y=? WHERE id=?", -1, &m_setLocationQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "UPDATE users SET level=?, xp=? WHERE id=?",
		-1, &m_setExperienceQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "UPDATE users SET powder=? WHERE id=?", -1, &m_setPowderQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "UPDATE users SET team=? WHERE id=?", -1, &m_setTeamQuery, NULL) != SQLITE_OK)
		return false;

	if (sqlite3_prepare_v2(m_db, "SELECT id, team, reputation FROM pits WHERE x=? AND y=?",
		-1, &m_readPitQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "UPDATE pits SET team=?, reputation=? WHERE x=? AND y=?",
		-1, &m_writePitQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "INSERT INTO pits (x, y, team, reputation) VALUES (?, ?, ?, ?)",
		-1, &m_insertPitQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "SELECT monsters.id, monsters.user FROM pit_monsters INNER JOIN "
		"monsters ON monsters.id = pit_monsters.monster WHERE pit=? ORDER BY ordering ASC",
		-1, &m_readPitMonstersQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "DELETE FROM pit_monsters WHERE pit=?",
		-1, &m_resetPitMonstersQuery, NULL) != SQLITE_OK)
		return false;
	if (sqlite3_prepare_v2(m_db, "INSERT INTO pit_monsters (pit, monster, ordering) VALUES (?, ?, ?)",
		-1, &m_insertPitMonsterQuery, NULL) != SQLITE_OK)
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
		result.team = (Team)sqlite3_column_int(m_loginQuery, 8);
		FinishStatement(m_loginQuery);
		result.valid = true;
	}

	return result;
}


DatabaseLoginResult Database::GetUserByID(uint64_t id, string& name)
{
	DatabaseLoginResult result;
	result.valid = false;

	if (sqlite3_reset(m_readUserQuery) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int64(m_readUserQuery, 1, id) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));

	if (sqlite3_step(m_readUserQuery) == SQLITE_ROW)
	{
		name = string((const char*)sqlite3_column_text(m_readUserQuery, 0),
			(size_t)sqlite3_column_bytes(m_readUserQuery, 0));
		result.flagged = sqlite3_column_int(m_readUserQuery, 1) != 0;
		result.banned = sqlite3_column_int(m_readUserQuery, 2) != 0;
		result.level = sqlite3_column_int(m_readUserQuery, 3);
		result.xp = sqlite3_column_int(m_readUserQuery, 4);
		result.powder = sqlite3_column_int(m_readUserQuery, 5);
		result.x = sqlite3_column_int(m_readUserQuery, 6);
		result.y = sqlite3_column_int(m_readUserQuery, 7);
		result.team = (Team)sqlite3_column_int(m_readUserQuery, 8);
		FinishStatement(m_readUserQuery);
		result.id = id;
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


map<uint32_t, uint32_t> Database::GetInventory(uint64_t user)
{
	if (sqlite3_reset(m_readInventoryQuery) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int64(m_readInventoryQuery, 1, user))
		throw DatabaseException(sqlite3_errmsg(m_db));

	int result = sqlite3_step(m_readInventoryQuery);
	if ((result != SQLITE_ROW) && (result != SQLITE_DONE))
		throw DatabaseException(sqlite3_errmsg(m_db));

	map<uint32_t, uint32_t> inventory;
	while (result == SQLITE_ROW)
	{
		uint32_t item = sqlite3_column_int(m_readInventoryQuery, 0);
		uint32_t count = sqlite3_column_int(m_readInventoryQuery, 1);
		inventory[item] = count;

		result = sqlite3_step(m_readInventoryQuery);
	}

	FinishStatement(m_readInventoryQuery);
	return inventory;
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


map<uint32_t, uint32_t> Database::GetMonstersSeen(uint64_t user)
{
	if (sqlite3_reset(m_readSeenQuery) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int64(m_readSeenQuery, 1, user))
		throw DatabaseException(sqlite3_errmsg(m_db));

	int result = sqlite3_step(m_readSeenQuery);
	if ((result != SQLITE_ROW) && (result != SQLITE_DONE))
		throw DatabaseException(sqlite3_errmsg(m_db));

	map<uint32_t, uint32_t> seen;
	while (result == SQLITE_ROW)
	{
		uint32_t species = sqlite3_column_int(m_readSeenQuery, 0);
		uint32_t count = sqlite3_column_int(m_readSeenQuery, 1);
		seen[species] = count;

		result = sqlite3_step(m_readSeenQuery);
	}

	FinishStatement(m_readSeenQuery);
	return seen;
}


map<uint32_t, uint32_t> Database::GetMonstersCaptured(uint64_t user)
{
	if (sqlite3_reset(m_readCapturedQuery) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int64(m_readCapturedQuery, 1, user))
		throw DatabaseException(sqlite3_errmsg(m_db));

	int result = sqlite3_step(m_readCapturedQuery);
	if ((result != SQLITE_ROW) && (result != SQLITE_DONE))
		throw DatabaseException(sqlite3_errmsg(m_db));

	map<uint32_t, uint32_t> captured;
	while (result == SQLITE_ROW)
	{
		uint32_t species = sqlite3_column_int(m_readCapturedQuery, 0);
		uint32_t count = sqlite3_column_int(m_readCapturedQuery, 1);
		captured[species] = count;

		result = sqlite3_step(m_readCapturedQuery);
	}

	FinishStatement(m_readCapturedQuery);
	return captured;
}


void Database::SetMonsterSeenCount(uint64_t user, uint32_t species, uint32_t count)
{
	PerformTransaction([&]() {
		if (sqlite3_reset(m_readSeenItemQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int64(m_readSeenItemQuery, 1, user))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_readSeenItemQuery, 2, species))
			throw DatabaseException(sqlite3_errmsg(m_db));

		int result = sqlite3_step(m_readSeenItemQuery);
		if ((result != SQLITE_ROW) && (result != SQLITE_DONE))
			throw DatabaseException(sqlite3_errmsg(m_db));

		if (result == SQLITE_ROW)
		{
			// Item already exists, update count
			FinishStatement(m_readSeenItemQuery);

			if (sqlite3_reset(m_writeSeenItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_writeSeenItemQuery, 1, count))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int64(m_writeSeenItemQuery, 2, user))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_writeSeenItemQuery, 3, species))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_step(m_writeSeenItemQuery) != SQLITE_DONE)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_clear_bindings(m_writeSeenItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
		}
		else
		{
			if (sqlite3_reset(m_insertSeenItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int64(m_insertSeenItemQuery, 1, user))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_insertSeenItemQuery, 2, species))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_insertSeenItemQuery, 3, count))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_step(m_insertSeenItemQuery) != SQLITE_DONE)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_clear_bindings(m_insertSeenItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
		}
	});
}


void Database::SetMonsterCapturedCount(uint64_t user, uint32_t species, uint32_t count)
{
	PerformTransaction([&]() {
		if (sqlite3_reset(m_readCapturedItemQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int64(m_readCapturedItemQuery, 1, user))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_readCapturedItemQuery, 2, species))
			throw DatabaseException(sqlite3_errmsg(m_db));

		int result = sqlite3_step(m_readCapturedItemQuery);
		if ((result != SQLITE_ROW) && (result != SQLITE_DONE))
			throw DatabaseException(sqlite3_errmsg(m_db));

		if (result == SQLITE_ROW)
		{
			// Item already exists, update count
			FinishStatement(m_readCapturedItemQuery);

			if (sqlite3_reset(m_writeCapturedItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_writeCapturedItemQuery, 1, count))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int64(m_writeCapturedItemQuery, 2, user))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_writeCapturedItemQuery, 3, species))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_step(m_writeCapturedItemQuery) != SQLITE_DONE)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_clear_bindings(m_writeCapturedItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
		}
		else
		{
			if (sqlite3_reset(m_insertCapturedItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int64(m_insertCapturedItemQuery, 1, user))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_insertCapturedItemQuery, 2, species))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_insertCapturedItemQuery, 3, count))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_step(m_insertCapturedItemQuery) != SQLITE_DONE)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_clear_bindings(m_insertCapturedItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
		}
	});
}


map<uint32_t, uint32_t> Database::GetTreats(uint64_t user)
{
	if (sqlite3_reset(m_readTreatsQuery) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int64(m_readTreatsQuery, 1, user))
		throw DatabaseException(sqlite3_errmsg(m_db));

	int result = sqlite3_step(m_readTreatsQuery);
	if ((result != SQLITE_ROW) && (result != SQLITE_DONE))
		throw DatabaseException(sqlite3_errmsg(m_db));

	map<uint32_t, uint32_t> treats;
	while (result == SQLITE_ROW)
	{
		uint32_t species = sqlite3_column_int(m_readTreatsQuery, 0);
		uint32_t count = sqlite3_column_int(m_readTreatsQuery, 1);
		treats[species] = count;

		result = sqlite3_step(m_readTreatsQuery);
	}

	FinishStatement(m_readTreatsQuery);
	return treats;
}


void Database::SetTreats(uint64_t user, uint32_t species, uint32_t count)
{
	PerformTransaction([&]() {
		if (sqlite3_reset(m_readTreatsItemQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int64(m_readTreatsItemQuery, 1, user))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_readTreatsItemQuery, 2, species))
			throw DatabaseException(sqlite3_errmsg(m_db));

		int result = sqlite3_step(m_readTreatsItemQuery);
		if ((result != SQLITE_ROW) && (result != SQLITE_DONE))
			throw DatabaseException(sqlite3_errmsg(m_db));

		if (result == SQLITE_ROW)
		{
			// Item already exists, update count
			FinishStatement(m_readTreatsItemQuery);

			if (sqlite3_reset(m_writeTreatsItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_writeTreatsItemQuery, 1, count))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int64(m_writeTreatsItemQuery, 2, user))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_writeTreatsItemQuery, 3, species))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_step(m_writeTreatsItemQuery) != SQLITE_DONE)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_clear_bindings(m_writeTreatsItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
		}
		else
		{
			if (sqlite3_reset(m_insertTreatsItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int64(m_insertTreatsItemQuery, 1, user))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_insertTreatsItemQuery, 2, species))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_insertTreatsItemQuery, 3, count))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_step(m_insertTreatsItemQuery) != SQLITE_DONE)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_clear_bindings(m_insertTreatsItemQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
		}
	});
}


vector<shared_ptr<Monster>> Database::GetMonsters(uint64_t user, const string& userName)
{
	if (sqlite3_reset(m_readMonstersQuery) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int64(m_readMonstersQuery, 1, user))
		throw DatabaseException(sqlite3_errmsg(m_db));

	int result = sqlite3_step(m_readMonstersQuery);
	if ((result != SQLITE_ROW) && (result != SQLITE_DONE))
		throw DatabaseException(sqlite3_errmsg(m_db));

	vector<shared_ptr<Monster>> monsters;
	while (result == SQLITE_ROW)
	{
		uint64_t id = sqlite3_column_int64(m_readMonstersQuery, 0);
		uint32_t species = sqlite3_column_int(m_readMonstersQuery, 1);
		string name = string((const char*)sqlite3_column_text(m_readMonstersQuery, 2),
			(size_t)sqlite3_column_bytes(m_readMonstersQuery, 2));
		uint32_t hp = sqlite3_column_int(m_readMonstersQuery, 3);
		uint32_t attack = sqlite3_column_int(m_readMonstersQuery, 4);
		uint32_t defense = sqlite3_column_int(m_readMonstersQuery, 5);
		uint32_t stamina = sqlite3_column_int(m_readMonstersQuery, 6);
		uint32_t size = sqlite3_column_int(m_readMonstersQuery, 7);
		uint32_t level = sqlite3_column_int(m_readMonstersQuery, 8);
		int32_t x = sqlite3_column_int(m_readMonstersQuery, 9);
		int32_t y = sqlite3_column_int(m_readMonstersQuery, 10);
		uint32_t spawnTime = sqlite3_column_int(m_readMonstersQuery, 11);
		uint32_t ball = sqlite3_column_int(m_readMonstersQuery, 12);
		uint32_t quickMove = sqlite3_column_int(m_readMonstersQuery, 13);
		uint32_t chargeMove = sqlite3_column_int(m_readMonstersQuery, 14);
		bool defending = sqlite3_column_int(m_readMonstersQuery, 15) != 0;

		shared_ptr<Monster> monster(new Monster(MonsterSpecies::GetByIndex(species), x, y, spawnTime));
		monster->SetID(id);
		monster->SetHP(hp);
		monster->SetIV(attack, defense, stamina);
		monster->SetSize(size);
		monster->SetLevel(level);
		monster->SetCapture(true, (ItemType)ball);
		monster->SetName(name);
		monster->SetMoves(Move::GetByIndex(quickMove), Move::GetByIndex(chargeMove));
		monster->SetDefending(defending);
		monster->SetOwner(user, userName);
		monsters.push_back(monster);

		result = sqlite3_step(m_readMonstersQuery);
	}

	FinishStatement(m_readMonstersQuery);
	return monsters;
}


uint64_t Database::AddMonster(uint64_t user, shared_ptr<Monster> monster)
{
	if (sqlite3_reset(m_insertMonsterQuery) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int64(m_insertMonsterQuery, 1, user))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 2, monster->GetSpecies()->GetIndex()))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_text(m_insertMonsterQuery, 3, monster->GetName().c_str(), (int)monster->GetName().size(),
		SQLITE_TRANSIENT) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 4, monster->GetCurrentHP()))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 5, monster->GetAttackIV()))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 6, monster->GetDefenseIV()))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 7, monster->GetStaminaIV()))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 8, monster->GetSize()))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 9, monster->GetLevel()))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 10, monster->GetSpawnX()))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 11, monster->GetSpawnY()))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 12, monster->GetSpawnTime()))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 13, (int)monster->GetBallType()))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 14, monster->GetQuickMove()->GetIndex()))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 15, monster->GetChargeMove()->GetIndex()))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_insertMonsterQuery, 16, monster->IsDefending() ? 1 : 0))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_step(m_insertMonsterQuery) != SQLITE_DONE)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_clear_bindings(m_insertMonsterQuery) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	return sqlite3_last_insert_rowid(m_db);
}


void Database::UpdateMonster(uint64_t user, shared_ptr<Monster> monster)
{
	PerformTransaction([&]() {
		if (sqlite3_reset(m_writeMonsterQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_writeMonsterQuery, 1, monster->GetSpecies()->GetIndex()))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_text(m_writeMonsterQuery, 2, monster->GetName().c_str(), (int)monster->GetName().size(),
			SQLITE_TRANSIENT) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_writeMonsterQuery, 3, monster->GetCurrentHP()))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_writeMonsterQuery, 4, monster->GetLevel()))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_writeMonsterQuery, 5, monster->GetQuickMove()->GetIndex()))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_writeMonsterQuery, 6, monster->GetChargeMove()->GetIndex()))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_writeMonsterQuery, 7, monster->IsDefending() ? 1 : 0))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int64(m_writeMonsterQuery, 8, user))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int64(m_writeMonsterQuery, 9, monster->GetID()))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_step(m_writeMonsterQuery) != SQLITE_DONE)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_clear_bindings(m_writeMonsterQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
	});
}


void Database::RemoveMonster(uint64_t user, shared_ptr<Monster> monster)
{
	PerformTransaction([&]() {
		if (sqlite3_reset(m_removeMonsterQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int64(m_removeMonsterQuery, 1, user))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int64(m_removeMonsterQuery, 2, monster->GetID()))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_step(m_removeMonsterQuery) != SQLITE_DONE)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_clear_bindings(m_removeMonsterQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
	});
}


void Database::SetLocation(uint64_t user, int32_t x, int32_t y)
{
	PerformTransaction([&]() {
		if (sqlite3_reset(m_setLocationQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_setLocationQuery, 1, x))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_setLocationQuery, 2, y))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int64(m_setLocationQuery, 3, user))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_step(m_setLocationQuery) != SQLITE_DONE)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_clear_bindings(m_setLocationQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
	});
}


void Database::SetExperience(uint64_t user, uint32_t level, uint32_t xp)
{
	PerformTransaction([&]() {
		if (sqlite3_reset(m_setExperienceQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_setExperienceQuery, 1, level))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_setExperienceQuery, 2, xp))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int64(m_setExperienceQuery, 3, user))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_step(m_setExperienceQuery) != SQLITE_DONE)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_clear_bindings(m_setExperienceQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
	});
}


void Database::SetPowder(uint64_t user, uint32_t powder)
{
	PerformTransaction([&]() {
		if (sqlite3_reset(m_setPowderQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_setPowderQuery, 1, powder))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int64(m_setPowderQuery, 2, user))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_step(m_setPowderQuery) != SQLITE_DONE)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_clear_bindings(m_setPowderQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
	});
}


void Database::SetTeam(uint64_t user, Team team)
{
	PerformTransaction([&]() {
		if (sqlite3_reset(m_setTeamQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_setTeamQuery, 1, (uint32_t)team))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int64(m_setTeamQuery, 2, user))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_step(m_setTeamQuery) != SQLITE_DONE)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_clear_bindings(m_setTeamQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
	});
}


PitStatus Database::GetPitStatus(int32_t x, int32_t y)
{
	if (sqlite3_reset(m_readPitQuery) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_readPitQuery, 1, x))
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int(m_readPitQuery, 2, y))
		throw DatabaseException(sqlite3_errmsg(m_db));

	int result = sqlite3_step(m_readPitQuery);
	if ((result != SQLITE_ROW) && (result != SQLITE_DONE))
		throw DatabaseException(sqlite3_errmsg(m_db));

	PitStatus status;
	status.x = x;
	status.y = y;

	if (result == SQLITE_DONE)
	{
		status.team = TEAM_UNASSIGNED;
		status.reputation = 0;
		return status;
	}

	uint64_t id = sqlite3_column_int64(m_readPitQuery, 0);
	status.team = (Team)sqlite3_column_int(m_readPitQuery, 1);
	status.reputation = sqlite3_column_int(m_readPitQuery, 2);

	FinishStatement(m_readPitQuery);

	if (sqlite3_reset(m_readPitMonstersQuery) != SQLITE_OK)
		throw DatabaseException(sqlite3_errmsg(m_db));
	if (sqlite3_bind_int64(m_readPitMonstersQuery, 1, id))
		throw DatabaseException(sqlite3_errmsg(m_db));
	result = sqlite3_step(m_readPitMonstersQuery);
	if ((result != SQLITE_ROW) && (result != SQLITE_DONE))
		throw DatabaseException(sqlite3_errmsg(m_db));

	while (result == SQLITE_ROW)
	{
		uint64_t monster = sqlite3_column_int64(m_readPitMonstersQuery, 0);
		uint64_t user = sqlite3_column_int64(m_readPitMonstersQuery, 1);

		shared_ptr<ServerPlayer> player = ClientHandler::GetPlayerByID(user);
		if (player)
		{
			vector<shared_ptr<Monster>> monsters = player->GetMonsters();
			shared_ptr<Monster> foundMonster;
			for (auto& i : monsters)
			{
				if (i->GetID() == monster)
				{
					foundMonster = i;
					break;
				}
			}

			if (foundMonster)
				status.defenders.push_back(foundMonster);
			else
				printf("Monster ID %d not found for player ID %d\n", (int)monster, (int)user);
		}
		else
		{
			printf("Player ID %d invalid\n", (int)user);
		}

		result = sqlite3_step(m_readPitMonstersQuery);
	}

	FinishStatement(m_readPitMonstersQuery);
	return status;
}


void Database::SetPitStatus(const PitStatus& pit)
{
	PerformTransaction([&]() {
		if (sqlite3_reset(m_readPitQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_readPitQuery, 1, pit.x))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int(m_readPitQuery, 2, pit.y))
			throw DatabaseException(sqlite3_errmsg(m_db));

		int result = sqlite3_step(m_readPitQuery);
		if ((result != SQLITE_ROW) && (result != SQLITE_DONE))
			throw DatabaseException(sqlite3_errmsg(m_db));

		uint64_t id;
		if (result == SQLITE_DONE)
		{
			if (sqlite3_reset(m_insertPitQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_insertPitQuery, 1, pit.x))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_insertPitQuery, 2, pit.y))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_insertPitQuery, 3, (uint32_t)pit.team))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_insertPitQuery, 4, pit.reputation))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_step(m_insertPitQuery) != SQLITE_DONE)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_clear_bindings(m_insertPitQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			id = sqlite3_last_insert_rowid(m_db);
		}
		else
		{
			id = sqlite3_column_int64(m_readPitQuery, 0);
			FinishStatement(m_readPitQuery);

			if (sqlite3_reset(m_writePitQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_writePitQuery, 1, (uint32_t)pit.team))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_writePitQuery, 2, pit.reputation))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_writePitQuery, 3, pit.x))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_writePitQuery, 4, pit.y))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_step(m_writePitQuery) != SQLITE_DONE)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_clear_bindings(m_writePitQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
		}

		if (sqlite3_reset(m_resetPitMonstersQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_bind_int64(m_resetPitMonstersQuery, 1, id))
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_step(m_resetPitMonstersQuery) != SQLITE_DONE)
			throw DatabaseException(sqlite3_errmsg(m_db));
		if (sqlite3_clear_bindings(m_resetPitMonstersQuery) != SQLITE_OK)
			throw DatabaseException(sqlite3_errmsg(m_db));

		for (size_t i = 0; i < pit.defenders.size(); i++)
		{
			if (sqlite3_reset(m_insertPitMonsterQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int64(m_insertPitMonsterQuery, 1, id))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int64(m_insertPitMonsterQuery, 2, pit.defenders[i]->GetID()))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_bind_int(m_insertPitMonsterQuery, 3, (uint32_t)i))
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_step(m_insertPitMonsterQuery) != SQLITE_DONE)
				throw DatabaseException(sqlite3_errmsg(m_db));
			if (sqlite3_clear_bindings(m_insertPitMonsterQuery) != SQLITE_OK)
				throw DatabaseException(sqlite3_errmsg(m_db));
		}
	});
}
