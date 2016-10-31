#include <map>
#include <stdlib.h>
#include "monster.h"

using namespace std;


vector<MonsterSpecies*> MonsterSpecies::m_list;
static map<string, Biome*> g_biomes;


Move::Move(const string& name, Element element, MoveType type, uint32_t power, uint32_t dps):
	m_name(name), m_element(element), m_type(type), m_power(power), m_dps(dps)
{
}


MonsterSpecies::MonsterSpecies(const string& image, const string& name, const string& description,
	Element type1, Element type2, uint32_t attack, uint32_t defense, uint32_t stamina,
	const vector<MonsterSpecies*>& evolutions, uint32_t evolutionCost,
	const vector<Move*>& quickMoves, const vector<Move*>& powerMoves,
	Biome* commonBiome, uint32_t commonWeight, uint32_t uncommonWeight):
	m_image(image), m_name(name), m_description(description),
	m_baseAttack(attack), m_baseDefense(defense), m_baseStamina(stamina),
	m_evolutions(evolutions), m_evolutionCost(evolutionCost),
	m_quickMoves(quickMoves), m_powerMoves(powerMoves),
	m_commonBiome(commonBiome), m_commonWeight(commonWeight), m_uncommonWeight(uncommonWeight)
{
	m_type[0] = type1;
	m_type[1] = type2;
	m_baseForm = this;

	for (auto& i : evolutions)
		i->SetBaseForm(this);
}


void MonsterSpecies::SetBaseForm(MonsterSpecies* baseForm)
{
	m_baseForm = baseForm;
	for (auto& i : m_evolutions)
		i->SetBaseForm(baseForm);
}


void MonsterSpecies::Init()
{
	Move* slice = new Move("Slice", Normal, QuickMove, 40, 12);
	Move* cut = new Move("Cut", Normal, QuickMove, 60, 15);
	Move* tackle = new Move("Tackle", Normal, QuickMove, 30, 10);
	Move* charge = new Move("Charge", Normal, FourChargeMove, 80, 24);
	Move* flop = new Move("Flop", Normal, QuickMove, 1, 1);
	Move* struggle = new Move("Struggle", Normal, FourChargeMove, 10, 5);
	Move* palindrome = new Move("Palindrome", Normal, QuickMove, 40, 18);
	Move* chomp = new Move("Chomp", Normal, ThreeChargeMove, 80, 23);

	Move* leafCutter = new Move("Leaf Cutter", Grass, QuickMove, 0, 0);
	Move* twigTwirl = new Move("Twig Twirl", Grass, QuickMove, 0, 0);

	Move* flare = new Move("Flare", Fire, QuickMove, 60, 15);
	Move* fireball = new Move("Fireball", Fire, QuickMove, 80, 20);
	Move* heatWave = new Move("Heat Wave", Fire, FourChargeMove, 70, 23);
	Move* flamethrower = new Move("Flamethrower", Fire, TwoChargeMove, 100, 28);
	Move* magmaBeam = new Move("Magma Beam", Fire, SingleChargeMove, 110, 29);
	Move* greatBallsOfFire = new Move("Great Balls of Fire", Fire, SingleChargeMove, 120, 32);
	Move* batteryFire = new Move("Battery Fire", Fire, TwoChargeMove, 90, 27);
	Move* handGrenade = new Move("Hand Grenade", Fire, SingleChargeMove, 120, 29);
	Move* burstFire = new Move("Burst Fire", Fire, QuickMove, 80, 20);
	Move* fullAuto = new Move("Full Auto", Fire, QuickMove, 20, 19);
	Move* aimedShot = new Move("Aimed Shot", Fire, SingleChargeMove, 150, 31);
	Move* burninate = new Move("Burninate", Fire, SingleChargeMove, 150, 33);
	Move* hotSauce = new Move("Hot Sauce", Fire, FourChargeMove, 80, 25);
	Move* charStar = new Move("Char Star", Fire, SingleChargeMove, 150, 33);

	Move* waterBlast = new Move("Water Blast", Water, QuickMove, 60, 18);
	Move* splash = new Move("Splash", Water, QuickMove, 5, 4);
	Move* pressureWash = new Move("Pressure Wash", Water, QuickMove, 40, 12);
	Move* steamJet = new Move("Steam Jet", Water, QuickMove, 80, 19);
	Move* heavyRain = new Move("Heavy Rain", Water, FourChargeMove, 80, 23);
	Move* flood = new Move("Flood", Water, ThreeChargeMove, 100, 24);
	Move* gusher = new Move("Gusher", Water, SingleChargeMove, 100, 28);
	Move* trickle = new Move("Trickle", Water, QuickMove, 20, 10);
	Move* tsunami = new Move("Tsunami", Water, SingleChargeMove, 120, 31);

	Move* zap = new Move("Zap", Electric, QuickMove, 10, 21);
	Move* staticCling = new Move("Static Cling", Electric, QuickMove, 6, 5);
	Move* staticDischarge = new Move("Static Discharge", Electric, QuickMove, 15, 19);
	Move* lightningBolt = new Move("Lightning Bolt", Electric, SingleChargeMove, 100, 29);
	Move* jumpStart = new Move("Jump Start", Electric, FourChargeMove, 70, 22);
	Move* shortCircuit = new Move("Short Circuit", Electric, QuickMove, 20, 18);

	Move* bugBite = new Move("Bug Bite", Bug, QuickMove, 0, 0);
	Move* mandibleMunch = new Move("Mandible Munch", Bug, QuickMove, 0, 0);
	Move* infestation = new Move("Infestation", Bug, QuickMove, 0, 0);
	Move* swarm = new Move("Swarm", Bug, QuickMove, 0, 0);
	Move* fleshEater = new Move("Flesh Eater", Bug, QuickMove, 0, 0);

	Move* infect = new Move("Infect", Poison, TwoChargeMove, 0, 0);
	Move* sting = new Move("Sting", Poison, QuickMove, 0, 0);
	Move* poisonFang = new Move("Poison Fang", Poison, ThreeChargeMove, 0, 0);
	Move* hemlockRain = new Move("Hemlock Spit", Poison, QuickMove, 0, 0);
	Move* botulismBite = new Move("Botulism Bite", Poison, QuickMove, 0, 0);

	Move* typeConfusion = new Move("Type Confusion", Psychic, QuickMove, 0, 0);
	Move* magicMissile = new Move("Magic Missile", Psychic, SingleChargeMove, 0, 0);
	Move* mindMelt = new Move("Mind Melt", Psychic, TwoChargeMove, 0, 0);
	Move* memoryCorruption = new Move("Memory Corruption", Psychic, SingleChargeMove, 0, 0);

	Move* wingAttack = new Move("Wing Attack", Flying, QuickMove, 0, 0);
	Move* diveBomb = new Move("Dive Bomb", Flying, QuickMove, 0, 0);
	Move* peck = new Move("Peck", Flying, QuickMove, 0, 0);
	Move* bigPecks = new Move("Big Pecks", Flying, ThreeChargeMove, 0, 0);
	Move* hurricane = new Move("Hurricane", Flying, SingleChargeMove, 0, 0);
	Move* cyclone = new Move("Cyclone", Flying, TwoChargeMove, 0, 0);

	Move* wub = new Move("Wub", Sound, QuickMove, 0, 0);
	Move* sonicBoom = new Move("Sonic Boom", Sound, SingleChargeMove, 0, 0);

	Move* dig = new Move("Dig", Ground, QuickMove, 0, 0);
	Move* trap = new Move("Trap", Ground, QuickMove, 0, 0);
	Move* quicksand = new Move("Quicksand", Ground, TwoChargeMove, 0, 0);
	Move* earthquake = new Move("Earthquake", Ground, SingleChargeMove, 0, 0);

	Move* punch = new Move("Punch", Fighting, QuickMove, 0, 0);
	Move* quickKick = new Move("Quick Kick", Fighting, QuickMove, 0, 0);
	Move* pound = new Move("Pound", Fighting, QuickMove, 0, 0);
	Move* boneBreak = new Move("Bone Break", Fighting, TwoChargeMove, 0, 0);
	Move* juggernaut = new Move("Juggernaut", Fighting, SingleChargeMove, 0, 0);

	Move* freeze = new Move("Freeze", Ice, QuickMove, 0, 0);
	Move* frostbite = new Move("Frostbite", Ice, QuickMove, 0, 0);
	Move* icePick = new Move("Ice Pick", Ice, QuickMove, 0, 0);
	Move* snowstorm = new Move("Snowstorm", Ice, TwoChargeMove, 0, 0);
	Move* blizzard = new Move("Blizzard", Ice, SingleChargeMove, 0, 0);
	Move* snowmageddon = new Move("Snowmageddon", Ice, SingleChargeMove, 0, 0);

	Move* sunBeam = new Move("Sun Beam", Light, QuickMove, 0, 0);
	Move* starlight = new Move("Starlight", Light, QuickMove, 0, 0);
	Move* laser = new Move("Frickin' Laser Beam", Light, TwoChargeMove, 0, 0);
	Move* nova = new Move("Nova", Light, SingleChargeMove, 0, 0);

	Move* sneakAttack = new Move("Sneak Attack", Dark, QuickMove, 0, 0);
	Move* backstab = new Move("Backstab", Dark, ThreeChargeMove, 0, 0);
	Move* bite = new Move("Bite", Dark, QuickMove, 0, 0);
	Move* ghostBlade = new Move("Ghost Blade", Dark, QuickMove, 0, 0);
	Move* hiddenDagger = new Move("Hidden Dagger", Dark, FourChargeMove, 0, 0);

	Biome* grassBiome = new Biome;
	Biome* waterBiome = new Biome;
	Biome* desertBiome = new Biome;
	Biome* cityBiome = new Biome;
	Biome* mountainBiome = new Biome;
	g_biomes["grass"] = grassBiome;
	g_biomes["water"] = waterBiome;
	g_biomes["desert"] = desertBiome;
	g_biomes["city"] = cityBiome;
	g_biomes["mountain"] = mountainBiome;

	MonsterSpecies* roppy = new MonsterSpecies("💥🐸 ", "Roppy", "This animatronic frog is assembled from a mishmash"
		"of other frog parts.", Grass, Grass, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, grassBiome, 5, 2);
	MonsterSpecies* poppy = new MonsterSpecies("✨🐸 ", "Poppy", "This frog prefers to be stacked up "
		"and the top frog always jumps first.", Grass, Grass, 0, 0, 0,
		vector<MonsterSpecies*>{roppy}, 100, vector<Move*>{}, vector<Move*>{}, grassBiome, 20, 8);
	MonsterSpecies* hoppy = new MonsterSpecies(" 🐸 ", "Hoppy", "A small frog that jumps right where it wants to go.", Grass, Grass, 0, 0, 0,
		vector<MonsterSpecies*>{poppy}, 25, vector<Move*>{}, vector<Move*>{}, grassBiome, 100, 15);
	m_list.push_back(hoppy);
	m_list.push_back(poppy);
	m_list.push_back(roppy);

	MonsterSpecies* kablion = new MonsterSpecies("💥🦁 ", "Kablion", "", Fire, Fire, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, desertBiome, 5, 2);
	MonsterSpecies* pyrion = new MonsterSpecies("🔥🦁 ", "Pyrion", "", Fire, Fire, 0, 0, 0,
		vector<MonsterSpecies*>{kablion}, 100, vector<Move*>{}, vector<Move*>{}, desertBiome, 20, 8);
	MonsterSpecies* frion = new MonsterSpecies(" 🦁 ", "Frion", "A cuddly lion that seems to blaze with an inner warmth.", Fire, Fire, 0, 0, 0,
		vector<MonsterSpecies*>{pyrion}, 25, vector<Move*>{}, vector<Move*>{}, desertBiome, 100, 15);
	m_list.push_back(frion);
	m_list.push_back(pyrion);
	m_list.push_back(kablion);

	MonsterSpecies* whalegun = new MonsterSpecies("🌊🐋 ", "Whalegun", "", Water, Water, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, waterBiome, 5, 2);
	MonsterSpecies* whaletail = new MonsterSpecies(" 🐋 ", "Whaletail", "", Water, Water, 0, 0, 0,
		vector<MonsterSpecies*>{whalegun}, 100, vector<Move*>{}, vector<Move*>{}, waterBiome, 20, 8);
	MonsterSpecies* wailer = new MonsterSpecies(" 🐳 ", "Wailer", "", Water, Water, 0, 0, 0,
		vector<MonsterSpecies*>{whaletail}, 25, vector<Move*>{}, vector<Move*>{}, waterBiome, 100, 15);
	m_list.push_back(wailer);
	m_list.push_back(whaletail);
	m_list.push_back(whalegun);

	MonsterSpecies* beezer = new MonsterSpecies(" 🐝 ", "Beezer", "", Bug, Bug, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{sting}, vector<Move*>{poisonFang}, grassBiome, 20, 10);
	MonsterSpecies* nutter = new MonsterSpecies(" 🌰 ", "Nutter", "", Bug, Bug, 0, 0, 0,
		vector<MonsterSpecies*>{beezer}, 50, vector<Move*>{}, vector<Move*>{}, grassBiome, 40, 20);
	MonsterSpecies* crawler = new MonsterSpecies(" 🐛 ", "Crawler", "", Bug, Bug, 0, 0, 0,
		vector<MonsterSpecies*>{nutter}, 12, vector<Move*>{}, vector<Move*>{}, grassBiome, 500, 300);
	m_list.push_back(crawler);
	m_list.push_back(nutter);
	m_list.push_back(beezer);

	MonsterSpecies* cock = new MonsterSpecies(" 🐓 ", "Cock", "", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{trickle, wingAttack}, vector<Move*>{bigPecks, gusher},
		grassBiome, 20, 10);
	MonsterSpecies* motherclucker = new MonsterSpecies(" 🐔 ", "Motherclucker", "", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, grassBiome, 20, 10);
	MonsterSpecies* clucker = new MonsterSpecies(" 🐥 ", "Clucker", "", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{cock, motherclucker}, 50, vector<Move*>{}, vector<Move*>{}, grassBiome, 60, 40);
	MonsterSpecies* chickling = new MonsterSpecies(" 🐣 ", "Chickling", "", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{clucker}, 12, vector<Move*>{}, vector<Move*>{}, grassBiome, 500, 350);
	m_list.push_back(chickling);
	m_list.push_back(clucker);
	m_list.push_back(cock);
	m_list.push_back(motherclucker);

	MonsterSpecies* windove = new MonsterSpecies("🌪🕊 ", "Windove", "It is able to control the power of the wind to "
		"effortlessly fly around the world.", Flying, Flying, 0, 0, 0, vector<MonsterSpecies*>{}, 0,
		vector<Move*>{diveBomb, wingAttack}, vector<Move*>{hurricane, cyclone}, cityBiome, 25, 15);
	MonsterSpecies* birdove = new MonsterSpecies(" 🕊 ", "Birdove", "", Flying, Flying, 0, 0, 0,
		vector<MonsterSpecies*>{windove}, 50, vector<Move*>{}, vector<Move*>{}, cityBiome, 100, 75);
	MonsterSpecies* dovelett = new MonsterSpecies(" 🐦 ", "Dovelett", "", Flying, Flying, 0, 0, 0,
		vector<MonsterSpecies*>{birdove}, 12, vector<Move*>{}, vector<Move*>{}, cityBiome, 1000, 750);
	m_list.push_back(dovelett);
	m_list.push_back(birdove);
	m_list.push_back(windove);

	MonsterSpecies* rattichewy = new MonsterSpecies("🧀🐀 ", "Rattichewy", "", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, cityBiome, 40, 35);
	MonsterSpecies* rattitat = new MonsterSpecies(" 🐀 ", "Rattitat", "", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{rattichewy}, 50, vector<Move*>{}, vector<Move*>{}, cityBiome, 1000, 900);
	m_list.push_back(rattitat);
	m_list.push_back(rattichewy);

	MonsterSpecies* woofer = new MonsterSpecies(" 🐕 ", "Woofer", "", Sound, Sound, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{wub}, vector<Move*>{sonicBoom}, cityBiome, 20, 10);
	MonsterSpecies* subhound = new MonsterSpecies(" 🐶 ", "Subhound", "", Sound, Sound, 0, 0, 0,
		vector<MonsterSpecies*>{woofer}, 50, vector<Move*>{}, vector<Move*>{}, cityBiome, 150, 75);
	m_list.push_back(subhound);
	m_list.push_back(woofer);

	MonsterSpecies* webadeth = new MonsterSpecies("🕸🕷 ", "Webadeth", "", Bug, Poison, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{poisonFang}, grassBiome, 20, 15);
	MonsterSpecies* spidra = new MonsterSpecies(" 🕷 ", "Spidra", "", Bug, Poison, 0, 0, 0,
		vector<MonsterSpecies*>{webadeth}, 50, vector<Move*>{}, vector<Move*>{}, grassBiome, 200, 150);
	m_list.push_back(spidra);
	m_list.push_back(webadeth);

	MonsterSpecies* drillboar = new MonsterSpecies(" 🐗 ", "Drillboar", "This goring, boring, boar is known for "
		"being hairy. Oh, and also boring.", Ground, Ground, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, grassBiome, 5, 3);
	MonsterSpecies* boghog = new MonsterSpecies(" 🐖 ", "Boghog", "Known for their surprising stealth, this porker "
		"creeps out of the mist and attacks you if unprepared.", Ground, Ground, 0, 0, 0,
		vector<MonsterSpecies*>{drillboar}, 100, vector<Move*>{}, vector<Move*>{}, grassBiome, 30, 10);
	MonsterSpecies* digpig = new MonsterSpecies(" 🐷 ", "Digpig", "Do you have any mud? Because this pig wants "
		"nothing more than to lie in it all day.", Ground, Ground, 0, 0, 0,
		vector<MonsterSpecies*>{boghog}, 25, vector<Move*>{}, vector<Move*>{}, grassBiome, 200, 50);
	m_list.push_back(digpig);
	m_list.push_back(boghog);
	m_list.push_back(drillboar);

	MonsterSpecies* ramstine = new MonsterSpecies(" 🐏 ", "Ramstine", "You're not quite sure what the noises coming "
		"out of this large sheep are, but they're quite gutteral and scary.", Normal, Grass, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, grassBiome, 15, 3);
	MonsterSpecies* sheepler = new MonsterSpecies(" 🐑 ", "Sheepler", "This docile sheep seems to do whatever "
		"everyone around it is doing.", Normal, Grass, 0, 0, 0,
		vector<MonsterSpecies*>{ramstine}, 50, vector<Move*>{}, vector<Move*>{}, grassBiome, 150, 30);
	m_list.push_back(sheepler);
	m_list.push_back(ramstine);

	MonsterSpecies* stingping = new MonsterSpecies(" 🦂 ", "Stingping", "The tricky part about the stinger on this "
		"dangerous beast is that it can trick your immune system into attacking itself.", Bug, Poison, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, desertBiome, 15, 5);
	MonsterSpecies* antler = new MonsterSpecies(" 🐜 ", "Antler", "This small bug is well known for attacking deer "
		"ticks.", Bug, Poison, 0, 0, 0,
		vector<MonsterSpecies*>{stingping}, 50, vector<Move*>{}, vector<Move*>{}, desertBiome, 200, 75);
	m_list.push_back(antler);
	m_list.push_back(stingping);

	MonsterSpecies* harerazer = new MonsterSpecies(" 🐇 ", "Harerazer", "Something about this rabbit gives you the "
		"creeps. Maybe it's the spikes, or the grooves that cross its skin.", Electric, Electric, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{lightningBolt}, desertBiome, 15, 3);
	MonsterSpecies* bunnybolt = new MonsterSpecies(" 🐰 ", "Bunnybolt", "This cuddly rabbit can give you quite a "
		"shock!", Electric, Electric, 0, 0, 0,
		vector<MonsterSpecies*>{harerazer}, 50, vector<Move*>{}, vector<Move*>{}, desertBiome, 150, 30);
	m_list.push_back(bunnybolt);
	m_list.push_back(harerazer);

	MonsterSpecies* wabbitwap = new MonsterSpecies("🕳🐇 ", "Wabbitwap", "", Normal, Ground, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{trap}, vector<Move*>{earthquake}, grassBiome, 10, 4);
	MonsterSpecies* wascaly = new MonsterSpecies("🕳🐰 ", "Wascaly", "", Normal, Ground, 0, 0, 0,
		vector<MonsterSpecies*>{wabbitwap}, 50, vector<Move*>{dig}, vector<Move*>{}, grassBiome, 100, 40);
	m_list.push_back(wascaly);
	m_list.push_back(wabbitwap);

	MonsterSpecies* ohkamel = new MonsterSpecies(" 🐪 ", "Ohkamel", "A very functional beast of burden that looks "
		"quite powerful except for its inability to work in a herd.", Normal, Water, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, desertBiome, 20, 2);
	MonsterSpecies* kamelcase = new MonsterSpecies(" 🐫 ", "Kamelcase", "An easy to work with camel with two "
		"humps that are exactly the same height.", Normal, Water, 0, 0, 0,
		vector<MonsterSpecies*>{ohkamel}, 50, vector<Move*>{}, vector<Move*>{}, desertBiome, 175, 20);
	m_list.push_back(kamelcase);
	m_list.push_back(ohkamel);

	MonsterSpecies* glostar = new MonsterSpecies("🌟🌟 ", "Glostar", "A brilliant star that overshadows "
		"all else.", Light, Light, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, mountainBiome, 5, 1);
	MonsterSpecies* costar = new MonsterSpecies("⭐⭐ ", "Costar", "Always stuck in the shadow of something brighter, "
		"and not happy about it.", Light, Light, 0, 0, 0,
		vector<MonsterSpecies*>{glostar}, 100, vector<Move*>{}, vector<Move*>{}, mountainBiome, 30, 5);
	MonsterSpecies* starem = new MonsterSpecies(" ⭐ ", "Starem", "A pretty, twinkling star.", Light, Light, 0, 0, 0,
		vector<MonsterSpecies*>{costar}, 25, vector<Move*>{}, vector<Move*>{}, mountainBiome, 150, 25);
	m_list.push_back(starem);
	m_list.push_back(costar);
	m_list.push_back(glostar);

	MonsterSpecies* stormikloud = new MonsterSpecies(" ⛈ ", "Stormikloud", "A massive storm capable of vast "
		"destruction.", Water, Electric, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, waterBiome, 10, 7);
	MonsterSpecies* electrikloud = new MonsterSpecies(" 🌩 ", "Electrikloud", "A dangerous cloud capable of both "
		"water and electric attacks.", Water, Electric, 0, 0, 0,
		vector<MonsterSpecies*>{stormikloud}, 100, vector<Move*>{}, vector<Move*>{}, waterBiome, 40, 25);
	MonsterSpecies* hoverkloud = new MonsterSpecies(" 🌥 ", "Hoverkloud", "A small cloud that seems to have trouble "
		"getting very high.", Water, Water, 0, 0, 0,
		vector<MonsterSpecies*>{electrikloud}, 25, vector<Move*>{}, vector<Move*>{}, waterBiome, 125, 80);
	m_list.push_back(hoverkloud);
	m_list.push_back(electrikloud);
	m_list.push_back(stormikloud);

	MonsterSpecies* snowmo = new MonsterSpecies("❄⛄ ", "Snowmo", "If you ask him for mercy, he'll just tell you "
		"there is no 'mo.", Ice, Ice, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{blizzard, snowmageddon}, mountainBiome, 5, 1);
	MonsterSpecies* snowblo = new MonsterSpecies("🌪⛄ ", "Snowblo", "The air that swirls around this snowman is "
		"bitter cold.", Ice, Ice, 0, 0, 0,
		vector<MonsterSpecies*>{snowmo}, 100, vector<Move*>{}, vector<Move*>{snowstorm, blizzard}, mountainBiome, 30, 3);
	MonsterSpecies* snowbro = new MonsterSpecies(" ⛄ ", "Snowbro", "A very chill snowman who seems to like just "
		"hanging out.", Ice, Ice, 0, 0, 0,
		vector<MonsterSpecies*>{snowblo}, 25, vector<Move*>{}, vector<Move*>{}, mountainBiome, 200, 10);
	m_list.push_back(snowbro);
	m_list.push_back(snowblo);
	m_list.push_back(snowmo);

	MonsterSpecies* krabber = new MonsterSpecies(" 🦀 ", "Krabber", "Watch out, they jump.", Water, Water, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, waterBiome, 20, 3);
	MonsterSpecies* shellkra = new MonsterSpecies(" 🐚 ", "Shellkra", "If you think you like popping shells, you "
		"better watch out, Shellkra might want revenge.", Water, Water, 0, 0, 0,
		vector<MonsterSpecies*>{krabber}, 50, vector<Move*>{}, vector<Move*>{}, waterBiome, 150, 10);
	m_list.push_back(shellkra);
	m_list.push_back(krabber);

	MonsterSpecies* oktokore = new MonsterSpecies(" 🐙 ", "Oktokore", "It has eight brains and has been known to "
		"capture eight prey at the same time.", Water, Water, 0, 0, 0, vector<MonsterSpecies*>{}, 0,
		vector<Move*>{}, vector<Move*>{}, waterBiome, 80, 8);
	m_list.push_back(oktokore);

	MonsterSpecies* snukinasnail = new MonsterSpecies(" 🐌 ", "Snukinasnail", "It slowly moves in the shadows but "
		"strikes its target fiercely when least expected.", Water, Dark, 0, 0, 0, vector<MonsterSpecies*>{}, 0,
		vector<Move*>{sneakAttack}, vector<Move*>{}, grassBiome, 75, 45);
	m_list.push_back(snukinasnail);

	MonsterSpecies* lazybug = new MonsterSpecies(" 🐞 ", "Lazybug", "This bug is too lazy to move, so it uses its "
		"psychic powers to force its food to come near.", Bug, Psychic, 0, 0, 0, vector<MonsterSpecies*>{}, 0,
		vector<Move*>{}, vector<Move*>{}, cityBiome, 125, 80);
	m_list.push_back(lazybug);

	MonsterSpecies* turtlejet = new MonsterSpecies("🌊🐢 ", "Turtlejet", "Capable of both extremely high and "
		"extremely low speeds, this animal is very power efficient.", Water, Water, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, waterBiome, 20, 4);
	MonsterSpecies* turtlewet = new MonsterSpecies(" 🐢 ", "Turtlewet", "The water that accompanies it wherever it "
		"goes differentiates it from other, more dry, turtles.", Water, Water, 0, 0, 0,
		vector<MonsterSpecies*>{turtlejet}, 50, vector<Move*>{}, vector<Move*>{}, waterBiome, 150, 30);
	m_list.push_back(turtlewet);
	m_list.push_back(turtlejet);

	MonsterSpecies* gatorath = new MonsterSpecies("💥🐊 ", "Gatorath", "An angry gator is never a pleasant foe. And "
		"even though you've never met this one, he already doesn't like you.", Water, Water, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, waterBiome, 10, 2);
	MonsterSpecies* gatorate = new MonsterSpecies(" 🐊 ", "Gatorate", "If you stop to ask what the gatorate, the "
		"answer is probably YOU.", Water, Water, 0, 0, 0,
		vector<MonsterSpecies*>{gatorath}, 50, vector<Move*>{}, vector<Move*>{}, waterBiome, 60, 20);
	m_list.push_back(gatorate);
	m_list.push_back(gatorath);

	MonsterSpecies* chiptune = new MonsterSpecies(" 🐿 ", "Chiptune", "The strange noises that come from this "
		"rodent's mouth sound nothing at all like crunching nuts.", Sound, Sound, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, grassBiome, 50, 30);
	m_list.push_back(chiptune);

	MonsterSpecies* cowabusta = new MonsterSpecies(" 🐄 ", "Cowabusta", "This bovine has serious rhythm. Just "
		"watch him MOO-VE!", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, grassBiome, 25, 10);
	MonsterSpecies* cowabucka = new MonsterSpecies(" 🐮 ", "Cowabucka", "If you try to go for a ride, you might find "
		"yourself cowed.", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{cowabusta}, 50, vector<Move*>{}, vector<Move*>{}, grassBiome, 120, 50);
	m_list.push_back(cowabucka);
	m_list.push_back(cowabusta);

	MonsterSpecies* monkadoo = new MonsterSpecies(" 🐒 ", "Monkadoo", "This odorous monkey appears to be fighting "
		"with a nasty biological weapon.", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, desertBiome, 15, 3);
	MonsterSpecies* monkasee = new MonsterSpecies(" 🐵 ", "Monkasee", "Careful what you do around this monkey. He's "
		"watching. Always watching.", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{monkadoo}, 50, vector<Move*>{}, vector<Move*>{}, desertBiome, 200, 50);
	m_list.push_back(monkasee);
	m_list.push_back(monkadoo);

	MonsterSpecies* unihorn = new MonsterSpecies(" 🦄 ", "Unihorn", "The startup time for this mythical creature "
		"appears short, and it's extremely highly valued.", Normal, Psychic, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, desertBiome, 10, 3);
	MonsterSpecies* warpsteed = new MonsterSpecies(" 🐎 ", "Warpsteed", "This horse canna work miracles, but it sure "
		"runs really fast.", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{unihorn}, 100, vector<Move*>{}, vector<Move*>{}, desertBiome, 50, 20);
	MonsterSpecies* fasteed = new MonsterSpecies(" 🐴 ", "Fasteed", "This purebread looks ready to race. Just let "
		"it warm up first.", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{warpsteed}, 25, vector<Move*>{}, vector<Move*>{}, desertBiome, 130, 40);
	m_list.push_back(fasteed);
	m_list.push_back(warpsteed);
	m_list.push_back(unihorn);

	MonsterSpecies* leptear = new MonsterSpecies(" 🐆 ", "Leptear", "", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, cityBiome, 2, 1);
	MonsterSpecies* kitear = new MonsterSpecies(" 🐈 ", "Kitear", "", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{leptear}, 100, vector<Move*>{}, vector<Move*>{}, cityBiome, 20, 10);
	MonsterSpecies* kitease = new MonsterSpecies(" 🐱 ", "Kitease", "An inviting cat that looks fun to play with but "
		"keeps backing up when you approach.", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{kitear}, 25, vector<Move*>{}, vector<Move*>{}, cityBiome, 300, 150);
	m_list.push_back(kitease);
	m_list.push_back(kitear);
	m_list.push_back(leptear);

	MonsterSpecies* jackfry = new MonsterSpecies("🔥🎃 ", "Jackfry", "", Fire, Dark, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, cityBiome, 5, 2);
	MonsterSpecies* jackflare = new MonsterSpecies(" 🎃 ", "Jackflare", "", Fire, Dark, 0, 0, 0,
		vector<MonsterSpecies*>{jackfry}, 50, vector<Move*>{}, vector<Move*>{}, cityBiome, 40, 20);
	m_list.push_back(jackflare);
	m_list.push_back(jackfry);

	MonsterSpecies* shroomdoom = new MonsterSpecies(" 🍄 ", "Shroomdoom", "", Dark, Poison, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, grassBiome, 75, 45);
	m_list.push_back(shroomdoom);

	MonsterSpecies* bonedread = new MonsterSpecies(" ☠ ", "Bonedread", "", Dark, Dark, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, desertBiome, 20, 4);
	MonsterSpecies* skullker = new MonsterSpecies(" 💀 ", "Skullker", "", Dark, Dark, 0, 0, 0,
		vector<MonsterSpecies*>{bonedread}, 50, vector<Move*>{}, vector<Move*>{}, desertBiome, 250, 40);
	m_list.push_back(skullker);
	m_list.push_back(bonedread);

	MonsterSpecies* spiriboo = new MonsterSpecies(" 👻 ", "Spiriboo", "Attracted to abandoned buildings. It enjoys "
		"giving jump scares to passerbys.", Dark, Dark, 0, 0, 0, vector<MonsterSpecies*>{}, 0,
		vector<Move*>{}, vector<Move*>{}, cityBiome, 80, 1);
	m_list.push_back(spiriboo);

	MonsterSpecies* impest = new MonsterSpecies(" 👿 ", "Impest", "", Dark, Psychic, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, mountainBiome, 200, 15);
	m_list.push_back(impest);

	MonsterSpecies* ogreat = new MonsterSpecies("👑👹 ", "Ogreat", "", Fighting, Fighting, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, mountainBiome, 30, 4);
	MonsterSpecies* ogrim = new MonsterSpecies(" 👹 ", "Ogrim", "", Fighting, Fighting, 0, 0, 0,
		vector<MonsterSpecies*>{ogreat}, 50, vector<Move*>{}, vector<Move*>{}, mountainBiome, 150, 35);
	m_list.push_back(ogrim);
	m_list.push_back(ogreat);

	MonsterSpecies* pilapoo = new MonsterSpecies(" 💩 ", "Pilapoo", "A sludge of incredibly vile nature gained a "
		"life of its own.", Poison, Poison, 0, 0, 0, vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{},
		cityBiome, 100, 35);
	m_list.push_back(pilapoo);

	MonsterSpecies* flaret = new MonsterSpecies("🔥👽 ", "Flaret", "", Fire, Fire, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, desertBiome, 5, 1);
	MonsterSpecies* electret = new MonsterSpecies("⚡👽 ", "Electret", "", Electric, Electric, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, cityBiome, 5, 1);
	MonsterSpecies* pouret = new MonsterSpecies("🌊👽 ", "Pouret", "", Water, Water, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, waterBiome, 5, 1);
	MonsterSpecies* freezet = new MonsterSpecies("❄👽 ", "Freezet", "", Ice, Ice, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, mountainBiome, 5, 1);
	MonsterSpecies* pixet = new MonsterSpecies(" 👾 ", "Pixet", "", Light, Sound, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, grassBiome, 5, 1);
	MonsterSpecies* eetee = new MonsterSpecies(" 👽 ", "Eetee", "An alien with an exotic genetic structure capable of "
		"quickly adapting to a wide variety of environments.", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{flaret, electret, pouret, freezet, pixet}, 25,
		vector<Move*>{}, vector<Move*>{}, cityBiome, 125, 125);
	m_list.push_back(eetee);
	m_list.push_back(flaret);
	m_list.push_back(electret);
	m_list.push_back(pouret);
	m_list.push_back(freezet);
	m_list.push_back(pixet);

	MonsterSpecies* flyver = new MonsterSpecies(" 💸 ", "Flyver", "Money has a way of disappearing"
	"--this monster does it by flying.", Normal, Flying, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, cityBiome, 10, 1);
	m_list.push_back(flyver);

	MonsterSpecies* wurmton = new MonsterSpecies("🌊🐉 ", "Wurmton", "", Water, Flying, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, waterBiome, 10, 1);
	MonsterSpecies* phishy = new MonsterSpecies(" 🐟 ", "Phishy", "A useless fish that can't do much of anything.",
		Water, Water, 0, 0, 0, vector<MonsterSpecies*>{wurmton}, 400, vector<Move*>{flop, splash},
		vector<Move*>{struggle}, waterBiome, 250, 5);
	m_list.push_back(phishy);
	m_list.push_back(wurmton);

	MonsterSpecies* bottabuzz = new MonsterSpecies("⚡🤖 ", "Bottabuzz", "", Electric, Electric, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, cityBiome, 10, 1);
	MonsterSpecies* botto = new MonsterSpecies(" 🤖 ", "Botto", "", Electric, Electric, 0, 0, 0,
		vector<MonsterSpecies*>{bottabuzz}, 50, vector<Move*>{}, vector<Move*>{}, cityBiome, 75, 5);
	m_list.push_back(botto);
	m_list.push_back(bottabuzz);

	MonsterSpecies* ohdaze = new MonsterSpecies(" 🌞 ", "Ohdaze", "", Light, Psychic, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{memoryCorruption}, desertBiome, 15, 5);
	MonsterSpecies* sundaze = new MonsterSpecies(" ☀ ", "Sundaze", "", Light, Light, 0, 0, 0,
		vector<MonsterSpecies*>{ohdaze}, 100, vector<Move*>{}, vector<Move*>{}, desertBiome, 120, 40);
	m_list.push_back(sundaze);
	m_list.push_back(ohdaze);

	MonsterSpecies* doomboom = new MonsterSpecies("💥💣 ", "Doomboom", "", Fire, Dark, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, desertBiome, 15, 10);
	MonsterSpecies* kabomb = new MonsterSpecies(" 💣 ", "Kabomb", "", Fire, Fire, 0, 0, 0,
		vector<MonsterSpecies*>{doomboom}, 50, vector<Move*>{}, vector<Move*>{}, desertBiome, 90, 60);
	m_list.push_back(kabomb);
	m_list.push_back(doomboom);

	MonsterSpecies* liarkake = new MonsterSpecies(" 🎂 ", "Lyarkake", "Fills the mind of anything around it with "
		"lies and broken promises.", Psychic, Psychic, 0, 0, 0, vector<MonsterSpecies*>{}, 0,
		vector<Move*>{}, vector<Move*>{}, cityBiome, 40, 10);
	m_list.push_back(liarkake);

	MonsterSpecies* tacocat = new MonsterSpecies(" 🌮 ", "Tacocat", "Looks like a delicious taco to draw its prey near.",
		Normal, Normal, 0, 0, 0, vector<MonsterSpecies*>{}, 0, vector<Move*>{palindrome}, vector<Move*>{hotSauce},
		cityBiome, 40, 10);
	m_list.push_back(tacocat);

	MonsterSpecies* tuxacool = new MonsterSpecies(" 🐧 ", "Tuxacool", "", Water, Ice, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, waterBiome, 90, 25);
	m_list.push_back(tuxacool);

	MonsterSpecies* panfoo = new MonsterSpecies(" 🐼 ", "Panfoo", "", Fighting, Fighting, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{}, desertBiome, 90, 25);
	m_list.push_back(panfoo);

	MonsterSpecies* crystrike = new MonsterSpecies(" 💎 ", "Crystrike", "", Light, Psychic, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{}, vector<Move*>{laser}, mountainBiome, 90, 25);
	m_list.push_back(crystrike);

	MonsterSpecies* galaxseven = new MonsterSpecies(" 📱 ", "Galaxseven", "A product release gone horribly wrong "
		"turned a smartphone into a monster.", Electric, Fire, 0, 0, 0, vector<MonsterSpecies*>{}, 0,
		vector<Move*>{shortCircuit}, vector<Move*>{batteryFire, handGrenade}, cityBiome, 150, 5);
	m_list.push_back(galaxseven);

	MonsterSpecies* burninator = new MonsterSpecies("🔥🐲 ", "Burninator", "It likes to burninate the peasants "
		"wherever it can find them.", Fire, Flying, 0, 0, 0, vector<MonsterSpecies*>{}, 0,
		vector<Move*>{}, vector<Move*>{}, grassBiome, 1, 1);
	MonsterSpecies* slithaburn = new MonsterSpecies("🔥🐍 ", "Slithaburn", "", Fire, Poison, 0, 0, 0,
		vector<MonsterSpecies*>{burninator}, 100, vector<Move*>{}, vector<Move*>{}, grassBiome, 8, 3);
	MonsterSpecies* slither = new MonsterSpecies(" 🐍 ", "Slither", "", Poison, Poison, 0, 0, 0,
		vector<MonsterSpecies*>{slithaburn}, 25, vector<Move*>{}, vector<Move*>{}, grassBiome, 30, 5);
	m_list.push_back(slither);
	m_list.push_back(slithaburn);
	m_list.push_back(burninator);

	MonsterSpecies* ehkaybear = new MonsterSpecies("🔫🐻 ", "Ehkaybear", "Commonly found guarding mysterious "
		"treasure chests and shooting anything that tries to open them.", Normal, Fire, 0, 0, 0,
		vector<MonsterSpecies*>{}, 0, vector<Move*>{fullAuto, burstFire}, vector<Move*>{aimedShot, handGrenade},
		grassBiome, 5, 3);
	MonsterSpecies* bearly = new MonsterSpecies(" 🐻 ", "Bearly", "", Normal, Normal, 0, 0, 0,
		vector<MonsterSpecies*>{ehkaybear}, 100, vector<Move*>{bite}, vector<Move*>{chomp}, grassBiome, 75, 15);
	m_list.push_back(bearly);
	m_list.push_back(ehkaybear);

	for (size_t i = 0; i < m_list.size(); i++)
	{
		m_list[i]->SetIndex(i + 1);

		grassBiome->spawns.push_back(SpawnType(m_list[i], (m_list[i]->GetCommonBiome() == grassBiome) ?
			m_list[i]->GetCommonWeight() : m_list[i]->GetUncommonWeight()));
		waterBiome->spawns.push_back(SpawnType(m_list[i], (m_list[i]->GetCommonBiome() == waterBiome) ?
			m_list[i]->GetCommonWeight() : m_list[i]->GetUncommonWeight()));
		desertBiome->spawns.push_back(SpawnType(m_list[i], (m_list[i]->GetCommonBiome() == desertBiome) ?
			m_list[i]->GetCommonWeight() : m_list[i]->GetUncommonWeight()));
		cityBiome->spawns.push_back(SpawnType(m_list[i], (m_list[i]->GetCommonBiome() == cityBiome) ?
			m_list[i]->GetCommonWeight() : m_list[i]->GetUncommonWeight()));
		mountainBiome->spawns.push_back(SpawnType(m_list[i], (m_list[i]->GetCommonBiome() == mountainBiome) ?
			m_list[i]->GetCommonWeight() : m_list[i]->GetUncommonWeight()));
	}
}


MonsterSpecies* MonsterSpecies::GetByIndex(uint32_t monster)
{
	if (monster == 0)
		return nullptr;
	if (monster > m_list.size())
		return nullptr;
	return m_list[monster - 1];
}


vector<MonsterSpecies*> MonsterSpecies::GetAll()
{
	return m_list;
}


vector<Element> MonsterSpecies::GetTypes() const
{
	if (m_type[0] == m_type[1])
		return vector<Element>{m_type[0]};
	return vector<Element>{m_type[0], m_type[1]};
}


Monster::Monster(MonsterSpecies* species, int32_t x, int32_t y, uint32_t spawnTime): m_species(species),
	m_x(x), m_y(y), m_spawnTime(spawnTime)
{
	m_name = species->GetName();
	m_currentHP = 0;
	m_attackIV = m_defenseIV = m_staminaIV = 0;
	m_size = 16;
	m_level = 1;
	m_captured = false;
	m_ball = ITEM_STANDARD_BALL;
}


void Monster::SetID(uint64_t id)
{
	m_id = id;
}


void Monster::SetIV(uint32_t attack, uint32_t def, uint32_t stamina)
{
	m_attackIV = attack;
	m_defenseIV = def;
	m_staminaIV = stamina;
}


void Monster::SetSize(uint32_t size)
{
	m_size = size;
}


void Monster::SetLevel(uint32_t level)
{
	m_level = level;
}


void Monster::SetCapture(bool captured, ItemType ball)
{
	m_captured = captured;
	m_ball = ball;
}


void Monster::ResetHP()
{
}


void Monster::Evolve()
{
	if (m_species->GetEvolutions().size() == 0)
		return;

	bool renamed = (m_name != m_species->GetName());

	size_t choice = rand() % m_species->GetEvolutions().size();
	m_species = m_species->GetEvolutions()[choice];

	if (!renamed)
		m_name = m_species->GetName();
}


Biome* Biome::GetByName(const string& name)
{
	auto i = g_biomes.find(name);
	if (i == g_biomes.end())
		return nullptr;
	return i->second;
}
