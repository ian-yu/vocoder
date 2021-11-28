/*
 * SRAM_driver.c
 *
 *  Created on: Oct 3, 2021
 *      Author: ianyu
 */

#include "SRAM_driver.h"
#include <F2837xD_Device.h>

static void SPIB_GPIO_Init();
static Uint16 SpiTransmit(Uint16 transmitdata);

static void SPIB_GPIO_Init() {    
    // Configure GPIO 63 (SPISIMOB)
    GpioCtrlRegs.GPBGMUX2.all |= 0xC0000000;
    GpioCtrlRegs.GPBMUX2.all |= 0xC0000000;
    GpioCtrlRegs.GPBDIR.bit.GPIO63 = 1;

    // Configure GPIO 64,65 (SPISOMIB, SPICLKB)
    GpioCtrlRegs.GPCGMUX1.all |= 0x0000000F;
    GpioCtrlRegs.GPCMUX1.all |= 0x0000000F;
    GpioCtrlRegs.GPCDIR.bit.GPIO64 = 0;
    GpioCtrlRegs.GPCDIR.bit.GPIO65 = 1;

    // Configure GPIO 66, 67 (CS0, CS1)
    GpioCtrlRegs.GPCGMUX1.all &= ~(0x000000F0);
    GpioCtrlRegs.GPCMUX1.all &= ~(0x000000F0);
    GpioDataRegs.GPCSET.bit.GPIO66 = 1;
    GpioDataRegs.GPCSET.bit.GPIO67 = 1;
    GpioCtrlRegs.GPCDIR.bit.GPIO66 = 1;
    GpioCtrlRegs.GPCDIR.bit.GPIO67 = 1;
}

void SPIB_Init() {
    EALLOW;

    SPIB_GPIO_Init();

    // Clear SW Reset
    SpibRegs.SPICCR.bit.SPISWRESET = 0;

    // Output on falling edge, latched on rising edge, CLK default low
    // CLKPOLARITY = 0, CLKPHASE = 0
    SpibRegs.SPICCR.bit.CLKPOLARITY = 0;
    SpibRegs.SPICTL.bit.CLK_PHASE = 0;

    // Master mode
    SpibRegs.SPICTL.bit.MASTER_SLAVE = 1;

    // Set LSPCLK to SYSCLK
    ClkCfgRegs.LOSPCP.bit.LSPCLKDIV = 0;

    // Enable HS mode
    SpibRegs.SPICCR.bit.HS_MODE = 1;
    
    // Baud rate
    SpibRegs.SPIBRR.bit.SPI_BIT_RATE = 0x05;

    // SPI character length
    SpibRegs.SPICCR.bit.SPICHAR = 0x7;

    // Enable Transmit
    SpibRegs.SPICTL.bit.TALK = 1;

    // Release SW Reset
    SpibRegs.SPICCR.bit.SPISWRESET = 1;

    // Run SPI when emulation suspended
    SpibRegs.SPIPRI.bit.FREE = 1;

    EDIS;
}

static Uint16 SpiTransmit(Uint16 transmitdata) {
    SpibRegs.SPITXBUF = transmitdata << 8;
    while(SpibRegs.SPISTS.bit.INT_FLAG != 1);
    Uint16 receive_data = SpibRegs.SPIRXBUF;    
    return receive_data;
}

void RamRead(Uint32 address, Uint32 len, Uint16 *read_buffer, Uint16 use_words, Uint16 CS) {
    if (CS == 0)    
        GpioDataRegs.GPCCLEAR.bit.GPIO66 = 1;
    else if (CS == 1)
        GpioDataRegs.GPCCLEAR.bit.GPIO67 = 1;

    SpiTransmit(READ_INSTR);
    SpiTransmit((Uint16)(address >> 16) & 0x00FF);
    SpiTransmit((Uint16)(address >> 8) & 0x00FF);
    SpiTransmit((Uint16)address & 0x00FF);

    // Dummy transmit
    SpiTransmit((Uint16)0x0);

    if (use_words == 0) {
        for (Uint32 i = 0; i < len; i++) {
            read_buffer[i] = SpiTransmit((Uint16)0x0);
            address++;
            if (address > 0x3FFFF)
                break;
        }
    }
    else {
        for (Uint32 i = 0; i < len; i++) {
            Uint16 lower_byte = SpiTransmit((Uint16)0x0);
            Uint16 upper_byte = SpiTransmit((Uint16)0x0);
            read_buffer[i] = ((upper_byte << 8) & 0xFF00) | (lower_byte & 0x00FF);
            address += 2;
            if (address > 0x3FFFF)
                break;
        }
    }    

    if (CS == 0)
        GpioDataRegs.GPCSET.bit.GPIO66 = 1;
    else if (CS == 1)
        GpioDataRegs.GPCSET.bit.GPIO67 = 1;    
}

void RamWrite(Uint32 address, Uint32 len, Uint16 *data, Uint16 use_words, Uint16 CS) {
    if (CS == 0)    
        GpioDataRegs.GPCCLEAR.bit.GPIO66 = 1;
    else if (CS == 1)
        GpioDataRegs.GPCCLEAR.bit.GPIO67 = 1;

    // Transmit Command and 24-bit address
    SpiTransmit(WRITE_INSTR);
    SpiTransmit((Uint16)(address >> 16) & 0x00FF);
    SpiTransmit((Uint16)(address >> 8) & 0x00FF);
    SpiTransmit((Uint16)address & 0x00FF);
    
    // Each element in data is treated as a byte or word?
    if (use_words == 0){
        for (Uint32 i = 0; i < len; i++) {
            SpiTransmit((Uint16)data[i]);
            address++;
            if (address > 0x3FFFF)
                break;
        }
    }
    else {
        for (Uint32 i = 0; i < len; i++) {
            Uint16 lower_byte = data[i] & 0x00FF;
            Uint16 upper_byte = (data[i] >> 8) & 0x00FF;
            SpiTransmit(lower_byte);
            SpiTransmit(upper_byte);
            address += 2;
            if (address > 0x3FFFF)
                break;
        }
    }

    if (CS == 0)
        GpioDataRegs.GPCSET.bit.GPIO66 = 1;
    else if (CS == 1)
        GpioDataRegs.GPCSET.bit.GPIO67 = 1;
}

void RamRead_16(Uint32 address, Uint32 len, Uint16 *read_buffer){

    Uint32 last_addr = address + len - 1;
    Uint32 byte_addr = (address << 1) & 0x0003FFFF;

    // Read RAM0 only
    if (last_addr <= SRAM0_MAX_ADDR) {
        RamRead(byte_addr, len, read_buffer, 1, 0);
    }
    // Read both RAM0 and RAM1
    else if ((address <= SRAM0_MAX_ADDR) && (last_addr >= SRAM1_MIN_ADDR) && (last_addr <= SRAM1_MAX_ADDR)) {
        Uint32 SRAM0_len = SRAM1_MIN_ADDR - address;
        Uint32 SRAM1_len = len - SRAM0_len;

        // Read first part from SRAM0
        RamRead(byte_addr, SRAM0_len, read_buffer, 1, 0);

        // Read second part from SRAM1
        RamRead(0x0, SRAM1_len, &read_buffer[SRAM0_len], 1, 1);
    }
    // Read RAM1 only
    else if ((address >= SRAM1_MIN_ADDR) && (last_addr <= SRAM1_MAX_ADDR)) {
        RamRead(byte_addr, len, read_buffer, 1, 1);
    }

}

void RamWrite_16(Uint32 address, Uint32 len, Uint16 *data){

    Uint32 last_addr = address + len - 1;
    Uint32 byte_addr = (address << 1) & 0x0003FFFF;

    // Write RAM0 only
    if (last_addr <= SRAM0_MAX_ADDR) {
        RamWrite(byte_addr, len, data, 1, 0);
    }
    // Write both RAM0 and RAM1
    else if ((address <= SRAM0_MAX_ADDR) && (last_addr > SRAM1_MIN_ADDR) && (last_addr <= SRAM1_MAX_ADDR)) {
        Uint32 SRAM0_len = SRAM1_MIN_ADDR - address;
        Uint32 SRAM1_len = len - SRAM0_len;

        // Write first part from SRAM0
        RamWrite(byte_addr, SRAM0_len, data, 1, 0);

        // Write second part from SRAM1
        RamWrite(0x0, SRAM1_len, &data[SRAM0_len], 1, 1);
    }
    // Write RAM1 only
    else if ((address >= SRAM1_MIN_ADDR) && (last_addr <= SRAM1_MAX_ADDR)) {
        RamWrite(byte_addr, len, data, 1, 1);
    }
}

Uint16 Read_Mode_Register(Uint16 CS) {
    if (CS == 0)    
        GpioDataRegs.GPCCLEAR.bit.GPIO66 = 1;
    else if (CS == 1)
        GpioDataRegs.GPCCLEAR.bit.GPIO67 = 1;

    SpiTransmit(RDMR);
    Uint16 receive_data = SpiTransmit((Uint16)0x0);

    if (CS == 0)
        GpioDataRegs.GPCSET.bit.GPIO66 = 1;
    else if (CS == 1)
        GpioDataRegs.GPCSET.bit.GPIO67 = 1;

    return receive_data;
}

void Write_Mode_Register(Uint16 mode, Uint16 CS) {
    if (CS == 0)    
        GpioDataRegs.GPCCLEAR.bit.GPIO66 = 1;
    else if (CS == 1)
        GpioDataRegs.GPCCLEAR.bit.GPIO67 = 1;

    SpiTransmit(WRMR);
    SpiTransmit(mode & 0x00FF);

    if (CS == 0)
        GpioDataRegs.GPCSET.bit.GPIO66 = 1;
    else if (CS == 1)
        GpioDataRegs.GPCSET.bit.GPIO67 = 1;
    
}

void RamWrite_constant(Uint32 address, Uint32 len, Uint16 data, Uint16 use_words, Uint16 CS) {
    if (CS == 0)
        GpioDataRegs.GPCCLEAR.bit.GPIO66 = 1;
    else if (CS == 1)
        GpioDataRegs.GPCCLEAR.bit.GPIO67 = 1;

    // Transmit Command and 24-bit address
    SpiTransmit(WRITE_INSTR);
    SpiTransmit((Uint16)(address >> 16) & 0x00FF);
    SpiTransmit((Uint16)(address >> 8) & 0x00FF);
    SpiTransmit((Uint16)address & 0x00FF);

    // Each element in data is treated as a byte or word?
    if (use_words == 0){
        for (Uint32 i = 0; i < len; i++) {
            SpiTransmit(data);
            address++;
            if (address > 0x3FFFF)
                break;
        }
    }
    else {
        Uint16 lower_byte = data & 0x00FF;
        Uint16 upper_byte = (data >> 8) & 0x00FF;
        for (Uint32 i = 0; i < len; i++) {
            SpiTransmit(lower_byte);
            SpiTransmit(upper_byte);
            address += 2;
            if (address > 0x3FFFF)
                break;
        }
    }

    if (CS == 0)
        GpioDataRegs.GPCSET.bit.GPIO66 = 1;
    else if (CS == 1)
        GpioDataRegs.GPCSET.bit.GPIO67 = 1;
}

void RamWrite_16_constant(Uint32 address, Uint32 len, Uint16 data) {
    Uint32 last_addr = address + len - 1;
    Uint32 byte_addr = (address << 1) & 0x0003FFFF;

    // Write RAM0 only
    if (last_addr <= SRAM0_MAX_ADDR) {
        RamWrite_constant(byte_addr, len, data, 1, 0);
    }
    // Write both RAM0 and RAM1
    else if ((address <= SRAM0_MAX_ADDR) && (last_addr > SRAM1_MIN_ADDR) && (last_addr <= SRAM1_MAX_ADDR)) {
        Uint32 SRAM0_len = SRAM1_MIN_ADDR - address;
        Uint32 SRAM1_len = len - SRAM0_len;

        // Write first part from SRAM0
        RamWrite_constant(byte_addr, SRAM0_len, data, 1, 0);

        // Write second part from SRAM1
        RamWrite_constant(0x0, SRAM1_len, data, 1, 1);
    }
    // Write RAM1 only
    else if ((address >= SRAM1_MIN_ADDR) && (last_addr <= SRAM1_MAX_ADDR)) {
        RamWrite_constant(byte_addr, len, data, 1, 1);
    }    
}


void RamClear() {
    RamWrite_constant(0, 0x40000, 0, 0, 0);
    RamWrite_constant(0, 0x40000, 0, 0, 1);
}

