#pragma once

#include "serversocket.h"

class ClientHandler
{
	SSLSocket* m_ssl;

	std::string ReadRequestPacket();
	void WriteResponse(const std::string& msg);

	void Login(const std::string& msg);

public:
	ClientHandler(SSLSocket* s);
	~ClientHandler();

	void ProcessRequests();
};
