#pragma once

#include "clientsocket.h"
#include "request.pb.h"

class ClientRequest
{
	static ClientRequest* m_requests;
	ClientSocket* m_ssl;

	void WriteRequest(request::Request_RequestType type, const std::string& msg);
	std::string ReadResponse();

public:
	ClientRequest(ClientSocket* ssl);
	~ClientRequest();

	static ClientRequest* GetClient() { return m_requests; }

	request::LoginResponse_AccountStatus Login(const std::string username, const std::string& password);
};
