#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <thread>
#include <sys/types.h>
#include <pthread.h>
#include <openssl/crypto.h>
#include "serversocket.h"
#include "clienthandler.h"
#include "processingthread.h"
#include "player.h"
#include "world.h"
#include "database.h"

using namespace std;


pthread_mutex_t* g_sslMutexes;


static void HandleConnection(SSL* ssl, int s)
{
	SSLSocket sock(ssl, s);
	if (!sock.Handshake())
		return;

	ClientHandler client(&sock);
	client.ProcessRequests();
}


static void ThreadIdCallback(CRYPTO_THREADID* id)
{
	CRYPTO_THREADID_set_pointer(id, (void*)pthread_self());
}


static void LockingCallback(int mode, int n, const char* file, int line)
{
	if (mode & CRYPTO_LOCK)
		pthread_mutex_lock(&g_sslMutexes[n]);
	else
		pthread_mutex_unlock(&g_sslMutexes[n]);
}


int main(int argc, char* argv[])
{
	signal(SIGPIPE, SIG_IGN);
	srand(time(NULL));

	MonsterSpecies::Init();
	Database::Init();
	World::Init(Database::GetDatabase());

	// Initialize OpenSSL
	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();

	g_sslMutexes = new pthread_mutex_t[CRYPTO_num_locks()];
	for (int i = 0; i < CRYPTO_num_locks(); i++)
		pthread_mutex_init(&g_sslMutexes[i], NULL);
	CRYPTO_THREADID_set_callback(ThreadIdCallback);
	CRYPTO_set_locking_callback(LockingCallback);

	// Create TLS only SSL context
	SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());

	// Load SSL certificate
	if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0)
	{
		fprintf(stderr, "Server certificate 'server.crt' is not valid\n");
		return 1;
	}

	if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0)
	{
		fprintf(stderr, "Server private key 'server.key' is not valid\n");
		return 1;
	}

	// Create and bind socket
	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0)
	{
		fprintf(stderr, "Failed to create listen socket\n");
		return 1;
	}

	int one = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(SERVER_PORT);

	if (::bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		fprintf(stderr, "Unable to bind to port %d\n", SERVER_PORT);
		return 1;
	}

	if (listen(s, SOMAXCONN) < 0)
	{
		fprintf(stderr, "Unable to listen on port %d\n", SERVER_PORT);
		return 1;
	}

	// Start connection loop
	ProcessingThread processing;
	while (true)
	{
		int clientSock = accept(s, NULL, NULL);
		if (clientSock < 0)
		{
			printf("[Accept] Error %m\n");
			usleep(100000);
			continue;
		}

		struct timeval timeout;
		timeout.tv_sec = 30;
		timeout.tv_usec = 0;
		setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
		setsockopt(clientSock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

		SSL* ssl = SSL_new(ctx);

		thread t(HandleConnection, ssl, clientSock);
		t.detach();
	}

	return 0;
}
