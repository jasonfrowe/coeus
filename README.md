# Coeus

Mysterious green orbs have appears at the edges of the solar system.  Only visible by visual sight, these orbits have proven imprevious to remote scans.  

Recently a fleet of small fighters emerged from the orbs setting up a blockade halting any movement.  Additionally, mobile battle stations have also spawed and are marching towards the Earth having already destroyed Mars.  If these battle stations reach the Earth it will spell certain doom for human-kind.

### Pilots Needed

Action in research labs across the globe under the code name Coeus have manufactored the Cerberus spacecraft.  Equiped with a rapid action thruster and powerful railgun, this ship should be capable of breaking the enemy blockade.  Your ship is also equiped an energy system to support a shield and a short-range electromagnetic pulse (EMP) that is capable of disabling and destroying the enemy.  Scientists believe that a close approach to the distant orbs may reveal a resonance weakness.  If this frequency can be discovered your ship can reconfigure its weapons to quickly shatter the orbits.

Jump in the newly created Cerberus Spacecraft.   Break the blockage, defeat the battle stations and destroy all the orbs to save the Earth!  

### Small fighters:

The enemy has a regenerating fleet of small fighters that will relentously hunt your ship.  The fighters are equiped with a tracker beam to slow your ship and drain energy from your ship.  This delays the recharge of your EMP and makes you extremely vunerable to Battlestation attacks.  Your railgun will make quick work of these drones.

### Battle Stations:

Slowly moving tanks equiped with a physical canon that can make quick work of your ship if you are not careful. The only way to destroy these bemouths is by deploying your EMP.  Once disabled, a Battlestation will short self-destruct.  Do not be nearby when this happens.

It is imperative that you do not let any battle station reach the Earth.  Once in range, a battle station is fully capable of destroying all life in a short timescale.  

### Mysterious Orbs:

The source of the enemies strength.  The orbs are able to spawn a continuous army of fighters and battle stations.  They must be destroyed.  Fly close to the orbs to complete a frequency sweep to discover any weakness, then exploit that weakness to scatter the orbs.

### Controlling your ship.

- Up : Apply thrust in the direction you are pointing
- Left : rotate Counter-clockwise
- Right : rotate Clockwise
- Down : Deploy your shield

- Left fire : Fire your rail gun
- Right fire : Fire the EMP 

- Escape : Exit game

### Development Todo: 

- ~~Add keyboard controls~~
- EMP
- Shield
- Orb attack
- ~~battle station bullet damage~~
- mini-map
- explosions
- ship animations
- add game start
- add game over
- add lives counter
- ~~Score~~
- ~~add ship energy and health~~
- ~~tracker beam~~

### Compiler and Usage Note

Code written for LLVM-MOS compiler.

VIA used to interface with a 7800 Atari Joystick.  Pico adapter required.

//Atari-7800 -> PICO GPIO Pins  
#define LF 27 // Left-Fire  
#define RF 26 // Right-Fire  
#define FF 22 // Fire  
#define RR 21 // Right  
#define LL 20 // Left  
#define DD 19 // Down  
#define UP 18 // Up  


//RP6502 / PICO GPIO Pins  
#define PA0 0 // Up  
#define PA1 1 // Down  
#define PA2 2 // Left  
#define PA3 3 // Right  
#define PA4 4 // Fire  
#define PA5 5 // Right-Fire  
#define PA6 6 // Left-Fire  
#define PA7 7 // Held low.  

