#include <F28x_Project.h>
#include "launchpad_drivers/launchpad_drivers.h"
#include "TI_FFT/fpu_cfft.h"
#include "TI_FFT/fpu_vector.h"
#include "TI_FFT/fpu_rfft.h"
#include "TI_FFT/fpu_fft_hamming.h"
#include "circ_buffer/circ_buffer.h"
#include "carriers.h"

#define FFT_STAGES      (9)                // 9
#define FFT_SIZE        (1 << FFT_STAGES)  // 512
#define WINDOW_SIZE     (FFT_SIZE)         // 512
#define DMA_SIZE        (FFT_SIZE >> 1)    // 256
#define FIFO_SIZE       (2048)
#define BANDS           (32)
#define BINS_PER_BAND   (WINDOW_SIZE/BANDS)
#define CARRIER_SAMPLES (256)

// Function Declarations

// buff_n = 0 - 3, 0 : mod_data[0:255], 1: mod_data[256:511], 2: mod_data[512:767], 3: mod_data[768:1023]
// back_or_front = 0, fills first half of FFT, back_or_front = 1, fills second half of FFT
void createFFT_inBuff(int16 buff_n, int16 back_or_front);

// DMA interrupt, triggers every 256 samples
interrupt void DMA_CH6_mod_ISR();

// MCBSPB RX interrupt, used to output samples from output FIFO
interrupt void MCBSPB_RX_OUT_ISR();

// Vocoder function
void vocode(int16 inbuff0, int16 inbuff1);

// Basic Input/Output test with FIFO
void inout_test();

// Run vocoder
void run_vocoder();

// FFT Buffers
#pragma DATA_SECTION(FFT_data1, "CFFTdata1")
float FFT_data1[FFT_SIZE + 2U];

#pragma DATA_SECTION(FFT_data2, "CFFTdata2")
float FFT_data2[FFT_SIZE + 2U];

#pragma DATA_SECTION(FFT_data1_carr, "CFFTdata3")
float FFT_data1_carr[FFT_SIZE + 2U];

#pragma DATA_SECTION(FFT_data2_carr, "CFFTdata4")
float FFT_data2_carr[FFT_SIZE + 2U];

// FFT structure
CFFT_F32_STRUCT cfft;
CFFT_F32_STRUCT_Handle hnd_cfft = &cfft;

CFFT_F32_STRUCT cfft_carr;
CFFT_F32_STRUCT_Handle hnd_cfft_carr = &cfft_carr;

// LR Modulator data - 4 * DMA_SIZE * 2 (left, right)
#pragma DATA_SECTION(mod_data, "ramgs1")
int16 mod_data[WINDOW_SIZE*4];

// Vocode output buffer
#pragma DATA_SECTION(vocode_buff, "ramgs0")
float vocode_buff[WINDOW_SIZE*2];

// DMA interrupt and variables
volatile int16 curr_buff = 0;
volatile int16 buff_full[4] = {0, 0, 0, 0};

// Output buffer
#pragma DATA_SECTION(out_buff, "ramgs2")
float out_buff[FIFO_SIZE];
cbuff out_FIFO;
cbuff_hnd out_FIFO_hnd = &out_FIFO;
Uint16 rd_en = 1;

// FFT hamming window
#pragma DATA_SECTION(fftWindow, "ramgs3")
const float fftWindow[FFT_SIZE >> 1] = HAMMING512;

// Carrier counter (0 - CARRIER_SAMPLES-1)
Uint32 carrier_count = 0;

// Microphone or line input
Uint16 mic_on = 0;

int main() {
    InitSysCtrl();
    Initialize_Interrupts();
    
    // Initialize WINDOW_SIZE/2 size CFFT
    CFFT_f32_setInputPtr(hnd_cfft, FFT_data1);
    CFFT_f32_setOutputPtr(hnd_cfft, FFT_data2);
    CFFT_f32_setTwiddlesPtr(hnd_cfft, CFFT_f32_twiddleFactors);
    CFFT_f32_setStages(hnd_cfft, FFT_STAGES - 1);
    CFFT_f32_setFFTSize(hnd_cfft, FFT_SIZE >> 1);

    // Initialize WINDOW_SIZE/2 size CFFT for carrier signal
    CFFT_f32_setInputPtr(hnd_cfft_carr, FFT_data1_carr);
    CFFT_f32_setOutputPtr(hnd_cfft_carr, FFT_data2_carr);
    CFFT_f32_setTwiddlesPtr(hnd_cfft_carr, CFFT_f32_twiddleFactors);
    CFFT_f32_setStages(hnd_cfft_carr, FFT_STAGES - 1);
    CFFT_f32_setFFTSize(hnd_cfft_carr, FFT_SIZE >> 1);

    // Initialize DMA
    init_DMA_pingpong((volatile int*)mod_data, DMA_SIZE, DMA_CH6_mod_ISR);

    // Enable global interrupts
    EALLOW;
    EnableInterrupts();
    EDIS;

    StartDMACH6();

    // Initializing Codec    
    EALLOW;
    PieCtrlRegs.PIEIER6.bit.INTx7 = 1;
    PieVectTable.MCBSPB_RX_INT = &MCBSPB_RX_OUT_ISR;
    IER |= M_INT6;
    EDIS;

    InitSPIA();
    InitMcBSPb(DSP_16b);
    InitAIC23(DSP_16b);

    // Initialize LCD, RAM, LEDs, switches, buttons
    Initialize_Launchpad();

    // Initialize FIFO
    init_cbuff(out_FIFO_hnd, out_buff, FIFO_SIZE);

    // Set up GPIO 139 to toggle
    EALLOW;
    GpioCtrlRegs.GPEDIR.bit.GPIO139 = 1;
    EDIS;

    while(1) {
        // Record 1s of audio into SRAM (48128 = 256 * 188 samples)
        if (buttonPressed(LEFT_BUTTON)) {
            Uint32 count = 0;
            rd_en = 0;
            Write_LED(0xFF);
            while (count < CARRIER_SAMPLES) {
                if (buff_full[0]) {
                    buff_full[0] = 0;
                    createFFT_inBuff(0, 0);
                    Uint32 wr_addr = count * DMA_SIZE * 2;
                    RamWrite_16(wr_addr, DMA_SIZE*2, (Uint16 *)CFFT_f32_getInputPtr(hnd_cfft));
                    count++;
                }
                if (buff_full[1]) {
                    buff_full[1] = 0;
                    createFFT_inBuff(1, 0);
                    Uint32 wr_addr = count * DMA_SIZE * 2;
                    RamWrite_16(wr_addr, DMA_SIZE*2, (Uint16 *)CFFT_f32_getInputPtr(hnd_cfft));
                    count++;
                }
                if (buff_full[2]) {
                    buff_full[2] = 0;
                    createFFT_inBuff(2, 0);
                    Uint32 wr_addr = count * DMA_SIZE * 2;
                    RamWrite_16(wr_addr, DMA_SIZE*2, (Uint16 *)CFFT_f32_getInputPtr(hnd_cfft));
                    count++;
                }
                if (buff_full[3]) {
                    buff_full[3] = 0;
                    createFFT_inBuff(3, 0);
                    Uint32 wr_addr = count * DMA_SIZE * 2;
                    RamWrite_16(wr_addr, DMA_SIZE*2, (Uint16 *)CFFT_f32_getInputPtr(hnd_cfft));
                    count++;
                }
            }

            Write_LED(0x00);
            rd_en = 1;
            while(buttonPressed(LEFT_BUTTON));
        }
        // If DIP switch is pressed and
        if (!DIP_Input(1) && mic_on) {
            mic_on = 0;
            InitAIC23(DSP_16b);
        }
        if (DIP_Input(1) && !mic_on) {
            mic_on = 1;
            InitAIC23_mic(DSP_16b);
        }
        if (DIP_Input(0)) {
            inout_test();
        }
        else {
            run_vocoder();
        }
    }
}

void createFFT_inBuff(int16 buff_n, int16 back_or_front) {
    float *in_buff = CFFT_f32_getInputPtr(hnd_cfft);
    if (back_or_front)
        in_buff = &(in_buff[FFT_SIZE >> 1]);
    for (int16 i = 0; i < DMA_SIZE; i++) {
        int16 buff_index = buff_n*DMA_SIZE*2 + 2*i;
        in_buff[i] = (float)(mod_data[buff_index] + mod_data[buff_index + 1])/2.0f;
    }
}

interrupt void DMA_CH6_mod_ISR() {
    // Curr buff finished filling
    buff_full[curr_buff] = 1;

    // Update current buffer
    curr_buff = (curr_buff + 1) & 3;
    EALLOW;
    DmaRegs.CH6.DST_ADDR_SHADOW = (Uint32)(volatile Uint16 *)&mod_data[curr_buff*DMA_SIZE*2];
    DmaRegs.CH6.DST_BEG_ADDR_SHADOW = (Uint32)(volatile Uint16 *)&mod_data[curr_buff*DMA_SIZE*2];
    EDIS;
    StartDMACH6();
    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP7;
}

interrupt void MCBSPB_RX_OUT_ISR() {
    // don't need read to clear flags because DMA clears flags !!
    float output_val = 0;
    if (rd_en)
        output_val = read_cbuff(out_FIFO_hnd);
    McbspbRegs.DXR1.all = (int16)output_val;
    McbspbRegs.DXR2.all = (int16)output_val;
    PieCtrlRegs.PIEACK.all |= PIEACK_GROUP6;
}

void inout_test() {
    if (buff_full[0]) {
        GpioDataRegs.GPETOGGLE.bit.GPIO139 = 1;
        buff_full[0] = 0;
        createFFT_inBuff(0, 0);
        write_cbuff(out_FIFO_hnd, hnd_cfft->InPtr, DMA_SIZE);
        GpioDataRegs.GPETOGGLE.bit.GPIO139 = 1;
    }
    if (buff_full[1]) {
        GpioDataRegs.GPETOGGLE.bit.GPIO139 = 1;
        buff_full[1] = 0;
        createFFT_inBuff(1, 0);
        write_cbuff(out_FIFO_hnd, hnd_cfft->InPtr, DMA_SIZE);
        GpioDataRegs.GPETOGGLE.bit.GPIO139 = 1;
    }
    if (buff_full[2]) {
        GpioDataRegs.GPETOGGLE.bit.GPIO139 = 1;
        buff_full[2] = 0;
        createFFT_inBuff(2, 0);
        write_cbuff(out_FIFO_hnd, hnd_cfft->InPtr, DMA_SIZE);
        GpioDataRegs.GPETOGGLE.bit.GPIO139 = 1;
    }
    if (buff_full[3]) {
        GpioDataRegs.GPETOGGLE.bit.GPIO139 = 1;
        buff_full[3] = 0;
        createFFT_inBuff(3, 0);
        write_cbuff(out_FIFO_hnd, hnd_cfft->InPtr, DMA_SIZE);
        GpioDataRegs.GPETOGGLE.bit.GPIO139 = 1;
    }
}

void vocode(int16 inbuff0, int16 inbuff1){
    // Create FFT buffer
    createFFT_inBuff(inbuff0, 0);
    createFFT_inBuff(inbuff1, 1);

    // Read in carrier data
    float *carrier_inbuff = CFFT_f32_getInputPtr(hnd_cfft_carr);
    Uint32 read_addr = carrier_count * DMA_SIZE * 2;
    Uint32 read_addr_next = read_addr + DMA_SIZE * 2;
    if (carrier_count == (CARRIER_SAMPLES-1))
        read_addr_next = 0;

    GpioDataRegs.GPETOGGLE.bit.GPIO139 = 1;
    RamRead_16(read_addr, DMA_SIZE * sizeof(float), (Uint16 *)carrier_inbuff);
    RamRead_16(read_addr_next, DMA_SIZE * sizeof(float), (Uint16 *)&carrier_inbuff[WINDOW_SIZE >> 1]);
    GpioDataRegs.GPETOGGLE.bit.GPIO139 = 1;


    carrier_count++;
    if (carrier_count > CARRIER_SAMPLES-1)
        carrier_count = 0;


    // Apply window and calculate FFT
    RFFT_f32_win(hnd_cfft->InPtr, (float *)&fftWindow, FFT_SIZE);
    CFFT_f32t(hnd_cfft);
    CFFT_f32_unpack(hnd_cfft);

    RFFT_f32_win(hnd_cfft_carr->InPtr, (float *)&fftWindow, FFT_SIZE);
    CFFT_f32t(hnd_cfft_carr);
    CFFT_f32_unpack(hnd_cfft_carr);

    // Get magnitude of carrier for scaling
    float *carr_fft_out = CFFT_f32_getCurrOutputPtr(hnd_cfft_carr);
    float *carr_fft_in = CFFT_f32_getCurrInputPtr(hnd_cfft_carr);
    CFFT_f32_setCurrInputPtr(hnd_cfft_carr, carr_fft_out);
    CFFT_f32_setCurrOutputPtr(hnd_cfft_carr, carr_fft_in);
    CFFT_f32_mag_TMU0(hnd_cfft_carr);
    Uint16 max_index = maxidx_SP_RV_2((const float *)carr_fft_in, WINDOW_SIZE >> 1);
    float carr_max_mag = carr_fft_in[max_index];

    // Get magnitude of modulator
    float *fft_output = CFFT_f32_getCurrOutputPtr(hnd_cfft);
    float *fft_input = CFFT_f32_getCurrInputPtr(hnd_cfft);
    CFFT_f32_setCurrInputPtr(hnd_cfft, fft_output);
    CFFT_f32_setCurrOutputPtr(hnd_cfft, fft_input);
    CFFT_f32_mag_TMU0(hnd_cfft);
    fft_output = CFFT_f32_getCurrOutputPtr(hnd_cfft);
    fft_input = CFFT_f32_getCurrInputPtr(hnd_cfft);

    // Calculate average energy in each band
    float mod_mags[BANDS >> 1];
    for (Uint16 i = 0; i < (BANDS >> 1); i++) {
        float temp_mag = 0;
        for (Uint16 j = 0; j < BINS_PER_BAND; j++) {
            Uint16 index = i * BINS_PER_BAND + j;
            temp_mag += fft_output[index];
        }
        mod_mags[i] = temp_mag/BINS_PER_BAND; // Averaging
    }

    // Multiply carrier signal by the average energy in modulator
    for (Uint16 i = 0; i < (WINDOW_SIZE >> 1); i++) {
        Uint16 mag_index = i/BINS_PER_BAND;
        Uint16 car_index = i*2;
        fft_input[car_index] = carr_fft_out[car_index] * mod_mags[mag_index] / carr_max_mag;
        fft_input[car_index + 1] = carr_fft_out[car_index + 1] * mod_mags[mag_index] / carr_max_mag;
        //fft_input[car_index] = carr_fft[car_index];
        //fft_input[car_index+1] = carr_fft[car_index+1];
    }

    // Re-pack
    CFFT_f32_pack(hnd_cfft);

    // Calculate IFFT
    fft_output = CFFT_f32_getCurrOutputPtr(hnd_cfft);
    fft_input = CFFT_f32_getCurrInputPtr(hnd_cfft);
    CFFT_f32_setInputPtr(hnd_cfft, fft_output);
    CFFT_f32_setOutputPtr(hnd_cfft, fft_input);
    ICFFT_f32t(hnd_cfft);
    fft_output = CFFT_f32_getOutputPtr(hnd_cfft);

    // Add to vocoder buffer
    Uint16 inbuff0_start = DMA_SIZE * inbuff0;
    for (Uint16 i = 0; i < DMA_SIZE; i++) {
        vocode_buff[inbuff0_start + i] += fft_output[i];
    }

    Uint16 inbuff1_start = DMA_SIZE * inbuff1;
    for (Uint16 i = 0; i < DMA_SIZE; i++) {
        vocode_buff[inbuff1_start + i] += fft_output[DMA_SIZE + i];
    }


    // Write to output FIFO
    write_cbuff(out_FIFO_hnd, &vocode_buff[inbuff0_start], DMA_SIZE);

    // Clear what was just output
    memset_fast(&vocode_buff[inbuff0_start], 0, DMA_SIZE * sizeof(float));
}

void run_vocoder() {
    if (buff_full[0]) {
        buff_full[0] = 0;
        vocode(3, 0);
    }
    if (buff_full[1]) {
        buff_full[1] = 0;
        vocode(0, 1);
    }
    if (buff_full[2]) {
        buff_full[2] = 0;
        vocode(1, 2);
    }
    if (buff_full[3]) {
        buff_full[3] = 0;
        vocode(2, 3);
    }
}
