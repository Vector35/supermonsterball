#pragma once

#include "serversocket.h"
#include "serverplayer.h"
#include <mutex>

class ClientHandler
{
	SSLSocket* m_ssl;
	std::shared_ptr<ServerPlayer> m_player;
	std::shared_ptr<Monster> m_encounter;

	static std::map<uint64_t, std::shared_ptr<ServerPlayer>> m_playerCache;
	static std::mutex m_playerCacheMutex;

	std::string ReadRequestPacket();
	void WriteResponse(const std::string& msg);

	void Login(const std::string& msg);
	void Register(const std::string& msg);
	void GetPlayerDetails();
	void GetMonsterList();
	void GetMonstersSeenAndCaptured();
	void GetTreats();
	void GetInventory();
	void GetMonstersInRange(const std::string& msg);
	void StartEncounter(const std::string& msg);
	void GiveSeed();
	void ThrowBall(const std::string& msg);
	void RunFromEncounter();
	void PowerUpMonster(const std::string& msg);
	void EvolveMonster(const std::string& msg);
	void TransferMonster(const std::string& msg);
	void SetMonsterName(const std::string& msg);
	void GetMapTiles(const std::string& msg);
	void GetRecentStops();
	void GetItemsFromStop(const std::string& msg);
	void SetTeam(const std::string& msg);
	void GetPitStatus(const std::string& msg);
	void AssignPitDefender(const std::string& msg);

public:
	ClientHandler(SSLSocket* s);
	~ClientHandler();

	void ProcessRequests();

	static std::shared_ptr<ServerPlayer> GetPlayerByID(uint64_t id);
};
