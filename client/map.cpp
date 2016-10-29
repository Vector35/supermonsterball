#include "map.h"
#include "world.h"

using namespace std;


MapRenderer::MapRenderer(Player* player): m_player(player)
{
	m_centerX = 0;
	m_centerY = 0;
}


void MapRenderer::Paint()
{
	Terminal* term = Terminal::GetTerminal();
	if ((term->GetWidth() < 1) || (term->GetHeight() < 3))
		return;

	size_t width = term->GetWidth();
	size_t height = term->GetHeight() - 2;

	int32_t leftX = m_centerX - (int32_t)(width / 2);
	int32_t topY = m_centerY - (int32_t)(height / 2);

	int32_t playerX = m_player->GetLastLocationX();
	int32_t playerY = m_player->GetLastLocationY();

	for (size_t y = 0; y < height; y++)
	{
		term->SetCursorPosition(0, y);

		string line;
		size_t x;
		for (x = 0; x < (width - 1); x++)
		{
			int32_t curX = leftX + (int32_t)x;
			int32_t curY = topY + (int32_t)y;

			// Show player avatar
			if ((curX == m_player->GetLastLocationX()) && (curY == m_player->GetLastLocationY()))
			{
				line += "🚶";
				continue;
			}

			// Don't draw monsters if they would hide the player avatar
			if ((((curX + 1) == playerX) || ((curX + 2) == playerX)) && (curY == playerY))
			{
				line += " ";
				continue;
			}

			// Draw monster at this location
			bool found = false;
			for (auto& i : m_sightings)
			{
				uint32_t distSq = ((curX - playerX) * (curX - playerX)) + (4 * ((curY - playerY) * (curY - playerY)));
				if (distSq > (CAPTURE_RADIUS * CAPTURE_RADIUS))
					continue;

				if ((curX == i.x) && (curY == i.y))
				{
					line += i.species->GetImage();
					found = true;
					break;
				}
			}
			if (found)
			{
				// Monsters are three characters wide
				x += 2;
				continue;
			}

			line += " ";
		}

		if (x < width)
			line += " ";
		term->Output(line);
	}
}


void MapRenderer::SetCenterLocation(int32_t x, int32_t y)
{
	m_centerX = x;
	m_centerY = y;
}


void MapRenderer::EnsurePlayerVisible()
{
	Terminal* term = Terminal::GetTerminal();
	int32_t width = (int32_t)term->GetWidth();
	int32_t height = (int32_t)term->GetHeight();
	int32_t playerX = m_player->GetLastLocationX();
	int32_t playerY = m_player->GetLastLocationY();
	int32_t distFromCenterX = playerX - m_centerX;
	int32_t distFromCenterY = playerY - m_centerY;

	if (distFromCenterX < (-width / 4))
		m_centerX = playerX + (width / 4);
	else if (distFromCenterX > (width / 4))
		m_centerX = playerX - (width / 4);
	if (distFromCenterY < (-height / 4))
		m_centerY = playerY + (height / 4);
	else if (distFromCenterY > (height / 4))
		m_centerY = playerY - (height / 4);
}


void MapRenderer::SetMonsters(const std::vector<MonsterSighting>& sightings)
{
	m_sightings = sightings;
}
