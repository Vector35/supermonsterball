#include <stdio.h>
#include <unistd.h>
#include "monster.h"
#include "world.h"
#include "inmemoryplayer.h"
#include "client.h"
#include "terminal.h"


int main(int argc, char* argv[])
{
	MonsterSpecies::Init();
	World::Init();
	Terminal::Init();

	Player* player = new InMemoryPlayer("Trainer");
	GameLoop(player);
	return 0;
}
