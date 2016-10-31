#include <algorithm>
#include "client.h"

using namespace std;


void ShowMonsterInfo(Player* player, MonsterSpecies* species)
{
	Terminal* term = Terminal::GetTerminal();
	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t width = 45;

	string desc = species->GetDescription();
	vector<string> descLines;
	for (size_t pos = 0; pos < desc.size(); )
	{
		size_t end = desc.find('\n', pos);
		string lineStr;
		if (end == string::npos)
		{
			lineStr = desc.substr(pos);
			pos = desc.size();
		}
		else
		{
			lineStr = desc.substr(pos, end - pos);
			pos = end + 1;
		}

		if (lineStr.size() > (width - 2))
		{
			// Line needs to be word wrapped
			size_t start = 0;
			for (size_t word = 0; word < lineStr.size(); )
			{
				size_t space = lineStr.find(' ', word);
				if (space == string::npos)
					space = lineStr.size();

				if (((space - start) > (width - 2)) && (word != start))
				{
					descLines.push_back(lineStr.substr(start, word - start));
					start = word;
				}

				word = space + 1;
			}
			if (start < lineStr.size())
				descLines.push_back(lineStr.substr(start));
		}
		else
		{
			// Line is short enough to be used directly
			descLines.push_back(lineStr);
		}
	}

	size_t height = 4;
	if (species->GetEvolutions().size() > 0)
		height += 2 + species->GetEvolutions().size();
	if (player->GetNumberCaptured(species) > 0)
		height += 1 + descLines.size();

	size_t x = centerX - (width / 2);
	size_t y = centerY - (height / 2);

	DrawBox(x - 1, y - 1, width + 2, height + 2, 234);

	term->BeginOutputQueue();
	term->SetColor(255, 234);

	term->SetCursorPosition(x + 1, y);
	term->Output(species->GetImage());
	term->Output(" ");
	term->Output(species->GetName());

	term->SetCursorPosition(x + 1, y + 1);
	term->SetColor(GetElementTextColor(species->GetTypes()[0]), GetElementColor(species->GetTypes()[0]));
	term->Output(" ");
	term->Output(GetElementName(species->GetTypes()[0]));
	term->Output(" ");

	term->SetColor(255, 234);
	term->Output(" ");

	if (species->GetTypes().size() > 1)
	{
		term->SetColor(GetElementTextColor(species->GetTypes()[1]), GetElementColor(species->GetTypes()[1]));
		term->Output(" ");
		term->Output(GetElementName(species->GetTypes()[1]));
		term->Output(" ");
		term->SetColor(255, 234);
	}

	term->SetCursorPosition(x + 1, y + 3);
	term->SetColor(255, 234);
	term->Output("Seen: ");
	char numStr[32];
	sprintf(numStr, "%d", player->GetNumberSeen(species));
	term->Output(numStr);

	term->Output("   Captured: ");
	sprintf(numStr, "%d", player->GetNumberCaptured(species));
	term->Output(numStr);

	size_t curY = y + 5;
	if (player->GetNumberCaptured(species) > 0)
	{
		for (auto& i : descLines)
		{
			term->SetCursorPosition(x + 1, curY);
			term->Output(i);
			curY++;
		}
		curY++;
	}

	if (species->GetEvolutions().size() > 0)
	{
		term->SetCursorPosition(x + 1, curY);
		term->Output("Evolves into:");
		curY++;

		vector<MonsterSpecies*> evolutions = species->GetEvolutions();
		sort(evolutions.begin(), evolutions.end(), [](MonsterSpecies* a, MonsterSpecies* b) {
			return a->GetIndex() < b->GetIndex();
		});

		for (auto& i : evolutions)
		{
			term->SetCursorPosition(x + 1, curY);
			curY++;

			if ((player->GetNumberSeen(i) == 0) && (player->GetNumberCaptured(i) == 0))
				term->SetColor(240, 234);
			else if (player->GetNumberCaptured(i) == 0)
				term->SetColor(248, 234);
			else
				term->SetColor(255, 234);

			sprintf(numStr, "#%d", i->GetIndex());
			for (size_t j = strlen(numStr); j < 4; j++)
				term->Output(" ");
			term->Output(numStr);
			term->Output("  ");

			size_t len = 6;
			if ((player->GetNumberSeen(i) == 0) && (player->GetNumberCaptured(i) == 0))
			{
				term->Output("       ???");
				len += 10;
			}
			else
			{
				if (player->GetNumberCaptured(i) > 0)
					term->Output("⚪  ");
				else
					term->Output("   ");
				len += 3;

				term->Output(i->GetImage());
				term->Output(" ");
				term->Output(i->GetName());
			}
		}
	}

	term->EndOutputQueue();

	while (!term->HasQuit())
	{
		string input = term->GetInput();
		if ((input == "\033") || (input == " ") || (input == "\r") || (input == "\n") || (input == "e") ||
			(input == "E") || (input == "q") || (input == "Q"))
			break;
	}
}


void ShowMonsterIndex(Player* player, MapRenderer* map)
{
	Terminal* term = Terminal::GetTerminal();

	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t width = 30;
	size_t height = 20;
	size_t x = centerX - (width / 2);
	size_t y = centerY - (height / 2);
	DrawBox(x - 1, y - 1, width + 2, height + 2, 234);

	size_t scroll = 0;
	size_t selected = 0;
	while (!term->HasQuit())
	{
		vector<MonsterSpecies*> list = MonsterSpecies::GetAll();
		if (selected >= list.size())
		{
			if (list.size() > 0)
				selected = list.size() - 1;
			else
				selected = 0;
		}
		if (scroll > selected)
			scroll = selected;

		term->BeginOutputQueue();
		for (size_t i = 0; i < height; i++)
		{
			term->SetCursorPosition(x + 1, y + i);

			if ((scroll + i) >= list.size())
			{
				term->SetColor(255, 234);
				for (size_t dx = 1; dx < (width - 1); dx++)
					term->Output(" ");
				continue;
			}

			if ((scroll + i) == selected)
				term->SetColor(234, 255);
			else
			{
				if ((player->GetNumberSeen(list[scroll + i]) == 0) && (player->GetNumberCaptured(list[scroll + i]) == 0))
					term->SetColor(240, 234);
				else if (player->GetNumberCaptured(list[scroll + i]) == 0)
					term->SetColor(248, 234);
				else
					term->SetColor(255, 234);
			}

			char numStr[32];
			sprintf(numStr, "#%d", list[scroll + i]->GetIndex());
			for (size_t j = strlen(numStr); j < 4; j++)
				term->Output(" ");
			term->Output(numStr);
			term->Output("  ");

			size_t len = 6;
			if ((player->GetNumberSeen(list[scroll + i]) == 0) && (player->GetNumberCaptured(list[scroll + i]) == 0))
			{
				term->Output("       ???");
				len += 10;
			}
			else
			{
				if (player->GetNumberCaptured(list[scroll + i]) > 0)
					term->Output("⚪  ");
				else
					term->Output("   ");
				len += 3;

				term->Output(list[scroll + i]->GetImage());
				term->Output(" ");
				term->Output(list[scroll + i]->GetName());
				len += 4 + list[scroll + i]->GetName().size();
			}

			for (size_t dx = 1 + len; dx < (width - 1); dx++)
				term->Output(" ");
		}
		term->EndOutputQueue();

		string input = term->GetInput();
		if ((input == "\033") || (input == "q") || (input == "Q"))
			break;

		if ((input == "\r") || (input == "\n") || (input == "e") || (input == "E") || (input == " "))
		{
			// Show details of selected monster, but only if the player has seen or captured one
			if (selected >= list.size())
				continue;
			if ((player->GetNumberSeen(list[selected]) == 0) &&
				(player->GetNumberCaptured(list[selected]) == 0))
				continue;
			map->Paint();
			ShowMonsterInfo(player, list[selected]);
			map->Paint();
			DrawBox(x - 1, y - 1, width + 2, height + 2, 234);
		}
		if (term->IsInputUpMovement(input))
		{
			if (selected == 0)
			{
				if (list.size() > 0)
				{
					selected = list.size() - 1;
					if (list.size() > height)
						scroll = selected - (height - 1);
					else
						scroll = 0;
				}
			}
			else
			{
				selected--;
				if (selected < scroll)
					scroll = selected;
			}
		}
		else if (term->IsInputDownMovement(input))
		{
			selected++;
			if (selected >= list.size())
			{
				selected = 0;
				scroll = 0;
			}
			else if (selected >= (scroll + height))
			{
				scroll = selected - (height - 1);
			}
		}
		else if (term->IsInputLeftMovement(input) || (input == "\033[5~"))
		{
			if (scroll > height)
			{
				scroll -= height;
				if (selected > height)
					selected -= height;
				else
					selected = 0;
			}
			else
			{
				scroll = 0;
				selected = 0;
			}
		}
		else if (term->IsInputRightMovement(input) || (input == "\033[6~"))
		{
			size_t maxScroll = 0;
			if (list.size() > height)
				maxScroll = (list.size() - 1) - (height - 1);
			if ((maxScroll > height) && (scroll < (maxScroll - height)))
			{
				scroll += height;
				selected += height;
				if (selected >= list.size())
				{
					if (list.size() > 0)
						selected = list.size() - 1;
					else
						selected = 0;
				}
			}
			else
			{
				scroll = maxScroll;
				if (list.size() > 0)
					selected = list.size() - 1;
				else
					selected = 0;
			}
		}
	}
}
