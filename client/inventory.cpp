#include "client.h"

using namespace std;


static void DrawInventoryOptions(size_t x, size_t y, size_t width, const vector<string>& options, int32_t selected)
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


static int32_t ShowInventoryOptions(size_t width, const vector<string>& options)
{
	Terminal* term = Terminal::GetTerminal();
	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t height = options.size();
	size_t x = centerX - (width / 2);
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

		DrawInventoryOptions(x, y, width, options, selected);

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


void ShowInventory(Player* player, MapRenderer* map)
{
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
	if (player->GetItemCount(ITEM_STANDARD_HEAL))
	{
		items.push_back(ITEM_STANDARD_HEAL);
		sprintf(nameStr, "%3u ⨯ 🍎  Standard Heal", player->GetItemCount(ITEM_STANDARD_HEAL));
		names.push_back(nameStr);
	}
	if (player->GetItemCount(ITEM_SUPER_HEAL))
	{
		items.push_back(ITEM_SUPER_HEAL);
		sprintf(nameStr, "%3u ⨯ 💉  Super Heal", player->GetItemCount(ITEM_SUPER_HEAL));
		names.push_back(nameStr);
	}
	if (player->GetItemCount(ITEM_KEG_OF_HEALTH))
	{
		items.push_back(ITEM_KEG_OF_HEALTH);
		sprintf(nameStr, "%3u ⨯ 🍺  Keg of Health", player->GetItemCount(ITEM_KEG_OF_HEALTH));
		names.push_back(nameStr);
	}
	if (player->GetItemCount(ITEM_MEGA_SEED))
	{
		items.push_back(ITEM_MEGA_SEED);
		sprintf(nameStr, "%3u ⨯ 🍇  Mega Seed", player->GetItemCount(ITEM_MEGA_SEED));
		names.push_back(nameStr);
	}

	while (true)
	{
		int32_t option = ShowInventoryOptions(30, names);
		if (option == -1)
			break;

		map->Paint();
	}
}
