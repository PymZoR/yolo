#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "unistd.h"
#include "userLEDmmap.h"

volatile void *gpio_addr;
volatile unsigned int *gpio_oe_addr;
volatile unsigned int *gpio_setdataout_addr;
volatile unsigned int *gpio_cleardataout_addr;
unsigned int reg;
int fd;

void initMmap() {
    fd = open("/dev/mem", O_RDWR);
    printf("Mapping %X - %X (size: %X)\n", GPIO1_START_ADDR, GPIO1_END_ADDR, GPIO1_SIZE);

    gpio_addr              = mmap(0, GPIO1_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO1_START_ADDR);
    gpio_oe_addr           = gpio_addr + GPIO_OE;
    gpio_setdataout_addr   = gpio_addr + GPIO_SETDATAOUT;
    gpio_cleardataout_addr = gpio_addr + GPIO_CLEARDATAOUT;

    if(gpio_addr == MAP_FAILED) {
        printf("failed GPIO mapped to %p\n", gpio_addr);
        printf("Unable to map GPIO\n");
        exit(EXIT_FAILURE);
    }
    printf("GPIO mapped to %p\n", gpio_addr);
    printf("GPIO OE mapped to %p\n", gpio_oe_addr);
    printf("GPIO SETDATAOUTADDR mapped to %p\n", gpio_setdataout_addr);
    printf("GPIO CLEARDATAOUT mapped to %p\n", gpio_cleardataout_addr);

    reg = *gpio_oe_addr;
    printf("GPIO1 configuration: %X\n", reg);
    reg = reg & (0xFFFFFFFF - USR1_LED);
    *gpio_oe_addr = reg;
    printf("GPIO1 configuration: %X\n", reg);
}

void closeMmap(){
    close(fd);
}

void setLeds(unsigned short i) {
    if (i > 8) {
        printf("setLEDs error: i should be between 0 and 8 included");
        return;
    }

    unsigned short led0 = i & 0x01;
    unsigned short led1 = i & 0x02;
    unsigned short led2 = i & 0x04;
    unsigned short led3 = i & 0x08;

    if (led0) {
        *gpio_setdataout_addr = USR0_LED;
    }
    if (led1) {
        *gpio_setdataout_addr = USR1_LED;
    }
    if (led2) {
        *gpio_setdataout_addr = USR2_LED;
    }
    if (led3) {
        *gpio_setdataout_addr = USR3_LED;
    }
}

void clearLeds() {
    *gpio_cleardataout_addr = USR0_LED;
    *gpio_cleardataout_addr = USR1_LED;
    *gpio_cleardataout_addr = USR2_LED;
    *gpio_cleardataout_addr = USR3_LED;
}
