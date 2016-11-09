#include <cstdlib>
#include <set>
#include "serverplayer.h"
#include "world.h"

using namespace std;


ServerPlayer::ServerPlayer(const string& name, uint64_t id): m_name(name)
{
	m_id = id;
	m_level = 1;
	m_xp = 0;
	m_powder = 0;
	m_x = SPAWN_X;
	m_y = SPAWN_Y;
	m_team = TEAM_UNASSIGNED;
	m_banReason = "Unknown, try again with another account";

	m_inventory[ITEM_STANDARD_BALL] = 20;
	m_inventory[ITEM_MEGA_SEED] = 5;
	Database::GetDatabase()->SetInventory(m_id, ITEM_STANDARD_BALL, m_inventory[ITEM_STANDARD_BALL]);
	Database::GetDatabase()->SetInventory(m_id, ITEM_MEGA_SEED, m_inventory[ITEM_MEGA_SEED]);
}


ServerPlayer::ServerPlayer(const string& name, const DatabaseLoginResult& login): m_name(name)
{
	m_id = login.id;
	m_level = login.level;
	m_xp = login.xp;
	m_powder = login.powder;
	m_x = login.x;
	m_y = login.y;
	m_team = login.team;
	m_banReason = "Unknown, try again with another account";

	map<uint32_t, uint32_t> inventory = Database::GetDatabase()->GetInventory(m_id);
	for (auto& i : inventory)
		m_inventory[(ItemType)i.first] = i.second;

	m_seen = Database::GetDatabase()->GetMonstersSeen(m_id);
	m_captured = Database::GetDatabase()->GetMonstersCaptured(m_id);
	m_treats = Database::GetDatabase()->GetTreats(m_id);

	vector<shared_ptr<Monster>> monsters = Database::GetDatabase()->GetMonsters(m_id, m_name);
	for (auto& i : monsters)
		m_monsters[i->GetID()] = i;
}


vector<shared_ptr<Monster>> ServerPlayer::GetMonsters()
{
	vector<shared_ptr<Monster>> result;
	for (auto& i : m_monsters)
		result.push_back(i.second);
	return result;
}


shared_ptr<Monster> ServerPlayer::GetMonsterByID(uint64_t id)
{
	auto i = m_monsters.find(id);
	if (i == m_monsters.end())
		return nullptr;
	return i->second;
}


uint32_t ServerPlayer::GetNumberCaptured(MonsterSpecies* species)
{
	auto i = m_captured.find(species->GetIndex());
	if (i == m_captured.end())
		return 0;
	return i->second;
}


uint32_t ServerPlayer::GetNumberSeen(MonsterSpecies* species)
{
	auto i = m_seen.find(species->GetIndex());
	if (i == m_seen.end())
		return 0;
	return i->second;
}


uint32_t ServerPlayer::GetTreatsForSpecies(MonsterSpecies* species)
{
	auto i = m_treats.find(species->GetBaseForm()->GetIndex());
	if (i == m_treats.end())
		return 0;
	return i->second;
}


uint32_t ServerPlayer::GetItemCount(ItemType type)
{
	auto i = m_inventory.find(type);
	if (i == m_inventory.end())
		return 0;
	return i->second;
}


bool ServerPlayer::UseItem(ItemType type)
{
	auto i = m_inventory.find(type);
	if (i == m_inventory.end())
		return false;
	if (i->second == 0)
		return false;
	i->second--;
	Database::GetDatabase()->SetInventory(m_id, type, i->second);
	return true;
}


void ServerPlayer::ReportLocation(int32_t x, int32_t y)
{
	m_x = x;
	m_y = y;
	Database::GetDatabase()->SetLocation(m_id, x, y);
}


vector<MonsterSighting> ServerPlayer::GetMonstersInRange()
{
	vector<SpawnPoint> spawns = World::GetWorld()->GetSpawnPointsInRange(m_x, m_y);
	vector<MonsterSighting> result;
	for (auto& i : spawns)
	{
		shared_ptr<Monster> monster = World::GetWorld()->GetMonsterAt(i.x, i.y, m_level);
		if (monster)
		{
			// Ensure the monster hasn't already been encountered
			bool valid = true;
			auto j = m_recentEncounters.find(monster->GetSpawnTime());
			if (j != m_recentEncounters.end())
			{
				for (auto& k : j->second)
				{
					if ((k->GetSpawnX() == i.x) && (k->GetSpawnY() == i.y))
					{
						// Already finished encounter with this one
						valid = false;
						break;
					}
				}
			}

			if (!valid)
				continue;

			MonsterSighting sighting;
			sighting.species = monster->GetSpecies();
			sighting.x = i.x;
			sighting.y = i.y;
			result.push_back(sighting);
		}
	}
	return result;
}


shared_ptr<Monster> ServerPlayer::StartWildEncounter(int32_t x, int32_t y)
{
	uint32_t dist = abs(x - m_x) + abs(y - m_y);
	if (dist > CAPTURE_RADIUS)
		return nullptr;

	m_encounter = World::GetWorld()->GetMonsterAt(x, y, m_level);
	m_encounter->SetOwner(m_id, m_name);
	m_seedGiven = false;
	return m_encounter;
}


bool ServerPlayer::GiveSeed()
{
	if (!m_encounter)
		return false;
	if (m_seedGiven)
		return false;
	if (!UseItem(ITEM_MEGA_SEED))
		return false;
	m_seedGiven = true;
	return true;
}


void ServerPlayer::EndEncounter(bool caught, ItemType ball)
{
	if (!m_encounter)
		return;

	m_encounter->SetCapture(caught, ball);
	m_recentEncounters[m_encounter->GetSpawnTime()].push_back(m_encounter);

	// Clear out old encounters
	set<uint32_t> toDelete;
	for (auto& i : m_recentEncounters)
	{
		if (i.first < (m_encounter->GetSpawnTime() - 2))
			toDelete.insert(i.first);
	}
	for (auto i : toDelete)
	{
		m_recentEncounters.erase(i);
	}

	m_encounter.reset();
}


void ServerPlayer::EarnExperience(uint32_t xp)
{
	m_xp += xp;
	while ((m_level < 40) && (m_xp >= GetTotalExperienceNeededForNextLevel()))
	{
		for (auto& i : GetItemsOnLevelUp(m_level))
		{
			m_inventory[i.type] += i.count;
			Database::GetDatabase()->SetInventory(m_id, i.type, m_inventory[i.type]);
		}
		m_level++;
	}
	Database::GetDatabase()->SetExperience(m_id, m_level, m_xp);
}


BallThrowResult ServerPlayer::ThrowBall(ItemType type)
{
	if (!m_encounter)
		return THROW_RESULT_RUN_AWAY_AFTER_ONE;
	if ((type != ITEM_STANDARD_BALL) && (type != ITEM_SUPER_BALL) && (type != ITEM_UBER_BALL))
	{
		EndEncounter(false);
		return THROW_RESULT_RUN_AWAY_AFTER_ONE;
	}

	if (!UseItem(type))
	{
		EndEncounter(false);
		return THROW_RESULT_RUN_AWAY_AFTER_ONE;
	}

	switch (rand() % 15)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		if (GetNumberCaptured(m_encounter->GetSpecies()) == 0)
			EarnExperience(600);
		else
			EarnExperience(100);
		m_seen[m_encounter->GetSpecies()->GetIndex()]++;
		Database::GetDatabase()->SetMonsterSeenCount(m_id, m_encounter->GetSpecies()->GetIndex(),
			m_seen[m_encounter->GetSpecies()->GetIndex()]);
		m_captured[m_encounter->GetSpecies()->GetIndex()]++;
		Database::GetDatabase()->SetMonsterCapturedCount(m_id, m_encounter->GetSpecies()->GetIndex(),
			m_captured[m_encounter->GetSpecies()->GetIndex()]);
		m_treats[m_encounter->GetSpecies()->GetBaseForm()->GetIndex()] += 3;
		Database::GetDatabase()->SetTreats(m_id, m_encounter->GetSpecies()->GetBaseForm()->GetIndex(),
			m_treats[m_encounter->GetSpecies()->GetBaseForm()->GetIndex()]);
		m_powder += 100;
		Database::GetDatabase()->SetPowder(m_id, m_powder);
		m_encounter->SetID(Database::GetDatabase()->AddMonster(m_id, m_encounter));
		m_monsters[m_encounter->GetID()] = m_encounter;
		EndEncounter(true, type);
		return THROW_RESULT_CATCH;
	case 6:
	case 7:
		return THROW_RESULT_BREAK_OUT_AFTER_ONE;
	case 8:
	case 9:
		return THROW_RESULT_BREAK_OUT_AFTER_TWO;
	case 10:
	case 11:
		return THROW_RESULT_BREAK_OUT_AFTER_THREE;
	case 12:
		m_seen[m_encounter->GetSpecies()->GetIndex()]++;
		Database::GetDatabase()->SetMonsterSeenCount(m_id, m_encounter->GetSpecies()->GetIndex(),
			m_seen[m_encounter->GetSpecies()->GetIndex()]);
		EndEncounter(false, type);
		return THROW_RESULT_RUN_AWAY_AFTER_ONE;
	case 13:
		m_seen[m_encounter->GetSpecies()->GetIndex()]++;
		Database::GetDatabase()->SetMonsterSeenCount(m_id, m_encounter->GetSpecies()->GetIndex(),
			m_seen[m_encounter->GetSpecies()->GetIndex()]);
		EndEncounter(false, type);
		return THROW_RESULT_RUN_AWAY_AFTER_TWO;
	default:
		m_seen[m_encounter->GetSpecies()->GetIndex()]++;
		Database::GetDatabase()->SetMonsterSeenCount(m_id, m_encounter->GetSpecies()->GetIndex(),
			m_seen[m_encounter->GetSpecies()->GetIndex()]);
		EndEncounter(false, type);
		return THROW_RESULT_RUN_AWAY_AFTER_THREE;
	}
}


void ServerPlayer::RunFromEncounter()
{
	EndEncounter(false);
}


bool ServerPlayer::PowerUpMonster(std::shared_ptr<Monster> monster)
{
	if (monster->GetLevel() >= GetLevel())
		return false;
	if (monster->GetLevel() >= 40)
		return false;
	if (monster->IsDefending())
		return false;

	if (GetTreatsForSpecies(monster->GetSpecies()) < GetPowerUpCost(monster->GetLevel()).treats)
		return false;
	if (GetPowder() < GetPowerUpCost(monster->GetLevel()).powder)
		return false;

	m_treats[monster->GetSpecies()->GetBaseForm()->GetIndex()] -= GetPowerUpCost(monster->GetLevel()).treats;
	Database::GetDatabase()->SetTreats(m_id, monster->GetSpecies()->GetBaseForm()->GetIndex(),
		m_treats[monster->GetSpecies()->GetBaseForm()->GetIndex()]);
	m_powder -= GetPowerUpCost(monster->GetLevel()).powder;
	monster->PowerUp();
	Database::GetDatabase()->SetPowder(m_id, m_powder);
	Database::GetDatabase()->UpdateMonster(m_id, monster);
	return true;
}


bool ServerPlayer::EvolveMonster(std::shared_ptr<Monster> monster)
{
	if (monster->GetSpecies()->GetEvolutions().size() == 0)
		return false;
	if (GetTreatsForSpecies(monster->GetSpecies()) < monster->GetSpecies()->GetEvolutionCost())
		return false;
	if (monster->IsDefending())
		return false;

	m_treats[monster->GetSpecies()->GetBaseForm()->GetIndex()] -= monster->GetSpecies()->GetEvolutionCost();
	monster->Evolve();

	if (GetNumberCaptured(monster->GetSpecies()) == 0)
		EarnExperience(1000);
	else
		EarnExperience(500);
	m_seen[monster->GetSpecies()->GetIndex()]++;
	Database::GetDatabase()->SetMonsterSeenCount(m_id, monster->GetSpecies()->GetIndex(),
		m_seen[monster->GetSpecies()->GetIndex()]);
	m_captured[monster->GetSpecies()->GetIndex()]++;
	Database::GetDatabase()->SetMonsterCapturedCount(m_id, monster->GetSpecies()->GetIndex(),
		m_captured[monster->GetSpecies()->GetIndex()]);
	m_treats[monster->GetSpecies()->GetBaseForm()->GetIndex()]++;
	Database::GetDatabase()->SetTreats(m_id, monster->GetSpecies()->GetBaseForm()->GetIndex(),
		m_treats[monster->GetSpecies()->GetBaseForm()->GetIndex()]);
	Database::GetDatabase()->UpdateMonster(m_id, monster);
	return true;
}


void ServerPlayer::TransferMonster(std::shared_ptr<Monster> monster)
{
	if (monster->IsDefending())
		return;

	auto i = m_monsters.find(monster->GetID());
	if (i == m_monsters.end())
		return;

	m_treats[monster->GetSpecies()->GetBaseForm()->GetIndex()]++;
	Database::GetDatabase()->SetTreats(m_id, monster->GetSpecies()->GetBaseForm()->GetIndex(),
		m_treats[monster->GetSpecies()->GetBaseForm()->GetIndex()]);
	Database::GetDatabase()->RemoveMonster(m_id, monster);
	m_monsters.erase(i);
}


void ServerPlayer::SetMonsterName(std::shared_ptr<Monster> monster, const string& name)
{
	if (monster->IsDefending())
		return;

	monster->SetName(name);
	Database::GetDatabase()->UpdateMonster(m_id, monster);
}


uint8_t ServerPlayer::GetMapTile(int32_t x, int32_t y)
{
	return World::GetWorld()->GetMapTile(x, y);
}


bool ServerPlayer::IsStopAvailable(int32_t x, int32_t y)
{
	if (GetMapTile(x, y) != TILE_STOP)
		return false;
	for (auto& i : m_recentStopsVisited)
	{
		if ((i.x == x) && (i.y == y) && ((time(NULL) - i.visitTime) < 300))
			return false;
	}
	return true;
}


map<ItemType, uint32_t> ServerPlayer::GetItemsFromStop(int32_t x, int32_t y)
{
	if (!IsStopAvailable(x, y))
		return map<ItemType, uint32_t>();

	// Clear out old visits from recent list
	for (size_t i = 0; i < m_recentStopsVisited.size(); )
	{
		if ((m_recentStopsVisited[i].visitTime - time(NULL)) > 300)
		{
			m_recentStopsVisited.erase(m_recentStopsVisited.begin() + i);
			continue;
		}
		i++;
	}

	if (m_recentStopsVisited.size() > MAX_STOPS_WITHIN_COOLDOWN)
		return map<ItemType, uint32_t>();

	map<ItemType, uint32_t> itemWeights;
	itemWeights[ITEM_STANDARD_BALL] = 20;
	itemWeights[ITEM_MEGA_SEED] = 3;
	if (GetLevel() >= 5)
		itemWeights[ITEM_STANDARD_HEAL] = 8;
	if (GetLevel() >= 10)
		itemWeights[ITEM_SUPER_BALL] = 7;
	if (GetLevel() >= 15)
		itemWeights[ITEM_SUPER_HEAL] = 3;
	if (GetLevel() >= 20)
		itemWeights[ITEM_UBER_BALL] = 3;
	if (GetLevel() >= 25)
		itemWeights[ITEM_KEG_OF_HEALTH] = 1;
	uint32_t totalWeight = 0;
	for (auto& i : itemWeights)
		totalWeight += i.second;

	map<ItemType, uint32_t> result;
	for (size_t count = 0; ; count++)
	{
		if (count >= 3)
		{
			if ((rand() % 10) > 3)
				break;
		}

		uint32_t value = rand() % totalWeight;
		uint32_t cur = 0;
		for (auto& i : itemWeights)
		{
			if (value < (cur + i.second))
			{
				result[i.first]++;
				break;
			}
			cur += i.second;
		}
	}

	for (auto& i : result)
	{
		m_inventory[i.first] += i.second;
		Database::GetDatabase()->SetInventory(m_id, i.first, m_inventory[i.first]);
	}

	// Add this visit to the recent list so that it can't be used until the cooldown expires
	RecentStopVisit visit;
	visit.x = x;
	visit.y = y;
	visit.visitTime = time(NULL);
	m_recentStopsVisited.push_back(visit);
	return result;
}


void ServerPlayer::SetTeam(Team team)
{
	if (m_team != TEAM_UNASSIGNED)
		return;
	if (m_level < 5)
		return;
	m_team = team;
	Database::GetDatabase()->SetTeam(m_id, team);
}


Team ServerPlayer::GetPitTeam(int32_t x, int32_t y)
{
	return World::GetWorld()->GetPitTeam(x, y);
}


uint32_t ServerPlayer::GetPitReputation(int32_t x, int32_t y)
{
	return World::GetWorld()->GetPitReputation(x, y);
}


vector<shared_ptr<Monster>> ServerPlayer::GetPitDefenders(int32_t x, int32_t y)
{
	return World::GetWorld()->GetPitDefenders(x, y);
}


bool ServerPlayer::AssignPitDefender(int32_t x, int32_t y, shared_ptr<Monster> monster)
{
	return World::GetWorld()->AssignPitDefender(x, y, m_team, monster);
}


bool ServerPlayer::StartPitBattle(int32_t x, int32_t y, vector<shared_ptr<Monster>> monsters)
{
	Team pitTeam = GetPitTeam(x, y);
	if ((x == PIT_OF_DOOM_X) && (y == PIT_OF_DOOM_Y))
	{
		if (m_level < 40)
		{
			printf("Player %s trying to battle Pit of Doom below level 40.\n", m_name.c_str());
			return false;
		}
	}
	else
	{
		if (pitTeam == TEAM_UNASSIGNED)
		{
			printf("Player %s trying to battle pit, but pit has no defenders.\n", m_name.c_str());
			return false;
		}
		if ((pitTeam == m_team) && (GetPitReputation(x, y) >= MAX_PIT_REPUTATION))
		{
			printf("Player %s trying to battle pit, but pit is already max reputation.\n", m_name.c_str());
			return false;
		}
	}

	if (m_team == TEAM_UNASSIGNED)
	{
		printf("Player %s trying to battle pit, but has no team.\n", m_name.c_str());
		return false;
	}
	if (monsters.size() == 0)
	{
		printf("Player %s trying to battle pit, but has no valid attackers.\n", m_name.c_str());
		return false;
	}

	set<uint64_t> seen;
	for (auto& i : monsters)
	{
		if (seen.count(i->GetID()) > 0)
		{
			printf("Player %s trying to battle pit, but has duplicate attackers.\n", m_name.c_str());
			return false;
		}
		if (i->GetOwnerID() != m_id)
		{
			printf("Player %s trying to battle pit, but is trying to attack with a monster that isn't theirs.\n", m_name.c_str());
			return false;
		}
		if (i->GetCurrentHP() == 0)
		{
			printf("Player %s trying to battle pit, but is trying to attack with a fainted monster.\n", m_name.c_str());
			return false;
		}
		if (i->IsDefending())
		{
			printf("Player %s trying to battle pit, but is trying to attack with a defender.\n", m_name.c_str());
			return false;
		}
		seen.insert(i->GetID());
	}

	vector<shared_ptr<Monster>> defenders = GetPitDefenders(x, y);
	if (defenders.size() == 0)
	{
		printf("Player %s trying to battle pit, but pit has no valid defenders.\n", m_name.c_str());
		return false;
	}

	shared_ptr<PitBattle> battle(new PitBattle(monsters, defenders, pitTeam == m_team, x, y, Database::GetDatabase()));
	m_battle = battle;
	return true;
}


vector<shared_ptr<Monster>> ServerPlayer::GetPitBattleDefenders()
{
	if (m_battle)
		return m_battle->GetDefenders();
	return vector<shared_ptr<Monster>>();
}


void ServerPlayer::SetAttacker(shared_ptr<Monster> monster)
{
	if (m_battle)
		m_battle->SetAttacker(monster);
}


PitBattleStatus ServerPlayer::StepPitBattle()
{
	if (m_battle)
		return m_battle->Step();

	PitBattleStatus status;
	status.state = PIT_BATTLE_WAITING_FOR_ACTION;
	status.charge = 0;
	status.attackerHP = 0;
	status.defenderHP = 0;
	return status;
}


void ServerPlayer::SetPitBattleAction(PitBattleAction action)
{
	if (m_battle)
		m_battle->SetAction(action);
}


uint32_t ServerPlayer::EndPitBattle()
{
	if (!m_battle)
		return 0;

	uint32_t reputationChange = m_battle->GetReputationChange();
	if (reputationChange == 0)
		return 0;

	if (m_battle->IsTraining())
	{
		reputationChange = World::GetWorld()->AddPitReputation(m_battle->GetPitX(), m_battle->GetPitY(),
			reputationChange);
	}
	else
	{
		reputationChange = World::GetWorld()->RemovePitReputation(m_battle->GetPitX(), m_battle->GetPitY(),
			reputationChange);
	}

	EarnExperience(reputationChange / 10);
	return reputationChange;
}


void ServerPlayer::HealMonster(std::shared_ptr<Monster> monster, ItemType type)
{
	if ((type != ITEM_STANDARD_HEAL) && (type != ITEM_SUPER_HEAL) && (type != ITEM_KEG_OF_HEALTH))
		return;
	if (monster->GetCurrentHP() == monster->GetMaxHP())
		return;
	if (!UseItem(type))
		return;

	if (type == ITEM_STANDARD_HEAL)
		monster->Heal(20);
	else if (type == ITEM_SUPER_HEAL)
		monster->Heal(60);
	else if (type == ITEM_KEG_OF_HEALTH)
		monster->Heal(1000);
	Database::GetDatabase()->UpdateMonster(m_id, monster);
}


void ServerPlayer::TravelToPitOfDoom()
{
	m_x = SPAWN_X;
	m_y = SPAWN_Y;
}


string ServerPlayer::GetLevel40Flag()
{
	if (m_level < 40)
		return "You are not level 40!";
	if (m_flaggedForBan)
		return string("Cheater! You were caught: ") + m_banReason;
	return "CSAW{Prepare4TrubbleAndMakeItDubble}";
}


string ServerPlayer::GetCatchEmAllFlag()
{
	for (auto& i : MonsterSpecies::GetAll())
	{
		if (GetNumberCaptured(i) == 0)
			return "You need to go catch 'em all.";
	}
	if (m_flaggedForBan)
		return string("Cheater! You were caught: ") + m_banReason;
	return "CSAW{ImDaBestLikeNo1EvarWuz}";
}
