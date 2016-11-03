#pragma once

#include <openssl/ssl.h>
#include <exception>
#include <string>


class SocketException: public std::exception
{
	std::string m_message;

public:
	SocketException(const std::string& msg): m_message(msg) {}
	virtual const char* what() const noexcept override { return m_message.c_str(); }
};


class DisconnectException: public std::exception
{
	std::string m_message;

public:
	DisconnectException(const std::string& msg): m_message(msg) {}
	virtual const char* what() const noexcept override { return m_message.c_str(); }
};


class SSLSocket
{
	SSL* m_ssl;
	BIO* m_bio;
	std::string m_lastErrorMessage;

public:
	SSLSocket(SSL* ssl, int s);
	~SSLSocket();

	bool Handshake();

	bool Read(void* buf, size_t len);
	bool Write(const void* buf, size_t len);

	uint8_t Read8();
	uint16_t Read16();
	uint32_t Read32();
	uint64_t Read64();
	std::string ReadString();
	float ReadFloat();
};
