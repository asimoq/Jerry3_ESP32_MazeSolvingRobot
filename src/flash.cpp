#include "flash.h"

uint8_t modeSelected;

void flash_init()
{
    EEPROM.begin(73);
    flash_loadSpeedMode();
}


void flash_savePID(double P, double I, double D)
{
    EEPROM.writeDouble(1+modeSelected*SPEED_MEM_SIZE + SPEED_MODE_P_ADDRESS, P);
    EEPROM.writeDouble(1+modeSelected*SPEED_MEM_SIZE + SPEED_MODE_I_ADDRESS, I);
    EEPROM.writeDouble(1+modeSelected*SPEED_MEM_SIZE + SPEED_MODE_D_ADDRESS, D);
    EEPROM.commit();
}

void flash_saveSpeedMode(uint8_t speedMode)
{
    if (speedMode >= 0 && speedMode < 3)
    {
        EEPROM.write(SPEED_MODE_ADDRESS, speedMode);
        EEPROM.commit();
        modeSelected = speedMode;
    }
}

void flash_loadPID(double *P, double *I, double *D)
{
    *P = EEPROM.readDouble(1+modeSelected*SPEED_MEM_SIZE + SPEED_MODE_P_ADDRESS);
    *I = EEPROM.readDouble(1+modeSelected*SPEED_MEM_SIZE + SPEED_MODE_I_ADDRESS);
    *D = EEPROM.readDouble(1+modeSelected*SPEED_MEM_SIZE + SPEED_MODE_D_ADDRESS);
    if (isnan(*P) || isnan(*I) || isnan(*D))
    {
        /* Load and save default values */
        flash_savePID(1, 0, 0);
        *P = 1;
        *I = 0;
        *D = 0;
    }
    
}

void flash_loadSpeedMode()
{
    modeSelected = EEPROM.readByte(SPEED_MODE_ADDRESS);
    if (!(modeSelected >= 0 && modeSelected < 3))
    {
        flash_saveSpeedMode(0); //Ha nincs normál adat, akkor írjon be 0-t
        modeSelected = 0;
    }
}

uint8_t flash_getModeSelected()
{
    return modeSelected;
}