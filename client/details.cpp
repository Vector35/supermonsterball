#include "client.h"
#include "terminal.h"

using namespace std;


void DrawMonsterDetails(size_t x, size_t y, size_t width, size_t height, Player* player, shared_ptr<Monster> monster)
{
	Terminal* term = Terminal::GetTerminal();
	DrawBox(x - 1, y - 1, width + 2, height + 2, 234);

	term->BeginOutputQueue();
	term->SetColor(255, 234);

	term->SetCursorPosition(x + 1, y);
	term->Output(monster->GetSpecies()->GetImage());
	term->Output(" ");
	term->Output(monster->GetName());

	char cpStr[32];
	sprintf(cpStr, "CP %d", monster->GetCP());
	term->SetCursorPosition(x + width - (strlen(cpStr) + 1), y);
	term->SetColor(81, 234);
	term->Output(cpStr);
	term->SetColor(255, 234);

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

	term->EndOutputQueue();
}


static bool ShowMonsterOptions(Player* player, shared_ptr<Monster> monster, size_t x, size_t y,
	size_t width, size_t height)
{
	Terminal* term = Terminal::GetTerminal();
	while (!term->HasQuit())
	{
		int32_t option = ShowBoxOptions(x, y, width, height,
			vector<string>{"Appraise", "Rename", "Transfer", "Cancel"});
		if ((option == -1) || (option == 3))
			break;

		if (option == 0)
		{
			// Appraise
			uint32_t totalIV = monster->GetAttackIV() + monster->GetDefenseIV() + monster->GetStaminaIV();
			if (totalIV <= 12)
			{
				ShowBoxText(x, y, width, height, "Your " +
					monster->GetName() + " is terrible.");
				ShowBoxText(x, y, width, height, "Get that awful thing out of my sight.");
				continue;
			}
			else if (totalIV < 23)
			{
				ShowBoxText(x, y, width, height, "Your " +
					monster->GetName() + " is not great.");
				ShowBoxText(x, y, width, height, "You should go find another one.");
				continue;
			}
			else if (totalIV < 30)
			{
				ShowBoxText(x, y, width, height, "Your " +
					monster->GetName() + " is just average.");
			}
			else if (totalIV < 37)
			{
				ShowBoxText(x, y, width, height, "Your " +
					monster->GetName() + " is strong and resourceful.");
			}
			else if (totalIV < 45)
			{
				ShowBoxText(x, y, width, height, "Your " +
					monster->GetName() + " is amazing!");
			}
			else if (totalIV == 45)
			{
				ShowBoxText(x, y, width, height, "Your " +
					monster->GetName() + " is perfect in every way!");
				ShowBoxText(x, y, width, height, "I've never seen one so strong!");
				continue;
			}

			uint32_t highestIV = monster->GetAttackIV();
			if (monster->GetDefenseIV() > highestIV)
				highestIV = monster->GetDefenseIV();
			if (monster->GetStaminaIV() > highestIV)
				highestIV = monster->GetStaminaIV();

			bool first = true;
			if (monster->GetAttackIV() == highestIV)
			{
				ShowBoxText(x, y, width, height, "Its strongest quality is its attack.");
				first = false;
			}
			if (monster->GetDefenseIV() == highestIV)
			{
				if (first)
					ShowBoxText(x, y, width, height, "Its strongest quality is its defense.");
				else
					ShowBoxText(x, y, width, height, "I'm just as happy with its defense.");
				first = false;
			}
			if (monster->GetStaminaIV() == highestIV)
			{
				if (first)
					ShowBoxText(x, y, width, height, "Its strongest quality is its stamina.");
				else
					ShowBoxText(x, y, width, height, "I'm just as happy with its stamina.");
			}

			if (highestIV < 9)
				ShowBoxText(x, y, width, height, "It isn't that great though.");
			else if (highestIV < 12)
				ShowBoxText(x, y, width, height, "It'll get the job done.");
			else if (highestIV < 15)
				ShowBoxText(x, y, width, height, "It is quite strong.");
			else
				ShowBoxText(x, y, width, height, "It is astoundingly good.");

			if (monster->GetAttackIV() == 0)
				ShowBoxText(x, y, width, height, "Its attack, though, is horrifyingly bad.");
			else if (monster->GetDefenseIV() == 0)
				ShowBoxText(x, y, width, height, "Its defense, though, is horrifyingly bad.");
			else if (monster->GetStaminaIV() == 0)
				ShowBoxText(x, y, width, height, "Its stamina, though, is horrifyingly bad.");
			else if (monster->GetAttackIV() <= 2)
				ShowBoxText(x, y, width, height, "Its attack, though, is terrible.");
			else if (monster->GetDefenseIV() <= 2)
				ShowBoxText(x, y, width, height, "Its defense, though, is terrible.");
			else if (monster->GetStaminaIV() <= 2)
				ShowBoxText(x, y, width, height, "Its stamina, though, is terrible.");

			if (monster->GetSize() == 0)
				ShowBoxText(x, y, width, height, "Your " + monster->GetName() + " is impossibly small.");
			else if (monster->GetSize() < 8)
				ShowBoxText(x, y, width, height, "Your " + monster->GetName() + " is very small.");
			else if (monster->GetSize() == 31)
				ShowBoxText(x, y, width, height, "Your " + monster->GetName() + " is simply colassal.");
			else if (monster->GetSize() > 29)
				ShowBoxText(x, y, width, height, "Your " + monster->GetName() + " is gigantic.");
			else if (monster->GetSize() > 24)
				ShowBoxText(x, y, width, height, "Your " + monster->GetName() + " is quite a big one.");
		}
		else if (option == 1)
		{
			// Rename
			DrawBoxText(x, y, width, height, "Name: ");
			string name = InputString(x + 7, y + height - 1, 16, 255, 234, monster->GetName());
			if (name.size() == 0)
				name = monster->GetSpecies()->GetName();
			if (name != monster->GetName())
				player->SetMonsterName(monster, name);
			DrawMonsterDetails(x, y, width, height, player, monster);
		}
		else if (option == 2)
		{
			// Transfer
			player->TransferMonster(monster);
			ShowBoxText(x, y, width, height, "You gave " +
				monster->GetName() + " to the Professor.");
			ShowBoxText(x, y, width, height, "You got one " +
				monster->GetSpecies()->GetBaseForm()->GetName() + " treat for it.");
			return false;
		}
	}
	return true;
}


void ShowMonsterDetails(Player* player, shared_ptr<Monster> monster)
{
	Terminal* term = Terminal::GetTerminal();
	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t detailBoxWidth = 46;
	size_t detailBoxHeight = 14;
	size_t detailBoxX = (centerX - (detailBoxWidth / 2)) | 1;
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
			if (!ShowMonsterOptions(player, monster, detailBoxX, detailBoxY, detailBoxWidth, detailBoxHeight))
				break;
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
