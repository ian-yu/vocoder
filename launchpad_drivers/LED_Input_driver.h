/*
 * LED_Input_driver.h
 *
 *  Created on: Oct 4, 2021
 *      Author: ianyu
 */

#ifndef LED_INPUT_DRIVER_H_
#define LED_INPUT_DRIVER_H_

#include <F28x_Project.h>

enum buttons {LEFT_BUTTON = 2, MIDDLE_BUTTON = 1, RIGHT_BUTTON = 0};

void LED_Input_Init();

void Write_LED(Uint16 led_data);

// Push button values in lower 3 bits and DIP in upper 4 bits
Uint16 Get_Input();

// True or false if button is pressed
Uint16 buttonPressed(Uint16 button);

Uint16 DIP_Input(Uint16 n);

#endif /* LED_INPUT_DRIVER_H_ */
