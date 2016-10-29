# Super Monster Ball

This is a CTF challenge released at CSAW 2016 that is a parody of the hit game Pokémon Go. The goal of the game can be reached through normal gameplay, but not within the time limits of the CTF. To win the game and get the flag at the CTF, you must reverse engineer the game and build a bot that can out-level any human player. But watch out, the periodic ban wave system might catch you cheating!

## Building and running

The game has been tested on Mac and Linux. The included Makefile will build the components of the game.

To run the game server, simply start `game_server`. The `game_client` executable is the client that is distributed to players during the CTF.

For testing, a `standalone` executable is also generated, which runs the gameplay with an in-memory virtual server.
