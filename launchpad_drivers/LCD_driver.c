/*
 * LCD_driver.c
 *
 *  Created on: Oct 3, 2021
 *      Author: ianyu
 */

#include <F2837xD_Device.h>
#include "LCD_driver.h"
#include "OneToOneI2CDriver.h"

static void Convert_Command_Reg(char command_byte, Uint16 *command_i2c_bytes);
static void Convert_Data_Reg(char data_byte, Uint16 *data_i2c_bytes);

static void Convert_Command_Reg(char command_byte, Uint16 *command_i2c_bytes) {
    command_i2c_bytes[0] = (command_byte & 0x00F0) | E_bm | command_bm;
    command_i2c_bytes[1] = (command_byte & 0x00F0) | command_bm;
    command_i2c_bytes[2] = ((command_byte << 4) & 0x00F0) | E_bm | command_bm;
    command_i2c_bytes[3] = ((command_byte << 4) & 0x00F0) | command_bm;

}

static void Convert_Data_Reg(char data_byte, Uint16 *data_i2c_bytes) {
    data_i2c_bytes[0] = (data_byte & 0x00F0) | E_bm | data_bm;
    data_i2c_bytes[1] = (data_byte & 0x00F0) | data_bm;
    data_i2c_bytes[2] = ((data_byte << 4) & 0x00F0) | E_bm | data_bm;
    data_i2c_bytes[3] = ((data_byte << 4) & 0x00F0) | data_bm;
}

void Write_Command_Reg(char command) {
    Uint16 command_i2c_bytes[4];
    Convert_Command_Reg(command, command_i2c_bytes);
    I2C_O2O_SendBytes(command_i2c_bytes, 4);
}

void Initialize_LCD() {
    Uint16 command_values[] = {0x33, 0x32, 0x28, 0x0F, 0x01};
    for (int i = 0; i < 5; i++) {
        Write_Command_Reg(command_values[i]);
    }
    for (int i = 0; i < 20000; i++);
}

void Write_Char_String(char *str) {
    int index = 0;
    Uint16 data_i2c_bytes[4];
    while (str[index] != 0x0) {
        Convert_Data_Reg(str[index], data_i2c_bytes);
        I2C_O2O_SendBytes(data_i2c_bytes, 4);
        index++;
    }
}
