#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <chrono>
#include "client.h"
#include "map.h"

using namespace std;


static const char* g_encounterGraphics[] =
{
	"      .                      .                . ",
	"               .                     🌕          ",
	"  .                  .          .        .      ",
	"  🌲🌲 🌲 🌳 🌲🌳 🌳 🌲🌳🌳🌲 🌳🌲  🌲 🌳 🌲🌲  🌳🌲🌳🌳🌲  🌲 🌲🌲 🌳 🌲  ",
	"                                                ",
	"                                                ",
	"                                                ",
	"                                                ",
	"                                                ",
	"                                                ",
};


static void DrawBox(size_t x, size_t y, size_t width, size_t height, uint32_t color)
{
	Terminal* term = Terminal::GetTerminal();
	term->SetColor(255, color);

	term->BeginOututQueue();

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

	term->EndOututQueue();
}


static string GetItemName(ItemType item)
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

		// Look for collision of player with a sighted monster
		vector<MonsterSighting> sightings = player->GetMonstersInRange();
		int32_t playerX = player->GetLastLocationX();
		int32_t playerY = player->GetLastLocationY();
		for (auto& i : sightings)
		{
			if (i.y != playerY)
				continue;
			if ((i.x != playerX) && ((i.x + 1) != playerX) && ((i.x + 2) != playerX))
				continue;

			// Transition to encounter
			Encounter(player, i.x, i.y);

			// Repaint map after encounter
			lastForcedRefresh = time(NULL);
			map.SetMonsters(player->GetMonstersInRange());
			map.Paint();
			break;
		}
	}
}


static void DrawEncounter(shared_ptr<Monster> monster, size_t x, size_t y, size_t width, size_t height)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOututQueue();
	for (size_t dy = 0; dy < height; dy++)
	{
		term->SetCursorPosition(x, y + dy);
		if (dy < 4)
			term->SetColor(242, 17);
		else
			term->SetColor(255, 23);
		term->Output(g_encounterGraphics[dy]);
	}
	term->SetCursorPosition(x + (width / 3), y + 6);
	term->Output(monster->GetSpecies()->GetImage());
	term->SetCursorPosition(x + ((width * 2) / 3), y + 6);
	term->Output("🚶");
	term->EndOututQueue();
}


static void DrawEncounterText(size_t x, size_t y, size_t width, size_t height, const string& text)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOututQueue();

	term->SetCursorPosition(x, y + height - 2);
	term->SetColor(255, 234);
	for (size_t dx = 0; dx < width; dx++)
		term->Output("┄");

	term->SetCursorPosition(x, y + height - 1);
	for (size_t dx = 0; dx < width; dx++)
		term->Output(" ");

	term->SetCursorPosition(x + 1, y + height - 1);
	term->Output(text);

	term->EndOututQueue();
}


static void EraseEncounterText(size_t x, size_t y, size_t width, size_t height)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOututQueue();
	for (size_t dy = (height - 2); dy < height; dy++)
	{
		term->SetCursorPosition(x, y + dy);
		if (dy < 4)
			term->SetColor(242, 17);
		else
			term->SetColor(255, 23);
		term->Output(g_encounterGraphics[dy]);
	}
	term->EndOututQueue();
}


static void ShowEncounterText(size_t x, size_t y, size_t width, size_t height, const string& text)
{
	DrawEncounterText(x, y, width, height, text);
	InterruptableWait(TEXT_WAIT_TIME);
	EraseEncounterText(x, y, width, height);
}


static void DrawEncounterOptions(size_t x, size_t y, size_t width, size_t height, const vector<string>& options,
	int32_t selected)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOututQueue();

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

	term->EndOututQueue();
}


static int32_t ShowEncounterOptions(size_t x, size_t y, size_t width, size_t height, const vector<string>& options)
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

		DrawEncounterOptions(x, y, width, height, options, selected);

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

	EraseEncounterText(x, y, width, height);
	return selected;
}


static void DrawEncounterLongOptions(size_t x, size_t y, size_t width, size_t height, const vector<string>& options,
	int32_t selected)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOututQueue();

	term->SetCursorPosition(x, y + height - (options.size() + 1));
	term->SetColor(255, 234);
	for (size_t dx = 0; dx < width; dx++)
		term->Output("┄");

	for (size_t i = 0; i < options.size(); i++)
	{
		if ((int32_t)i == selected)
			term->SetColor(234, 255);
		else
			term->SetColor(255, 234);

		term->SetCursorPosition(x, y + height + i - options.size());
		for (size_t dx = 0; dx < width; dx++)
			term->Output(" ");

		term->SetCursorPosition(x + 1, y + height + i - options.size());
		term->Output(options[i]);
	}

	term->EndOututQueue();
}


static int32_t ShowEncounterLongOptions(size_t x, size_t y, size_t width, size_t height, const vector<string>& options)
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

		DrawEncounterLongOptions(x, y, width, height, options, selected);

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


static void AnimateThrow(size_t x, size_t y, size_t width, size_t height, const std::string& item)
{
	Terminal* term = Terminal::GetTerminal();
	size_t sourceX = x + ((width * 2) / 3);
	size_t sourceY = y + 6;
	size_t destX = x + (width / 3);
	size_t dist = sourceX - destX;

	term->BeginOututQueue();
	term->SetColor(255, 23);

	for (size_t curX = sourceX - 1; curX > (destX + 1); curX--)
	{
		size_t yOffset = 0;
		if (curX > (sourceX - (dist / 3)))
			yOffset = -1;
		else if (curX > (destX + (dist / 3)))
			yOffset = -2;
		else if (curX > (destX + (dist / 6)))
			yOffset = -1;

		term->SetCursorPosition(curX, sourceY + yOffset);
		term->Output(item);
		term->EndOututQueue();

		usleep(66000);

		term->BeginOututQueue();
		term->SetCursorPosition(curX, sourceY + yOffset);
		term->Output(" ");
	}

	term->EndOututQueue();
}


static void AnimateCatchResult(shared_ptr<Monster> monster, size_t x, size_t y, size_t width, size_t height,
	const std::string& item, BallThrowResult result)
{
	Terminal* term = Terminal::GetTerminal();
	size_t monsterY = y + 6;
	size_t monsterX = x + (width / 3);

	term->BeginOututQueue();
	term->SetColor(255, 23);
	term->SetCursorPosition(monsterX, monsterY);
	term->Output(" " + item + " ");
	term->EndOututQueue();

	size_t shakes;
	switch (result)
	{
	case THROW_RESULT_BREAK_OUT_AFTER_ONE:
	case THROW_RESULT_RUN_AWAY_AFTER_ONE:
		shakes = 1;
		break;
	case THROW_RESULT_BREAK_OUT_AFTER_TWO:
	case THROW_RESULT_RUN_AWAY_AFTER_TWO:
		shakes = 2;
		break;
	case THROW_RESULT_BREAK_OUT_AFTER_THREE:
	case THROW_RESULT_RUN_AWAY_AFTER_THREE:
		shakes = 3;
		break;
	default:
		shakes = 3;
		break;
	}

	for (size_t i = 0; i < shakes; i++)
	{
		term->BeginOututQueue();
		term->SetCursorPosition(monsterX, monsterY);
		term->Output(item + "  ");
		term->EndOututQueue();

		usleep(66000);

		term->BeginOututQueue();
		term->SetCursorPosition(monsterX, monsterY);
		term->Output(" " + item + " ");
		term->EndOututQueue();

		usleep(66000);

		term->BeginOututQueue();
		term->SetCursorPosition(monsterX, monsterY);
		term->Output("  " + item);
		term->EndOututQueue();

		usleep(66000);

		term->BeginOututQueue();
		term->SetCursorPosition(monsterX, monsterY);
		term->Output(" " + item + " ");
		term->EndOututQueue();

		usleep(500000);
	}

	if (result == THROW_RESULT_CATCH)
	{
		term->BeginOututQueue();
		term->SetCursorPosition(monsterX - 1, monsterY);
		term->Output(" ✨" + item + "✨ ");
		term->EndOututQueue();

		usleep(125000);

		term->BeginOututQueue();
		term->SetCursorPosition(monsterX - 1, monsterY);
		term->Output("✨ " + item + " ✨");
		term->EndOututQueue();

		usleep(125000);

		term->BeginOututQueue();
		term->SetCursorPosition(monsterX - 1, monsterY);
		term->Output("  " + item + "  ");
		term->EndOututQueue();

		return;
	}

	term->BeginOututQueue();
	term->SetCursorPosition(monsterX, monsterY);
	term->Output(" 💥 ");
	term->EndOututQueue();

	usleep(250000);

	term->BeginOututQueue();
	term->SetCursorPosition(monsterX, monsterY);
	term->Output(monster->GetSpecies()->GetImage());
	term->EndOututQueue();

	sleep(1);

	if ((result == THROW_RESULT_RUN_AWAY_AFTER_ONE) || (result == THROW_RESULT_RUN_AWAY_AFTER_TWO) ||
		(result == THROW_RESULT_RUN_AWAY_AFTER_THREE))
	{
		term->BeginOututQueue();
		term->SetCursorPosition(monsterX, monsterY);
		term->Output(" 💨 ");
		term->EndOututQueue();

		sleep(1);

		term->BeginOututQueue();
		term->SetCursorPosition(monsterX, monsterY);
		term->Output("   ");
		term->EndOututQueue();
		return;
	}
}


void Encounter(Player* player, int32_t x, int32_t y)
{
	Terminal* term = Terminal::GetTerminal();

	// Get the monster object that we are going to encounter
	shared_ptr<Monster> monster = player->StartWildEncounter(x, y);
	if (!monster)
		return;

	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t encounterBoxWidth = strlen(g_encounterGraphics[0]);
	size_t encounterBoxHeight = sizeof(g_encounterGraphics) / sizeof(char*);
	size_t encounterBoxX = centerX - (encounterBoxWidth / 2);
	size_t encounterBoxY = centerY - (encounterBoxHeight / 2);

	DrawBox(encounterBoxX - 1, encounterBoxY - 1, encounterBoxWidth + 2, encounterBoxHeight + 2, 234);
	DrawEncounter(monster, encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight);
	ShowEncounterText(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
		string("A wild ") + monster->GetSpecies()->GetName() + " appeared!");

	ItemType ball = ITEM_STANDARD_BALL;
	while (!term->HasQuit())
	{
		// If player doesn't have any more of the current ball choice, choose a different ball
		if (player->GetItemCount(ball) == 0)
		{
			ball = ITEM_STANDARD_BALL;
			if (player->GetItemCount(ball) == 0)
			{
				ball = ITEM_SUPER_BALL;
				if (player->GetItemCount(ball) == 0)
				{
					ball = ITEM_UBER_BALL;
					if (player->GetItemCount(ball) == 0)
					{
						// Ain't got no balls
						player->RunFromEncounter();
						ShowEncounterText(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
							"You don't have the balls to catch it.");
						return;
					}
				}
			}
		}

		int32_t option = ShowEncounterOptions(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
			vector<string>{string("Throw ") + GetItemName(ball), string("Items"), string("Run Away")});
		if ((option == -1) || (option == 2))
		{
			// Run away
			player->RunFromEncounter();
			ShowEncounterText(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
				"You ran away safely.");
			return;
		}
		else if (option == 1)
		{
			// Choose item
			vector<ItemType> items;
			vector<string> names;
			char nameStr[32];
			if (player->GetItemCount(ITEM_STANDARD_BALL))
			{
				items.push_back(ITEM_STANDARD_BALL);
				sprintf(nameStr, "%3u ⨯ ⚪  Standard Ball", player->GetItemCount(ITEM_STANDARD_BALL));
				names.push_back(nameStr);
			}
			if (player->GetItemCount(ITEM_SUPER_BALL))
			{
				items.push_back(ITEM_SUPER_BALL);
				sprintf(nameStr, "%3u ⨯ 🔵  Super Ball", player->GetItemCount(ITEM_SUPER_BALL));
				names.push_back(nameStr);
			}
			if (player->GetItemCount(ITEM_UBER_BALL))
			{
				items.push_back(ITEM_UBER_BALL);
				sprintf(nameStr, "%3u ⨯ 🔴  Über Ball", player->GetItemCount(ITEM_UBER_BALL));
				names.push_back(nameStr);
			}
			if (player->GetItemCount(ITEM_MEGA_SEED))
			{
				items.push_back(ITEM_MEGA_SEED);
				sprintf(nameStr, "%3u ⨯ 🍇  Mega Seed", player->GetItemCount(ITEM_MEGA_SEED));
				names.push_back(nameStr);
			}

			names.push_back("Cancel");
			option = ShowEncounterLongOptions(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
				names);
			DrawEncounter(monster, encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight);

			if ((option == -1) || (option >= (int32_t)items.size()))
				continue;

			ItemType item = items[option];
			if (item == ITEM_MEGA_SEED)
			{
				if (player->GiveSeed())
				{
					AnimateThrow(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight, "🍇");

					ShowEncounterText(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
						"You gave the wild " + monster->GetSpecies()->GetName() + " a Mega Seed.");
					ShowEncounterText(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
						"The wild " + monster->GetSpecies()->GetName() + " appears more friendly.");
				}
				else
				{
					ShowEncounterText(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
						"The wild " + monster->GetSpecies()->GetName() + " isn't hungry.");
				}
				continue;
			}

			ball = item;
			continue;
		}
		else if (option == 0)
		{
			// Throw ball
			BallThrowResult result = player->ThrowBall(ball);

			string ballImage = "⚪";
			if (ball == ITEM_SUPER_BALL)
				ballImage = "🔵";
			else if (ball == ITEM_UBER_BALL)
				ballImage = "🔴";

			AnimateThrow(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight, ballImage);
			AnimateCatchResult(monster, encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
				ballImage, result);

			switch (result)
			{
			case THROW_RESULT_BREAK_OUT_AFTER_ONE:
			case THROW_RESULT_BREAK_OUT_AFTER_TWO:
			case THROW_RESULT_BREAK_OUT_AFTER_THREE:
				ShowEncounterText(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
					"The wild " + monster->GetSpecies()->GetName() + " broke out.");
				break;
			case THROW_RESULT_RUN_AWAY_AFTER_ONE:
			case THROW_RESULT_RUN_AWAY_AFTER_TWO:
			case THROW_RESULT_RUN_AWAY_AFTER_THREE:
				ShowEncounterText(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
					"The wild " + monster->GetSpecies()->GetName() + " ran away.");
				break;
			case THROW_RESULT_CATCH:
				ShowEncounterText(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
					monster->GetSpecies()->GetName() + " was caught!");
				break;
			}

			if (result == THROW_RESULT_CATCH)
			{
				// Caught monster, show details screen
				break;
			}

			if ((result == THROW_RESULT_RUN_AWAY_AFTER_ONE) || (result == THROW_RESULT_RUN_AWAY_AFTER_TWO) ||
				(result == THROW_RESULT_RUN_AWAY_AFTER_THREE))
			{
				// Ran away, encounter is complete
				break;
			}

			// Broke out, give player another chance
			continue;
		}
	}
}


void InterruptableWait(uint32_t ms)
{
	Terminal* term = Terminal::GetTerminal();
	chrono::steady_clock::time_point start = chrono::steady_clock::now();
	while (!term->HasQuit())
	{
		if (term->GetInput().size() != 0)
			break;

		chrono::steady_clock::time_point cur = chrono::steady_clock::now();
		if (chrono::duration_cast<chrono::milliseconds>(cur - start).count() >= ms)
			break;
	}
}
