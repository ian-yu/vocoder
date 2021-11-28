#include <F28x_Project.h>
#include "launchpad_drivers/launchpad_drivers.h"
#include "TI_FFT/fpu_cfft.h"
#include "TI_FFT/fpu_vector.h"

#define FFT_STAGES (9)
#define FFT_SIZE (1 << FFT_STAGES)

volatile int16 calc_done[2] = {1, 1};
volatile int16 buff_full[2] = {0, 0};
volatile int16 curr_buff = 0; // The current buffer that data is being sampled into

interrupt void DMA_CH6_pingpong_ISR();
interrupt void MCBSPB_TX_OUT_ISR();
void FFT_bhv(int16 buff);

// FFT Buffers
#pragma DATA_SECTION(xn, "ramgs0");
volatile int16 xn[2][N*2];

#pragma DATA_SECTION(FFT_inBuff, "CFFTdata1");
float FFT_inBuff[FFT_SIZE + 2U];

#pragma DATA_SECTION(FFT_outBuff, "CFFTdata2");
float FFT_outBuff[FFT_SIZE + 2U];

void createFFT_inBuff(int16 buff);

// CFFT structure (N/2)
CFFT_F32_STRUCT cfft;
CFFT_F32_STRUCT_Handle hnd_cfft = &cfft;

int main() {
    InitSysCtrl();
    DINT;
    EALLOW;
    // Init PIE
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // Initialize N/2 point CFFT
    CFFT_f32_setInputPtr(hnd_cfft, FFT_inBuff);
    CFFT_f32_setOutputPtr(hnd_cfft, FFT_outBuff);
    CFFT_f32_setTwiddlesPtr(hnd_cfft, CFFT_f32_twiddleFactors);
    CFFT_f32_setStages(hnd_cfft, FFT_STAGES-1);
    CFFT_f32_setFFTSize(hnd_cfft, FFT_SIZE >> 1);
    CFFT_f32_sincostable(hnd_cfft);

    // Initialize DMA
    EALLOW;
    PieVectTable.DMA_CH6_INT = &DMA_CH6_pingpong_ISR;
    EDIS;
    init_DMA_pingpong((volatile int*)xn[0], N);

    // Enable interrupt
    EALLOW;
    EnableInterrupts();
    EDIS;

    // Start DMA
    StartDMACH6();

    // Initializing Codec and associated interrupt
    InitSPIA();
    InitMcBSPb(DSP_16b);
    InitAIC23(DSP_16b);

    /*
    EALLOW;
    McbspbRegs.MFFINT.bit.RINT = 0;
    McbspbRegs.MFFINT.bit.XINT = 1;
    PieCtrlRegs.PIEIER6.bit.INTx7 = 1;
    PieVectTable.MCBSPB_TX_INT = &MCBSPB_TX_OUT_ISR;
    EDIS;
    */

    // Initialize LCD, RAM, LEDs, switches, buttons
    Initialize_Launchpad();

    while (1) {
        if (buff_full[0]) {
            FFT_bhv(0);
        }
        if (buff_full[1]) {
            FFT_bhv(1);
        }
    }
}

// Interrupt on transfer finish
interrupt void DMA_CH6_pingpong_ISR() {
    // Just finished transferring buff 0
    GpioDataRegs.GPETOGGLE.bit.GPIO139 = 1;
    if (curr_buff == 0) {
        buff_full[0] = 1;

        // Only start reading into buff 1 if DFT is done
        if (calc_done[1]) {
            curr_buff = 1;
            EALLOW;
            DmaRegs.CH6.DST_ADDR_SHADOW = (Uint32)(volatile Uint16 *)&xn[1][0];
            DmaRegs.CH6.DST_BEG_ADDR_SHADOW = (Uint32)(volatile Uint16 *)&xn[1][0];
            EDIS;
            StartDMACH6();
        }
    }
    else if (curr_buff == 1) {
        buff_full[1] = 1;
        if (calc_done[0]) {
            curr_buff = 0;
            EALLOW;
            DmaRegs.CH6.DST_ADDR_SHADOW = (Uint32)(volatile Uint16 *)&xn[0][0];
            DmaRegs.CH6.DST_BEG_ADDR_SHADOW = (Uint32)(volatile Uint16 *)&xn[0][0];
            EDIS;
            StartDMACH6();
        }
    }
    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP7;
}

interrupt void MCBSPB_TX_OUT_ISR() {
    float *ifft_output = CFFT_f32_getOutputPtr(hnd_cfft);
    if (calc_done[curr_buff] && (output_index < 512)) {
        McbspbRegs.DXR1.all = ifft_output[output_index];
        McbspbRegs.DXR2.all = ifft_output[output_index];
        output_index++;
    }
    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP6;
}

void createFFT_inBuff(int16 buff) {
    float *in_buff = hnd_cfft->InPtr;
    for (int16 i = 0; i < N; i++) {
        in_buff[i] = (float)(xn[buff][2*i] + xn[buff][2*i+1])/2.0f;
    }
}

void FFT_bhv(int16 buff) {
    calc_done[buff] = 0;
    buff_full[buff] = 0;

    // Create source buffer for FFT
    createFFT_inBuff(buff);

    // Calculate FFT and unpack
    CFFT_f32t(hnd_cfft);
    CFFT_f32_unpack(hnd_cfft);

    // Swap input and output
    float *fft_output = CFFT_f32_getOutputPtr(hnd_cfft);
    float *fft_input = CFFT_f32_getInputPtr(hnd_cfft);
    CFFT_f32_setCurrInputPtr(hnd_cfft, fft_output);
    CFFT_f32_setCurrOutputPtr(hnd_cfft, fft_input);

    // Re-pack and swap input and output
    CFFT_f32_pack(hnd_cfft);
    fft_output = CFFT_f32_getOutputPtr(hnd_cfft);
    fft_input = CFFT_f32_getInputPtr(hnd_cfft);
    CFFT_f32_setCurrInputPtr(hnd_cfft, fft_output);
    CFFT_f32_setCurrOutputPtr(hnd_cfft, fft_input);

    // Calculate IFFT
    ICFFT_f32t(hnd_cfft);

    calc_done[buff] = 1;
}
