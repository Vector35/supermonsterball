#include "clientsocket.h"
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/err.h>

using namespace std;


ClientSocket::ClientSocket(SSL* ssl) : m_ssl(ssl)
{
}


ClientSocket::~ClientSocket()
{
	SSL_shutdown(m_ssl);
	SSL_free(m_ssl);
}


void ClientSocket::ReadChecked(void* buf, size_t len)
{
	if (!Read(buf, len))
		throw DisconnectException(m_lastErrorMessage);
}


uint8_t ClientSocket::Read8()
{
	uint8_t val;
	if (!Read(&val, sizeof(val)))
		throw DisconnectException(m_lastErrorMessage);
	return val;
}


uint16_t ClientSocket::Read16()
{
	uint16_t val;
	if (!Read(&val, sizeof(val)))
		throw DisconnectException(m_lastErrorMessage);
	return val;
}


uint32_t ClientSocket::Read32()
{
	uint32_t val;
	if (!Read(&val, sizeof(val)))
		throw DisconnectException(m_lastErrorMessage);
	return val;
}


uint64_t ClientSocket::Read64()
{
	uint64_t val;
	if (!Read(&val, sizeof(val)))
		throw DisconnectException(m_lastErrorMessage);
	return val;
}


string ClientSocket::ReadString()
{
	uint16_t len = Read16();

	char* bytes = new char[(size_t)len + 1];
	bytes[len] = 0;
	if (!Read(bytes, len))
		throw DisconnectException(m_lastErrorMessage);

	string result(bytes);
	delete[] bytes;
	return result;
}


bool ClientSocket::Read(void* buf, size_t len)
{
	while (len > 0)
	{
		int result = SSL_read(m_ssl, buf, (int)len);
		if (result <= 0)
		{
			int error = SSL_get_error(m_ssl, result);
			if ((error == SSL_ERROR_WANT_READ) || (error == SSL_ERROR_WANT_WRITE))
				continue;

			if ((error == SSL_ERROR_SYSCALL) || (error == SSL_ERROR_ZERO_RETURN))
			{
				m_lastErrorMessage = "Peer ended connection";
				return false;
			}

			char msg[256];
			ERR_error_string_n(error, msg, 256);
			m_lastErrorMessage = msg;
			return false;
		}

		len -= result;
		buf = (void*)((size_t)buf + result);
	}
	return true;
}


bool ClientSocket::Write(const void* buf, size_t len)
{
	while (len > 0)
	{
		int result = SSL_write(m_ssl, buf, (int)len);
		if (result <= 0)
		{
			int error = SSL_get_error(m_ssl, result);
			if ((error == SSL_ERROR_WANT_READ) || (error == SSL_ERROR_WANT_WRITE))
				continue;

			if ((error == SSL_ERROR_SYSCALL) || (error == SSL_ERROR_ZERO_RETURN))
			{
				m_lastErrorMessage = "Peer ended connection";
				return false;
			}

			char msg[256];
			ERR_error_string_n(error, msg, 256);
			m_lastErrorMessage = msg;
			return false;
		}

		len -= result;
		buf = (const void*)((size_t)buf + result);
	}
	return true;
}
