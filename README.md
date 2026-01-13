# Super Mario Odyssey Online - PLUS

Welcome to the official repository for the Super Mario Odyssey Online - PLUS (SMOO-Plus) mod! 

SMOO-Plus is a version of the Super Mario Odyssey Online mod which adds multiple new features, optimisations and a LOT more


## Features

* Explore Kingdoms together with up to 16 People
* Almost every capture in the game is synced between players
* Full 2D and Costume models syncing
* Moon Collection is shared between all players
* Custom Configuration Menu
* Support for custom gamemodes 
### Available Gamemodes
* Hide and Seek
* Sardines
* Freeze Tag

## SMO Version Support

* 1.0

## Installation Tutorial

Before installing, Ensure that your switch is hacked. If not, follow [This Guide](https://switch.homebrew.guide/) to get your switch setup for modding. Make sure you set up a way to block Nintendo's servers as you will need to have your switch connected to the internet for this mod to work!

1. Download the latest mod build from either from the [Releases](https://github.com/DaDev123/NEW-SMOO-Plus/releases/latest) tab. (Alternatively, build from source)
2. Extract the downloaded zip onto the root of your Switch's SD card.
3. If you need to host an online server, head over to the [Super Mario Odyssey Online - PLUS Server](https://github.com/KleinTimmi/Smoo-Plus-Server) repository and follow the instructions there to set up the server.
4. Launch the game! Upon first time bootup, the mod should ask for a server IP to save to the games common save file. This IP address will be the server you wish to connect to every time you launch the game with the mod installed. (Note: un-installing the mod and launching the game will remove the server IP from the common save file.)

## Gamemode Info
### Hide and Seek
* Depending on Group size, select who will start as seekers at the beginning of each round and a kingdom to hide in. 
* Each player has a timer on the top right of the screen that will increase while they are hiding during a round. 
* When a seeker gets close enough to a player, the player will die and respawn as a seeker.
* During the round, hiders who die by other means will also become seekers upon respawning.
* If a hider loads into a new stage (via a pipe, door, etc.) the hider will get 5 seconds of tag invincibility to prevent spawn point camping.
* The player with the most time at the end of a round (or set of rounds) is considered the winner.
* While not a concrete rule, it's generally agreed upon that hiding should not be done out of bounds, inside objects that don't sync across games yet, and inside objects that completely conceal a player from others (such as trees).

### Sardines
* Depending on Group size, select who will start as the sardine at the beginning of each round and a kingdom to hide in. 
* Each player has a timer on the top right of the screen that will increase while they are hiding during a round. 
* When a seeker gets close enough to a player, the seeker will join the sardine team and hide with them.
* The player with the most time at the end of a round (or set of rounds) is considered the winner.
* While not a concrete rule, it's generally agreed upon that hiding should not be done out of bounds, inside objects that don't sync across games yet, and inside objects that completely conceal a player from others (such as trees).

### Freeze Tag
* Depending on Group size, select who will start as the chasers at the beginning of each round and a kingdom to chase in. 
* Each player has a score next to their name that will increase for rescuing, surviving, and winning a round if they are a runner, if they are a chaser, it will increase when they tag a runner or win a round
* When a chaser gets close enough to a runner, the runner will freeze and will wait to be rescued by another runner
* The player with the highest score at the end of a round (or set of rounds) is considered the winner.

## Gamemode Controls
### Hide and Seek
- Left D-Pad: Decrease time
- Right D-Pad: Increase Time
- L + D-Pad Down: Reset Time
- D-Pad Up: Switch from Hider/Seeker
### Sardines
- Left D-Pad: Decrease time
- Right D-Pad: Increase Time
- L + D-Pad Down: Reset Time
- D-Pad Up: Switch from Sardine/Pack of Sardine
### Freeze Tag
- R + D-Pad Up: Start Round
- R + D-Pad Down: End Round
- L + D-Pad Down: Reset Score
- D-Pad Up: Switch from Runner/Chaser

# Building

### Prerequisites

- CMake + GNUMake
- cURL
- Clang, LLVM, LLD 20 or later
- Python 3.10, `pyelftools`, `mmh`, and `lz4` packages

### Building

1. Run `make -j32 setup`
2. a) Run `make -j32 release` to build a release version (will be put in the `package` folder)  
   b) Run `make -j32 debug` to build a debug version (will be in `build/sd`)

Check out the hakkun [README](./sys/README.md) for more info

---

# Contributors 

- [MrKatzenGaming](https://github.com/MrKatzenGaming) Hakkun Port of SMOO+ and Adding a bunch of new features
- [KleinTimmi](https://github.com/KleinTimmi) Adding a bunch of new features aswell as adding new packets in the server
- [Dimenzio](https://github.com/grafdimenzio) Wrote the Web Interface
- [Kgamer77](https://github.com/Kgamer77) Added Most of the New Server Packets
- [Neorix](https://github.com/Neorix09) Wrote the Majority of the new server code

# Original SMOO 

- [CraftyBoss](https://github.com/CraftyBoss) Created SMOO
- [Sanae](https://github.com/sanae6) Wrote the majority of the server code
- [Shadow](https://github.com/shadowninja108) original author of starlight, the tool used to make this entire mod possible
- [GRAnimated](https://github.com/GRAnimated)

# Credits
- [LibHakkun](https://github.com/fruityloops1/LibHakkun)
- [imgui](https://github.com/ocornut/imgui)
- [OdysseyDecomp](https://github.com/shibbo/OdysseyDecomp)
- [open-ead](https://github.com/open-ead/sead) sead Headers
