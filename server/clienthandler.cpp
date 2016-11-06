#include <string.h>
#include <time.h>
#include "clienthandler.h"
#include "processingthread.h"
#include "database.h"
#include "world.h"
#include "request.pb.h"

using namespace std;
using namespace request;


map<uint64_t, shared_ptr<ServerPlayer>> ClientHandler::m_playerCache;
mutex ClientHandler::m_playerCacheMutex;


ClientHandler::ClientHandler(SSLSocket* s): m_ssl(s), m_player(nullptr)
{
}


ClientHandler::~ClientHandler()
{
}


string ClientHandler::ReadRequestPacket()
{
	uint16_t len = m_ssl->Read16();
	char* data = new char[len];
	if (!m_ssl->Read(data, len))
	{
		delete[] data;
		throw SocketException("Incomplete request");
	}

	string result(data, len);
	delete[] data;
	return result;
}


void ClientHandler::WriteResponse(const string& msg)
{
	uint32_t len = (uint32_t)msg.size();
	string buf((char*)&len, sizeof(len));
	buf += msg;
	m_ssl->Write(buf.c_str(), buf.size());
}


void ClientHandler::Login(const std::string& msg)
{
	LoginRequest request;
	if (!request.ParseFromString(msg))
		throw SocketException("Bad login request format");

	DatabaseLoginResult result;
	ProcessingThread::Instance()->Process([&]() {
		result = Database::GetDatabase()->Login(request.username(), request.password());

		if (result.valid)
		{
			unique_lock<mutex> lock(m_playerCacheMutex);
			auto i = m_playerCache.find(result.id);
			if (i == m_playerCache.end())
			{
				shared_ptr<ServerPlayer> player(new ServerPlayer(request.username(), result));
				m_playerCache[result.id] = player;
				m_player = player;
			}
			else
			{
				m_player = i->second;
			}
		}
	});

	LoginResponse response;
	if (!result.valid)
		response.set_status(LoginResponse_AccountStatus_InvalidUsernameOrPassword);
	else if (result.banned)
		response.set_status(LoginResponse_AccountStatus_AccountBanned);
	else
		response.set_status(LoginResponse_AccountStatus_LoginOK);
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::Register(const std::string& msg)
{
	RegisterRequest request;
	if (!request.ParseFromString(msg))
		throw SocketException("Bad register request format");

	bool validName = true;
	if (request.username().size() == 0)
		validName = false;
	if (request.username().size() > 16)
		validName = false;
	for (size_t i = 0; i < request.username().size(); i++)
	{
		if ((request.username()[i] < ' ') || (request.username()[i] > 0x7e))
			validName = false;
	}

	if (!validName)
	{
		RegisterResponse response;
		response.set_status(RegisterResponse_RegisterStatus_InvalidOrDuplicateUsername);
		WriteResponse(response.SerializeAsString());
		return;
	}

	if (request.password().size() == 0)
	{
		RegisterResponse response;
		response.set_status(RegisterResponse_RegisterStatus_BadPassword);
		WriteResponse(response.SerializeAsString());
		return;
	}

	DatabaseRegisterResult result;
	ProcessingThread::Instance()->Process([&]() {
		result = Database::GetDatabase()->Register(request.username(), request.password());
		if (result.valid)
		{
			unique_lock<mutex> lock(m_playerCacheMutex);
			shared_ptr<ServerPlayer> player(new ServerPlayer(request.username(), result.id));
			m_playerCache[result.id] = player;
			m_player = player;
		}
	});

	RegisterResponse response;
	if (!result.valid)
		response.set_status(RegisterResponse_RegisterStatus_InvalidOrDuplicateUsername);
	else
		response.set_status(RegisterResponse_RegisterStatus_RegisterOK);
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::GetPlayerDetails()
{
	GetPlayerDetailsResponse response;
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		response.set_level(m_player->GetLevel());
		response.set_xp(m_player->GetTotalExperience());
		response.set_powder(m_player->GetPowder());
		response.set_x(m_player->GetLastLocationX());
		response.set_y(m_player->GetLastLocationY());
	});
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::GetMonsterList()
{
	GetMonsterListResponse response;
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		vector<shared_ptr<Monster>> monsters = m_player->GetMonsters();
		for (auto& i : monsters)
		{
			GetMonsterListResponse_MonsterDetails* monster = response.add_monsters();
			monster->set_id(i->GetID());
			monster->set_species(i->GetSpecies()->GetIndex());
			monster->set_name(i->GetName());
			monster->set_hp(i->GetCurrentHP());
			monster->set_attack(i->GetAttackIV());
			monster->set_defense(i->GetDefenseIV());
			monster->set_stamina(i->GetStaminaIV());
			monster->set_size(i->GetSize());
			monster->set_level(i->GetLevel());
			monster->set_x(i->GetSpawnX());
			monster->set_y(i->GetSpawnY());
			monster->set_spawntime(i->GetSpawnTime());
			monster->set_ball((uint32_t)i->GetBallType());
			monster->set_quickmove(0);
			monster->set_chargemove(0);
		}
	});
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::GetMonstersSeenAndCaptured()
{
	GetMonstersSeenAndCapturedResponse response;
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		map<uint32_t, uint32_t> seen = m_player->GetNumberSeen();
		for (auto& i : seen)
		{
			GetMonstersSeenAndCapturedResponse_SpeciesAndCount* item = response.add_seen();
			item->set_species((uint32_t)i.first);
			item->set_count(i.second);
		}

		map<uint32_t, uint32_t> captured = m_player->GetNumberCaptured();
		for (auto& i : captured)
		{
			GetMonstersSeenAndCapturedResponse_SpeciesAndCount* item = response.add_captured();
			item->set_species((uint32_t)i.first);
			item->set_count(i.second);
		}
	});
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::GetTreats()
{
	GetTreatsResponse response;
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		map<uint32_t, uint32_t> treats = m_player->GetTreats();
		for (auto& i : treats)
		{
			GetTreatsResponse_SpeciesAndCount* item = response.add_treats();
			item->set_species((uint32_t)i.first);
			item->set_count(i.second);
		}
	});
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::GetInventory()
{
	GetInventoryResponse response;
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		map<ItemType, uint32_t> inventory = m_player->GetInventory();
		for (auto& i : inventory)
		{
			GetInventoryResponse_InventoryItem* item = response.add_items();
			item->set_item((uint32_t)i.first);
			item->set_count(i.second);
		}
	});
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::GetMonstersInRange(const string& msg)
{
	GetMonstersInRangeRequest request;
	if (!request.ParseFromString(msg))
		throw SocketException("Bad sightings request format");

	GetMonstersInRangeResponse response;
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		m_player->ReportLocation(request.x(), request.y());

		vector<MonsterSighting> sightings = m_player->GetMonstersInRange();
		for (auto& i : sightings)
		{
			GetMonstersInRangeResponse_MonsterSighting* sighting = response.add_sightings();
			sighting->set_species(i.species->GetIndex());
			sighting->set_x(i.x);
			sighting->set_y(i.y);
		}
	});
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::StartEncounter(const string& msg)
{
	StartEncounterRequest request;
	if (!request.ParseFromString(msg))
		throw SocketException("Bad sightings request format");

	StartEncounterResponse response;
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		m_player->ReportLocation(request.x(), request.y());
		shared_ptr<Monster> monster = m_player->StartWildEncounter(request.x(), request.y());
		if (monster)
		{
			m_encounter = monster;
			response.set_valid(true);
			response.set_species(monster->GetSpecies()->GetIndex());
			response.set_attack(monster->GetAttackIV());
			response.set_defense(monster->GetDefenseIV());
			response.set_stamina(monster->GetStaminaIV());
			response.set_size(monster->GetSize());
			response.set_level(monster->GetLevel());
			response.set_spawntime(monster->GetSpawnTime());
			response.set_quickmove(0);
			response.set_chargemove(0);
		}
		else
		{
			response.set_valid(false);
		}
	});
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::GiveSeed()
{
	GiveSeedResponse response;
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");
		response.set_ok(m_player->GiveSeed());
	});
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::ThrowBall(const string& msg)
{
	ThrowBallRequest request;
	if (!request.ParseFromString(msg))
		throw SocketException("Bad throw request format");

	ThrowBallResponse response;
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		BallThrowResult result;
		if (m_encounter)
			result = m_player->ThrowBall((ItemType)request.ball());
		else
			result = THROW_RESULT_RUN_AWAY_AFTER_ONE;
		switch (result)
		{
		case THROW_RESULT_CATCH:
			response.set_result(ThrowBallResponse_BallThrowResult_THROW_RESULT_CATCH);
			response.set_catchid(m_encounter->GetID());
			break;
		case THROW_RESULT_BREAK_OUT_AFTER_ONE:
			response.set_result(ThrowBallResponse_BallThrowResult_THROW_RESULT_BREAK_OUT_AFTER_ONE);
			break;
		case THROW_RESULT_BREAK_OUT_AFTER_TWO:
			response.set_result(ThrowBallResponse_BallThrowResult_THROW_RESULT_BREAK_OUT_AFTER_TWO);
			break;
		case THROW_RESULT_BREAK_OUT_AFTER_THREE:
			response.set_result(ThrowBallResponse_BallThrowResult_THROW_RESULT_BREAK_OUT_AFTER_THREE);
			break;
		case THROW_RESULT_RUN_AWAY_AFTER_ONE:
			response.set_result(ThrowBallResponse_BallThrowResult_THROW_RESULT_RUN_AWAY_AFTER_ONE);
			break;
		case THROW_RESULT_RUN_AWAY_AFTER_TWO:
			response.set_result(ThrowBallResponse_BallThrowResult_THROW_RESULT_RUN_AWAY_AFTER_TWO);
			break;
		case THROW_RESULT_RUN_AWAY_AFTER_THREE:
			response.set_result(ThrowBallResponse_BallThrowResult_THROW_RESULT_RUN_AWAY_AFTER_THREE);
			break;
		default:
			break;
		}
	});
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::RunFromEncounter()
{
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");
		m_player->RunFromEncounter();
		m_encounter.reset();
	});
	WriteResponse("");
}


void ClientHandler::PowerUpMonster(const string& msg)
{
	PowerUpMonsterRequest request;
	if (!request.ParseFromString(msg))
		throw SocketException("Bad power up request format");

	PowerUpMonsterResponse response;
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		shared_ptr<Monster> monster = m_player->GetMonsterByID(request.id());
		if (monster)
		{
			response.set_ok(m_player->PowerUpMonster(monster));
			response.set_level(monster->GetLevel());
		}
		else
		{
			response.set_ok(false);
		}
	});
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::EvolveMonster(const string& msg)
{
	EvolveMonsterRequest request;
	if (!request.ParseFromString(msg))
		throw SocketException("Bad evolve request format");

	EvolveMonsterResponse response;
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		shared_ptr<Monster> monster = m_player->GetMonsterByID(request.id());
		if (monster)
		{
			response.set_ok(m_player->EvolveMonster(monster));
			response.set_species(monster->GetSpecies()->GetIndex());
			response.set_name(monster->GetName());
			response.set_hp(monster->GetCurrentHP());
			response.set_quickmove(0);
			response.set_chargemove(0);
		}
		else
		{
			response.set_ok(false);
		}
	});
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::TransferMonster(const string& msg)
{
	TransferMonsterRequest request;
	if (!request.ParseFromString(msg))
		throw SocketException("Bad transfer request format");

	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		shared_ptr<Monster> monster = m_player->GetMonsterByID(request.id());
		if (monster)
			m_player->TransferMonster(monster);
	});
	WriteResponse("");
}


void ClientHandler::SetMonsterName(const string& msg)
{
	SetMonsterNameRequest request;
	if (!request.ParseFromString(msg))
		throw SocketException("Bad transfer request format");

	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		bool validName = true;
		if (request.name().size() == 0)
			validName = false;
		if (request.name().size() > 16)
			validName = false;
		for (size_t i = 0; i < request.name().size(); i++)
		{
			if ((request.name()[i] < ' ') || (request.name()[i] > 0x7e))
				validName = false;
		}

		shared_ptr<Monster> monster = m_player->GetMonsterByID(request.id());
		if (monster && validName)
			m_player->SetMonsterName(monster, request.name());
	});
	WriteResponse("");
}


void ClientHandler::GetMapTiles(const string& msg)
{
	GetMapTilesRequest request;
	if (!request.ParseFromString(msg))
		throw SocketException("Bad map tile request format");

	int32_t baseX = request.x() & ~(GRID_SIZE - 1);
	int32_t baseY = request.y() & ~(GRID_SIZE - 1);
	uint8_t data[GRID_SIZE * GRID_SIZE / 2];
	World* world = World::GetWorld();
	memset(data, 0, sizeof(data));
	for (int32_t y = 0; y < GRID_SIZE; y++)
	{
		for (int32_t x = 0; x < GRID_SIZE; x++)
		{
			uint8_t tile = world->GetMapTile(baseX + x, baseY + y) & 0xf;
			data[(y * GRID_SIZE / 2) + (x / 2)] |= tile << (4 * (x & 1));
		}
	}

	GetMapTilesResponse response;
	response.set_data(string((char*)data, GRID_SIZE * GRID_SIZE / 2));
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::GetRecentStops()
{
	GetRecentStopsResponse response;
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		vector<RecentStopVisit> stops = m_player->GetRecentStops();
		time_t cur = time(NULL);
		for (auto& i : stops)
		{
			GetRecentStopsResponse_RecentStopVisit* visit = response.add_stops();
			visit->set_x(i.x);
			visit->set_y(i.y);
			visit->set_t(cur - i.visitTime);
		}
	});
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::GetItemsFromStop(const string& msg)
{
	GetItemsFromStopRequest request;
	if (!request.ParseFromString(msg))
		throw SocketException("Bad items request format");

	GetItemsFromStopResponse response;
	ProcessingThread::Instance()->Process([&]() {
		if (!m_player)
			throw SocketException("No active player");

		map<ItemType, uint32_t> items = m_player->GetItemsFromStop(request.x(), request.y());
		for (auto& i : items)
		{
			GetItemsFromStopResponse_InventoryItem* item = response.add_items();
			item->set_item((uint32_t)i.first);
			item->set_count(i.second);
		}
	});
	WriteResponse(response.SerializeAsString());
}


void ClientHandler::ProcessRequests()
{
	try
	{
		while (true)
		{
			string requestString = ReadRequestPacket();
			Request request;
			if (!request.ParseFromString(requestString))
				throw SocketException("Bad request format");

			switch (request.type())
			{
			case Request_RequestType_Login:
				Login(request.data());
				break;
			case Request_RequestType_Register:
				Register(request.data());
				break;
			case Request_RequestType_GetPlayerDetails:
				GetPlayerDetails();
				break;
			case Request_RequestType_GetMonsterList:
				GetMonsterList();
				break;
			case Request_RequestType_GetMonstersSeenAndCaptured:
				GetMonstersSeenAndCaptured();
				break;
			case Request_RequestType_GetTreats:
				GetTreats();
				break;
			case Request_RequestType_GetInventory:
				GetInventory();
				break;
			case Request_RequestType_GetMonstersInRange:
				GetMonstersInRange(request.data());
				break;
			case Request_RequestType_StartEncounter:
				StartEncounter(request.data());
				break;
			case Request_RequestType_GiveSeed:
				GiveSeed();
				break;
			case Request_RequestType_ThrowBall:
				ThrowBall(request.data());
				break;
			case Request_RequestType_RunFromEncounter:
				RunFromEncounter();
				break;
			case Request_RequestType_PowerUpMonster:
				PowerUpMonster(request.data());
				break;
			case Request_RequestType_EvolveMonster:
				EvolveMonster(request.data());
				break;
			case Request_RequestType_TransferMonster:
				TransferMonster(request.data());
				break;
			case Request_RequestType_SetMonsterName:
				SetMonsterName(request.data());
				break;
			case Request_RequestType_GetMapTiles:
				GetMapTiles(request.data());
				break;
			case Request_RequestType_GetRecentStops:
				GetRecentStops();
				break;
			case Request_RequestType_GetItemsFromStop:
				GetItemsFromStop(request.data());
				break;
			default:
				throw SocketException("Bad request type");
			}
		}
	}
	catch (DisconnectException&)
	{
	}
	catch (exception& e)
	{
		printf("ERROR: %s\n", e.what());
	}
}
