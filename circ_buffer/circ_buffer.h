#ifndef CIRC_BUFFER_CIRC_BUFFER_H_
#define CIRC_BUFFER_CIRC_BUFFER_H_

#include <F28x_Project.h>

// Size must be a power of 2 !!

typedef struct{
    float *buffer;
    int16 size;
    int16 w_index;
    int16 r_index;
    Uint32 w_count;
    Uint32 r_count;
    int32 wr_diff;
} cbuff;

typedef cbuff* cbuff_hnd;

void init_cbuff(cbuff_hnd self, float *buffer_, int16 size_);

// n must be less than size of buffer
void write_cbuff(cbuff_hnd self, float *write_arr, int16 n);
float read_cbuff(cbuff_hnd self);

#endif /* CIRC_BUFFER_CIRC_BUFFER_H_ */
