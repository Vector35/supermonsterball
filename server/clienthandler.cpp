#include "clienthandler.h"
#include "processingthread.h"
#include "database.h"
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
