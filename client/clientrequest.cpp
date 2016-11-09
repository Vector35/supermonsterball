#include <time.h>
#include "clientrequest.h"
#include "world.h"

using namespace std;
using namespace request;


ClientRequest* ClientRequest::m_requests = nullptr;


ClientRequest::ClientRequest(ClientSocket* ssl): m_ssl(ssl)
{
	m_requests = this;
	m_id = 0;
}


ClientRequest::~ClientRequest()
{
}


void ClientRequest::WriteRequest(request::Request_RequestType type, const string& msg)
{
	if (msg.size() > 0xffff)
		throw NetworkException("Request too large");

	Request request;
	request.set_type(type);
	request.set_data(msg);
	string serialized = request.SerializeAsString();

	uint16_t len = (uint16_t)serialized.size();
	string buf((char*)&len, sizeof(len));
	buf += serialized;
	m_ssl->Write(buf.c_str(), buf.size());
}


string ClientRequest::ReadResponse()
{
	uint32_t len = m_ssl->Read32();
	char* data = new char[len];
	if (!m_ssl->Read(data, len))
	{
		delete[] data;
		throw NetworkException("Incomplete response");
	}
	string result(data, len);
	delete[] data;
	return result;
}


LoginResponse_AccountStatus ClientRequest::Login(const string username, const string& password)
{
	LoginRequest request;
	request.set_username(username);
	request.set_password(password);
	WriteRequest(Request_RequestType_Login, request.SerializeAsString());

	LoginResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid login response");
	if (response.status() == LoginResponse_AccountStatus_LoginOK)
	{
		m_id = response.id();
		m_name = username;
	}
	return response.status();
}


RegisterResponse_RegisterStatus ClientRequest::Register(const string username, const string& password)
{
	RegisterRequest request;
	request.set_username(username);
	request.set_password(password);
	WriteRequest(Request_RequestType_Register, request.SerializeAsString());

	RegisterResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid login response");
	if (response.status() == RegisterResponse_RegisterStatus_RegisterOK)
	{
		m_id = response.id();
		m_name = username;
	}
	return response.status();
}


GetPlayerDetailsResponse ClientRequest::GetPlayerDetails()
{
	WriteRequest(Request_RequestType_GetPlayerDetails, "");
	GetPlayerDetailsResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid player details response");
	return response;
}


vector<shared_ptr<Monster>> ClientRequest::GetMonsterList()
{
	WriteRequest(Request_RequestType_GetMonsterList, "");
	GetMonsterListResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid monster list response");

	vector<shared_ptr<Monster>> result;
	for (int i = 0; i < response.monsters_size(); i++)
	{
		shared_ptr<Monster> monster(new Monster(MonsterSpecies::GetByIndex(response.monsters(i).species()),
			response.monsters(i).x(), response.monsters(i).y(), response.monsters(i).spawntime()));
		monster->SetID(response.monsters(i).id());
		monster->SetOwner(m_id, m_name);
		monster->SetName(response.monsters(i).name());
		monster->SetHP(response.monsters(i).hp());
		monster->SetIV(response.monsters(i).attack(), response.monsters(i).defense(), response.monsters(i).stamina());
		monster->SetSize(response.monsters(i).size());
		monster->SetLevel(response.monsters(i).level());
		monster->SetCapture(true, (ItemType)response.monsters(i).ball());
		monster->SetMoves(Move::GetByIndex(response.monsters(i).quickmove()),
			Move::GetByIndex(response.monsters(i).chargemove()));
		monster->SetDefending(response.monsters(i).defending());
		result.push_back(monster);
	}
	return result;
}


GetMonstersSeenAndCapturedResponse ClientRequest::GetMonstersSeenAndCaptured()
{
	WriteRequest(Request_RequestType_GetMonstersSeenAndCaptured, "");
	GetMonstersSeenAndCapturedResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid seen and captured response");
	return response;
}


map<uint32_t, uint32_t> ClientRequest::GetTreats()
{
	WriteRequest(Request_RequestType_GetTreats, "");
	GetTreatsResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid treats response");

	map<uint32_t, uint32_t> result;
	for (int i = 0; i < response.treats_size(); i++)
		result[response.treats(i).species()] = response.treats(i).count();
	return result;
}


map<ItemType, uint32_t> ClientRequest::GetInventory()
{
	WriteRequest(Request_RequestType_GetInventory, "");
	GetInventoryResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid inventory response");

	map<ItemType, uint32_t> result;
	for (int i = 0; i < response.items_size(); i++)
		result[(ItemType)response.items(i).item()] = response.items(i).count();
	return result;
}


vector<MonsterSighting> ClientRequest::GetMonstersInRange(int32_t x, int32_t y)
{
	GetMonstersInRangeRequest request;
	request.set_x(x);
	request.set_y(y);
	WriteRequest(Request_RequestType_GetMonstersInRange, request.SerializeAsString());

	GetMonstersInRangeResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid sightings response");

	vector<MonsterSighting> result;
	for (int i = 0; i < response.sightings_size(); i++)
	{
		MonsterSighting sighting;
		sighting.species = MonsterSpecies::GetByIndex(response.sightings(i).species());
		sighting.x = response.sightings(i).x();
		sighting.y = response.sightings(i).y();
		result.push_back(sighting);
	}
	return result;
}


shared_ptr<Monster> ClientRequest::StartEncounter(int32_t x, int32_t y)
{
	StartEncounterRequest request;
	request.set_x(x);
	request.set_y(y);
	WriteRequest(Request_RequestType_StartEncounter, request.SerializeAsString());

	StartEncounterResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid encounter response");
	if (!response.valid())
		return nullptr;

	shared_ptr<Monster> monster(new Monster(MonsterSpecies::GetByIndex(response.species()), x, y,
		response.spawntime()));
	monster->SetOwner(m_id, m_name);
	monster->SetIV(response.attack(), response.defense(), response.stamina());
	monster->SetSize(response.size());
	monster->SetLevel(response.level());
	monster->SetMoves(Move::GetByIndex(response.quickmove()), Move::GetByIndex(response.chargemove()));
	monster->ResetHP();
	return monster;
}


bool ClientRequest::GiveSeed()
{
	WriteRequest(Request_RequestType_GiveSeed, "");

	GiveSeedResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid seed response");
	return response.ok();
}


BallThrowResult ClientRequest::ThrowBall(ItemType ball, std::shared_ptr<Monster> monster)
{
	ThrowBallRequest request;
	request.set_ball((uint32_t)ball);
	WriteRequest(Request_RequestType_ThrowBall, request.SerializeAsString());

	ThrowBallResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid ball throw response");

	switch (response.result())
	{
	case ThrowBallResponse_BallThrowResult_THROW_RESULT_CATCH:
		monster->SetID(response.catchid());
		monster->SetCapture(true, ball);
		return THROW_RESULT_CATCH;
	case ThrowBallResponse_BallThrowResult_THROW_RESULT_BREAK_OUT_AFTER_ONE:
		return THROW_RESULT_BREAK_OUT_AFTER_ONE;
	case ThrowBallResponse_BallThrowResult_THROW_RESULT_BREAK_OUT_AFTER_TWO:
		return THROW_RESULT_BREAK_OUT_AFTER_TWO;
	case ThrowBallResponse_BallThrowResult_THROW_RESULT_BREAK_OUT_AFTER_THREE:
		return THROW_RESULT_BREAK_OUT_AFTER_THREE;
	case ThrowBallResponse_BallThrowResult_THROW_RESULT_RUN_AWAY_AFTER_ONE:
		return THROW_RESULT_RUN_AWAY_AFTER_ONE;
	case ThrowBallResponse_BallThrowResult_THROW_RESULT_RUN_AWAY_AFTER_TWO:
		return THROW_RESULT_RUN_AWAY_AFTER_TWO;
	case ThrowBallResponse_BallThrowResult_THROW_RESULT_RUN_AWAY_AFTER_THREE:
		return THROW_RESULT_RUN_AWAY_AFTER_THREE;
	default:
		throw NetworkException("Invalid ball throw response");
	}
}


void ClientRequest::RunFromEncounter()
{
	WriteRequest(Request_RequestType_RunFromEncounter, "");
	ReadResponse();
}


bool ClientRequest::PowerUpMonster(shared_ptr<Monster> monster)
{
	PowerUpMonsterRequest request;
	request.set_id(monster->GetID());
	WriteRequest(Request_RequestType_PowerUpMonster, request.SerializeAsString());

	PowerUpMonsterResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid power up response");

	if (!response.ok())
		return false;

	monster->SetLevel(response.level());
	return true;
}


bool ClientRequest::EvolveMonster(shared_ptr<Monster> monster)
{
	EvolveMonsterRequest request;
	request.set_id(monster->GetID());
	WriteRequest(Request_RequestType_EvolveMonster, request.SerializeAsString());

	EvolveMonsterResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid evolve response");

	if (!response.ok())
		return false;

	monster->SetSpecies(MonsterSpecies::GetByIndex(response.species()));
	monster->SetName(response.name());
	monster->SetHP(response.hp());
	monster->SetMoves(Move::GetByIndex(response.quickmove()), Move::GetByIndex(response.chargemove()));
	return true;
}


void ClientRequest::TransferMonster(shared_ptr<Monster> monster)
{
	TransferMonsterRequest request;
	request.set_id(monster->GetID());
	WriteRequest(Request_RequestType_TransferMonster, request.SerializeAsString());
	ReadResponse();
}


void ClientRequest::SetMonsterName(shared_ptr<Monster> monster, const string& name)
{
	SetMonsterNameRequest request;
	request.set_id(monster->GetID());
	request.set_name(name);
	WriteRequest(Request_RequestType_SetMonsterName, request.SerializeAsString());
	ReadResponse();
}


void ClientRequest::GetMapTiles(int32_t x, int32_t y, uint8_t* data)
{
	GetMapTilesRequest request;
	request.set_x(x);
	request.set_y(y);
	WriteRequest(Request_RequestType_GetMapTiles, request.SerializeAsString());

	GetMapTilesResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid map tile response");
	if (response.data().size() != (GRID_SIZE * GRID_SIZE / 2))
		throw NetworkException("Invalid map tile response");

	for (int32_t dy = 0; dy < GRID_SIZE; dy++)
		for (int32_t dx = 0; dx < GRID_SIZE; dx++)
			data[(dy * GRID_SIZE) + dx] = (response.data()[(dy * GRID_SIZE / 2) + (dx / 2)] >> (4 * (dx & 1))) & 0xf;
}


vector<RecentStopVisit> ClientRequest::GetRecentStops()
{
	WriteRequest(Request_RequestType_GetRecentStops, "");

	GetRecentStopsResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid recent stops response");

	vector<RecentStopVisit> result;
	time_t cur = time(NULL);
	for (int i = 0; i < response.stops_size(); i++)
	{
		RecentStopVisit visit;
		visit.x = response.stops(i).x();
		visit.y = response.stops(i).y();
		visit.visitTime = cur - response.stops(i).t();
		result.push_back(visit);
	}
	return result;
}


map<ItemType, uint32_t> ClientRequest::GetItemsFromStop(int32_t x, int32_t y)
{
	GetItemsFromStopRequest request;
	request.set_x(x);
	request.set_y(y);
	WriteRequest(Request_RequestType_GetItemsFromStop, request.SerializeAsString());

	GetItemsFromStopResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid items response");

	map<ItemType, uint32_t> result;
	for (int i = 0; i < response.items_size(); i++)
		result[(ItemType)response.items(i).item()] = response.items(i).count();
	return result;
}


void ClientRequest::SetTeam(Team team)
{
	SetTeamRequest request;
	switch (team)
	{
	case TEAM_RED:
		request.set_team(SetTeamRequest_Team_TEAM_RED);
		break;
	case TEAM_BLUE:
		request.set_team(SetTeamRequest_Team_TEAM_BLUE);
		break;
	case TEAM_YELLOW:
		request.set_team(SetTeamRequest_Team_TEAM_YELLOW);
		break;
	default:
		return;
	}
	WriteRequest(Request_RequestType_SetTeam, request.SerializeAsString());
	ReadResponse();
}


PitStatus ClientRequest::GetPitStatus(int32_t x, int32_t y)
{
	GetPitStatusRequest request;
	request.set_x(x);
	request.set_y(y);
	WriteRequest(Request_RequestType_GetPitStatus, request.SerializeAsString());

	GetPitStatusResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid pit status response");

	PitStatus status;
	switch (response.team())
	{
	case GetPitStatusResponse_Team_TEAM_RED:
		status.team = TEAM_RED;
		break;
	case GetPitStatusResponse_Team_TEAM_BLUE:
		status.team = TEAM_BLUE;
		break;
	case GetPitStatusResponse_Team_TEAM_YELLOW:
		status.team = TEAM_YELLOW;
		break;
	default:
		status.team = TEAM_UNASSIGNED;
		break;
	}

	status.reputation = response.reputation();

	for (int i = 0; i < response.defenders_size(); i++)
	{
		shared_ptr<Monster> monster(new Monster(MonsterSpecies::GetByIndex(response.defenders(i).species()),
			0, 0, 0));
		monster->SetID(response.defenders(i).id());
		monster->SetIV(response.defenders(i).attack(), response.defenders(i).defense(),
			response.defenders(i).stamina());
		monster->SetSize(response.defenders(i).size());
		monster->SetLevel(response.defenders(i).level());
		monster->SetName(response.defenders(i).name());
		monster->SetHP(response.defenders(i).hp());
		monster->SetMoves(Move::GetByIndex(response.defenders(i).quickmove()),
			Move::GetByIndex(response.defenders(i).chargemove()));
		monster->SetOwner(response.defenders(i).owner(), response.defenders(i).ownername());
		monster->SetDefending(true);
		status.defenders.push_back(monster);
	}
	return status;
}


bool ClientRequest::AssignPitDefender(int32_t x, int32_t y, shared_ptr<Monster> monster)
{
	AssignPitDefenderRequest request;
	request.set_x(x);
	request.set_y(y);
	request.set_id(monster->GetID());
	WriteRequest(Request_RequestType_AssignPitDefender, request.SerializeAsString());

	AssignPitDefenderResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid pit assign response");
	return response.ok();
}


bool ClientRequest::StartPitBattle(int32_t x, int32_t y, vector<shared_ptr<Monster>> monsters,
	vector<shared_ptr<Monster>>& defenders)
{
	StartPitBattleRequest request;
	request.set_x(x);
	request.set_y(y);
	for (auto& i : monsters)
		request.add_monsters(i->GetID());
	WriteRequest(Request_RequestType_StartPitBattle, request.SerializeAsString());

	StartPitBattleResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid pit battle response");

	if (response.ok())
	{
		defenders.clear();
		for (int i = 0; i < response.defenders_size(); i++)
		{
			shared_ptr<Monster> monster(new Monster(MonsterSpecies::GetByIndex(response.defenders(i).species()),
				0, 0, 0));
			monster->SetID(response.defenders(i).id());
			monster->SetIV(response.defenders(i).attack(), response.defenders(i).defense(),
				response.defenders(i).stamina());
			monster->SetSize(response.defenders(i).size());
			monster->SetLevel(response.defenders(i).level());
			monster->SetName(response.defenders(i).name());
			monster->SetHP(response.defenders(i).hp());
			monster->SetMoves(Move::GetByIndex(response.defenders(i).quickmove()),
				Move::GetByIndex(response.defenders(i).chargemove()));
			monster->SetOwner(response.defenders(i).owner(), response.defenders(i).ownername());
			monster->SetDefending(true);
			defenders.push_back(monster);
		}
	}

	return response.ok();
}


void ClientRequest::SetAttacker(shared_ptr<Monster> monster)
{
	SetAttackerRequest request;
	request.set_monster(monster->GetID());
	WriteRequest(Request_RequestType_SetAttacker, request.SerializeAsString());
	ReadResponse();
}


PitBattleStatus ClientRequest::StepPitBattle(vector<shared_ptr<Monster>> defenders)
{
	WriteRequest(Request_RequestType_StepPitBattle, "");

	StepPitBattleResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid step pit battle response");

	PitBattleStatus status;
	switch (response.state())
	{
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_ATTACK_QUICK_MOVE_NOT_EFFECTIVE:
		status.state = PIT_BATTLE_ATTACK_QUICK_MOVE_NOT_EFFECTIVE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_ATTACK_QUICK_MOVE_EFFECTIVE:
		status.state = PIT_BATTLE_ATTACK_QUICK_MOVE_EFFECTIVE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_ATTACK_QUICK_MOVE_SUPER_EFFECTIVE:
		status.state = PIT_BATTLE_ATTACK_QUICK_MOVE_SUPER_EFFECTIVE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_ATTACK_CHARGE_MOVE_NOT_EFFECTIVE:
		status.state = PIT_BATTLE_ATTACK_CHARGE_MOVE_NOT_EFFECTIVE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_ATTACK_CHARGE_MOVE_EFFECTIVE:
		status.state = PIT_BATTLE_ATTACK_CHARGE_MOVE_EFFECTIVE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_ATTACK_CHARGE_MOVE_SUPER_EFFECTIVE:
		status.state = PIT_BATTLE_ATTACK_CHARGE_MOVE_SUPER_EFFECTIVE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_DEFEND_QUICK_MOVE_NOT_EFFECTIVE:
		status.state = PIT_BATTLE_DEFEND_QUICK_MOVE_NOT_EFFECTIVE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_DEFEND_QUICK_MOVE_EFFECTIVE:
		status.state = PIT_BATTLE_DEFEND_QUICK_MOVE_EFFECTIVE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_DEFEND_QUICK_MOVE_SUPER_EFFECTIVE:
		status.state = PIT_BATTLE_DEFEND_QUICK_MOVE_SUPER_EFFECTIVE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_DEFEND_QUICK_MOVE_DODGE:
		status.state = PIT_BATTLE_DEFEND_QUICK_MOVE_DODGE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_DEFEND_CHARGE_MOVE_NOT_EFFECTIVE:
		status.state = PIT_BATTLE_DEFEND_CHARGE_MOVE_NOT_EFFECTIVE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_DEFEND_CHARGE_MOVE_EFFECTIVE:
		status.state = PIT_BATTLE_DEFEND_CHARGE_MOVE_EFFECTIVE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_DEFEND_CHARGE_MOVE_SUPER_EFFECTIVE:
		status.state = PIT_BATTLE_DEFEND_CHARGE_MOVE_SUPER_EFFECTIVE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_DEFEND_CHARGE_MOVE_DODGE:
		status.state = PIT_BATTLE_DEFEND_CHARGE_MOVE_DODGE;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_ATTACK_FAINT:
		status.state = PIT_BATTLE_ATTACK_FAINT;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_DEFEND_FAINT:
		status.state = PIT_BATTLE_DEFEND_FAINT;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_NEW_OPPONENT:
		status.state = PIT_BATTLE_NEW_OPPONENT;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_WIN:
		status.state = PIT_BATTLE_WIN;
		break;
	case StepPitBattleResponse_PitBattleState_PIT_BATTLE_LOSE:
		status.state = PIT_BATTLE_LOSE;
		break;
	default:
		status.state = PIT_BATTLE_WAITING_FOR_ACTION;
		break;
	}

	status.charge = response.charge();
	status.attackerHP = response.attackerhp();
	status.defenderHP = response.defenderhp();

	for (auto& i : defenders)
	{
		if (i->GetID() == response.opponent())
		{
			status.opponent = i;
			break;
		}
	}

	return status;
}


void ClientRequest::SetPitBattleAction(PitBattleAction action)
{
	SetPitBattleActionRequest request;
	switch (action)
	{
	case PIT_ACTION_ATTACK_QUICK_MOVE:
		request.set_action(SetPitBattleActionRequest_PitBattleAction_PIT_ACTION_ATTACK_QUICK_MOVE);
		break;
	case PIT_ACTION_ATTACK_CHARGE_MOVE:
		request.set_action(SetPitBattleActionRequest_PitBattleAction_PIT_ACTION_ATTACK_CHARGE_MOVE);
		break;
	case PIT_ACTION_DODGE:
		request.set_action(SetPitBattleActionRequest_PitBattleAction_PIT_ACTION_DODGE);
		break;
	default:
		request.set_action(SetPitBattleActionRequest_PitBattleAction_PIT_ACTION_NOT_CHOSEN);
		break;
	}

	WriteRequest(Request_RequestType_SetPitBattleAction, request.SerializeAsString());
	ReadResponse();
}


uint32_t ClientRequest::EndPitBattle()
{
	WriteRequest(Request_RequestType_EndPitBattle, "");

	EndPitBattleResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid end pit battle response");

	return response.reputation();
}


void ClientRequest::HealMonster(shared_ptr<Monster> monster, ItemType item, map<ItemType, uint32_t>& inventory)
{
	HealMonsterRequest request;
	request.set_monster(monster->GetID());
	request.set_item((uint32_t)item);
	WriteRequest(Request_RequestType_HealMonster, request.SerializeAsString());

	HealMonsterResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid heal response");

	monster->SetHP(response.hp());
	inventory[item] = response.count();
}


string ClientRequest::GetLevel40Flag()
{
	WriteRequest(Request_RequestType_GetLevel40Flag, "");

	GetLevel40FlagResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid flag response");

	return response.flag();
}


string ClientRequest::GetCatchEmAllFlag()
{
	WriteRequest(Request_RequestType_GetCatchEmAllFlag, "");

	GetCatchEmAllFlagResponse response;
	if (!response.ParseFromString(ReadResponse()))
		throw NetworkException("Invalid flag response");

	return response.flag();
}
