#pragma once

#include "serversocket.h"
#include "serverplayer.h"
#include <mutex>

class ClientHandler
{
	SSLSocket* m_ssl;
	std::shared_ptr<ServerPlayer> m_player;

	static std::map<uint64_t, std::shared_ptr<ServerPlayer>> m_playerCache;
	static std::mutex m_playerCacheMutex;

	std::string ReadRequestPacket();
	void WriteResponse(const std::string& msg);

	void Login(const std::string& msg);
	void Register(const std::string& msg);

public:
	ClientHandler(SSLSocket* s);
	~ClientHandler();

	void ProcessRequests();
};
