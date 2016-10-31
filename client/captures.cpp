#include <algorithm>
#include "client.h"

using namespace std;


void ShowMonsterList(Player* player, MapRenderer* map)
{
	Terminal* term = Terminal::GetTerminal();

	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t width = 40;
	size_t height = 16;
	size_t x = centerX - (width / 2);
	size_t y = centerY - ((height + 2) / 2);
	DrawBox(x - 1, y - 1, width + 2, height + 4, 234);

	static uint32_t sortOrder = 0;
	static vector<string> sortOrderNames = vector<string>{"Index", "CP", "Alphabetical"};

	size_t scroll = 0;
	size_t selected = 0;
	while (!term->HasQuit())
	{
		vector<shared_ptr<Monster>> list = player->GetMonsters();
		switch (sortOrder)
		{
		case 1:
			sort(list.begin(), list.end(), [](shared_ptr<Monster> a, shared_ptr<Monster> b) {
				if (a->GetCP() == b->GetCP())
					return a->GetID() < b->GetID();
				return b->GetCP() < a->GetCP();
			});
			break;
		case 2:
			sort(list.begin(), list.end(), [](shared_ptr<Monster> a, shared_ptr<Monster> b) {
				if (a->GetName() == b->GetName())
					return a->GetID() < b->GetID();
				return a->GetName() < b->GetName();
			});
			break;
		default:
			sort(list.begin(), list.end(), [](shared_ptr<Monster> a, shared_ptr<Monster> b) {
				if (a->GetSpecies()->GetIndex() == b->GetSpecies()->GetIndex())
				{
					if (a->GetCP() == b->GetCP())
						return a->GetID() < b->GetID();
					return b->GetCP() < a->GetCP();
				}
				return a->GetSpecies()->GetIndex() < b->GetSpecies()->GetIndex();
			});
			break;
		}

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
				term->SetColor(255, 234);

			term->Output(list[scroll + i]->GetSpecies()->GetImage());
			term->Output(" ");
			term->Output(list[scroll + i]->GetName());

			for (size_t dx = 12 + list[scroll + i]->GetName().size(); dx < (width - 1); dx++)
				term->Output(" ");

			char cpStr[32];
			sprintf(cpStr, "CP %d", list[scroll + i]->GetCP());
			for (size_t dx = strlen(cpStr); dx < 7; dx++)
				term->Output(" ");
			term->Output(cpStr);
			term->Output(" ");
		}

		term->SetColor(255, 234);
		term->SetCursorPosition(x, y + height);
		for (size_t i = 0; i < width; i++)
			term->Output("┄");
		term->SetCursorPosition(x, y + height + 1);
		term->Output("Sort \033[4mO\033[24mrder: ");
		term->Output(sortOrderNames[sortOrder]);
		for (size_t i = 0; i < (width - (12 + sortOrderNames[sortOrder].size())); i++)
			term->Output(" ");

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
			DrawBox(x - 1, y - 1, width + 2, height + 4, 234);
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
		else if ((input == "o") || (input == "O"))
		{
			sortOrder++;
			if (sortOrder >= 3)
				sortOrder = 0;
		}
	}
}
