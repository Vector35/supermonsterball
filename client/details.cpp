#include "client.h"
#include "terminal.h"

using namespace std;


void DrawMonsterDetails(size_t x, size_t y, size_t width, size_t height, Player* player, shared_ptr<Monster> monster)
{
	Terminal* term = Terminal::GetTerminal();
	DrawBox(x - 1, y - 1, width + 2, height + 2, 234);

	term->BeginOututQueue();
	term->SetColor(255, 234);

	term->SetCursorPosition(x + 1, y);
	term->Output(monster->GetSpecies()->GetImage());
	term->Output(" ");
	term->Output(monster->GetName());

	term->SetCursorPosition(x + 1, y + 1);
	term->SetColor(GetElementTextColor(monster->GetSpecies()->GetTypes()[0]),
		GetElementColor(monster->GetSpecies()->GetTypes()[0]));
	term->Output(" ");
	term->Output(GetElementName(monster->GetSpecies()->GetTypes()[0]));
	term->Output(" ");

	term->SetColor(255, 234);
	term->Output(" ");

	if (monster->GetSpecies()->GetTypes().size() > 1)
	{
		term->SetColor(GetElementTextColor(monster->GetSpecies()->GetTypes()[1]),
			GetElementColor(monster->GetSpecies()->GetTypes()[1]));
		term->Output(" ");
		term->Output(GetElementName(monster->GetSpecies()->GetTypes()[1]));
		term->Output(" ");
		term->SetColor(255, 234);
	}

	size_t progress;
	if (player->GetLevel() == 1)
		progress = width - 2;
	else
		progress = ((monster->GetLevel() - 1) * (width - 2)) / (player->GetLevel() - 1);
	term->SetCursorPosition(x + 1, y + 2);
	term->SetColor(255, 234);
	for (uint32_t i = 0; i < progress; i++)
		term->Output("━");
	term->SetColor(239, 234);
	for (uint32_t i = progress; i < (width - 2); i++)
		term->Output("━");

	term->SetCursorPosition(x + 1, y + 3);
	term->SetColor(255, 234);
	term->Output("Quick Move");
	term->SetCursorPosition(x + width - 4, y + 3);
	char powerStr[32];
	sprintf(powerStr, "%3d", 20);
	term->Output(powerStr);

	term->SetCursorPosition(x + 1, y + 4);
	Element moveType = Normal;
	term->SetColor(GetElementTextColor(moveType), GetElementColor(moveType));
	term->Output(" ");
	term->Output(GetElementName(moveType));
	term->Output(" ");

	term->SetCursorPosition(x + 1, y + 6);
	term->SetColor(255, 234);
	term->Output("Charge Move");
	term->SetCursorPosition(x + width - 4, y + 6);
	sprintf(powerStr, "%3d", 20);
	term->Output(powerStr);

	term->SetCursorPosition(x + 1, y + 7);
	term->SetColor(GetElementTextColor(moveType), GetElementColor(moveType));
	term->Output(" ");
	term->Output(GetElementName(moveType));
	term->Output(" ");

	size_t curY = y + 9;

	if (monster->GetLevel() < 40)
	{
		term->SetCursorPosition(x + 1, curY);
		term->SetColor(255, 234);
		term->Output("Power up:  🍬  ");
		sprintf(powerStr, "%d", player->GetPowerUpCost(monster->GetLevel()).treats);
		term->Output(powerStr);

		term->SetColor(240, 234);
		sprintf(powerStr, "/%d", player->GetTreatsForSpecies(monster->GetSpecies()));
		term->Output(powerStr);

		term->SetColor(255, 234);
		term->Output(" Treats");
		curY++;

		term->SetCursorPosition(x + 1, curY);
		term->Output("           💊  ");
		sprintf(powerStr, "%d", player->GetPowerUpCost(monster->GetLevel()).powder);
		term->Output(powerStr);

		term->SetColor(240, 234);
		sprintf(powerStr, "/%d", player->GetPowder());
		term->Output(powerStr);

		term->SetColor(255, 234);
		term->Output(" Mystery Powder");
		curY++;
	}

	if (monster->GetSpecies()->GetEvolutions().size() > 0)
	{
		term->SetCursorPosition(x + 1, curY);
		term->SetColor(255, 234);
		term->Output("Evolution: 🍬  ");
		sprintf(powerStr, "%d", monster->GetSpecies()->GetEvolutionCost());
		term->Output(powerStr);

		term->SetColor(240, 234);
		sprintf(powerStr, "/%d", player->GetTreatsForSpecies(monster->GetSpecies()));
		term->Output(powerStr);

		term->SetColor(255, 234);
		term->Output(" ⨯ Treats");
		curY++;
	}

	term->EndOututQueue();
}


static void ShowMonsterOptions(Player* player, shared_ptr<Monster> monster, size_t x, size_t y,
	size_t width, size_t height)
{
	Terminal* term = Terminal::GetTerminal();
	while (!term->HasQuit())
	{
		int32_t option = ShowBoxOptions(x, y, width, height,
			vector<string>{"Appraise", "Rename", "Transfer", "Cancel"});
		if ((option == -1) || (option == 3))
			break;
	}
}


void ShowMonsterDetails(Player* player, shared_ptr<Monster> monster)
{
	Terminal* term = Terminal::GetTerminal();
	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t detailBoxWidth = 45;
	size_t detailBoxHeight = 14;
	size_t detailBoxX = centerX - (detailBoxWidth / 2);
	size_t detailBoxY = centerY - (detailBoxHeight / 2);

	DrawMonsterDetails(detailBoxX, detailBoxY, detailBoxWidth, detailBoxHeight, player, monster);

	while (!term->HasQuit())
	{
		int32_t powerUpOption = -1;
		int32_t evolveOption = -1;

		vector<string> optionText;
		optionText.push_back("Done");
		optionText.push_back("Options");

		if ((monster->GetLevel() < player->GetLevel()) &&
			(player->GetTreatsForSpecies(monster->GetSpecies()) >= player->GetPowerUpCost(monster->GetLevel()).treats) &&
			(player->GetPowder() >= player->GetPowerUpCost(monster->GetLevel()).powder))
		{
			powerUpOption = (int32_t)optionText.size();
			optionText.push_back("Power Up");
		}

		if ((monster->GetSpecies()->GetEvolutions().size() > 0) &&
			(player->GetTreatsForSpecies(monster->GetSpecies()) >= monster->GetSpecies()->GetEvolutionCost()))
		{
			evolveOption = (int32_t)optionText.size();
			optionText.push_back("Evolve");
		}

		int32_t option = ShowBoxOptions(detailBoxX, detailBoxY, detailBoxWidth, detailBoxHeight, optionText);
		if ((option == -1) || (option == 0))
			break;

		if (option == 1)
		{
			ShowMonsterOptions(player, monster, detailBoxX, detailBoxY, detailBoxWidth, detailBoxHeight);
		}
		else if (option == powerUpOption)
		{
			if (!player->PowerUpMonster(monster))
				continue;

			ShowBoxText(detailBoxX, detailBoxY, detailBoxWidth, detailBoxHeight, "You powered up " +
				monster->GetName() + ".");
			DrawMonsterDetails(detailBoxX, detailBoxY, detailBoxWidth, detailBoxHeight, player, monster);
		}
		else if (option == evolveOption)
		{
			string oldSpecies = monster->GetSpecies()->GetName();
			if (!player->EvolveMonster(monster))
				continue;

			ShowBoxText(detailBoxX, detailBoxY, detailBoxWidth, detailBoxHeight, "Your " +
				oldSpecies + " evolved into " + monster->GetSpecies()->GetName() + ".");
			DrawMonsterDetails(detailBoxX, detailBoxY, detailBoxWidth, detailBoxHeight, player, monster);
		}
	}
}
