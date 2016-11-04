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
#include "clientplayer.h"
#include "client.h"

using namespace std;
using namespace request;


static Player* ShowLoginPage()
{
	Terminal* term = Terminal::GetTerminal();
	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t width = 40;
	size_t height = 6;
	size_t x = (centerX - (width / 2)) | 1;
	size_t y = centerY - (height / 2);
	DrawBox(x - 1, y - 1, width + 2, height + 2, 234);

	term->BeginOutputQueue();
	term->SetColor(255, 234);

	term->SetCursorPosition(centerX - (strlen("Login") / 2), y + 1);
	term->Output("Login");

	term->SetCursorPosition(x + 1, y + 3);
	term->Output("Username:");
	term->SetCursorPosition(x + 1, y + 4);
	term->Output("Password:");

	string username = InputString(x + 11, y + 3, 16, 255, 234, "");
	if (username == "")
		return nullptr;
	string password = InputString(x + 11, y + 4, 16, 255, 234, "", true);
	if (password == "")
		return nullptr;

	LoginResponse_AccountStatus status = ClientRequest::GetClient()->Login(username, password);
	if (status == LoginResponse_AccountStatus_LoginOK)
		return new ClientPlayer(username);

	return nullptr;
}


static Player* ShowRegisterPage()
{
	Terminal* term = Terminal::GetTerminal();
	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t width = 40;
	size_t height = 7;
	size_t x = (centerX - (width / 2)) | 1;
	size_t y = centerY - (height / 2);
	DrawBox(x - 1, y - 1, width + 2, height + 2, 234);

	term->BeginOutputQueue();
	term->SetColor(255, 234);

	term->SetCursorPosition(centerX - (strlen("Register") / 2), y + 1);
	term->Output("Register");

	term->SetCursorPosition(x + 1, y + 3);
	term->Output("Username:");
	term->SetCursorPosition(x + 1, y + 4);
	term->Output("Password:");
	term->SetCursorPosition(x + 1, y + 5);
	term->Output("Password:");

	string username = InputString(x + 11, y + 3, 16, 255, 234, "");
	if (username == "")
		return nullptr;
	string password = InputString(x + 11, y + 4, 16, 255, 234, "", true);
	if (password == "")
		return nullptr;
	string passwordRepeat = InputString(x + 11, y + 5, 16, 255, 234, "", true);
	if (passwordRepeat == "")
		return nullptr;

	if (password != passwordRepeat)
	{
		return nullptr;
	}

	RegisterResponse_RegisterStatus status = ClientRequest::GetClient()->Register(username, password);
	if (status == RegisterResponse_RegisterStatus_RegisterOK)
		return new ClientPlayer(username);

	return nullptr;
}


static void DrawStartupOptions(size_t x, size_t y, size_t width, const vector<string>& options, int32_t selected)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();

	for (size_t i = 0; i < options.size(); i++)
	{
		if ((int32_t)i == selected)
			term->SetColor(234, 255);
		else
			term->SetColor(255, 234);

		term->SetCursorPosition(x, y + i);
		for (size_t dx = 0; dx < width; dx++)
			term->Output(" ");

		term->SetCursorPosition(x + 1, y + i);
		term->Output(options[i]);
	}

	term->EndOutputQueue();
}


static int32_t ShowStartupOptions(size_t width, const vector<string>& options)
{
	Terminal* term = Terminal::GetTerminal();
	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t height = options.size();
	size_t x = (centerX - (width / 2)) | 1;
	size_t y = centerY - (height / 2);
	DrawBox(x - 1, y - 1, width + 2, height + 2, 234);

	int32_t selected = 0;
	while (true)
	{
		if (term->HasQuit())
		{
			selected = -1;
			break;
		}

		DrawStartupOptions(x, y, width, options, selected);

		string input = term->GetInput();
		if (term->IsInputUpMovement(input))
		{
			selected--;
			if (selected < 0)
				selected = (int32_t)(options.size() - 1);
		}
		else if (term->IsInputDownMovement(input))
		{
			selected++;
			if (selected >= (int32_t)options.size())
				selected = 0;
		}
		else if ((input == "q") || (input == "Q") || (input == "\033"))
		{
			selected = -1;
			break;
		}
		else if ((input == "\n") || (input == "\r") || (input == " ") || (input == "e") || (input == "E"))
		{
			break;
		}
	}

	return selected;
}


static Player* ShowStartupMenu()
{
	while (true)
	{
		int32_t option = ShowStartupOptions(20, vector<string>{"Login", "New Account", "Quit Game"});
		if ((option == -1) || (option == 2))
			return nullptr;

		if (option == 0)
		{
			Player* player = ShowLoginPage();
			if (player)
				return player;
		}

		if (option == 1)
		{
			Player* player = ShowRegisterPage();
			if (player)
				return player;
		}
	}
}


static void StartGameClient()
{
	Terminal::GetTerminal()->HideCursor();
	Player* player = ShowStartupMenu();
}


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
		StartGameClient();
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
