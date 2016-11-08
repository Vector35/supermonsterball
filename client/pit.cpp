#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include "client.h"
#include "player.h"
#include "map.h"

using namespace std;


static const char* g_battleGraphics[] =
{
	"           .                      .                       ",
	"                    .                     🌕               ",
	"       .                  .          .        .           ",
	" 🌳     🌲🌲 🌲 🌳 🌲🌳 🌳 🌲🌳🌳🌲 🌳🌲  🌲 🌳 🌲🌲  🌳🌲🌳🌳🌲  🌲 🌲🌲 🌳 🌲  🌳  🌳 ",
	"                                                          ",
	"                                                          ",
	"                                                          ",
	"                                                          ",
	"                                                          ",
	"                                                          ",
};


static void DrawBattle(shared_ptr<Monster> attacker, shared_ptr<Monster> defender, size_t x, size_t y,
	size_t width, size_t height)
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
		term->Output(g_battleGraphics[dy]);
	}
	term->SetCursorPosition(x + ((width * 2) / 5), y + 6);
	if (attacker)
		term->Output(attacker->GetSpecies()->GetImage());
	term->SetCursorPosition(x + ((width * 3) / 5), y + 5);
	if (defender)
		term->Output(defender->GetSpecies()->GetImage());
	term->EndOutputQueue();
}


static void DrawBattleStatus(shared_ptr<Monster> attacker, shared_ptr<Monster> defender, size_t x, size_t y,
	size_t width, size_t height, uint32_t charge)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();

	if (defender)
	{
		term->SetCursorPosition(x + width - (defender->GetName().size() + 1), y);
		term->SetColor(255, 17);
		term->Output(defender->GetName());
		term->SetCursorPosition(x + width - 19, y + 1);
		term->SetColor(255, 17);
		term->Output("HP ");
		size_t health = (defender->GetCurrentHP() * 15) / defender->GetMaxHP();
		term->SetColor(83, 17);
		for (size_t i = 0; i < health; i++)
			term->Output("━");
		term->SetColor(246, 17);
		for (size_t i = health; i < 15; i++)
			term->Output("━");
	}

	if (attacker)
	{
		term->SetCursorPosition(x + 1, y + height - 5);
		term->SetColor(255, 23);
		term->Output(attacker->GetName());
		term->SetCursorPosition(x + 1, y + height - 4);
		term->SetColor(255, 23);
		term->Output("HP ");
		size_t health = (attacker->GetCurrentHP() * 15) / attacker->GetMaxHP();
		term->SetColor(83, 23);
		for (size_t i = 0; i < health; i++)
			term->Output("━");
		term->SetColor(246, 23);
		for (size_t i = health; i < 15; i++)
			term->Output("━");
		term->SetCursorPosition(x + 1, y + height - 3);
		term->SetColor(255, 23);
		term->Output("Charge ");
		size_t chargeBar = (charge * 11) / 100;
		term->SetColor(255, 23);
		for (size_t i = 0; i < chargeBar; i++)
			term->Output("━");
		term->SetColor(246, 23);
		for (size_t i = chargeBar; i < 11; i++)
			term->Output("━");
	}
}


void EraseBattleText(size_t x, size_t y, size_t height)
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
		term->Output(g_battleGraphics[dy]);
	}
	term->EndOutputQueue();
}


static void ShowBattleText(size_t x, size_t y, size_t width, size_t height, const string& text)
{
	DrawBoxText(x, y, width, height, text);
	InterruptableWait(TEXT_WAIT_TIME);
	EraseBattleText(x, y, height);
}


static void ShowBattleAttackerHit(size_t x, size_t y, size_t width, shared_ptr<Monster> monster)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();
	term->SetColor(255, 23);
	term->SetCursorPosition(x + ((width * 2) / 5), y + 6);
	term->Output("✨💥✨");
	term->EndOutputQueue();

	usleep(150000);

	term->BeginOutputQueue();
	term->SetCursorPosition(x + ((width * 2) / 5), y + 6);
	term->Output(monster->GetSpecies()->GetImage());
	term->EndOutputQueue();
}


static void ShowBattleAttackerChange(size_t x, size_t y, size_t width, shared_ptr<Monster> monster)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();
	term->SetColor(255, 23);
	term->SetCursorPosition(x + ((width * 2) / 5), y + 6);
	term->Output(" 💨 ");
	term->EndOutputQueue();

	usleep(250000);

	term->BeginOutputQueue();
	term->SetCursorPosition(x + ((width * 2) / 5), y + 6);
	term->Output(monster->GetSpecies()->GetImage());
	term->EndOutputQueue();
}


static void ShowBattleAttackerFaint(size_t x, size_t y, size_t width)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();
	term->SetColor(255, 23);
	term->SetCursorPosition(x + ((width * 2) / 5), y + 6);
	term->Output(" ⚰ ");
	term->EndOutputQueue();

	usleep(250000);
}


static void ShowBattleDefenderHit(size_t x, size_t y, size_t width, shared_ptr<Monster> monster)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();
	term->SetColor(255, 23);
	term->SetCursorPosition(x + ((width * 3) / 5), y + 5);
	term->Output("✨💥✨");
	term->EndOutputQueue();

	usleep(150000);

	term->BeginOutputQueue();
	term->SetCursorPosition(x + ((width * 3) / 5), y + 5);
	term->Output(monster->GetSpecies()->GetImage());
	term->EndOutputQueue();
}


static void ShowBattleDefenderChange(size_t x, size_t y, size_t width, shared_ptr<Monster> monster)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();
	term->SetColor(255, 23);
	term->SetCursorPosition(x + ((width * 3) / 5), y + 5);
	term->Output(" 💨 ");
	term->EndOutputQueue();

	usleep(250000);

	term->BeginOutputQueue();
	term->SetCursorPosition(x + ((width * 3) / 5), y + 5);
	term->Output(monster->GetSpecies()->GetImage());
	term->EndOutputQueue();
}


static void ShowBattleDefenderFaint(size_t x, size_t y, size_t width)
{
	Terminal* term = Terminal::GetTerminal();
	term->BeginOutputQueue();
	term->SetColor(255, 23);
	term->SetCursorPosition(x + ((width * 3) / 5), y + 5);
	term->Output(" ⚰ ");
	term->EndOutputQueue();

	usleep(250000);
}


static bool SelectBattleAction(Player* player, MapRenderer* map, size_t x, size_t y, size_t width, size_t height,
	vector<shared_ptr<Monster>> battleTeam, shared_ptr<Monster>& attacker, const PitBattleStatus& status)
{
	Terminal* term = Terminal::GetTerminal();
	while (!term->HasQuit())
	{
		int32_t quickMove = -1;
		int32_t chargeMove = -1;
		int32_t change = -1;
		int32_t run = -1;

		vector<string> options;
		quickMove = (int32_t)options.size();
		options.push_back(attacker->GetQuickMove()->GetName());

		uint32_t neededCharge = 100;
		if (attacker->GetChargeMove()->GetType() == TwoChargeMove)
			neededCharge = 50;
		else if (attacker->GetChargeMove()->GetType() == ThreeChargeMove)
			neededCharge = 33;
		else if (attacker->GetChargeMove()->GetType() == FourChargeMove)
			neededCharge = 25;
		if (status.charge >= neededCharge)
		{
			chargeMove = (int32_t)options.size();
			options.push_back(attacker->GetChargeMove()->GetName());
		}

		change = (int32_t)options.size();
		options.push_back("Switch");
		run = (int32_t)options.size();
		options.push_back("Run");

		int32_t selection = ShowBoxOptions(x, y, width, height, options);
		if ((selection == -1) || (selection == run))
			return false;

		if (selection == quickMove)
		{
			player->SetPitBattleAction(PIT_ACTION_ATTACK_QUICK_MOVE);
			break;
		}
		else if (selection == chargeMove)
		{
			player->SetPitBattleAction(PIT_ACTION_ATTACK_CHARGE_MOVE);
			break;
		}
		else if (selection == change)
		{
			return false;
		}
	}

	return true;
}


static void PitBattle(Player* player, MapRenderer* map, vector<shared_ptr<Monster>> battleTeam, bool training)
{
	Terminal* term = Terminal::GetTerminal();
	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t width = strlen(g_battleGraphics[0]);
	size_t height = sizeof(g_battleGraphics) / sizeof(char*);
	size_t x = (centerX - (width / 2)) | 1;
	size_t y = centerY - (height / 2);

	vector<shared_ptr<Monster>> remainingAttackers = battleTeam;
	shared_ptr<Monster> attacker = remainingAttackers[0];
	shared_ptr<Monster> defender = player->GetPitBattleDefenders()[0];
	player->SetAttacker(attacker);

	DrawBox(x - 1, y - 1, width + 2, height + 2, 234);
	DrawBattle(attacker, nullptr, x, y, width, height);
	DrawBattleStatus(attacker, nullptr, x, y, width, height, 0);
	ShowBattleText(x, y, width, height, "Go " + attacker->GetName() + "!");

	bool done = false;
	shared_ptr<Monster> oldAttacker;
	while ((!term->HasQuit()) && (!done))
	{
		PitBattleStatus status = player->StepPitBattle();
		attacker->SetHP(status.attackerHP);
		defender->SetHP(status.defenderHP);

		switch (status.state)
		{
		case PIT_BATTLE_WAITING_FOR_ACTION:
			oldAttacker = attacker;
			if (!SelectBattleAction(player, map, x, y, width, height, remainingAttackers, attacker, status))
				done = true;
			if (attacker->GetID() != oldAttacker->GetID())
			{
				ShowBattleAttackerChange(x, y, width, attacker);
				ShowBattleText(x, y, width, height, "Go " + attacker->GetName() + "!");
			}
			break;
		case PIT_BATTLE_ATTACK_QUICK_MOVE_NOT_EFFECTIVE:
			ShowBattleDefenderHit(x, y, width, defender);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, attacker->GetName() + " used " +
				attacker->GetQuickMove()->GetName() + ".");
			ShowBattleText(x, y, width, height, "It's not very effective.");
			break;
		case PIT_BATTLE_ATTACK_QUICK_MOVE_EFFECTIVE:
			ShowBattleDefenderHit(x, y, width, defender);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, attacker->GetName() + " used " +
				attacker->GetQuickMove()->GetName() + ".");
			break;
		case PIT_BATTLE_ATTACK_QUICK_MOVE_SUPER_EFFECTIVE:
			ShowBattleDefenderHit(x, y, width, defender);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, attacker->GetName() + " used " +
				attacker->GetQuickMove()->GetName() + ".");
			ShowBattleText(x, y, width, height, "It's super effective!");
			break;
		case PIT_BATTLE_ATTACK_CHARGE_MOVE_NOT_EFFECTIVE:
			ShowBattleDefenderHit(x, y, width, defender);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, attacker->GetName() + " used " +
				attacker->GetChargeMove()->GetName() + ".");
			ShowBattleText(x, y, width, height, "It's not very effective.");
			break;
		case PIT_BATTLE_ATTACK_CHARGE_MOVE_EFFECTIVE:
			ShowBattleDefenderHit(x, y, width, defender);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, attacker->GetName() + " used " +
				attacker->GetChargeMove()->GetName() + ".");
			break;
		case PIT_BATTLE_ATTACK_CHARGE_MOVE_SUPER_EFFECTIVE:
			ShowBattleDefenderHit(x, y, width, defender);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, attacker->GetName() + " used " +
				attacker->GetChargeMove()->GetName() + ".");
			ShowBattleText(x, y, width, height, "It's super effective!");
			break;
		case PIT_BATTLE_DEFEND_QUICK_MOVE_NOT_EFFECTIVE:
			ShowBattleAttackerHit(x, y, width, attacker);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, defender->GetName() + " used " +
				defender->GetQuickMove()->GetName() + ".");
			ShowBattleText(x, y, width, height, "It's not very effective.");
			break;
		case PIT_BATTLE_DEFEND_QUICK_MOVE_EFFECTIVE:
			ShowBattleAttackerHit(x, y, width, attacker);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, defender->GetName() + " used " +
				defender->GetQuickMove()->GetName() + ".");
			break;
		case PIT_BATTLE_DEFEND_QUICK_MOVE_SUPER_EFFECTIVE:
			ShowBattleAttackerHit(x, y, width, attacker);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, defender->GetName() + " used " +
				defender->GetQuickMove()->GetName() + ".");
			ShowBattleText(x, y, width, height, "It's super effective!");
			break;
		case PIT_BATTLE_DEFEND_QUICK_MOVE_DODGE:
			ShowBattleAttackerHit(x, y, width, attacker);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, defender->GetName() + " used " +
				defender->GetQuickMove()->GetName() + ".");
			ShowBattleText(x, y, width, height, attacker->GetName() + " dodged the attack.");
			break;
		case PIT_BATTLE_DEFEND_CHARGE_MOVE_NOT_EFFECTIVE:
			ShowBattleAttackerHit(x, y, width, attacker);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, defender->GetName() + " used " +
				defender->GetChargeMove()->GetName() + ".");
			ShowBattleText(x, y, width, height, "It's not very effective.");
			break;
		case PIT_BATTLE_DEFEND_CHARGE_MOVE_EFFECTIVE:
			ShowBattleAttackerHit(x, y, width, attacker);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, defender->GetName() + " used " +
				defender->GetChargeMove()->GetName() + ".");
			break;
		case PIT_BATTLE_DEFEND_CHARGE_MOVE_SUPER_EFFECTIVE:
			ShowBattleAttackerHit(x, y, width, attacker);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, defender->GetName() + " used " +
				defender->GetChargeMove()->GetName() + ".");
			ShowBattleText(x, y, width, height, "It's super effective!");
			break;
		case PIT_BATTLE_DEFEND_CHARGE_MOVE_DODGE:
			ShowBattleAttackerHit(x, y, width, attacker);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, defender->GetName() + " used " +
				defender->GetChargeMove()->GetName() + ".");
			ShowBattleText(x, y, width, height, attacker->GetName() + " dodged the attack.");
			break;
		case PIT_BATTLE_ATTACK_FAINT:
			ShowBattleAttackerFaint(x, y, width);
			ShowBattleText(x, y, width, height, attacker->GetName() + " fainted.");
			for (auto i = remainingAttackers.begin(); i != remainingAttackers.end(); ++i)
			{
				if ((*i)->GetID() == attacker->GetID())
				{
					remainingAttackers.erase(i);
					break;
				}
			}
			if (remainingAttackers.size() == 0)
			{
				ShowBattleText(x, y, width, height, player->GetName() + " lost the battle.");
				done = true;
				break;
			}
			attacker = remainingAttackers[0];
			player->SetAttacker(attacker);
			ShowBattleAttackerChange(x, y, width, attacker);
			ShowBattleText(x, y, width, height, "Go " + attacker->GetName() + "!");
			break;
		case PIT_BATTLE_DEFEND_FAINT:
			ShowBattleDefenderFaint(x, y, width);
			ShowBattleText(x, y, width, height, defender->GetName() + " fainted.");
			break;
		case PIT_BATTLE_NEW_OPPONENT:
			if (!status.opponent)
			{
				done = true;
				break;
			}
			ShowBattleDefenderChange(x, y, width, status.opponent);
			DrawBattleStatus(attacker, defender, x, y, width, height, status.charge);
			ShowBattleText(x, y, width, height, status.opponent->GetName() + " is ready to defend the pit.");
			break;
		case PIT_BATTLE_WIN:
			ShowBattleText(x, y, width, height, player->GetName() + " won the battle!");
			done = true;
			break;
		case PIT_BATTLE_LOSE:
			ShowBattleText(x, y, width, height, player->GetName() + " lost the battle.");
			done = true;
			break;
		default:
			done = true;
			return;
		}

		if (!status.opponent)
		{
			done = true;
		}
		else if (defender->GetID() != status.opponent->GetID())
		{
			defender = status.opponent;
			DrawBattle(attacker, defender, x, y, width, height);
		}
	}

	uint32_t reputation = player->EndPitBattle();
	if (reputation != 0)
	{
		char msg[128];
		if (training)
			sprintf(msg, "Training increased pit reputation by %d.", reputation);
		else
			sprintf(msg, "Pit reputation reduced by %d.", reputation);
		ShowBattleText(x, y, width, height, msg);
	}
}


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
		if (i->IsDefending())
			continue;
		battleTeam.push_back(i);
	}

	Terminal* term = Terminal::GetTerminal();
	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t width = 76;
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
			if (owner == player->GetTeam())
				sprintf(maxRepStr, "/%d", Player::GetReputationRequirementForLevel(nextLevel));
			else
				sprintf(maxRepStr, "/%d", Player::GetReputationRequirementForLevel(level));
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
			if (owner == player->GetTeam())
				sprintf(maxRepStr, "/%d", Player::GetReputationRequirementForLevel(nextLevel));
			else
				sprintf(maxRepStr, "/%d", Player::GetReputationRequirementForLevel(level));
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
			if (owner == player->GetTeam())
				sprintf(maxRepStr, "/%d", Player::GetReputationRequirementForLevel(nextLevel));
			else
				sprintf(maxRepStr, "/%d", Player::GetReputationRequirementForLevel(level));
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
			term->BeginOutputQueue();
			term->SetCursorPosition(baseX, baseY + height - 1);
			term->SetColor(255, 234);
			for (size_t dx = 0; dx < width; dx++)
				term->Output(" ");
			term->EndOutputQueue();

			int32_t selected = 0;
			while (true)
			{
				if (term->HasQuit())
					return;

				term->BeginOutputQueue();
				for (size_t i = 0; i < 6; i++)
				{
					if (i == (size_t)selected)
						term->SetColor(16, 255);
					else
						term->SetColor(240, 234);
					term->SetCursorPosition(baseX, baseY + 3 + i);
					for (size_t dx = 0; dx < ((width / 2) - 1); dx++)
						term->Output(" ");

					term->SetCursorPosition(baseX + 1, baseY + 3 + i);
					sprintf(repStr, "%d. ", (int)i + 1);
					term->Output(repStr);
					if (i < battleTeam.size())
					{
						if (i == (size_t)selected)
							term->SetColor(16, 255);
						else
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
				term->EndOutputQueue();

				string input = term->GetInput();
				if (term->IsInputUpMovement(input))
				{
					selected--;
					if (selected < 0)
						selected = 5;
				}
				else if (term->IsInputDownMovement(input))
				{
					selected++;
					if (selected >= 6)
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

			if (selected == -1)
				continue;

			shared_ptr<Monster> toAssign = ShowMonsterList(player, map, true, false, false);
			if (toAssign)
			{
				shared_ptr<Monster> old;
				if ((size_t)selected < battleTeam.size())
					old = battleTeam[selected];

				bool alreadyThere = false;
				for (size_t i = 0; i < battleTeam.size(); i++)
				{
					if (battleTeam[i]->GetID() == toAssign->GetID())
					{
						battleTeam[i] = old;
						while (battleTeam.size() < (size_t)selected)
							battleTeam.push_back(shared_ptr<Monster>());
						battleTeam[selected] = toAssign;
						alreadyThere = true;
						break;
					}
				}

				if (!alreadyThere)
				{
					while (battleTeam.size() < (size_t)selected)
						battleTeam.push_back(shared_ptr<Monster>());
					battleTeam[selected] = toAssign;
				}
			}
			map->Paint();
			continue;
		}
		if (selection == battle)
		{
			// Get rid of any blank entries in the battle team
			vector<shared_ptr<Monster>> finalBattleTeam;
			for (auto& i : battleTeam)
			{
				if (i)
					finalBattleTeam.push_back(i);
			}

			if (!player->StartPitBattle(x, y, finalBattleTeam))
			{
				player->ForcePitRefresh();
				continue;
			}

			map->Paint();

			PitBattle(player, map, finalBattleTeam, owner == player->GetTeam());
			player->ForcePitRefresh();
			break;
		}
		break;
	}
}
