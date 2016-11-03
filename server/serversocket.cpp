#include "serversocket.h"
#include <openssl/err.h>

using namespace std;


SSLSocket::SSLSocket(SSL* ssl, int s): m_ssl(ssl)
{
	m_bio = BIO_new_socket(s, BIO_CLOSE);
	SSL_set_bio(m_ssl, m_bio, m_bio);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
}


SSLSocket::~SSLSocket()
{
	SSL_shutdown(m_ssl);
	SSL_free(m_ssl);
}


bool SSLSocket::Handshake()
{
	while (true)
	{
		int result = SSL_accept(m_ssl);
		if (result <= 0)
		{
			int error = SSL_get_error(m_ssl, result);
			if ((error == SSL_ERROR_WANT_READ) || (error == SSL_ERROR_WANT_WRITE))
				continue;
			return false;
		}
		break;
	}

	return true;
}


bool SSLSocket::Read(void* buf, size_t len)
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


bool SSLSocket::Write(const void* buf, size_t len)
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
			printf("Write failed: %s\n", msg);
			return false;
		}

		len -= result;
		buf = (const void*)((size_t)buf + result);
	}
	return true;
}


uint8_t SSLSocket::Read8()
{
	uint8_t val;
	if (!Read(&val, sizeof(val)))
		throw DisconnectException(m_lastErrorMessage);
	return val;
}


uint16_t SSLSocket::Read16()
{
	uint16_t val;
	if (!Read(&val, sizeof(val)))
		throw DisconnectException(m_lastErrorMessage);
	return val;
}


uint32_t SSLSocket::Read32()
{
	uint32_t val;
	if (!Read(&val, sizeof(val)))
		throw DisconnectException(m_lastErrorMessage);
	return val;
}


uint64_t SSLSocket::Read64()
{
	uint64_t val;
	if (!Read(&val, sizeof(val)))
		throw DisconnectException(m_lastErrorMessage);
	return val;
}


string SSLSocket::ReadString()
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


float SSLSocket::ReadFloat()
{
	float val;
	if (!Read(&val, sizeof(val)))
		throw DisconnectException(m_lastErrorMessage);
	return val;
}
