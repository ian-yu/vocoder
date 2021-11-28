/*
 * SRAM_driver.h
 *
 *  Created on: Oct 3, 2021
 *      Author: ianyu
 */

#ifndef SRAM_DRIVER_H_
#define SRAM_DRIVER_H_

#include <F28x_Project.h>

#define WRITE_INSTR (0x02)
#define READ_INSTR (0x03)
#define RDMR (0x05)
#define WRMR (0x01)
#define byte_mode (0x00)
#define page_mode (0x80)
#define sequential_mode (0x40)


// Total RAM is 256K x 16, but each individual RAM is 256K x 8
// Total RAM address is word address
// Individual RAM address is byte address
// These are the boundaries for WORD addresses
#define SRAM0_MIN_ADDR (0x000000)
#define SRAM0_MAX_ADDR (0x01FFFF)
#define SRAM1_MIN_ADDR (0x020000)
#define SRAM1_MAX_ADDR (0x03FFFF)

void SPIB_Init();

// These two functions take in physical RAM addresses (byte), NOT words
void RamRead(Uint32 address, Uint32 len, Uint16 *read_buffer, Uint16 use_words, Uint16 CS);

void RamWrite(Uint32 address, Uint32 len, Uint16 *data, Uint16 use_words, Uint16 CS);

// Assumes little endian format, these are WORD addresses
void RamRead_16(Uint32 address, Uint32 len, Uint16 *read_buffer);

void RamWrite_16(Uint32 address, Uint32 len, Uint16 *data);

Uint16 Read_Mode_Register(Uint16 CS);

void Write_Mode_Register(Uint16 mode, Uint16 CS);

// Writes repeated constant
void RamWrite_constant(Uint32 address, Uint32 len, Uint16 data, Uint16 use_words, Uint16 CS);
void RamWrite_16_constant(Uint32 address, Uint32 len, Uint16 data);

void RamClear();


#endif /* SRAM_DRIVER_H_ */
