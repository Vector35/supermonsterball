#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include "client.h"
#include "map.h"

using namespace std;


void GameLoop(Player* player)
{
	Terminal* term = Terminal::GetTerminal();
	term->ClearScreen();
	term->HideCursor();

	MapRenderer map(player);
	map.SetCenterLocation(player->GetLastLocationX(), player->GetLastLocationY());
	map.SetMonsters(player->GetMonstersInRange());
	map.Paint();

	time_t lastForcedRefresh = time(NULL);

	while (!term->HasQuit())
	{
		if (term->HasSizeChanged())
		{
			term->UpdateWindowSize();
			map.SetCenterLocation(player->GetLastLocationX(), player->GetLastLocationY());
			map.Paint();
		}

		if (lastForcedRefresh != time(NULL))
		{
			lastForcedRefresh = time(NULL);
			map.SetMonsters(player->GetMonstersInRange());
			map.Paint();
		}

		string input = term->GetInput();
		if (input.size() == 0)
			continue;
		if ((input == "q") || (input == "Q"))
			break;

		if (input == "\033[A")
		{
			int32_t x = player->GetLastLocationX();
			int32_t y = player->GetLastLocationY() - 1;
			player->ReportLocation(x, y);
			map.EnsurePlayerVisible();
			map.Paint();
		}
		if (input == "\033[B")
		{
			int32_t x = player->GetLastLocationX();
			int32_t y = player->GetLastLocationY() + 1;
			player->ReportLocation(x, y);
			map.EnsurePlayerVisible();
			map.Paint();
		}
		if (input == "\033[C")
		{
			int32_t x = player->GetLastLocationX() + 1;
			int32_t y = player->GetLastLocationY();
			player->ReportLocation(x, y);
			map.EnsurePlayerVisible();
			map.Paint();
		}
		if (input == "\033[D")
		{
			int32_t x = player->GetLastLocationX() - 1;
			int32_t y = player->GetLastLocationY();
			player->ReportLocation(x, y);
			map.EnsurePlayerVisible();
			map.Paint();
		}
	}
}
