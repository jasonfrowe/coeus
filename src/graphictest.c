#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "usb_hid_keys.h"


#define SPRITE_CONFIG 0xFF10
#define SPACESHIP_DATA 0xE100 // 

static const int16_t sin_fix8[] = {
      0,  66, 128, 181, 222, 247, 256, 247, 222, 181, 128,  66,
      0, -66,-128,-181,-222,-247,-256,-247,-222,-181,-128, -66,
      0
};

static const int16_t cos_fix8[] = {
    256, 247, 222, 181, 128,  66,   0, -66,-128,-181,-222,-247,
   -256,-247,-222,-181,-120, -66,   0,  66, 128, 181, 222, 247,
    256
};

static const int16_t t2_fix8[] = {
      0,  37,  81, 128, 175, 219, 256, 285, 303, 309, 303, 285,
    256, 219, 175, 128,  81,  37,   0, -29, -47, -53, -47, -29,
      0
};

uint8_t ri = 0;
uint8_t ri_max = 24;

struct __VIA6522 {
    unsigned char pb;
    unsigned char pa;
    unsigned char ddrb;
    unsigned char ddra;
};
#define VIAp (*(volatile struct __VIA6522 *)0xFFD0)


#define JOYSTICK_INPUT 0xFF20 // Gamepad

#define KEYBOARD_INPUT 0xFF30 // Keyboard
// 256 bytes HID code max, stored in 32 uint8
#define KEYBOARD_BYTES 32
uint8_t keystates[KEYBOARD_BYTES] = {0};
#define key(code) (keystates[code >> 3] & (1 << (code & 7)))

uint16_t vlen = 57600;
uint8_t yellow = 240 - 16*4;


// Initial position of spacecraft
int x = 100;
int y = 100;
int vx = 1;
int vy = 1;

static inline void set(int x, int y, int colour)
{
    RIA.addr0 =  (x / 1) + (320 / 1 * y);
    RIA.step0 = 0;
    uint8_t bit = colour;
    RIA.rw0 = bit;
}

static inline void unset(int x, int y)
{
    RIA.addr0 =  (x / 1) + (320 / 1 * y);
    RIA.step0 = 0;
    uint8_t bit = 0xFE;
    RIA.rw0 &= ~bit;
}

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

    xram0_struct_set(0xFF00, vga_mode3_config_t, x_pos_px, 0);
    xram0_struct_set(0xFF00, vga_mode3_config_t, y_pos_px, 0);
    xram0_struct_set(0xFF00, vga_mode3_config_t, width_px, 320);
    xram0_struct_set(0xFF00, vga_mode3_config_t, height_px, 180);
    xram0_struct_set(0xFF00, vga_mode3_config_t, xram_data_ptr, 0);
    xram0_struct_set(0xFF00, vga_mode3_config_t, xram_palette_ptr, 0xFFFF);

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

    // Clear out extended memory for video
    
    RIA.addr0 = 0;
    RIA.step0 = 1;
    for (unsigned i = vlen; i--;)
        RIA.rw0 = 0;    

    // set(11, 20);
    // set(12, 21);
    // set(10, 22);
    // set(11, 22);
    // set(12, 22);

    // unset(12, 22);


    //Turn on controller
    // xregn(0, 0, 2, 1, JOYSTICK_INPUT);
    // xregn(0, 0, 0, 1, KEYBOARD_INPUT);

}

int main(void)
{
    setup();

    uint8_t v; //Testing for V-sync

    uint8_t r1 = 0;
    uint8_t k = 0;
    uint8_t c1 = 0;

    int xtry = x;
    int ytry = y;

    int val;

    while (k == 0) { //Infinite Game Loop
    
        uint16_t i = 0;
        uint16_t j = 0;
        uint16_t ii = 0;
        while (i < 320) {
            j = 0;
            while (j < 180){
                r1 = random(0, 100);
                if (r1 < 10){
                    c1 = j + random(20, 50);
                } else{
                    c1 = 0;
                }
                
                set(i, j, c1);
                j++;

                if (RIA.vsync == v)
                    continue;
                v = RIA.vsync;

                //Joystick
                
                // uint8_t gp_x;
                // RIA.addr0 = KEYBOARD_INPUT;
                // gp_x = RIA.rw0;
                // if (gp_x > 200){
                //     vx *= -1;
                // }

                //Keyboard
                // fill the keystates bitmask array

                // for (ii = 0; ii < KEYBOARD_BYTES; ii++) {
                //     uint8_t new_keys;
                //     RIA.addr0 = KEYBOARD_INPUT + ii;
                //     new_keys = RIA.rw0;
                //     keystates[ii] = new_keys;
                // }

                // if (!(keystates[0] & 1)) {
                //     if (key(KEY_RIGHT)){
                //         vx *= -1;
                //     }
                // }

                // Arcade Stick via GPIO

                VIAp.ddra = 0; //GPIO as input (probably only need to do once...)

                vx=0; 
                vy=0;
                if (VIAp.pa > 0){

                    if (VIAp.pa & 0x01){
                        vy -= cos_fix8[ri] >> 7;
                        vx -= sin_fix8[ri] >> 7;
                    }
                    if ((VIAp.pa & 0x02) >> 1){
                        vy += cos_fix8[ri] >> 7;
                        vx += sin_fix8[ri] >> 7;
                    }

                    //Rotate counter
                    if ((VIAp.pa & 0x04) >> 2){
                        // vx-=1;
                        // update rotation
                        if (ri == ri_max){
                            ri = 0;
                        } else {
                            ri += 1;
                        } 
                    }

                    if ((VIAp.pa & 0x08) >> 3){
                        // vx+=1;
                        // update rotation
                        if (ri == 0){
                            ri = ri_max;
                        } else {
                            ri -= 1;
                        }
                    }

                }

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
                xram0_struct_set(SPRITE_CONFIG, vga_mode4_asprite_t, transform[5],  16*t2_fix8[ri_max - ri - 1]);

                //Update position
                xtry = x + vx;
                ytry = y + vy;

                if (xtry > 0 && xtry < 320 - 16)
                    x = xtry;
                if (ytry > 0 && ytry < 180 - 16)
                    y = ytry;

                


            }
            i++; 


        }


    }
}