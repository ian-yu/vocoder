/*
 * initAIC23.c
 */

#include <stdint.h>
#include <F28x_Project.h>
#include "AIC23.h"
#include "InitAIC23.h"

/***************** User Functions *****************/

void InitAIC23(audioMode_t mode) {
    SmallDelay();
    uint16_t command;
    command = reset();
    SpiaTransmit(command);
    SmallDelay();
    command = softpowerdown();       // Power down everything except device and clocks
    SpiaTransmit(command);
    SmallDelay();
    command = linput_volctl(LIV);    // Unmute left line input and maintain default volume
    SpiaTransmit(command);
    SmallDelay();
    command = rinput_volctl(RIV);    // Unmute right line input and maintain default volume
    SpiaTransmit(command);
    SmallDelay();
    command = lhp_volctl(LHV);       // Left headphone volume control
    SpiaTransmit(command);
    SmallDelay();
    command = rhp_volctl(RHV);       // Right headphone volume control
    SpiaTransmit(command);
    SmallDelay();
    command = nomicaaudpath();      // Turn on DAC, mute mic
    SpiaTransmit(command);
    SmallDelay();
    command = digaudiopath();       // Disable DAC mute, add de-emph
    SpiaTransmit(command);
    SmallDelay();

    // Set digital audio interface to selected audio mode
    switch(mode) {
    case I2S_32b:
        command = I2S32digaudinterface();
        break;
    case I2S_16b:
        command = I2S16digaudinterface();
        break;
    case DSP_32b:
        command = DSP32digaudinterface();
        break;
    case DSP_16b:
        command = DSP16digaudinterface();
        break;
    }
    SpiaTransmit(command);
    SmallDelay();
    command = CLKsampleratecontrol (SR48);
    SpiaTransmit(command);
    SmallDelay();

    command = digact();             // Activate digital interface
    SpiaTransmit(command);
    SmallDelay();
    command = nomicpowerup();      // Turn everything on except Mic.
    SpiaTransmit(command);
}

void InitAIC23_mic(audioMode_t mode) {
    SmallDelay();
    uint16_t command;
    command = reset();
    SpiaTransmit(command);
    SmallDelay();
    command = softpowerdown();       // Power down everything except device and clocks
    SpiaTransmit(command);
    SmallDelay();
    command = linput_volctl(LIV);    // Unmute left line input and maintain default volume
    SpiaTransmit(command);
    SmallDelay();
    command = rinput_volctl(RIV);    // Unmute right line input and maintain default volume
    SpiaTransmit(command);
    SmallDelay();
    command = lhp_volctl(LHV);       // Left headphone volume control
    SpiaTransmit(command);
    SmallDelay();
    command = rhp_volctl(RHV);       // Right headphone volume control
    SpiaTransmit(command);
    SmallDelay();
    command = aaudpath();
    SpiaTransmit(command);
    SmallDelay();
    command = digaudiopath();       // Disable DAC mute, add de-emph
    SpiaTransmit(command);
    SmallDelay();

    // Set digital audio interface to selected audio mode
    switch(mode) {
    case I2S_32b:
        command = I2S32digaudinterface();
        break;
    case I2S_16b:
        command = I2S16digaudinterface();
        break;
    case DSP_32b:
        command = DSP32digaudinterface();
        break;
    case DSP_16b:
        command = DSP16digaudinterface();
        break;
    }
    SpiaTransmit(command);
    SmallDelay();
    command = CLKsampleratecontrol (SR48);
    SpiaTransmit(command);
    SmallDelay();

    command = digact();             // Activate digital interface
    SpiaTransmit(command);
    SmallDelay();
    command = fullpowerup();      // Turn everything on except Mic.
    SpiaTransmit(command);
}


void InitMcBSPb(audioMode_t mode)
{
    /* Init McBSPb GPIO Pins */

    //modify the GPxMUX, GPxGMUX, GPxQSEL
    //all pins should be set to asynch qualification

    /*
     * MDXB -> GPIO24
     * MDRB -> GPIO25
     * MCLKRB -> GPIO60
     * MCLKXB -> GPIO26
     * MFSRB -> GPIO61
     * MFSXB -> GPIO27
     */
    EALLOW;

    // MDXB -> GPIO24 (GPIOA)

    GpioCtrlRegs.GPAGMUX2.bit.GPIO24 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 3;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO24 = 3;

    // MDRB -> GPIO25 (GPIOA)

    GpioCtrlRegs.GPAGMUX2.bit.GPIO25 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO25 = 3;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO25 = 3;

    // MFSRB -> GPIO61 (GPIOB)

    GpioCtrlRegs.GPBGMUX2.bit.GPIO61 = 0;
    GpioCtrlRegs.GPBMUX2.bit.GPIO61 = 1;
    GpioCtrlRegs.GPBQSEL2.bit.GPIO61 = 3;

    // MFSXB -> GPIO27 (GPIOA)

    GpioCtrlRegs.GPAGMUX2.bit.GPIO27 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO27 = 3;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO27 = 3;

    // MCLKRB -> GPIO60 (GPIOB)

    GpioCtrlRegs.GPBGMUX2.bit.GPIO60 = 0;
    GpioCtrlRegs.GPBMUX2.bit.GPIO60 = 1;
    GpioCtrlRegs.GPBQSEL2.bit.GPIO60 = 3;

    // MCLKXB -> GPIO26 (GPIOA)

    GpioCtrlRegs.GPAGMUX2.bit.GPIO26 = 0;
    GpioCtrlRegs.GPAMUX2.bit.GPIO26 = 3;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO26 = 3;
    EDIS;

    /* Init McBSPb for I2S mode */
    EALLOW;
    McbspbRegs.SPCR2.all = 0; // Reset FS generator, sample rate generator & transmitter
    McbspbRegs.SPCR1.all = 0; // Reset Receiver, Right justify word
    McbspbRegs.SPCR1.bit.RJUST = 2; // left-justify word in DRR and zero-fill LSBs
    McbspbRegs.MFFINT.all=0x0; // Disable all interrupts
    McbspbRegs.SPCR1.bit.RINTM = 0; // McBSP interrupt flag - RRDY
    McbspbRegs.SPCR2.bit.XINTM = 0; // McBSP interrupt flag - XRDY
    // Clear Receive Control Registers
    McbspbRegs.RCR2.all = 0x0;
    McbspbRegs.RCR1.all = 0x0;
    // Clear Transmit Control Registers
    McbspbRegs.XCR2.all = 0x0;
    McbspbRegs.XCR1.all = 0x0;

    // Configure McBSP based on audio mode
    switch(mode) {
    case I2S_32b:
        // Set Receive/Transmit to 32-bit operation
        McbspbRegs.RCR2.bit.RWDLEN2 = 5;
        McbspbRegs.RCR1.bit.RWDLEN1 = 5;
        McbspbRegs.XCR2.bit.XWDLEN2 = 5;
        McbspbRegs.XCR1.bit.XWDLEN1 = 5;
        McbspbRegs.RCR2.bit.RPHASE = 1; // Dual-phase frame for receive
        McbspbRegs.RCR1.bit.RFRLEN1 = 0; // Receive frame length = 1 word in phase 1
        McbspbRegs.RCR2.bit.RFRLEN2 = 0; // Receive frame length = 1 word in phase 2
        McbspbRegs.XCR2.bit.XPHASE = 1; // Dual-phase frame for transmit
        McbspbRegs.XCR1.bit.XFRLEN1 = 0; // Transmit frame length = 1 word in phase 1
        McbspbRegs.XCR2.bit.XFRLEN2 = 0; // Transmit frame length = 1 word in phase 2
        // I2S mode: R/XDATDLY = 1 always
        McbspbRegs.RCR2.bit.RDATDLY = 1;
        McbspbRegs.XCR2.bit.XDATDLY = 1;
        break;
    case I2S_16b:
        // Set Receive/Transmit to 32-bit operation
        McbspbRegs.RCR2.bit.RWDLEN2 = 5;
        McbspbRegs.RCR1.bit.RWDLEN1 = 5;
        McbspbRegs.XCR2.bit.XWDLEN2 = 5;
        McbspbRegs.XCR1.bit.XWDLEN1 = 5;
        McbspbRegs.RCR2.bit.RPHASE = 1; // Dual-phase frame for receive
        McbspbRegs.RCR1.bit.RFRLEN1 = 0; // Receive frame length = 1 word in phase 1
        McbspbRegs.RCR2.bit.RFRLEN2 = 0; // Receive frame length = 1 word in phase 2
        McbspbRegs.XCR2.bit.XPHASE = 1; // Dual-phase frame for transmit
        McbspbRegs.XCR1.bit.XFRLEN1 = 0; // Transmit frame length = 1 word in phase 1
        McbspbRegs.XCR2.bit.XFRLEN2 = 0; // Transmit frame length = 1 word in phase 2
        // I2S mode: R/XDATDLY = 1 always
        McbspbRegs.RCR2.bit.RDATDLY = 1;
        McbspbRegs.XCR2.bit.XDATDLY = 1;
        break;
    case DSP_32b:
        // Set Receive/Transmit to 32-bit operation
        McbspbRegs.RCR2.bit.RWDLEN2 = 5;
        McbspbRegs.RCR1.bit.RWDLEN1 = 5;
        McbspbRegs.XCR2.bit.XWDLEN2 = 5;
        McbspbRegs.XCR1.bit.XWDLEN1 = 5;
        McbspbRegs.RCR2.bit.RPHASE = 1; // Dual-phase frame for receive
        McbspbRegs.RCR1.bit.RFRLEN1 = 0; // Receive frame length = 1 word in phase 1
        McbspbRegs.RCR2.bit.RFRLEN2 = 0; // Receive frame length = 1 word in phase 2
        McbspbRegs.XCR2.bit.XPHASE = 1; // Dual-phase frame for transmit
        McbspbRegs.XCR1.bit.XFRLEN1 = 0; // Transmit frame length = 1 word in phase 1
        McbspbRegs.XCR2.bit.XFRLEN2 = 0; // Transmit frame length = 1 word in phase 2
        // DSP mode: R/XDATDLY = 0 always
        McbspbRegs.RCR2.bit.RDATDLY = 0;
        McbspbRegs.XCR2.bit.XDATDLY = 0;
        break;
    case DSP_16b:
        // Set Receive/Transmit to 32-bit operation
        McbspbRegs.RCR2.bit.RWDLEN2 = 0;
        McbspbRegs.RCR1.bit.RWDLEN1 = 5;
        McbspbRegs.XCR2.bit.XWDLEN2 = 0;
        McbspbRegs.XCR1.bit.XWDLEN1 = 5;
        McbspbRegs.RCR2.bit.RPHASE = 0; // Single-phase frame for receive
        McbspbRegs.RCR1.bit.RFRLEN1 = 0; // Receive frame length = 1 word in phase 1
        McbspbRegs.RCR2.bit.RFRLEN2 = 0; // Receive frame length = 1 word in phase 2
        McbspbRegs.XCR2.bit.XPHASE = 0; // Single-phase frame for transmit
        McbspbRegs.XCR1.bit.XFRLEN1 = 0; // Transmit frame length = 1 word in phase 1
        McbspbRegs.XCR2.bit.XFRLEN2 = 0; // Transmit frame length = 1 word in phase 2
        // DSP mode: R/XDATDLY = 0 always
        McbspbRegs.RCR2.bit.RDATDLY = 0;
        McbspbRegs.XCR2.bit.XDATDLY = 0;
        break;
    }

    // Frame Width = 1 CLKG period, CLKGDV must be 1 as slave
    McbspbRegs.SRGR1.all = 0x0001;
    McbspbRegs.PCR.all=0x0000;
    // Transmit frame synchronization is supplied by an external source via the FSX pin
    McbspbRegs.PCR.bit.FSXM = 0;
    // Receive frame synchronization is supplied by an external source via the FSR pin
    McbspbRegs.PCR.bit.FSRM = 0;
    // Select sample rate generator to be signal on MCLKR pin
    McbspbRegs.PCR.bit.SCLKME = 1;
    McbspbRegs.SRGR2.bit.CLKSM = 0;
    // Receive frame-synchronization pulses are active low - (L-channel first)
    McbspbRegs.PCR.bit.FSRP = 1;
    // Transmit frame-synchronization pulses are active low - (L-channel first)
    McbspbRegs.PCR.bit.FSXP = 1;
    // Receive data is sampled on the rising edge of MCLKR
    McbspbRegs.PCR.bit.CLKRP = 1;



    // Transmit data is sampled on the rising edge of CLKX
    McbspbRegs.PCR.bit.CLKXP = 1;

    // The transmitter gets its clock signal from MCLKX
    McbspbRegs.PCR.bit.CLKXM = 0;
    // The receiver gets its clock signal from MCLKR
    McbspbRegs.PCR.bit.CLKRM = 0;
    // Enable Receive Interrupt
    McbspbRegs.MFFINT.bit.RINT = 1;
    // Ignore unexpected frame sync
    //McbspbRegs.XCR2.bit.XFIG = 1;
    McbspbRegs.SPCR2.all |=0x00C0; // Frame sync & sample rate generators pulled out of reset
    delay_loop();
    McbspbRegs.SPCR2.bit.XRST=1; // Enable Transmitter
    McbspbRegs.SPCR1.bit.RRST=1; // Enable Receiver
    EDIS;
}


void InitSPIA()
{
    /* Init GPIO pins for SPIA */

    //enable pullups for each pin
    //set to asynch qualification
    //configure each mux

    //SPISTEA -> GPIO19
    //SPISIMOA -> GPIO58
    //SPICLKA -> GPIO18

    EALLOW;

    //enable pullups
    GpioCtrlRegs.GPAPUD.bit.GPIO19 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO18 = 0;
    GpioCtrlRegs.GPBPUD.bit.GPIO58 = 0;

    GpioCtrlRegs.GPAGMUX2.bit.GPIO19 = 0;
    GpioCtrlRegs.GPAGMUX2.bit.GPIO18 = 0;
    GpioCtrlRegs.GPBGMUX2.bit.GPIO58 = 3;

    GpioCtrlRegs.GPAMUX2.bit.GPIO19 = 1;
    GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 1;
    GpioCtrlRegs.GPBMUX2.bit.GPIO58 = 3;

    //asynch qual
    GpioCtrlRegs.GPAQSEL2.bit.GPIO19 = 3;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO18 = 3;
    GpioCtrlRegs.GPBQSEL2.bit.GPIO58 = 3;

    EDIS;

    /* Init SPI peripheral */
    SpiaRegs.SPICCR.all = 0x5F; //CLKPOL = 0, SOMI = SIMO (loopback), 16 bit characters
    SpiaRegs.SPICTL.all = 0x06; //master mode, enable transmissions
    SpiaRegs.SPIBRR.all = 50; //gives baud rate of approx 850 kHz

    SpiaRegs.SPICCR.bit.SPISWRESET = 1;
    SpiaRegs.SPIPRI.bit.FREE = 1;

}
void SpiaTransmit(uint16_t data)
{
    /* Transmit 16 bit data */
    SpiaRegs.SPIDAT = data; //send data to SPI register
    while(SpiaRegs.SPISTS.bit.INT_FLAG == 0); //wait until the data has been sent
    Uint16 dummyLoad = SpiaRegs.SPIRXBUF; //reset flag
}
