#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <chrono>
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
	chrono::steady_clock::time_point lastMovement = chrono::steady_clock::now();

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

		chrono::steady_clock::time_point curTime = chrono::steady_clock::now();
		if (chrono::duration_cast<chrono::milliseconds>(curTime - lastMovement).count() >= MIN_MOVEMENT_INTERVAL)
		{
			if (term->IsInputUpMovement(input))
			{
				lastMovement = chrono::steady_clock::now();
				int32_t x = player->GetLastLocationX();
				int32_t y = player->GetLastLocationY() - 1;
				player->ReportLocation(x, y);
				map.EnsurePlayerVisible();
				map.Paint();
			}
			if (term->IsInputDownMovement(input))
			{
				lastMovement = chrono::steady_clock::now();
				int32_t x = player->GetLastLocationX();
				int32_t y = player->GetLastLocationY() + 1;
				player->ReportLocation(x, y);
				map.EnsurePlayerVisible();
				map.Paint();
			}
		}
		if (chrono::duration_cast<chrono::milliseconds>(curTime - lastMovement).count() >= (MIN_MOVEMENT_INTERVAL / 2))
		{
			if (term->IsInputLeftMovement(input))
			{
				lastMovement = chrono::steady_clock::now();
				int32_t x = player->GetLastLocationX() - 1;
				int32_t y = player->GetLastLocationY();
				player->ReportLocation(x, y);
				map.EnsurePlayerVisible();
				map.Paint();
			}
			if (term->IsInputRightMovement(input))
			{
				lastMovement = chrono::steady_clock::now();
				int32_t x = player->GetLastLocationX() + 1;
				int32_t y = player->GetLastLocationY();
				player->ReportLocation(x, y);
				map.EnsurePlayerVisible();
				map.Paint();
			}
		}
	}
}
