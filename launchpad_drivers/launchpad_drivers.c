#include "launchpad_drivers.h"

void Initialize_Launchpad() {
    // Initialize i2c with 5 KHz clk
    I2C_O2O_Master_Init(LCD_ADDR, 200.0, 5.0);
    Initialize_LCD();

    // Initializing SPIB and LEDs, switches, and buttons
    SPIB_Init();
    LED_Input_Init();

    // Initializing RAM to sequential mode
    Write_Mode_Register(sequential_mode, 0);
    Write_Mode_Register(sequential_mode, 1);

    // Clear RAM initially
    RamClear();
}

void Initialize_Interrupts() {
    DINT;
    EALLOW;
    // Init PIE
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();
}
