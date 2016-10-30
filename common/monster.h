#pragma once

#include <string>
#include <vector>
#include <inttypes.h>
#include "item.h"

enum Element
{
	Normal,
	Grass,
	Fire,
	Water,
	Electric,
	Bug,
	Poison,
	Psychic,
	Flying,
	Sound,
	Ground,
	Fighting,
	Ice,
	Light,
	Dark
};

enum MoveType
{
	QuickMove,
	SingleChargeMove,
	TwoChargeMove,
	ThreeChargeMove,
	FourChargeMove
};

class Move
{
	std::string m_name;
	Element m_element;
	MoveType m_type;
	uint32_t m_power, m_dps;

public:
	Move(const std::string& name, Element element, MoveType type, uint32_t power, uint32_t dps);

	const std::string& GetName() const { return m_name; }
	Element GetElement() const { return m_element; }
	MoveType GetType() const { return m_type; }
	uint32_t GetPower() const { return m_power; }
	uint32_t GetDamagePerSecond() const { return m_dps; }
};

struct Biome;

class MonsterSpecies
{
	uint32_t m_index;
	std::string m_image;
	std::string m_name;
	std::string m_description;
	Element m_type[2];
	uint32_t m_baseAttack, m_baseDefense, m_baseStamina;
	std::vector<MonsterSpecies*> m_evolutions;
	uint32_t m_evolutionCost;
	std::vector<Move*> m_quickMoves, m_powerMoves;

	static std::vector<MonsterSpecies*> m_list;

	Biome* m_commonBiome;
	uint32_t m_commonWeight, m_uncommonWeight;

public:
	MonsterSpecies(const std::string& image, const std::string& name, const std::string& description,
		Element type1, Element type2, uint32_t attack, uint32_t defense, uint32_t stamina,
		const std::vector<MonsterSpecies*>& evolutions, uint32_t evolutionCost,
		const std::vector<Move*>& quickMoves, const std::vector<Move*>& powerMoves,
		Biome* commonBiome, uint32_t commonWeight, uint32_t uncommonWeight);

	static void Init();
	static MonsterSpecies* GetByIndex(uint32_t monster);
	static std::vector<MonsterSpecies*> GetAll();

	uint32_t GetIndex() const { return m_index; }
	void SetIndex(uint32_t i) { m_index = i; }

	const std::string& GetImage() const { return m_image; }
	const std::string& GetName() const { return m_name; }
	const std::string& GetDescription() const { return m_description; }
	std::vector<Element> GetTypes() const;

	uint32_t GetBaseAttack() const { return m_baseAttack; }
	uint32_t GetBaseDefense() const { return m_baseDefense; }
	uint32_t GetBaseStamina() const { return m_baseStamina; }

	const std::vector<MonsterSpecies*>& GetEvolutions() const { return m_evolutions; }
	uint32_t GetEvolutionCost() const { return m_evolutionCost; }

	const std::vector<Move*>& GetQuickMoves() const { return m_quickMoves; }
	const std::vector<Move*>& GetPowerMoves() const { return m_powerMoves; }

	Biome* GetCommonBiome() const { return m_commonBiome; }
	uint32_t GetCommonWeight() const { return m_commonWeight; }
	uint32_t GetUncommonWeight() const { return m_uncommonWeight; }
};

class Monster
{
	MonsterSpecies* m_species;
	std::string m_name;
	uint32_t m_currentHP;
	uint32_t m_attackIV, m_defenseIV, m_staminaIV;
	uint32_t m_level;
	int32_t m_x, m_y;
	uint32_t m_spawnTime;
	bool m_captured;
	ItemType m_ball;

public:
	Monster(MonsterSpecies* species, int32_t x, int32_t y, uint32_t spawnTime);

	MonsterSpecies* GetSpecies() const { return m_species; }
	uint32_t GetAttackIV() const { return m_attackIV; }
	uint32_t GetDefenseIV() const { return m_defenseIV; }
	uint32_t GetStaminaIV() const { return m_staminaIV; }
	uint32_t GetLevel() const { return m_level; }
	uint32_t GetCurrentHP() const { return m_currentHP; }
	int32_t GetSpawnX() const { return m_x; }
	int32_t GetSpawnY() const { return m_y; }
	uint32_t GetSpawnTime() const { return m_spawnTime; }
	bool WasCaptured() const { return m_captured; }
	ItemType GetBallType() const { return m_ball; }

	void SetIV(uint32_t attack, uint32_t def, uint32_t stamina);
	void SetLevel(uint32_t level);
	void SetCapture(bool captured, ItemType ball);
	void ResetHP();
};

struct MonsterSighting
{
	MonsterSpecies* species;
	int32_t x, y;
};

struct SpawnType
{
	MonsterSpecies* species;
	uint32_t weight;
	SpawnType(MonsterSpecies* s, uint32_t w): species(s), weight(w) {}
};

struct Biome
{
	std::vector<SpawnType> spawns;
	static Biome* GetByName(const std::string& name);
};
