#include "flash.h"

uint8_t init_flash(double *P, double *I, double *D)
{
    uint8_t modeSelected;
    EEPROM.begin(73);

    modeSelected = EEPROM.readByte(SPEED_MODE_ADDRESS);

    if (modeSelected >= 0 && modeSelected < 3)
    {
        *P = EEPROM.readDouble(1+modeSelected*SPEED_BLOCK_SIZE + SPEED_MODE_P_ADDRESS);
        *I = EEPROM.readDouble(1+modeSelected*SPEED_BLOCK_SIZE + SPEED_MODE_I_ADDRESS);
        *D = EEPROM.readDouble(1+modeSelected*SPEED_BLOCK_SIZE + SPEED_MODE_D_ADDRESS);
    }
    else
    {
        return 0;
    }
    return 1;
}


void flash_savePID(double P, double I, double D)
{
    uint8_t modeSelected;
    modeSelected = EEPROM.readByte(SPEED_MODE_ADDRESS);
    EEPROM.write(1+modeSelected*SPEED_BLOCK_SIZE + SPEED_MODE_P_ADDRESS, P);
    EEPROM.write(1+modeSelected*SPEED_BLOCK_SIZE + SPEED_MODE_I_ADDRESS, I);
    EEPROM.write(1+modeSelected*SPEED_BLOCK_SIZE + SPEED_MODE_D_ADDRESS, D);
    EEPROM.commit();
}

void flash_saveSpeedMode(uint8_t speedMode)
{
    EEPROM.write(SPEED_MODE_ADDRESS, speedMode);
    EEPROM.commit();
}