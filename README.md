# Unnamed 2D Platformer

## How to see all the games features
Currently the game has two maps, one a town and one a regular area, enter the door on the left (down while over door) to move between the maps

To switch through stamina based weapons, hit the x key, to fire the weapon press z with stamina. The whip does not require stamina, it uses the a key.
Weapons:
    - Whip
    - Axe
    - Shuriken
    - Knife
    - Bomb

Items can be used by hitting the 1-5 keys, form top to bottom on the left hand menu
    - Health
    - Speed
    - Stamina
    - Invisibility
    - Time Stop


At least one of each type of enemy exists in the small test area.
    - Flying Skull
    - Bat
    - Golem
    - Mushroom
    - Magician

Spacebar can be used to interact with NPCs, pressing the z key at a shop will let you purchase whatever item is being sold if you have the money.

## Original Readme's Build Process
### Build Process

Before you can build the example code we are providing for you, you will need to obtain the libraries required
by the source code
 - SDL2
 - SDL2_image
 - SDL2_mixer
 - SDL2_ttf
There are additional sub modules that are needed for this project to work as well, but they can be pulled right from within the project.
Performable from the following steps from the root of the cloned git repository within a terminal. 

Make sure you fetch submodules: `git submodule update --init --recursive`
Go into each submodule's src directory and type:
`make`
`make static`

Once each submodule has been made you can go into the base project src folder anre simply type:
`make`

You should now have a `gf2d` binary within the root of your git repository. Executing this will start your game.
