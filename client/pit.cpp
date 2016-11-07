#include <algorithm>
#include "client.h"
#include "player.h"
#include "map.h"

using namespace std;


static void ShowPitUnderlevelMessage()
{
	Terminal* term = Terminal::GetTerminal();
	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t width = 50;
	size_t height = 1;
	size_t x = (centerX - (width / 2)) | 1;
	size_t y = centerY - (height / 2);

	DrawBox(x - 1, y - 1, width + 2, height + 2, 234);

	term->SetCursorPosition(x + 1, y);
	term->SetColor(255, 234);
	term->Output("You must be level 5 to battle in the pits.");

	while (!term->HasQuit())
	{
		string input = term->GetInput();
		if ((input == " ") || (input == "\r") || (input == "\n") || (input == "e") || (input == "E") || (input == "\033"))
			break;
	}
}


static int32_t PickTeam(Player* player)
{
	Terminal* term = Terminal::GetTerminal();
	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t width = 60;
	size_t height = 10;
	size_t x = (centerX - (width / 2)) | 1;
	size_t y = centerY - (height / 2);
	DrawBox(x - 1, y - 1, width + 2, height + 2, 234);

	term->BeginOutputQueue();
	term->SetCursorPosition(x + 1, y);
	term->SetColor(255, 234);
	term->Output("Pick a team:");
	term->EndOutputQueue();

	static const uint32_t colors[3] = {203, 63, 227};
	static const char* teamNames[3] = {"Team Fury", "Team Sage", "Team Impulse"};
	static const char* teamDesc[3] = {
		"Aims to train monsters to become as strong as possible.",
		"Studies monsters to discover their hidden potential.",
		"Enjoys the friendship between human and monster."
	};
	int32_t selected = 0;
	while (true)
	{
		if (term->HasQuit())
		{
			selected = -1;
			break;
		}

		term->BeginOutputQueue();
		for (int32_t i = 0; i < 3; i++)
		{
			if (i == selected)
				term->SetColor(16, colors[i]);
			else
				term->SetColor(colors[i], 234);

			term->SetCursorPosition(x, y + 2 + (i * 3));
			for (size_t dx = 0; dx < width; dx++)
				term->Output(" ");
			term->SetCursorPosition(x, y + 3 + (i * 3));
			for (size_t dx = 0; dx < width; dx++)
				term->Output(" ");

			term->SetCursorPosition(x + 1, y + 2 + (i * 3));
			term->Output(teamNames[i]);
			term->SetCursorPosition(x + 1, y + 3 + (i * 3));
			term->Output(teamDesc[i]);
		}
		term->EndOutputQueue();

		string input = term->GetInput();
		if (term->IsInputUpMovement(input))
		{
			selected--;
			if (selected < 0)
				selected = 2;
		}
		else if (term->IsInputDownMovement(input))
		{
			selected++;
			if (selected >= 3)
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


void StartPitInteraction(Player* player, MapRenderer* map, int32_t x, int32_t y)
{
	if (player->GetTeam() == TEAM_UNASSIGNED)
	{
		if (player->GetLevel() < 5)
		{
			ShowPitUnderlevelMessage();
			return;
		}

		switch (PickTeam(player))
		{
		case 0:
			player->SetTeam(TEAM_RED);
			break;
		case 1:
			player->SetTeam(TEAM_BLUE);
			break;
		case 2:
			player->SetTeam(TEAM_YELLOW);
			break;
		default:
			return;
		}

		map->Paint();
	}

	vector<shared_ptr<Monster>> battleTeam;
	vector<shared_ptr<Monster>> yourMonsters = player->GetMonsters();
	sort(yourMonsters.begin(), yourMonsters.end(), [](shared_ptr<Monster> a, shared_ptr<Monster> b) {
		return a->GetCP() > b->GetCP();
	});
	for (auto& i : yourMonsters)
	{
		if (battleTeam.size() >= 6)
			break;
		if (i->GetCurrentHP() == 0)
			continue;
		battleTeam.push_back(i);
	}

	Terminal* term = Terminal::GetTerminal();
	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t width = 70;
	size_t height = 15;
	size_t baseX = (centerX - (width / 2)) | 1;
	size_t baseY = centerY - (height / 2);

	while (true)
	{
		player->ForcePitRefresh();
		Team owner = player->GetPitTeam(x, y);
		uint32_t reputation = player->GetPitReputation(x, y);
		uint32_t level = Player::GetPitLevelByReputation(reputation);
		uint32_t nextLevel = (level >= 10) ? level : (level + 1);
		vector<shared_ptr<Monster>> defenders = player->GetPitDefenders(x, y);
		reverse(defenders.begin(), defenders.end());

		DrawBox(baseX - 1, baseY - 1, width + 2, height + 2, 234);

		term->BeginOutputQueue();
		term->SetCursorPosition(baseX + 1, baseY);
		char repStr[128];
		char maxRepStr[32];
		switch (owner)
		{
		case TEAM_RED:
			term->SetColor(203, 234);
			sprintf(repStr, "Level %d Team Fury pit", level);
			term->Output(repStr);
			sprintf(repStr, "%d", reputation);
			sprintf(maxRepStr, "/%d", Player::GetReputationRequirementForLevel(nextLevel));
			term->SetCursorPosition(baseX + width - (strlen(repStr) + strlen(maxRepStr) + 1), baseY);
			term->Output(repStr);
			term->SetColor(240, 234);
			term->Output(maxRepStr);
			break;
		case TEAM_BLUE:
			term->SetColor(63, 234);
			sprintf(repStr, "Level %d Team Sage pit", level);
			term->Output(repStr);
			sprintf(repStr, "%d", reputation);
			sprintf(maxRepStr, "/%d", Player::GetReputationRequirementForLevel(nextLevel));
			term->SetCursorPosition(baseX + width - (strlen(repStr) + strlen(maxRepStr) + 1), baseY);
			term->Output(repStr);
			term->SetColor(240, 234);
			term->Output(maxRepStr);
			break;
		case TEAM_YELLOW:
			term->SetColor(227, 234);
			sprintf(repStr, "Level %d Team Impulse pit", level);
			term->Output(repStr);
			sprintf(repStr, "%d", reputation);
			sprintf(maxRepStr, "/%d", Player::GetReputationRequirementForLevel(nextLevel));
			term->SetCursorPosition(baseX + width - (strlen(repStr) + strlen(maxRepStr) + 1), baseY);
			term->Output(repStr);
			term->SetColor(240, 234);
			term->Output(maxRepStr);
			break;
		default:
			term->SetColor(255, 234);
			term->Output("Unclaimed pit, claim it for your team!");
			break;
		}

		term->SetCursorPosition(baseX, baseY + 1);
		term->SetColor(255, 234);
		for (size_t dx = 0; dx < width; dx++)
			term->Output("┄");

		for (size_t dy = 0; dy < 11; dy++)
		{
			term->SetCursorPosition(baseX + (width / 2), baseY + 2 + dy);
			term->Output("┆");
		}

		term->SetCursorPosition(baseX + 1, baseY + 2);
		term->Output("Your battle team:");

		for (size_t i = 0; i < 6; i++)
		{
			term->SetCursorPosition(baseX + 1, baseY + 3 + i);
			sprintf(repStr, "%d. ", (int)i + 1);
			term->SetColor(240, 234);
			term->Output(repStr);
			if (i < battleTeam.size())
			{
				term->SetColor(255, 234);
				term->Output(battleTeam[i]->GetSpecies()->GetImage());
				term->Output(" ");
				term->Output(battleTeam[i]->GetName());
				term->SetCursorPosition(baseX + (width / 2) - 9, baseY + 3 + i);
				sprintf(repStr, "CP %d", battleTeam[i]->GetCP());
				term->Output(repStr);
			}
			else
			{
				term->Output("Unassigned");
			}
		}

		term->SetCursorPosition(baseX + (width / 2) + 2, baseY + 2);
		term->SetColor(255, 234);
		term->Output("Pit defenders:");

		bool hasMonsterAssigned = false;
		for (size_t i = 0; i < defenders.size(); i++)
		{
			if (defenders[i]->GetOwnerID() == player->GetID())
				hasMonsterAssigned = true;
			term->SetCursorPosition(baseX + (width / 2) + 2, baseY + 3 + i);
			sprintf(repStr, "%2d. ", (int)i + 1);
			term->SetColor(240, 234);
			term->Output(repStr);
			term->SetColor(255, 234);
			term->Output(defenders[i]->GetSpecies()->GetImage());
			term->Output(" ");
			term->Output(defenders[i]->GetName());
			term->SetCursorPosition(baseX + width - 9, baseY + 3 + i);
			sprintf(repStr, "CP %d", defenders[i]->GetCP());
			term->Output(repStr);
		}

		term->EndOutputQueue();

		int assignDefender = -1;
		int changeBattleTeam = -1;
		int battle = -1;
		vector<string> options;
		if ((!hasMonsterAssigned) && ((owner == TEAM_UNASSIGNED) || ((owner == player->GetTeam()) &&
			(defenders.size() < level))))
		{
			assignDefender = (int)options.size();
			options.push_back("Assign Defender");
		}
		if ((battleTeam.size() != 0) && (owner != TEAM_UNASSIGNED) && (((owner == player->GetTeam()) && (level < 10)) ||
			(owner != player->GetTeam())))
		{
			changeBattleTeam = (int)options.size();
			options.push_back("Change Battle Team");
			battle = (int)options.size();
			if (owner == player->GetTeam())
				options.push_back("Train in the Pit");
			else
				options.push_back("Attack the Pit");
		}
		options.push_back("Done");
		int32_t selection = ShowBoxOptions(baseX, baseY, width, height, options);
		if (selection == -1)
			break;
		if (selection == assignDefender)
		{
			shared_ptr<Monster> toAssign = ShowMonsterList(player, map, true, false);
			if (toAssign)
				player->AssignPitDefender(x, y, toAssign);
			map->Paint();
			continue;
		}
		if (selection == changeBattleTeam)
		{
			continue;
		}
		if (selection == battle)
		{
			break;
		}
		break;
	}
}
