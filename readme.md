# Coop Shooter based on Unreal Engine

Play in single or multiplayer, survive as many waves as you can in this third person shooter game. Each round more and more enemies spawn.
If there is at least one player standing fallen players are revived before each new round.

![Game](https://raw.githubusercontent.com/dodie/unreal-coop-shooter-game/main/screenshot.png "Game")

Download the [release](https://github.com/dodie/unreal-coop-shooter-game/releases/), extract, start exe it on Win64, have fun.
In multiplayer mode when connecting to remote machines you might have to specify the port `7777` in addition to the host IP address.


# Controls

- Move: WASD, Q, E
- Shoot: Left Mouse Button
- Aim: Right Mouse Button
- Switch Weapons: Mouse Wheel


# Encyclopedia

## Enemies

### Soldier Bot

They attack the players with a lightweight rifles. They seek cover as they advance, when wounded they try to retreat into a safe position so they can heal up.

![Soldier Bot](https://raw.githubusercontent.com/dodie/unreal-coop-shooter-game/main/soldierbot.jpg "Soldier Bot")


### Tracker Bot

These machines roll toward the players just to explode when they are near. When there are bunch of them in one place they power each other, resulting in higher explosion damage.
They are really bad at climbing or slowing down when they move fast.

![Tracker Bot](https://raw.githubusercontent.com/dodie/unreal-coop-shooter-game/main/trackerbot.jpg "Tracker Bot")


## Weapons

### Rifle

Fires armor armour-piercing rounds at very high muzzle velocity.

- RPM: 600
- Damage: 20 (80 on head)

![Rifle](https://raw.githubusercontent.com/dodie/unreal-coop-shooter-game/main/rifle.jpg "Rifle")


### Grenade Launcher

Launches bouncy grenades that explode after 1 second.

- RPM: 120
- Damage: 200, less when the target is further from the explosion. Be careful with friendly fire!

![Grenade Launcher](https://raw.githubusercontent.com/dodie/unreal-coop-shooter-game/main/glauncher.jpg "Grenade Launcher")


### Explosive Barrel Launcher

Places explosive barrels that explode after a certain amount of damage is dealt to them. 

- RPM: 60
- Damage: 200, less when the target is further from the explosion. Be careful with friendly fire!

![barrel Launcher](https://raw.githubusercontent.com/dodie/unreal-coop-shooter-game/main/blauncher.jpg "Barrel Launcher")

## Maps

### Desert Arena

![Desert Arena](https://raw.githubusercontent.com/dodie/unreal-coop-shooter-game/main/desert_arena.jpg "Desert Arena")


# Development

- Requires Unreal Engine 4.17.*


# License

This project is licensed under the Unreal Engine 4 EULA.


# Credits

 - Based on the course [Unreal Engine 4 Mastery: Create Multiplayer Games with C++](https://www.udemy.com/course/unrealengine-cpp/) by Tom Looman. Also many assets are provided as resources for the course.
 - Rifle sound: http://www.m-klier.de/
 - Menu sound: @Joth_Music
 - Death sound: Copyright 2012 Iwan 'qubodup' Gabovitch http://qubodup.net
 
