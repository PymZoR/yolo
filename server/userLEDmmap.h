#ifndef _BEAGLEBONE_GPIO_H_
#define _BEAGLEBONE_GPIO_H_

#define GPIO1_START_ADDR 0x4804C000
#define GPIO1_END_ADDR 0x4804DFFF
#define GPIO1_SIZE (GPIO1_END_ADDR - GPIO1_START_ADDR)
#define GPIO_OE 0x134
#define GPIO_SETDATAOUT 0x194
#define GPIO_CLEARDATAOUT 0x190
#define GPIO_DATAOUT 0x13c

#define LED0                      GPIO1_BASE+GPIO_DATAOUT
#define LED1                      GPIO1_BASE+GPIO_DATAOUT
#define LED2                      GPIO1_BASE+GPIO_DATAOUT
#define LED3                      GPIO1_BASE+GPIO_DATAOUT

#define LED0_SET                  GPIO1_BASE+GPIO_SETDATAOUT
#define LED1_SET                  GPIO1_BASE+GPIO_SETDATAOUT
#define LED2_SET                  GPIO1_BASE+GPIO_SETDATAOUT
#define LED3_SET                  GPIO1_BASE+GPIO_SETDATAOUT

#define LED0_CLR                  GPIO1_BASE+GPIO_CLEARDATAOUT
#define LED1_CLR                  GPIO1_BASE+GPIO_CLEARDATAOUT
#define LED2_CLR                  GPIO1_BASE+GPIO_CLEARDATAOUT
#define LED3_CLR                  GPIO1_BASE+GPIO_CLEARDATAOUT

#define USR0_LED (1<<21)
#define USR1_LED (1<<22)
#define USR2_LED (1<<23)
#define USR3_LED (1<<24)

#endif
