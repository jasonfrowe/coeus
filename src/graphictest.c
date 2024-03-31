#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
// #include "usb_hid_keys.h"

#define MAPSIZE 2048 // Size of world (world coordinates)
#define SWIDTH 320 // width of visible screen
#define SHEIGHT 180 // height of visible screen

//Boundary for scrolling
#define BBX 100
#define BBY 60
static int16_t BX1 = BBX;
static int16_t BX2 = SWIDTH - BBX;
static int16_t BY1 = BBY;
static int16_t BY2 = SHEIGHT - BBY;

//Screen position at start (world coordinates)
//This position labels 0,0 on the screen 
static uint16_t sx = MAPSIZE / 2;
static uint16_t sy = MAPSIZE / 2;
static uint16_t sx_old = MAPSIZE / 2;
static uint16_t sy_old = MAPSIZE / 2;

//Background stars
#define NSTAR 64
#define STARFIELD_X 512 //Size of starfield
#define STARFIELD_Y 256
static int16_t star_x[NSTAR] = {0};      //X-position -- World coordinates
static int16_t star_y[NSTAR] = {0};      //Y-position -- World coordinates
static int16_t star_x_old[NSTAR] = {0};  //prev X-position -- World coordinates
static int16_t star_y_old[NSTAR] = {0};  //prev Y-position -- World coordinates
static uint8_t star_colour[NSTAR] = {0};  //Colour

//Memory addresses
#define BITMAP_CONFIG 0xFF00
#define SPRITE_CONFIG 0xFF10
#define SPACESHIP_DATA 0xE100 //Extended RAM

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
static const int16_t sin_fix8[] = {
    0, 65, 127, 180, 220, 246, 255, 246, 220, 180, 127, 65, 
    0, -65, -127, -180, -220, -246, -255, -246, -220, -180, 
    -127, -65, 0
};

// Pre-calulated Angles: 255*cos(theta)
static const int16_t cos_fix8[] = {
    255, 246, 220, 180, 127, 65, 0, -65, -127, -180, -220, 
    -246, -255, -246, -220, -180, -127, -65, 0, 65, 127, 180, 
    220, 246, 255
};

// Pre-calulated Affine offsets: 181*sin(theta - pi/4) + 127
static const int16_t t2_fix8[] = {
    0, 36, 80, 127, 173, 217, 254, 283, 301, 308, 301, 283, 
    254, 217, 173, 127, 80, 36, 0, -29, -47, -54, -47, -29, 0
};

static uint8_t ri = 0;            // current rotation info for spaceship
static const uint8_t ri_max = 23; // max rotations 

// Spacecraft properties
#define SHIP_ROT_SPEED 3 // How fast the spaceship can rotate, must be >= 1. 

// Initial position and velocity of spacecraft
static int x = 160;  //Screen-coordinates. 
static int y = 90;
static int vx = 0;
static int vy = 0;

// Properties for bullets
#define NBULLET 16  // maximum good-guy bullets
#define NBULLET_TIMER_MAX 10 // Sets interval for new bullets when fire button is held
static const uint8_t bullet_v = 4; //Bullet Speed
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
static uint8_t bullet_timer = 0;             //delay timer for new bullets

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

static void setup()
{

    // xregn : address 1, 0 ,0 then we are setting '1' bit.  That bit will be set to 2
    xregn(1, 0, 0, 1, 2); // set 320x180 canvas, last number selects Canvas size 

    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, x_pos_px, 0);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, y_pos_px, 0);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, width_px, 320);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, height_px, 180);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, xram_data_ptr, 0);
    xram0_struct_set(BITMAP_CONFIG, vga_mode3_config_t, xram_palette_ptr, 0xFFFF);

    // xregn : address 1, 0 ,1 then we are setting '3' bits.  Thoses bits will be set to 3, 3, 0xFF00
    xregn(1, 0, 1, 3, 3, 3, 0xFF00); // Mode 3, 4-bit colour.  2nd last is bit depth, last is address config

    xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, transform[0],  cos_fix8[ri]);
    xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, transform[1], -sin_fix8[ri]);
    xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, transform[2],  0);
    xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, transform[3],  sin_fix8[ri]);
    xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, transform[4],  cos_fix8[ri]);
    xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, transform[5],  0);

    xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, x_pos_px, x);
    xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, y_pos_px, y);
    xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, xram_sprite_ptr, 0xE100);
    xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, log_size, 4);
    xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, has_opacity_metadata, false);

    xregn(1, 0, 1, 5, 4, 1, SPRITE_CONFIG, 1, 0);

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

    //Plot stars
    int16_t dx = 0;
    int16_t dy = 0;
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

        // Arcade Stick via GPIO

        vx=0; // Velocity to apply to Spacecraft.
        vy=0;

        // Copy positions during vblank
        RIA.step0 = sizeof(vga_mode4_asprite_t);
        RIA.step1 = sizeof(vga_mode4_asprite_t);
        RIA.addr0 = SPRITE_CONFIG + 12;
        RIA.addr1 = SPRITE_CONFIG + 13;

        val = x;
        RIA.rw0 = val & 0xff;
        RIA.rw1 = (val >> 8) & 0xff;

        RIA.addr0 = SPRITE_CONFIG + 14;
        RIA.addr1 = SPRITE_CONFIG + 15;

        val = y;
        RIA.rw0 = val & 0xff;
        RIA.rw1 = (val >> 8) & 0xff;

        // Update rotation
        xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, transform[0],  cos_fix8[ri]);
        xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, transform[1], -sin_fix8[ri]);
        xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, transform[2],  16*t2_fix8[ri]);
        xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, transform[3],  sin_fix8[ri]);
        xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, transform[4],  cos_fix8[ri]);
        xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, transform[5],  
            16*t2_fix8[ri_max - ri + 1]);

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
                vx = -sin_fix8[ri];
                vy = -cos_fix8[ri];
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
                        bullet_x[bullet_c] = x+8;
                        bullet_y[bullet_c] = y+8;
                        bullet_c += 1;
                        if (bullet_c >= NBULLET){
                            bullet_c = 0;
                        }
                    }
                }
            } else {
                bullet_timer = NBULLET_TIMER_MAX; //If button is released we can immediately fire again.
            }

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
            dx = (xtry - x);
            sx += dx;
            if (sx > MAPSIZE){
                sx -= MAPSIZE;
            }
            if (sx <= 0){
                sx += MAPSIZE;
            }
        }
            
        if (ytry > BY1 && ytry < BY2){
            y = ytry;
        } else {
            dy = (ytry - y);
            sy += dy;
            if (sy > MAPSIZE){
                sy -= MAPSIZE;
            }
            if (sy <= 0){
                sy += MAPSIZE;
            }
        }

        //Update stars
        if (dx != 0 || dy != 0){
            plot_stars(dx, dy);
            dx = 0;
            dy = 0;
        }

        //Update bullets
        for (uint8_t ii = 0; ii < NBULLET; ii++) {
            if (bullet_status[ii] >= 0){
                set(bullet_x[ii], bullet_y[ii], 0x00);

                bvx = -sin_fix8[bullet_status[ii]];
                bvy = -cos_fix8[bullet_status[ii]];
                bvxapp = ( (bvx + bvxrem[ii]) >> 6);
                bvyapp = ( (bvy + bvyrem[ii]) >> 6);
                bvxrem[ii] = bvx + bvxrem[ii] - bvxapp * 64;
                bvyrem[ii] = bvy + bvyrem[ii] - bvyapp * 64;
                bullet_x[ii] += bvxapp;
                bullet_y[ii] += bvyapp;

                // bullet_x[ii] -= sin_fix8[bullet_status[ii]] >> 6;
                // bullet_y[ii] -= cos_fix8[bullet_status[ii]] >> 6;

                if (bullet_x[ii] > 0 && bullet_x[ii] < 320 && bullet_y[ii] > 0 && bullet_y[ii] < 180){
                    set(bullet_x[ii], bullet_y[ii], 0xFF);
                } else {
                    bullet_status[ii] = -1;
                }
            }
        }
            

    } //end of infinite loop
}// end of main