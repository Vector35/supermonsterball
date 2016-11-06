#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <chrono>
#include "client.h"
#include "map.h"

using namespace std;


void DrawBox(size_t x, size_t y, size_t width, size_t height, uint32_t color)
{
	Terminal* term = Terminal::GetTerminal();
	term->SetColor(255, color);

	term->BeginOutputQueue();

	term->SetCursorPosition(x, y);
	term->Output("╭");
	for (size_t dx = 0; dx < (width - 2); dx++)
		term->Output("─");
	term->Output("╮");

	for (size_t dy = 1; dy < (height - 1); dy++)
	{
		term->SetCursorPosition(x, y + dy);
		term->Output("│");
		for (size_t dx = 0; dx < (width - 2); dx++)
			term->Output(" ");
		term->Output("│");
	}

	term->SetCursorPosition(x, y + height - 1);
	term->Output("╰");
	for (size_t dx = 0; dx < (width - 2); dx++)
		term->Output("─");
	term->Output("╯");

	term->EndOutputQueue();
}


void DrawBoxText(size_t x, size_t y, size_t width, size_t height, const string& text)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();

	term->SetCursorPosition(x, y + height - 2);
	term->SetColor(255, 234);
	for (size_t dx = 0; dx < width; dx++)
		term->Output("┄");

	term->SetCursorPosition(x, y + height - 1);
	for (size_t dx = 0; dx < width; dx++)
		term->Output(" ");

	term->SetCursorPosition(x + 1, y + height - 1);
	term->Output(text);

	term->EndOutputQueue();
}


void EraseBoxText(size_t x, size_t y, size_t width, size_t height)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();
	term->SetColor(255, 234);
	for (size_t dy = (height - 2); dy < height; dy++)
	{
		term->SetCursorPosition(x, y + dy);
		for (size_t dx = 0; dx < width; dx++)
			term->Output(" ");
	}
	term->EndOutputQueue();
}


void ShowBoxText(size_t x, size_t y, size_t width, size_t height, const string& text)
{
	DrawBoxText(x, y, width, height, text);
	InterruptableWait(TEXT_WAIT_TIME);
	EraseBoxText(x, y, width, height);
}


static void DrawBoxOptions(size_t x, size_t y, size_t width, size_t height, const vector<string>& options,
	int32_t selected)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();

	term->SetCursorPosition(x, y + height - 2);
	term->SetColor(255, 234);
	for (size_t dx = 0; dx < width; dx++)
		term->Output("┄");

	term->SetCursorPosition(x, y + height - 1);
	for (size_t dx = 0; dx < width; dx++)
		term->Output(" ");

	size_t optionWidth = options.size() - 1;
	for (auto& i : options)
		optionWidth += i.size() + 2;

	term->SetCursorPosition(x + (width / 2) - (optionWidth / 2), y + height - 1);

	for (size_t i = 0; i < options.size(); i++)
	{
		if ((int32_t)i == selected)
			term->SetColor(234, 255);
		else
			term->SetColor(255, 234);
		term->Output(string(" ") + options[i] + string(" "));

		term->SetColor(255, 234);
		term->Output(" ");
	}

	term->EndOutputQueue();
}


int32_t ShowBoxOptions(size_t x, size_t y, size_t width, size_t height, const vector<string>& options)
{
	Terminal* term = Terminal::GetTerminal();

	int32_t selected = 0;
	while (true)
	{
		if (term->HasQuit())
		{
			selected = -1;
			break;
		}

		DrawBoxOptions(x, y, width, height, options, selected);

		string input = term->GetInput();
		if (term->IsInputLeftMovement(input))
		{
			selected--;
			if (selected < 0)
				selected = (int32_t)(options.size() - 1);
		}
		else if (term->IsInputRightMovement(input))
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


string GetItemName(ItemType item)
{
	switch (item)
	{
	case ITEM_STANDARD_BALL:
		return "Standard Ball";
	case ITEM_SUPER_BALL:
		return "Super Ball";
	case ITEM_UBER_BALL:
		return "Über Ball";
	case ITEM_STANDARD_HEAL:
		return "Standard Heal";
	case ITEM_SUPER_HEAL:
		return "Super Heal";
	case ITEM_KEG_OF_HEALTH:
		return "Keg of Health";
	case ITEM_MEGA_SEED:
		return "Mega Seed";
	case ITEM_EGG:
		return "Egg";
	default:
		return "MissingNo";
	}
}


string GetElementName(Element element)
{
	switch (element)
	{
	case Normal:
		return "Normal";
	case Grass:
		return "Grass";
	case Fire:
		return "Fire";
	case Water:
		return "Water";
	case Electric:
		return "Electric";
	case Bug:
		return "Bug";
	case Poison:
		return "Poison";
	case Psychic:
		return "Psychic";
	case Flying:
		return "Flying";
	case Sound:
		return "Sound";
	case Ground:
		return "Ground";
	case Fighting:
		return "Fighting";
	case Ice:
		return "Ice";
	case Light:
		return "Light";
	case Dark:
		return "Dark";
	default:
		return "MissingNo";
	}
}


uint32_t GetElementColor(Element element)
{
	switch (element)
	{
	case Normal:
		return 250;
	case Grass:
		return 41;
	case Fire:
		return 202;
	case Water:
		return 33;
	case Electric:
		return 226;
	case Bug:
		return 148;
	case Poison:
		return 91;
	case Psychic:
		return 207;
	case Flying:
		return 75;
	case Sound:
		return 50;
	case Ground:
		return 136;
	case Fighting:
		return 215;
	case Ice:
		return 159;
	case Light:
		return 229;
	case Dark:
		return 238;
	default:
		return 16;
	}
}


uint32_t GetElementTextColor(Element element)
{
	switch (element)
	{
	case Poison:
	case Ground:
	case Dark:
		return 255;
	default:
		return 16;
	}
}


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

		if ((input == "\033") || (input == "`") || (input == "m") || (input == "M") || (input == " ") ||
			(input == "\r") || (input == "\n") || (input == "e") || (input == "E") ||
			(input == "q") || (input == "Q"))
		{
			if (ShowMainMenu(player, &map))
				break;

			lastForcedRefresh = time(NULL);
			map.SetMonsters(player->GetMonstersInRange());
			map.Paint();
			continue;
		}

		chrono::steady_clock::time_point curTime = chrono::steady_clock::now();
		if (chrono::duration_cast<chrono::milliseconds>(curTime - lastMovement).count() >= MIN_MOVEMENT_INTERVAL)
		{
			if (term->IsInputUpMovement(input))
			{
				int32_t x = player->GetLastLocationX();
				int32_t y = player->GetLastLocationY() - 1;
				if (player->IsStopAvailable(x, y))
					GetAndShowItemsFromStop(player, x, y);
				else if (player->IsMapTileTraversable(x, y))
				{
					lastMovement = chrono::steady_clock::now();
					player->ReportLocation(x, y);
					map.EnsurePlayerVisible();
				}
				map.Paint();
			}
			if (term->IsInputDownMovement(input))
			{
				int32_t x = player->GetLastLocationX();
				int32_t y = player->GetLastLocationY() + 1;
				if (player->IsStopAvailable(x, y))
					GetAndShowItemsFromStop(player, x, y);
				else if (player->IsMapTileTraversable(x, y))
				{
					lastMovement = chrono::steady_clock::now();
					player->ReportLocation(x, y);
					map.EnsurePlayerVisible();
				}
				map.Paint();
			}
			if (term->IsInputLeftMovement(input))
			{
				int32_t x = player->GetLastLocationX() - 1;
				int32_t y = player->GetLastLocationY();
				if (player->IsStopAvailable(x, y))
					GetAndShowItemsFromStop(player, x, y);
				else if (player->IsMapTileTraversable(x, y))
				{
					lastMovement = chrono::steady_clock::now();
					player->ReportLocation(x, y);
					map.EnsurePlayerVisible();
				}
				map.Paint();
			}
			if (term->IsInputRightMovement(input))
			{
				int32_t x = player->GetLastLocationX() + 1;
				int32_t y = player->GetLastLocationY();
				if (player->IsStopAvailable(x, y))
					GetAndShowItemsFromStop(player, x, y);
				else if (player->IsMapTileTraversable(x, y))
				{
					lastMovement = chrono::steady_clock::now();
					player->ReportLocation(x, y);
					map.EnsurePlayerVisible();
				}
				map.Paint();
			}
		}

		// Look for collision of player with a sighted monster
		vector<MonsterSighting> sightings = player->GetMonstersInRange();
		int32_t playerX = player->GetLastLocationX();
		int32_t playerY = player->GetLastLocationY();
		for (auto& i : sightings)
		{
			if (i.y != playerY)
				continue;
			if ((i.x != playerX) && ((i.x + 1) != playerX))
				continue;

			// Transition to encounter
			shared_ptr<Monster> caught = Encounter(player, i.x, i.y);

			// Repaint map after encounter
			lastForcedRefresh = time(NULL);
			map.SetMonsters(player->GetMonstersInRange());
			map.Paint();

			if (caught)
			{
				// Caught a monster, show the details
				ShowMonsterDetails(player, caught);

				// Repaint map after details
				lastForcedRefresh = time(NULL);
				map.SetMonsters(player->GetMonstersInRange());
				map.Paint();
			}
			break;
		}
	}
}


void InterruptableWait(uint32_t ms)
{
	Terminal* term = Terminal::GetTerminal();
	chrono::steady_clock::time_point start = chrono::steady_clock::now();
	while (!term->HasQuit())
	{
		string input = term->GetInput();
		if ((input == " ") || (input == "\r") || (input == "\n") || (input == "e") || (input == "E") || (input == "\033"))
			break;

		chrono::steady_clock::time_point cur = chrono::steady_clock::now();
		if (chrono::duration_cast<chrono::milliseconds>(cur - start).count() >= ms)
			break;
	}
}


string InputString(size_t x, size_t y, size_t width, uint32_t foregroundColor, uint32_t backgroundColor,
	const string& defaultString, bool isPassword)
{
	Terminal* term = Terminal::GetTerminal();
	string result = defaultString;

	while (!term->HasQuit())
	{
		term->BeginOutputQueue();
		term->SetCursorPosition(x, y);
		term->SetColor(foregroundColor, backgroundColor);

		if (isPassword)
		{
			string bullets = "";
			for (size_t i = 0; i < result.size(); i++)
				bullets += "•";
			term->Output(bullets);
		}
		else
		{
			term->Output(result);
		}

		term->SetColor(backgroundColor, foregroundColor);
		term->Output(" ");
		term->SetColor(foregroundColor, backgroundColor);
		for (size_t i = result.size(); i < width; i++)
			term->Output(" ");
		term->EndOutputQueue();

		string input = term->GetInput();
		if (input == "\033")
			break;
		else if (input == "\b")
		{
			if (result.size() > 0)
				result = result.substr(0, result.size() - 1);
		}
		else if ((input == "\r") || (input == "\n"))
		{
			term->SetCursorPosition(x + input.size(), y);
			term->Output(" ");
			return result;
		}
		else if ((result.size() < width) && (input.size() == 1) && (input[0] >= ' ') && (input[0] <= 0x7e))
		{
			result += input;
		}
	}

	return defaultString;
}


static void DrawMainMenuOptions(size_t x, size_t y, size_t width, const vector<string>& options, int32_t selected)
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


static int32_t ShowMainMenuOptions(size_t width, const vector<string>& options)
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

		DrawMainMenuOptions(x, y, width, options, selected);

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


bool ShowMainMenu(Player* player, MapRenderer* map)
{
	int32_t option = ShowMainMenuOptions(20, vector<string>{"Continue", "Your Captures", "Monster Index", "Inventory",
		"Quit Game"});
	if ((option == -1) || (option == 0))
		return false;

	map->Paint();

	if (option == 1)
	{
		ShowMonsterList(player, map);
		return false;
	}

	if (option == 2)
	{
		ShowMonsterIndex(player, map);
		return false;
	}

	if (option == 3)
	{
		ShowInventory(player, map);
		return false;
	}

	return true;
}
