#include <algorithm>
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
	size_t height = term->GetHeight() - 1;

	int32_t leftX = m_centerX - (int32_t)(width / 2);
	int32_t topY = m_centerY - (int32_t)(height / 2);

	int32_t playerX = m_player->GetLastLocationX();
	int32_t playerY = m_player->GetLastLocationY();

	for (size_t y = 0; y < height; y++)
	{
		term->BeginOutputQueue();
		term->SetCursorPosition(0, y);
		term->SetColor(255, 29);

		size_t x;
		for (x = 0; x < (width - 1); x++)
		{
			int32_t curX = leftX + (int32_t)x;
			int32_t curY = topY + (int32_t)y;

			// Show player avatar
			if ((curX == m_player->GetLastLocationX()) && (curY == m_player->GetLastLocationY()))
			{
				term->Output("🚶");
				continue;
			}

			// Don't draw monsters if they would hide the player avatar
			if ((((curX + 1) == playerX) || ((curX + 2) == playerX)) && (curY == playerY))
			{
				term->Output(" ");
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
					term->Output(i.species->GetImage());
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

			term->Output(" ");
		}

		if (x < width)
			term->Output(" ");

		term->EndOutputQueue();
	}

	// Show player name and level in the status bar
	term->BeginOutputQueue();
	term->SetCursorPosition(0, height);
	term->SetColor(255, 16);
	term->ClearLine();

	term->Output(m_player->GetName());

	term->SetColor(250, 16);
	term->Output("  Lv ");
	char levelStr[32];
	sprintf(levelStr, "%d ", m_player->GetLevel());
	term->Output(levelStr);

	if (m_player->GetLevel() < 40)
	{
		// Level progress bar
		uint32_t progress = m_player->GetTotalExperience() - m_player->GetTotalExperienceNeededForCurrentLevel();
		uint32_t needed = m_player->GetTotalExperienceNeededForNextLevel() -
			m_player->GetTotalExperienceNeededForCurrentLevel();
		uint32_t bars = (progress * 10) / needed;
		term->SetColor(255, 16);
		for (uint32_t i = 0; i < bars; i++)
			term->Output("━");
		term->SetColor(238, 16);
		for (uint32_t i = bars; i < 10; i++)
			term->Output("━");
	}

	// Show nearby monsters in the status bar
	std::vector<MonsterSighting> nearby = m_sightings;
	sort(nearby.begin(), nearby.end(), [&](const MonsterSighting& a, const MonsterSighting& b) {
		uint32_t aDistSq = ((a.x - playerX) * (a.x - playerX)) + (4 * ((a.y - playerY) * (a.y - playerY)));
		uint32_t bDistSq = ((b.x - playerX) * (b.x - playerX)) + (4 * ((b.y - playerY) * (b.y - playerY)));
		return aDistSq < bDistSq;
	});

	term->SetCursorPosition(width - (8 + (6 * 5)), height);
	term->SetColor(255, 16);
	term->Output("Nearby: ");
	if (nearby.size() == 0)
	{
		term->SetColor(242, 16);
		term->Output("None");
	}
	else
	{
		for (size_t i = 0; (i < 6) && (i < nearby.size()); i++)
		{
			term->Output(nearby[i].species->GetImage());

			uint32_t distSq = ((nearby[i].x - playerX) * (nearby[i].x - playerX)) +
				(4 * ((nearby[i].y - playerY) * (nearby[i].y - playerY)));
			if (distSq < (16 * 16))
				term->Output("  ");
			else if (distSq < (25 * 25))
				term->Output(". ");
			else if (distSq < (40 * 40))
				term->Output("⁚ ");
			else
				term->Output("⁝ ");
		}
	}

	term->EndOutputQueue();
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
