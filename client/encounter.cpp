#include <unistd.h>
#include <string.h>
#include "client.h"
#include "terminal.h"

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


static void DrawEncounter(shared_ptr<Monster> monster, size_t x, size_t y, size_t width, size_t height)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();
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
	term->SetCursorPosition(x + 1, y);
	term->SetColor(255, 17);
	term->Output(monster->GetName());
	char cpStr[32];
	sprintf(cpStr, "CP %d", monster->GetCP());
	term->SetCursorPosition(x + 1, y + 1);
	term->Output(cpStr);
	term->EndOutputQueue();
}


void EraseEncounterText(size_t x, size_t y, size_t height)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();
	for (size_t dy = (height - 2); dy < height; dy++)
	{
		term->SetCursorPosition(x, y + dy);
		if (dy < 4)
			term->SetColor(242, 17);
		else
			term->SetColor(255, 23);
		term->Output(g_encounterGraphics[dy]);
	}
	term->EndOutputQueue();
}


static void ShowEncounterText(size_t x, size_t y, size_t width, size_t height, const string& text)
{
	DrawBoxText(x, y, width, height, text);
	InterruptableWait(TEXT_WAIT_TIME);
	EraseEncounterText(x, y, height);
}


static void DrawEncounterLongOptions(size_t x, size_t y, size_t width, size_t height, const vector<string>& options,
	int32_t selected)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();

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

	term->EndOutputQueue();
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


static void AnimateThrow(size_t x, size_t y, size_t width, const std::string& item)
{
	Terminal* term = Terminal::GetTerminal();
	size_t sourceX = x + ((width * 2) / 3);
	size_t sourceY = y + 6;
	size_t destX = x + (width / 3);
	size_t dist = sourceX - destX;

	term->BeginOutputQueue();
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
		term->EndOutputQueue();

		usleep(66000);

		term->BeginOutputQueue();
		term->SetCursorPosition(curX, sourceY + yOffset);
		term->Output("  ");
	}

	term->EndOutputQueue();
}


static void AnimateCatchResult(shared_ptr<Monster> monster, size_t x, size_t y, size_t width,
	const std::string& item, BallThrowResult result)
{
	Terminal* term = Terminal::GetTerminal();
	size_t monsterY = y + 6;
	size_t monsterX = x + (width / 3);

	term->BeginOutputQueue();
	term->SetColor(255, 23);
	term->SetCursorPosition(monsterX, monsterY);
	term->Output(" " + item + " ");
	term->EndOutputQueue();

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
		term->BeginOutputQueue();
		term->SetCursorPosition(monsterX, monsterY);
		term->Output(item + "  ");
		term->EndOutputQueue();

		usleep(66000);

		term->BeginOutputQueue();
		term->SetCursorPosition(monsterX, monsterY);
		term->Output(" " + item + " ");
		term->EndOutputQueue();

		usleep(66000);

		term->BeginOutputQueue();
		term->SetCursorPosition(monsterX, monsterY);
		term->Output("  " + item);
		term->EndOutputQueue();

		usleep(66000);

		term->BeginOutputQueue();
		term->SetCursorPosition(monsterX, monsterY);
		term->Output(" " + item + " ");
		term->EndOutputQueue();

		usleep(500000);
	}

	if (result == THROW_RESULT_CATCH)
	{
		term->BeginOutputQueue();
		term->SetCursorPosition(monsterX - 1, monsterY);
		term->Output(" ✨" + item + "✨ ");
		term->EndOutputQueue();

		usleep(125000);

		term->BeginOutputQueue();
		term->SetCursorPosition(monsterX - 1, monsterY);
		term->Output("✨ " + item + " ✨");
		term->EndOutputQueue();

		usleep(125000);

		term->BeginOutputQueue();
		term->SetCursorPosition(monsterX - 1, monsterY);
		term->Output("  " + item + "  ");
		term->EndOutputQueue();

		return;
	}

	term->BeginOutputQueue();
	term->SetCursorPosition(monsterX, monsterY);
	term->Output(" 💥 ");
	term->EndOutputQueue();

	usleep(250000);

	term->BeginOutputQueue();
	term->SetCursorPosition(monsterX, monsterY);
	term->Output(monster->GetSpecies()->GetImage());
	term->EndOutputQueue();

	sleep(1);

	if ((result == THROW_RESULT_RUN_AWAY_AFTER_ONE) || (result == THROW_RESULT_RUN_AWAY_AFTER_TWO) ||
		(result == THROW_RESULT_RUN_AWAY_AFTER_THREE))
	{
		term->BeginOutputQueue();
		term->SetCursorPosition(monsterX, monsterY);
		term->Output(" 💨 ");
		term->EndOutputQueue();

		sleep(1);

		term->BeginOutputQueue();
		term->SetCursorPosition(monsterX, monsterY);
		term->Output("   ");
		term->EndOutputQueue();
		return;
	}
}


shared_ptr<Monster> Encounter(Player* player, int32_t x, int32_t y)
{
	Terminal* term = Terminal::GetTerminal();

	// Get the monster object that we are going to encounter
	shared_ptr<Monster> monster = player->StartWildEncounter(x, y);
	if (!monster)
		return nullptr;

	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t encounterBoxWidth = strlen(g_encounterGraphics[0]);
	size_t encounterBoxHeight = sizeof(g_encounterGraphics) / sizeof(char*);
	size_t encounterBoxX = (centerX - (encounterBoxWidth / 2)) | 1;
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
						return nullptr;
					}
				}
			}
		}

		int32_t option = ShowBoxOptions(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
			vector<string>{string("Throw ") + GetItemName(ball), string("Items"), string("Run Away")});
		EraseEncounterText(encounterBoxX, encounterBoxY, encounterBoxHeight);
		if ((option == -1) || (option == 2))
		{
			// Run away
			player->RunFromEncounter();
			ShowEncounterText(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
				"You ran away safely.");
			return nullptr;
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
					AnimateThrow(encounterBoxX, encounterBoxY, encounterBoxWidth, "🍇");

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
			uint32_t alreadyCaught = player->GetNumberCaptured(monster->GetSpecies());
			BallThrowResult result = player->ThrowBall(ball);

			string ballImage = "⚪";
			if (ball == ITEM_SUPER_BALL)
				ballImage = "🔵";
			else if (ball == ITEM_UBER_BALL)
				ballImage = "🔴";

			AnimateThrow(encounterBoxX, encounterBoxY, encounterBoxWidth, ballImage);
			AnimateCatchResult(monster, encounterBoxX, encounterBoxY, encounterBoxWidth,
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
				if (alreadyCaught == 0)
				{
					ShowEncounterText(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
						player->GetName() + " earned 1400 XP.");
				}
				else
				{
					ShowEncounterText(encounterBoxX, encounterBoxY, encounterBoxWidth, encounterBoxHeight,
						player->GetName() + " earned 400 XP.");
				}
				break;
			}

			if (result == THROW_RESULT_CATCH)
			{
				// Caught monster, show details screen
				return monster;
			}

			if ((result == THROW_RESULT_RUN_AWAY_AFTER_ONE) || (result == THROW_RESULT_RUN_AWAY_AFTER_TWO) ||
				(result == THROW_RESULT_RUN_AWAY_AFTER_THREE))
			{
				// Ran away, encounter is complete
				return nullptr;
			}

			// Broke out, give player another chance
			continue;
		}
	}

	// No catch
	return nullptr;
}
