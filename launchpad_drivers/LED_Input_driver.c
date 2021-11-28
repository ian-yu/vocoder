/*
 * LED_Input_driver.c
 *
 *  Created on: Oct 4, 2021
 *      Author: ianyu
 */

#include <F28x_Project.h>
#include "LED_Input_driver.h"

void LED_Input_Init() {
    EALLOW;

    // Setting up LEDs

    GpioCtrlRegs.GPAGMUX1.all &= 0xFFFF0000;
    GpioCtrlRegs.GPAMUX1.all &= 0xFFFF0000;
    GpioCtrlRegs.GPADIR.all |= 0x0000000FF;

    // Setting up Switches/Buttons
    GpioCtrlRegs.GPAGMUX1.all &= 0x0F00FFFF;
    GpioCtrlRegs.GPAGMUX2.all &= 0xFFFFFFFC;
    GpioCtrlRegs.GPAMUX1.all &= 0x0F00FFFF;
    GpioCtrlRegs.GPAMUX2.all &= 0xFFFFFFFC;
    GpioCtrlRegs.GPAPUD.all &= 0xFFFE30FF;
    GpioCtrlRegs.GPADIR.all &= 0xFFFE30FF;

    // Turn off all LEDs
    GpioDataRegs.GPASET.bit.GPIO7 = 1;
    GpioDataRegs.GPASET.bit.GPIO6 = 1;
    GpioDataRegs.GPASET.bit.GPIO5 = 1;
    GpioDataRegs.GPASET.bit.GPIO4 = 1;
    GpioDataRegs.GPASET.bit.GPIO3 = 1;
    GpioDataRegs.GPASET.bit.GPIO2 = 1;
    GpioDataRegs.GPASET.bit.GPIO1 = 1;
    GpioDataRegs.GPASET.bit.GPIO0 = 1;

    EDIS;

}

void Write_LED(Uint16 led_data) {
    EALLOW;
    led_data = ~led_data;
    GpioDataRegs.GPADAT.all = (GpioDataRegs.GPADAT.all & 0xFFFFFF00) | ((Uint32)led_data & 0x000000FF);
    EDIS;
}

Uint16 Get_Input() {
    Uint16 input_data = GpioDataRegs.GPADAT.bit.GPIO16 << 6 | \
                        GpioDataRegs.GPADAT.bit.GPIO15 << 5 | \
                        GpioDataRegs.GPADAT.bit.GPIO14 << 4 | \
                        GpioDataRegs.GPADAT.bit.GPIO11 << 3 | \
                        GpioDataRegs.GPADAT.bit.GPIO10 << 2 | \
                        GpioDataRegs.GPADAT.bit.GPIO9 << 1 | \
                        GpioDataRegs.GPADAT.bit.GPIO8 & 0x007F;
    return input_data;
}

Uint16 buttonPressed(Uint16 button) {
    if (button == LEFT_BUTTON)
        return !GpioDataRegs.GPADAT.bit.GPIO10;
    else if (button == MIDDLE_BUTTON)
        return !GpioDataRegs.GPADAT.bit.GPIO9;
    else if (button == RIGHT_BUTTON)
        return !GpioDataRegs.GPADAT.bit.GPIO8;
    return 0;
}

Uint16 DIP_Input(Uint16 n) {
    return (Get_Input() >> (3 + n)) & 0x01;
}
