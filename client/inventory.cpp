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
	size_t x = (centerX - (width / 2)) | 1;
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


static string GetStringForInventoryItem(ItemType item, uint32_t count)
{
	char nameStr[64];
	switch (item)
	{
	case ITEM_STANDARD_BALL:
		sprintf(nameStr, "%3u ⨯ ⚪  Standard Ball", count);
		break;
	case ITEM_SUPER_BALL:
		sprintf(nameStr, "%3u ⨯ 🔵  Super Ball", count);
		break;
	case ITEM_UBER_BALL:
		sprintf(nameStr, "%3u ⨯ 🔴  Über Ball", count);
		break;
	case ITEM_STANDARD_HEAL:
		sprintf(nameStr, "%3u ⨯ 🍎  Standard Heal", count);
		break;
	case ITEM_SUPER_HEAL:
		sprintf(nameStr, "%3u ⨯ 💉  Super Heal", count);
		break;
	case ITEM_KEG_OF_HEALTH:
		sprintf(nameStr, "%3u ⨯ 🍺  Keg of Health", count);
		break;
	case ITEM_MEGA_SEED:
		sprintf(nameStr, "%3u ⨯ 🍇  Mega Seed", count);
		break;
	default:
		sprintf(nameStr, "%3u ⨯ ?  MissingNo", count);
		break;
	}
	return nameStr;
}


void ShowInventory(Player* player, MapRenderer* map)
{
	vector<ItemType> items;
	vector<string> names;
	static vector<ItemType> itemOrdering {ITEM_STANDARD_BALL, ITEM_SUPER_BALL, ITEM_UBER_BALL,
		ITEM_STANDARD_HEAL, ITEM_SUPER_HEAL, ITEM_KEG_OF_HEALTH, ITEM_MEGA_SEED};
	for (auto& i : itemOrdering)
	{
		if (player->GetItemCount(i))
		{
			items.push_back(i);
			names.push_back(GetStringForInventoryItem(i, player->GetItemCount(i)));
		}
	}

	while (true)
	{
		int32_t option = ShowInventoryOptions(30, names);
		if (option == -1)
			break;

		map->Paint();
	}
}


void GetAndShowItemsFromStop(Player* player, int32_t x, int32_t y)
{
	map<ItemType, uint32_t> items = player->GetItemsFromStop(x, y);
	if (items.size() == 0)
		return;

	Terminal* term = Terminal::GetTerminal();
	size_t centerX = term->GetWidth() / 2;
	size_t centerY = term->GetHeight() / 2;
	size_t width = 30;
	size_t height = items.size() + 2;
	size_t boxX = (centerX - (width / 2)) | 1;
	size_t boxY = centerY - (height / 2);
	DrawBox(boxX - 1, boxY - 1, width + 2, height + 2, 234);

	term->BeginOutputQueue();
	term->SetColor(255, 234);

	term->SetCursorPosition(boxX + 1, boxY);
	term->Output("Items Obtained:");
	term->SetCursorPosition(boxX, boxY + 1);
	for (size_t dx = 0; dx < width; dx++)
		term->Output("┄");

	size_t i = 0;
	for (auto& item : items)
	{
		term->SetCursorPosition(boxX + 1, boxY + i + 2);
		term->Output(GetStringForInventoryItem(item.first, item.second));

		i++;
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
