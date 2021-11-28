/*
 * launchpad_drivers.h
 *
 *  Created on: Oct 9, 2021
 *      Author: ianyu
 */

#ifndef LAUNCHPAD_DRIVERS_H_
#define LAUNCHPAD_DRIVERS_H_

#include "LCD_driver.h"
#include "LED_Input_driver.h"
#include "OneToOneI2CDriver.h"
#include "SRAM_driver.h"
#include "AIC23.h"
#include "InitAIC23.h"
#include "DMA_driver.h"

void Initialize_Launchpad();
void Initialize_Interrupts();

#endif /* LAUNCHPAD_DRIVERS_H_ */
