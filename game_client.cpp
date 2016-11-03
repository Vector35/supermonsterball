#include <stdio.h>
#include <openssl/ssl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include "monster.h"
#include "terminal.h"
#include "player.h"
#include "clientsocket.h"
#include "clientrequest.h"

using namespace std;
using namespace request;


int main(int argc, char* argv[])
{
	// Initialize OpenSSL
	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();

	SSL_CTX* context = SSL_CTX_new(TLS_client_method());

	if (SSL_CTX_load_verify_locations(context, "server.crt", NULL) <= 0)
	{
		fprintf(stderr, "Failed to load server certificate.\n");
		return 1;
	}

	SSL_CTX_set_verify(context, SSL_VERIFY_PEER, NULL);

	const char* server = "localhost";
	if (argc > 1)
		server = argv[1];

	hostent* ent = gethostbyname(server);
	if ((!ent) || (ent->h_addrtype != AF_INET))
	{
		fprintf(stderr, "Failed to look up server host (%s).\n", server);
		return 1;
	}

	uint16_t port = SERVER_PORT;
	if (argc > 2)
		port = (uint16_t)atoi(argv[2]);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = *(struct in_addr*)ent->h_addr_list[0];

	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0)
	{
		fprintf(stderr, "Failed to create socket for server connection.\n");
		return 1;
	}

	if (connect(s, (sockaddr*)&addr, sizeof(addr)) < 0)
	{
		fprintf(stderr, "Unable to connect to server.\n");
		return 1;
	}

	int one = 1;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof(int));

	struct timeval timeout;
	timeout.tv_sec = 30;
	timeout.tv_usec = 0;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

	SSL* ssl = SSL_new(context);
	BIO* bio = BIO_new_socket((int)s, BIO_CLOSE);
	SSL_set_bio(ssl, bio, bio);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

	while (true)
	{
		int result = SSL_connect(ssl);
		if (result <= 0)
		{
			int error = SSL_get_error(ssl, result);
			if ((error == SSL_ERROR_WANT_READ) || (error == SSL_ERROR_WANT_WRITE))
				continue;

			SSL_free(ssl);
			fprintf(stderr, "Server SSL handshake was invalid.\n");
			return 1;
		}

		break;
	}

	MonsterSpecies::Init();
	Terminal::Init();

	ClientSocket* sock = new ClientSocket(ssl);
	ClientRequest* requests = new ClientRequest(sock);

	try
	{
		LoginResponse_AccountStatus status = ClientRequest::GetClient()->Login("test", "test");
		printf("%d\n", status);
	}
	catch (exception& e)
	{
		Terminal::TerminalReset();
		fprintf(stderr, "ERROR: %s\n", e.what());
		return 1;
	}

	delete requests;
	return 0;
}
