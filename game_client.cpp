#include <stdio.h>
#include "monster.h"
#include "terminal.h"


int main(int argc, char* argv[])
{
	MonsterSpecies::Init();
	Terminal::Init();

	size_t queuedEvolutions = 0;
	for (auto monster : MonsterSpecies::GetAll())
	{
		printf("%3d %s %s\n", monster->GetIndex(), monster->GetImage().c_str(), monster->GetName().c_str());
		if (queuedEvolutions)
			queuedEvolutions--;
		if (monster->GetEvolutions().size() > 0)
			queuedEvolutions += monster->GetEvolutions().size();
		else if (queuedEvolutions == 0)
			printf("\n");
	}

	return 0;
}
