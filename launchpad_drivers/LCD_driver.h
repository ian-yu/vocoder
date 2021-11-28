/*
 * LCD_driver.h
 *
 *  Created on: Oct 3, 2021
 *      Author: ianyu
 */

#ifndef LCD_DRIVER_H_
#define LCD_DRIVER_H_

#define E_bm 0x04
#define command_bm 0x08
#define data_bm 0x9
#define LCD_ADDR (0x27)

void Write_Command_Reg(char command);

void Initialize_LCD();

void Write_Char_String(char *str);

#endif /* LCD_DRIVER_H_ */
