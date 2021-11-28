#include "circ_buffer.h"
#include "../TI_FFT/fpu_vector.h"

void init_cbuff(cbuff_hnd self, float *buffer_, int16 size_) {
    self->buffer = buffer_;
    self->size = size_;
    self->w_index = 0;
    self->r_index = 0;
    self->w_count = 0;
    self->r_count = 0;
    self->wr_diff = 0;
}

void write_cbuff(cbuff_hnd self, float *write_arr, int16 n) {
    if (n > self->size)
        return;
    //if (self->w_index + n > self->r_index)
    //    return;
    int16 size_no_wrap = self->size - self->w_index;
    float *wr_ptr = &(self->buffer[self->w_index]);
    if (n <= size_no_wrap)
        memcpy_fast(wr_ptr, write_arr, n*sizeof(float));
    else {
        int16 size_wrap = n - size_no_wrap;
        memcpy_fast(wr_ptr, write_arr, size_no_wrap*sizeof(float));
        memcpy_fast(self->buffer, &write_arr[size_no_wrap], size_wrap*sizeof(float));
    }
    self->w_index = (self->w_index + n) & (self->size - 1);
    self->w_count += n;
    self->wr_diff += n;
}

float read_cbuff(cbuff_hnd self) {
    if (self->r_count >= self->w_count)
        return 0;
    float val = self->buffer[self->r_index];
    self->r_index = (self->r_index + 1) & (self->size - 1);
    self->r_count += 1;
    self->wr_diff -= 1;
    return val;
}
