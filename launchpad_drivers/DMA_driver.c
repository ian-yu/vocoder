#include "DMA_driver.h"
#include "F28x_Project.h"

void init_DMA_pingpong(volatile int16 *xn, int16 N, PINT dma_int) {

    EALLOW;
    PieVectTable.DMA_CH6_INT = dma_int;
    EDIS;

    DMAInitialize();

    volatile Uint16 *DMA_CH6_Source = (volatile Uint16 *)&McbspbRegs.DRR2.all;
    volatile Uint16 *DMA_CH6_Dest = (volatile Uint16 *)&xn[0];

    // configure DMA CH6 -> modified from TI code
    DMACH6AddrConfig(DMA_CH6_Dest, DMA_CH6_Source);
    DMACH6BurstConfig(1,1,1); // Two words per burst (L, R)
    DMACH6TransferConfig(N-1,0,1); // N bursts per transfer
    DMACH6ModeConfig(74,PERINT_ENABLE,ONESHOT_DISABLE,CONT_DISABLE,
                     SYNC_DISABLE,SYNC_SRC,OVRFLOW_DISABLE,SIXTEEN_BIT,
                     CHINT_END,CHINT_ENABLE);
    DMACH6WrapConfig(0, 0, N, 0); // Wrap source after each burst, don't wrap dest

    //something about a bandgap voltage -> stolen from TI code
    EALLOW;
    CpuSysRegs.SECMSEL.bit.PF2SEL = 1;
    EDIS;

    //interrupt enabling (need to enable global interrupts in main)
    EALLOW;
    PieCtrlRegs.PIEIER7.bit.INTx6 = 1;   // Enable PIE Group 7, INT 6 (DMA CH6)
    IER |= M_INT7;                       // Enable CPU INT6
    EDIS;
    // Need to start DMA in main
}
