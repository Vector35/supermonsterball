#pragma once

#include <string>
#include <exception>
#include <openssl/ssl.h>

#ifndef WIN32
#define __noexcept noexcept
#else
#define __noexcept
#endif


class NetworkException : public std::exception
{
	std::string m_message;

public:
	NetworkException() : m_message("Internal error") {}
	NetworkException(const std::string& msg) : m_message(msg) {}
	virtual const char* what() const __noexcept override { return m_message.c_str(); }
};


class DisconnectException : public std::exception
{
	std::string m_message;

public:
	DisconnectException() : m_message("Connection to server lost") {}
	DisconnectException(const std::string& msg) : m_message(msg) {}
	virtual const char* what() const __noexcept override{ return m_message.c_str(); }
};


class ClientSocket
{
protected:
	SSL* m_ssl;
	std::string m_lastErrorMessage;

public:
	ClientSocket(SSL* ssl);
	~ClientSocket();

	bool Read(void* buf, size_t len);
	bool Write(const void* buf, size_t len);

	void ReadChecked(void* buf, size_t len);
	uint8_t Read8();
	uint16_t Read16();
	uint32_t Read32();
	uint64_t Read64();
	std::string ReadString();

	const std::string& GetLastErrorMessage() const { return m_lastErrorMessage; }
};
