#include "clienthandler.h"
#include "request.pb.h"

using namespace std;
using namespace request;


ClientHandler::ClientHandler(SSLSocket* s): m_ssl(s)
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

	LoginResponse response;
	response.set_status(LoginResponse_AccountStatus_LoginOK);
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
			default:
				throw SocketException("Bad request type");
			}
		}
	}
	catch (DisconnectException& e)
	{
		printf("Disconnect: %s\n", e.what());
	}
	catch (exception& e)
	{
		printf("ERROR: %s\n", e.what());
	}
}
