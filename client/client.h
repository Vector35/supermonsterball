#pragma once

#include "player.h"
#include "map.h"

#define TEXT_WAIT_TIME 3000

void GameLoop(Player* player);
std::shared_ptr<Monster> Encounter(Player* player, int32_t x, int32_t y);
void ShowMonsterDetails(Player* player, std::shared_ptr<Monster> monster);
void ShowMonsterList(Player* player, MapRenderer* map);
void ShowInventory(Player* player, MapRenderer* map);
bool ShowMainMenu(Player* player, MapRenderer* map);

void InterruptableWait(uint32_t ms);

void DrawBox(size_t x, size_t y, size_t width, size_t height, uint32_t color);
void DrawBoxText(size_t x, size_t y, size_t width, size_t height, const std::string& text);
void EraseBoxText(size_t x, size_t y, size_t width, size_t height);
void ShowBoxText(size_t x, size_t y, size_t width, size_t height, const std::string& text);
int32_t ShowBoxOptions(size_t x, size_t y, size_t width, size_t height, const std::vector<std::string>& options);
std::string GetItemName(ItemType item);
std::string GetElementName(Element element);
uint32_t GetElementColor(Element element);
uint32_t GetElementTextColor(Element element);
std::string InputString(size_t x, size_t y, size_t width, uint32_t foregroundColor, uint32_t backgroundColor,
	const std::string& defaultString);
