#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "hwlib.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <inttypes.h>



/* Address definitions */
#define START_ADDRESS 0xFF200000
#define HW_REGS_BASE ( 0xFC000000 )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )
#define SWITCH_BASE  0x00000000
#define HEX5_BASE 0x00000010
#define HEX4_BASE 0x00000020
#define HEX3_BASE 0x00000030
#define HEX2_BASE 0x00000040
#define HEX1_BASE 0x00000050
#define HEX0_BASE 0x00000060

/* Seven segment displays and switch definitions */
#define HEX54 hex5 * 16 + hex4
#define HEX32 hex3 * 16 + hex2
#define HEX10 hex1 * 16 + hex0

int hex5 = 255;
int hex4 = 255;
int hex3 = 255;
int hex2 = 255;
int hex1 = 255;
int hex0 = 255;
int sw0;//1 <-, 0 ->


int get_display_value(int value) {
    switch (value) {
        case 0:
            return 0b1000000;//64
        case 1:
            return 0b1111001;//121
        case 2:
            return 0b0100100;//36
        case 3:
            return 0b0110000;//48
        case 4:
            return 0b0011001;//25
        case 5:
            return 0b0010010;//18
        case 6:
            return 0b0000010;//2
        case 7:
            return 0b1111000;//120
        case 8:
            return 0b0000000;//0
        case 9:
            return 0b0010000;//16
        case 10:    // A
            return 0b0001000;//8
        case 11:    // b
            return 0b0000011;//3
        case 12:    //C
            return 0b0000110;//6
        case 13:    //d
            return 0b0100001;//33
        case 14:    //E
            return 0b0000110;//6
        case 15:    //F
            return 0b0001110;//14
        default:    //Display off
            return 0b1111111;//255
    }
}

void input(char c) {
    if (sw0) {
        hex5 = hex3;
        hex4 = hex2;
        hex3 = hex1;
        hex2 = hex0;
        hex1 = c / 16;
        hex0 = c % 16;
    } else {
        hex1 = hex3;
        hex0 = hex2;
        hex2 = hex4;
        hex3 = hex5;
        hex5 = c / 16;
        hex4 = c % 16;
    }
}

void print_buffer() {
    printf("========================================\n");
    printf("CHA: [%c,%c,%c]\n", HEX54, HEX32, HEX10);
    printf("HEX: [%0x,%0x,%0x]\n", HEX54, HEX32, HEX10);
    printf("DEC: [%d,%d,%d]\n", HEX54, HEX32, HEX10);
    printf("----------------------------------------\n");
}

int main(int argc, char *argv[]) {
    char c; // Used for character input
    void *virtual_base;
    int fd;

    // Open /dev/mem
    if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf("ERROR: could not open \"/dev/mem\"...\n");
        return (1);
    }

    // Get virtual addr that maps to physical
    virtual_base = mmap(NULL, HW_REGS_SPAN, (PROT_READ | PROT_WRITE),
                        MAP_SHARED, fd, HW_REGS_BASE);
    if (virtual_base == MAP_FAILED) {
        printf("ERROR: mmap() failed...\n");
        close(fd);
        return (1);
    }

    volatile unsigned int *SW0 = NULL;
    volatile unsigned int *hex_0_addr = NULL;
    volatile unsigned int *hex_1_addr = NULL;
    volatile unsigned int *hex_2_addr = NULL;
    volatile unsigned int *hex_3_addr = NULL;
    volatile unsigned int *hex_4_addr = NULL;
    volatile unsigned int *hex_5_addr = NULL;

    SW0 = virtual_base + ((unsigned long) (START_ADDRESS + SWITCH_BASE) & (unsigned long) (HW_REGS_MASK));
    hex_0_addr = virtual_base + ((unsigned long) (START_ADDRESS + HEX0_BASE) & (unsigned long) (HW_REGS_MASK));
    hex_1_addr = virtual_base + ((unsigned long) (START_ADDRESS + HEX1_BASE) & (unsigned long) (HW_REGS_MASK));
    hex_2_addr = virtual_base + ((unsigned long) (START_ADDRESS + HEX2_BASE) & (unsigned long) (HW_REGS_MASK));
    hex_3_addr = virtual_base + ((unsigned long) (START_ADDRESS + HEX3_BASE) & (unsigned long) (HW_REGS_MASK));
    hex_4_addr = virtual_base + ((unsigned long) (START_ADDRESS + HEX4_BASE) & (unsigned long) (HW_REGS_MASK));
    hex_5_addr = virtual_base + ((unsigned long) (START_ADDRESS + HEX5_BASE) & (unsigned long) (HW_REGS_MASK));

    // Turn off seven segment displays
    *hex_0_addr = get_display_value(hex0);
    *hex_1_addr = get_display_value(hex1);
    *hex_2_addr = get_display_value(hex2);
    *hex_3_addr = get_display_value(hex3);
    *hex_4_addr = get_display_value(hex4);
    *hex_5_addr = get_display_value(hex5);
    print_buffer();

    // Main loop
    while (1) {
        if (SW0[0] == 1) {
            sw0 = 0;
        } else {
            sw0 = 1;
        }
        printf("Enter character to be entered on %s side:\n", sw0 ? "right" : "left");
        scanf(" %c", &c);
        fflush(stdin);
        input(c);
        print_buffer();
        *hex_0_addr = get_display_value(hex0);
        *hex_1_addr = get_display_value(hex1);
        *hex_2_addr = get_display_value(hex2);
        *hex_3_addr = get_display_value(hex3);
        *hex_4_addr = get_display_value(hex4);
        *hex_5_addr = get_display_value(hex5);
    }
    return 0;

}