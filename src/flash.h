#ifndef FLASH_H
#define FLASH_H

#include <EEPROM.h>

#define SPEED_MEM_SIZE (24u) //byte
#define SPEED_BLOCK_SIZE (8u) //byte

#define SPEED_MODE_0 0
#define SPEED_MODE_1 1
#define SPEED_MODE_2 2

#define SPEED_MODE_ADDRESS 0

#define SPEED_MODE_P_ADDRESS (0 * (SPEED_BLOCK_SIZE))
#define SPEED_MODE_I_ADDRESS (1 * (SPEED_BLOCK_SIZE))
#define SPEED_MODE_D_ADDRESS (2 * (SPEED_BLOCK_SIZE))

uint8_t flash_init(double *P, double *I, double *D);
void flash_savePID(double P, double I, double D);
void flash_saveSpeedMode(uint8_t speedMode);

#endif