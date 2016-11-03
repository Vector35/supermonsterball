#include "clientrequest.h"

using namespace std;
using namespace request;


ClientRequest* ClientRequest::m_requests = nullptr;


ClientRequest::ClientRequest(ClientSocket* ssl): m_ssl(ssl)
{
	m_requests = this;
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
	return response.status();
}
