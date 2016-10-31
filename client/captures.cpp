#include "client.h"

using namespace std;


void ShowMonsterList(Player* player, MapRenderer* map)
{
	Terminal* term = Terminal::GetTerminal();

	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t width = 40;
	size_t height = 20;
	size_t x = centerX - (width / 2);
	size_t y = centerY - (height / 2);
	DrawBox(x - 1, y - 1, width + 2, height + 2, 234);

	size_t scroll = 0;
	size_t selected = 0;
	while (!term->HasQuit())
	{
		vector<shared_ptr<Monster>> list = player->GetMonsters();
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

			if (i >= list.size())
			{
				term->SetColor(255, 234);
				for (size_t dx = 1; dx < (width - 1); dx++)
					term->Output(" ");
				continue;
			}

			if ((scroll + i) == selected)
				term->SetColor(234, 255);
			else
				term->SetColor(255, 234);

			term->Output(list[scroll + i]->GetSpecies()->GetImage());
			term->Output(" ");
			term->Output(list[scroll + i]->GetName());

			for (size_t dx = 4 + list[scroll + i]->GetName().size(); dx < (width - 1); dx++)
				term->Output(" ");
		}
		term->EndOutputQueue();

		string input = term->GetInput();
		if ((input == "\033") || (input == "q") || (input == "Q"))
			break;

		if ((input == "\r") || (input == "\n") || (input == "e") || (input == "E") || (input == " "))
		{
			// Show details of selected monster
			if (selected >= list.size())
				continue;
			map->Paint();
			ShowMonsterDetails(player, list[selected]);
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
						scroll = list.size() - (height - 1);
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
	}
}
