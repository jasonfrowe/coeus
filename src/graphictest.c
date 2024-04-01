#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
// #include "usb_hid_keys.h"

//World and screen sizes
#define MAPSIZE 2048 // Size of world (world coordinates)
#define MMAPSIZE -2048
#define MAPSIZED2 1024 
#define MMAPSIZED2 -1024 
#define MAPSIZEM1 1023 
#define SWIDTH 320 // width of visible screen
#define SHEIGHT 180 // height of visible screen
static int16_t dx = 0;
static int16_t dy = 0;

//Sprite locations
#define SPACESHIP_DATA  0xE100  //Spaceship Sprite
#define EARTH_DATA      0xE180  //Earth Sprite
#define STATION_DATA    0xE980  //Enemy Station Sprite
#define BATTLE_DATA     0xEB80  //Enemy battle station Sprite
#define FIGHTER_DATA    0xEC00  //Enemy fighter Sprite

//XRAM Memory addresses
#define VGA_CONFIG_START 0xEC20 //Start of graphic config addresses
unsigned BITMAP_CONFIG;   //Bitmap Config 
unsigned SPACECRAFT_CONFIG;   //Spacecraft Sprite Config - Affine 
unsigned EARTH_CONFIG;    //Earth Sprite Config - Standard 
unsigned STATION_CONFIG;  //Enemy station sprite config
unsigned BATTLE_CONFIG;   //Enemy battle station sprite config 
unsigned FIGHTER_CONFIG;  //Enemy fighter sprite config

#define NSTATION_MAX 5  //Number of enemy battle stations
#define NBATTLE_MAX  5  //Number of portable battle stations 
#define NFIGHTER_MAX 30  //Number of fighters

static uint8_t nstation = NSTATION_MAX; //
static int16_t station_x[NSTATION_MAX] = {0}; 
static int16_t station_y[NSTATION_MAX] = {0}; 

static uint8_t nbattle = NBATTLE_MAX; //
static int16_t battle_x[NBATTLE_MAX] = {0}; 
static int16_t battle_y[NBATTLE_MAX] = {0}; 

static uint8_t nfighter = NFIGHTER_MAX; //
static int16_t fighter_x[NFIGHTER_MAX] = {0}; 
static int16_t fighter_y[NFIGHTER_MAX] = {0}; 
static int16_t fighter_dx[NFIGHTER_MAX] = {0}; 
static int16_t fighter_dy[NFIGHTER_MAX] = {0}; 
static int16_t fighter_vx[NFIGHTER_MAX] = {0}; 
static int16_t fighter_vy[NFIGHTER_MAX] = {0}; 
static int16_t fighter_vxi[NFIGHTER_MAX] = {0}; 
static int16_t fighter_vyi[NFIGHTER_MAX] = {0}; 
static int16_t fighter_xrem[NFIGHTER_MAX] = {0}; 
static int16_t fighter_yrem[NFIGHTER_MAX] = {0}; 

static uint8_t nsprites = 0;

//Boundary for scrolling
#define BBX 100
#define BBY 60
static int16_t BX1 = BBX;
static int16_t BX2 = SWIDTH - BBX;
static int16_t BY1 = BBY;
static int16_t BY2 = SHEIGHT - BBY;

//Frame counter
uint16_t update_sch = 0; 

//Screen position at start (world coordinates)
//This position labels 0,0 on the screen 
// static uint16_t sx = MAPSIZE / 2;
// static uint16_t sy = MAPSIZE / 2;
// static uint16_t sx_old = MAPSIZE / 2;
// static uint16_t sy_old = MAPSIZE / 2;

//Background stars
#define NSTAR 32
#define STARFIELD_X 512 //Size of starfield
#define STARFIELD_Y 256
static int16_t star_x[NSTAR] = {0};      //X-position -- World coordinates
static int16_t star_y[NSTAR] = {0};      //Y-position -- World coordinates
static int16_t star_x_old[NSTAR] = {0};  //prev X-position -- World coordinates
static int16_t star_y_old[NSTAR] = {0};  //prev Y-position -- World coordinates
static uint8_t star_colour[NSTAR] = {0};  //Colour

// access to 6522
struct __VIA6522 {
    unsigned char pb;
    unsigned char pa;
    unsigned char ddrb;
    unsigned char ddra;
};
#define VIAp (*(volatile struct __VIA6522 *)0xFFD0)

static const uint16_t vlen = 57600; // Extended Memory space for bitmap graphics

// Pre-calulated Angles: 255*sin(theta)
static const int16_t sin_fix[] = {
    0, 65, 127, 180, 220, 246, 255, 246, 220, 180, 127, 65, 
    0, -65, -127, -180, -220, -246, -255, -246, -220, -180, 
    -127, -65, 0
};

// Pre-calulated Angles: 255*cos(theta)
static const int16_t cos_fix[] = {
    255, 246, 220, 180, 127, 65, 0, -65, -127, -180, -220, 
    -246, -255, -246, -220, -180, -127, -65, 0, 65, 127, 180, 
    220, 246, 255
};

// // Pre-calulated Affine offsets: 181*sin(theta - pi/4) + 127
// static const int16_t t2_fix8[] = {
//     0, 576, 1280, 2032, 2768, 3472, 4064, 4528, 4816, 4928, 
//     4816, 4528, 4064, 3472, 2768, 2032, 1280, 576, 0, -464, 
//     -752, -864, -752, -464
// };

static const int16_t t2_fix4[] = {
    0, 288, 640, 1016, 1384, 1736, 2032, 2264, 2408, 2464, 
    2408, 2264, 2032, 1736, 1384, 1016, 640, 288, 0, -232, 
    -376, -432, -376, -232, 0
};

static uint8_t ri = 0;            // current rotation info for spaceship
static const uint8_t ri_max = 23; // max rotations 

// Spacecraft properties
#define SHIP_ROT_SPEED 3 // How fast the spaceship can rotate, must be >= 1. 

// Earth properties
static int16_t earth_x = SWIDTH/2 - 16;
static int16_t earth_y = SHEIGHT/2 - 16;

// Initial position and velocity of spacecraft
static int16_t x = 160;  //Screen-coordinates. 
static int16_t y = 90;
static int16_t vx = 0;
static int16_t vy = 0;

// Properties for bullets
#define NBULLET 8  // maximum good-guy bullets
#define NBULLET_TIMER_MAX 8 // Sets interval for new bullets when fire button is held
static const uint8_t bullet_v = 4; //Bullet Speed (not implemented -- fixed at the moment)
static int16_t bvx = 0;
static int16_t bvy = 0;
static int16_t bvxapp = 0;
static int16_t bvyapp = 0;
static int16_t bvxrem[NBULLET] = {0};
static int16_t bvyrem[NBULLET] = {0};

static uint16_t bullet_x[NBULLET] = {0};     //X-position
static uint16_t bullet_y[NBULLET] = {0};     //Y-position
static int8_t bullet_status[NBULLET] = {0};  //Status of bullets
static uint8_t bullet_c = 0;                 //Counter for bullets
static uint16_t bullet_timer = 0;            //delay timer for new bullets

// Routine for placing a single dot on the screen
static inline void set(int16_t x, int16_t y, uint8_t colour)
{
    RIA.addr0 =  (x / 1) + (320 / 1 * y);
    RIA.step0 = 0;
    // uint8_t bit = colour;
    // RIA.rw0 = bit;
    RIA.rw0 = colour;
}

//Functions for generating randoms
#define swap(a, b) { uint16_t t = a; a = b; b = t; }

uint16_t random(uint16_t low_limit, uint16_t high_limit)
{
    if (low_limit > high_limit) {
        swap(low_limit, high_limit);
    }

    return (uint16_t)((rand() % (high_limit-low_limit)) + low_limit);
}

static void earth_setup()
{
    EARTH_CONFIG  =  SPACECRAFT_CONFIG + sizeof(vga_mode4_asprite_t);

    xram0_struct_set(EARTH_CONFIG, vga_mode4_sprite_t, x_pos_px, earth_x);
    xram0_struct_set(EARTH_CONFIG, vga_mode4_sprite_t, y_pos_px, earth_y);
    xram0_struct_set(EARTH_CONFIG, vga_mode4_sprite_t, xram_sprite_ptr, EARTH_DATA);
    xram0_struct_set(EARTH_CONFIG, vga_mode4_sprite_t, log_size, 5);
    xram0_struct_set(EARTH_CONFIG, vga_mode4_sprite_t, has_opacity_metadata, false);

    xregn(1, 0, 1, 5, 4, 0, EARTH_CONFIG, 1, 0);
}

static void enemy_setup()
{
    STATION_CONFIG = EARTH_CONFIG + sizeof(vga_mode4_sprite_t);

    nsprites = 0;

    for (uint8_t i = 0; i < nstation; i++) {

        unsigned ptr = STATION_CONFIG + i * sizeof(vga_mode4_sprite_t);

        station_x[i] = random(1, MAPSIZEM1);
        station_y[i] = random(1, MAPSIZEM1);
        if (station_x[i] > MAPSIZED2){
            station_x[i] -= MAPSIZE;
        }
        if (station_y[i] > MAPSIZED2){
            station_y[i] -= MAPSIZE;
        }

        xram0_struct_set(ptr, vga_mode4_sprite_t, x_pos_px, station_x[i]);
        xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, station_y[i]);
        xram0_struct_set(ptr, vga_mode4_sprite_t, xram_sprite_ptr, STATION_DATA);
        xram0_struct_set(ptr, vga_mode4_sprite_t, log_size, 4);
        xram0_struct_set(ptr, vga_mode4_sprite_t, has_opacity_metadata, false);

    }
    nsprites += nstation;

    BATTLE_CONFIG = STATION_CONFIG + nstation * sizeof(vga_mode4_sprite_t);

    for (uint8_t i = 0; i < nbattle; i++) {

        unsigned ptr = BATTLE_CONFIG + i * sizeof(vga_mode4_sprite_t);

        battle_x[i] = random(1, MAPSIZEM1);
        battle_y[i] = random(1, MAPSIZEM1);
        if (battle_x[i] > MAPSIZED2){
            battle_x[i] -= MAPSIZE;
        }
        if (battle_y[i] > MAPSIZED2){
            battle_y[i] -= MAPSIZE;
        }

        xram0_struct_set(ptr, vga_mode4_sprite_t, x_pos_px, battle_x[i]);
        xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, battle_y[i]);
        xram0_struct_set(ptr, vga_mode4_sprite_t, xram_sprite_ptr, BATTLE_DATA);
        xram0_struct_set(ptr, vga_mode4_sprite_t, log_size, 3);
        xram0_struct_set(ptr, vga_mode4_sprite_t, has_opacity_metadata, false);
    }
    nsprites += nbattle;

    FIGHTER_CONFIG = BATTLE_CONFIG + nbattle * sizeof(vga_mode4_sprite_t);

    for (uint8_t i = 0; i < nfighter; i++) {

        unsigned ptr = FIGHTER_CONFIG + i * sizeof(vga_mode4_sprite_t);

        fighter_x[i] = random(1, MAPSIZEM1);
        fighter_y[i] = random(1, MAPSIZEM1);
        fighter_vxi[i] = random(16, 512);
        fighter_vyi[i] = random(16, 512);
        if (fighter_x[i] > MAPSIZED2){
            fighter_x[i] -= MAPSIZE;
        }
        if (fighter_y[i] > MAPSIZED2){
            fighter_y[i] -= MAPSIZE;
        }

        xram0_struct_set(ptr, vga_mode4_sprite_t, x_pos_px, fighter_x[i]);
        xram0_struct_set(ptr, vga_mode4_sprite_t, y_pos_px, fighter_y[i]);
        xram0_struct_set(ptr, vga_mode4_sprite_t, xram_sprite_ptr, FIGHTER_DATA);
        xram0_struct_set(ptr, vga_mode4_sprite_t, log_size, 2);
        xram0_struct_set(ptr, vga_mode4_sprite_t, has_opacity_metadata, false);
    }
    nsprites += nfighter;

    xregn(1, 0, 1, 5, 4, 0, STATION_CONFIG, nsprites, 1);
}

static void battle_update(int16_t dx, int16_t dy)
{

    RIA.step0 = sizeof(vga_mode4_sprite_t);
    RIA.step1 = sizeof(vga_mode4_sprite_t);
    RIA.addr0 = BATTLE_CONFIG;
    RIA.addr1 = BATTLE_CONFIG + 1;
    

    for (uint8_t i = 0; i < nbattle; i++) {

        battle_x[i] -= dx;
        if (battle_x[i] <= MMAPSIZED2){
            battle_x[i] += MAPSIZE;
        }
        if (battle_x[i] > MAPSIZED2){
            battle_x[i] -= MAPSIZE;
        }

        RIA.rw0 = battle_x[i] & 0xff;
        RIA.rw1 = (battle_x[i] >> 8) & 0xff;

    }

    RIA.addr0 = BATTLE_CONFIG + 2;
    RIA.addr1 = BATTLE_CONFIG + 3;

    for (uint8_t i = 0; i < nbattle; i++) {

        battle_y[i] -= dy;
        if (battle_y[i] <= MMAPSIZED2){
            battle_y[i] += MAPSIZE;
        }
        if (battle_y[i] > MAPSIZED2){
            battle_y[i] -= MAPSIZE;
        }    

        RIA.rw0 = battle_y[i] & 0xff;
        RIA.rw1 = (battle_y[i] >> 8) & 0xff;

    }
}

static void station_update(int16_t dx, int16_t dy)
{

    RIA.step0 = sizeof(vga_mode4_sprite_t);
    RIA.step1 = sizeof(vga_mode4_sprite_t);
    RIA.addr0 = STATION_CONFIG;
    RIA.addr1 = STATION_CONFIG + 1;
    

    for (uint8_t i = 0; i < nstation; i++) {

        station_x[i] -= dx;
        if (station_x[i] <= MMAPSIZED2){
            station_x[i] += MAPSIZE;
        }
        if (station_x[i] > MAPSIZED2){
            station_x[i] -= MAPSIZE;
        }

        RIA.rw0 = station_x[i] & 0xff;
        RIA.rw1 = (station_x[i] >> 8) & 0xff;
        
    }

    RIA.addr0 = STATION_CONFIG + 2;
    RIA.addr1 = STATION_CONFIG + 3;

    for (uint8_t i = 0; i < nstation; i++) {

        station_y[i] -= dy;
        if (station_y[i] <= MMAPSIZED2){
            station_y[i] += MAPSIZE;
        }
        if (station_y[i] > MAPSIZED2){
            station_y[i] -= MAPSIZE;
        }    

        RIA.rw0 = station_y[i] & 0xff;
        RIA.rw1 = (station_y[i] >> 8) & 0xff;

    }
}

static void fighter_update()
{

    RIA.step0 = sizeof(vga_mode4_sprite_t);
    RIA.step1 = sizeof(vga_mode4_sprite_t);
    RIA.addr0 = FIGHTER_CONFIG;
    RIA.addr1 = FIGHTER_CONFIG + 1;
    

    for (uint8_t i = 0; i < nfighter; i++) {

        fighter_x[i] -= dx - fighter_dx[i];
        if (fighter_x[i] <= MMAPSIZED2){
            fighter_x[i] += MAPSIZE;
        }
        if (fighter_x[i] > MAPSIZED2){
            fighter_x[i] -= MAPSIZE;
        }

        RIA.rw0 = fighter_x[i] & 0xff;
        RIA.rw1 = (fighter_x[i] >> 8) & 0xff;
        
    }

    RIA.addr0 = FIGHTER_CONFIG + 2;
    RIA.addr1 = FIGHTER_CONFIG + 3;

    for (uint8_t i = 0; i < nfighter; i++) {

        fighter_y[i] -= dy - fighter_dy[i];
        if (fighter_y[i] <= MMAPSIZED2){
            fighter_y[i] += MAPSIZE;
        }
        if (fighter_y[i] > MAPSIZED2){
            fighter_y[i] -= MAPSIZE;
        }    

        RIA.rw0 = fighter_y[i] & 0xff;
        RIA.rw1 = (fighter_y[i] >> 8) & 0xff;

    }

}

void fighter_attack()
{
    
    int16_t fdx;
    int16_t fdy;
    int16_t fvxapp;
    int16_t fvyapp;

    for (uint8_t i = 0; i < nfighter; i++) {
        fdx = x - fighter_x[i];
        fdy = y - fighter_y[i];

        if (abs(fdx) < 30 && abs(fdy) < 30) {
            fighter_dx[i] =  0;
            fighter_dy[i] =  0;
            fighter_xrem[i] = 0;
            fighter_yrem[i] = 0;
        } else {
            if (update_sch%30 == 0){ 
                if ((rand() >> 15) +1){
                    if (fdx > 0){
                        fighter_vx[i] = fighter_vxi[i]; //-fighter_vx[i];
                    } else {
                        fighter_vx[i] = -fighter_vxi[i];
                    }
                }

                if ((rand() >> 15) +1){
                    if (fdy > 0){
                        fighter_vy[i] = fighter_vyi[i]; // -fighter_vy[i];
                    } else {
                        fighter_vy[i] = -fighter_vyi[i];
                    }
                }
                
            }


            fvxapp = (fighter_vx[i] + fighter_xrem[i]) >> 8;
            fvyapp = (fighter_vy[i] + fighter_yrem[i]) >> 8;
            fighter_xrem[i] = fighter_vx[i] + fighter_xrem[i] - fvxapp * 256;
            fighter_yrem[i] = fighter_vy[i] + fighter_yrem[i] - fvyapp * 256;
            fighter_dx[i] = fvxapp;
            fighter_dy[i] = fvyapp;

            // printf("dx %d %d \n",i, fighter_vx[i]);

        }

    }

    fighter_update();

}

static void earth_update(int16_t dx, int16_t dy)
{
    // Update positions

    earth_x = earth_x - dx;
    if (earth_x <= MMAPSIZED2){
        earth_x += MAPSIZE;
    }
    if (earth_x > MAPSIZED2){
        earth_x -= MAPSIZE;
    }

    earth_y = earth_y - dy;
    if (earth_y <= MMAPSIZED2){
        earth_y += MAPSIZE;
    }
    if (earth_y > MAPSIZED2){
        earth_y -= MAPSIZE;
    }


    RIA.step0 = sizeof(vga_mode4_sprite_t);
    RIA.step1 = sizeof(vga_mode4_sprite_t);
    RIA.addr0 = EARTH_CONFIG;
    RIA.addr1 = EARTH_CONFIG + 1;

    RIA.rw0 = earth_x & 0xff;
    RIA.rw1 = (earth_x >> 8) & 0xff;

    RIA.addr0 = EARTH_CONFIG + 2;
    RIA.addr1 = EARTH_CONFIG + 3;

    RIA.rw0 = earth_y & 0xff;
    RIA.rw1 = (earth_y >> 8) & 0xff;
}

static void ship_update()
{
    // Copy positions during vblank
    RIA.step0 = sizeof(vga_mode4_asprite_t);
    RIA.step1 = sizeof(vga_mode4_asprite_t);
    RIA.addr0 = SPACECRAFT_CONFIG + 12;
    RIA.addr1 = SPACECRAFT_CONFIG + 13;

    //val = x;
    RIA.rw0 = x & 0xff;
    RIA.rw1 = (x >> 8) & 0xff;

    RIA.addr0 = SPACECRAFT_CONFIG + 14;
    RIA.addr1 = SPACECRAFT_CONFIG + 15;

    //val = y;
    RIA.rw0 = y & 0xff;
    RIA.rw1 = (y >> 8) & 0xff;

    // Update rotation
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[0],  cos_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[1], -sin_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[2],  t2_fix4[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[3],  sin_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[4],  cos_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[5],  
        t2_fix4[ri_max - ri + 1]);
}

static void setup()
{

    BITMAP_CONFIG = VGA_CONFIG_START;  //Bitmap Config - Planet 0

    // xregn : address 1, 0 ,0 then we are setting '1' bit.  That bit will be set to 2
    xregn(1, 0, 0, 1, 2); // set 320x180 canvas, last number selects Canvas size 

    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, x_pos_px, 0);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, y_pos_px, 0);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, width_px, 320);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, height_px, 180);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, xram_data_ptr, 0);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, xram_palette_ptr, 0xFFFF);

    // xregn : address 1, 0 ,1 then we are setting '3' bits.  Thoses bits will be set to 3, 3, 0xFF00
    xregn(1, 0, 1, 4, 3, 3, BITMAP_CONFIG, 1); // Mode 3, 4-bit colour.  2nd last is bit depth, last is address config

    SPACECRAFT_CONFIG =  BITMAP_CONFIG + sizeof(vga_mode3_config_t);

    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[0],  cos_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[1], -sin_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[2],  0);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[3],  sin_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[4],  cos_fix[ri]);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, transform[5],  0);

    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, x_pos_px, x);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, y_pos_px, y);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, xram_sprite_ptr, SPACESHIP_DATA);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, log_size, 3);
    xram0_struct_set(SPACECRAFT_CONFIG, vga_mode4_asprite_t, has_opacity_metadata, false);

    xregn(1, 0, 1, 5, 4, 1, SPACECRAFT_CONFIG, 1, 2);

    // Clear out extended memory for bit-map video
    
    RIA.addr0 = 0;
    RIA.step0 = 1;
    for (unsigned i = vlen; i--;)
        RIA.rw0 = 0;    

}

void setup_stars() //Set up random positions and colours for our stars.
{
    for (uint8_t i = 0; i < NSTAR; i++) {

        star_x[i] = random(1, STARFIELD_X);
        star_y[i] = random(1, STARFIELD_Y);
        star_colour[i] = random(1, 255);
        star_x_old[i] = star_x[i];
        star_y_old[i] = star_y[i];

    }
}

void plot_stars(int16_t dx, int16_t dy)
{
    for (uint8_t i = 0; i < NSTAR; i++){

        // Clear previous stars
        if (star_x_old[i] > 0 && star_x_old[i] < 320 && star_y_old[i] > 0 && star_y_old[i] < 180){
            set(star_x_old[i], star_y_old[i], 0x00);
        }

        star_x[i] = star_x_old[i] - dx;
        if (star_x[i] <= 0){
            star_x[i] += STARFIELD_X;
        }
        if (star_x[i] > STARFIELD_X){
            star_x[i] -= STARFIELD_X;
        }
        star_x_old[i] = star_x[i];
        
        star_y[i] = star_y_old[i] - dy;
        if (star_y[i] <= 0){
            star_y[i] += STARFIELD_Y;
        }
        if (star_y[i] > STARFIELD_Y){
            star_y[i] -= STARFIELD_Y;
        }
        star_y_old[i] = star_y[i];

         if (star_x[i] > 0 && star_x[i] < 320 && star_y[i] > 0 && star_y[i] < 180){
            set(star_x[i], star_y[i], star_colour[i]);
            // printf("stars %d, %d, %d \n", star_x[i], star_y[i], star_colour[i]);
        } 

    }
}

int main(void)
{

    setup(); //Set up Graphics
    setup_stars(); // Set up stars
    earth_setup(); // Set up Earth.
    enemy_setup(); // Set up Enemies

    // Motion of the screen
    dx = 0;
    dy = 0;

    //Plot stars
    plot_stars(dx, dy);

    // Makes rotation of ship slower
    uint16_t iframe = 0;
    uint16_t iframe_old = SHIP_ROT_SPEED;

    uint8_t v; //Used to test for V-sync

    int xtry = x; //For displaying Space ship
    int ytry = y;

    int xrem = 0; //Tracking remainder for smooth motion of space ship
    int yrem = 0;

    int vxapp = 0; //Applied motion to the space ship position
    int vyapp = 0;

    int val; //used for updating sprites

    uint8_t tdelay = 0;     // Counter for thrust/momentum/friction 
    uint8_t tdelay_max = 8; // momentum
    uint8_t tcount = 0;

    int thrust_x = 0;       // Initialize amount of thrust applied (acts like momentum)
    int thrust_y = 0;

    int16_t thx = 0; //Checking thrust for max allowed values.
    int16_t thy = 0;

    VIAp.ddra = 0; //set GPIO as input (probably only need to do once...)

    while (1) { //Infinite Game Loop

        if (RIA.vsync == v) //Run updates on V-sync.  Ideally this is 1/60 s 
            continue;
        v = RIA.vsync; 

        vx=0; // Velocity to apply to Spacecraft.
        vy=0;

        ship_update();


        // Arcade Stick via GPIO ///

        // We only periodically sample left/right to make rotation easier to control
        if (iframe >= iframe_old){
            iframe = 0;

            if (VIAp.pa > 0){

                //Rotate counter
                if ((VIAp.pa & 0x04) >> 2){ //Rotate left
                    // update rotation
                    if (ri == ri_max){
                        ri = 0;
                    } else {
                        ri += 1;
                    } 
                }

                if ((VIAp.pa & 0x08) >> 3){ //Rotate right
                    // update rotation
                    if (ri == 0){
                        ri = ri_max;
                    } else {
                        ri -= 1;
                    }
                }

            }
        }
        iframe+=1;

        if (VIAp.pa > 0){ //test if joystick has input
            // Up direction -- applies thrust..
            if (VIAp.pa & 0x01){
                vx = -sin_fix[ri];
                vy = -cos_fix[ri];
                tdelay = 0;
            }
            // Down direction
            if ((VIAp.pa & 0x02) >> 1){
                // Will apply Shield with Down
            }

            // Left fire button -- Bullets 
            if ((VIAp.pa & 0x50) == 0x50){
                if (bullet_timer > NBULLET_TIMER_MAX){
                    bullet_timer = 0;
                    if (bullet_status[bullet_c] < 0){
                        bullet_status[bullet_c] = ri;
                        bullet_x[bullet_c] = x+4;
                        bullet_y[bullet_c] = y+4;
                        bullet_c += 1;
                        if (bullet_c >= NBULLET){
                            bullet_c = 0;
                        }
                    }
                }
            } 
            // else { // Does not work?? 
            //     bullet_timer = NBULLET_TIMER_MAX; //If button is released we can immediately fire again.
            // }

            // Right fire button -- EMP
            if ((VIAp.pa & 0x50) == 0x30){
                // Will apply BIG BOMB here.
            }

        }
        bullet_timer += 1;
        
        //Update position
        vxapp = ( (vx + xrem + thrust_x ) >> 9); //Apply velocity, remainder and momentum 
        vyapp = ( (vy + yrem + thrust_y ) >> 9); // 9 and 512 must balance (e.g., 2^9 = 512)
        xrem = vx + xrem + thrust_x - vxapp * 512; //Update remainder
        yrem = vy + yrem + thrust_y - vyapp * 512;
        xtry = x + vxapp; //Update ship position
        ytry = y + vyapp;

        //Update thrust if joystick is held.
        thx = thrust_x + (vx >> 4);
        if (thx < 1024 && thx > -1024){
            thrust_x = thx;
        }
        thy = thrust_y + (vy >> 4);
        if (thy < 1024 && thy > -1024){
            thrust_y = thy;
        }

        //Update momentum by applying friction 
        if (tdelay < tdelay_max && tcount > 50){
            tdelay += 1;
            tcount = 0;
            // thrust_x = thrust_x >> 1;
            // thrust_y = thrust_y >> 1;
            if (vx == 0){
                thrust_x = thrust_x >> 1;
            }
            if (vy == 0){
                thrust_y = thrust_y >> 1;
            }
        }
        if (tdelay >= tdelay_max){
            thrust_x = 0;
            thrust_y = 0;
        }
        tcount += 1;

        // Keep spacecraft in bounds
        if (xtry > BX1 && xtry < BX2){
            x = xtry;
        } else {
            dx += (xtry - x);
            // sx += dx;
            // if (sx > MAPSIZE){
            //     sx -= MAPSIZE;
            // }
            // if (sx <= 0){
            //     sx += MAPSIZE;
            // }
        }
            
        if (ytry > BY1 && ytry < BY2){
            y = ytry;
        } else {
            dy += (ytry - y);
            // sy += dy;
            // if (sy > MAPSIZE){
            //     sy -= MAPSIZE;
            // }
            // if (sy <= 0){
            //     sy += MAPSIZE;
            // }
        }

        //Update fighters..
        fighter_attack();

        //Update stars and stationary sprites for screen scroll 
        if (dx != 0 || dy != 0){

            plot_stars(dx, dy);

            earth_update(dx, dy);

            station_update(dx, dy);

            battle_update(dx, dy);

            // fighter_update(dx, dy);

            dx = 0;
            dy = 0;
            
        }

        

        //Update bullets
        for (uint8_t ii = 0; ii < NBULLET; ii++) {
            if (bullet_status[ii] >= 0){
                set(bullet_x[ii], bullet_y[ii], 0x00);

                bvx = -sin_fix[bullet_status[ii]];
                bvy = -cos_fix[bullet_status[ii]];
                bvxapp = ( (bvx + bvxrem[ii]) >> 6);
                bvyapp = ( (bvy + bvyrem[ii]) >> 6);
                bvxrem[ii] = bvx + bvxrem[ii] - bvxapp * 64; //Expensive...
                bvyrem[ii] = bvy + bvyrem[ii] - bvyapp * 64;
                bullet_x[ii] += bvxapp;
                bullet_y[ii] += bvyapp;

                if (bullet_x[ii] > 0 && bullet_x[ii] < 320 && bullet_y[ii] > 0 && bullet_y[ii] < 180){
                    set(bullet_x[ii], bullet_y[ii], 0xFF);
                } else {
                    bullet_status[ii] = -1;
                }
            }
        }

    update_sch += 1;
            

    } //end of infinite loop
}// end of main