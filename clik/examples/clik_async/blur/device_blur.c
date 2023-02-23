// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "device_blur.h"

__kernel void copy_and_pad_hor(__global uint *src, __global uint *dst,
                               exec_state_t *item) {
  // Figure out array slice to copy
  uint tid = get_global_id(0, item);
  uint src_idx = WIDTH * tid;
  uint src_idx_end = src_idx + WIDTH;
  uint dst_idx = EXTENDED_WIDTH * (tid + 1);

  // Duplicate the first element
  dst[dst_idx++] = src[src_idx];

  // Copy over the middle
  while (src_idx < src_idx_end) {
    dst[dst_idx++] = src[src_idx++];
  }

  // Duplicate the last element
  dst[dst_idx] = src[src_idx - 1];
}

__kernel void pad_vert(__global uint *dst, exec_state_t *item) {
  uint tid = get_global_id(0, item);
  uint gap = HEIGHT * EXTENDED_WIDTH;

  // Duplicate the topmost and the bottommost pixel.
  dst[tid] = dst[tid + EXTENDED_WIDTH];
  dst[tid + gap + EXTENDED_WIDTH] = dst[tid + gap];
}

__kernel void blur(__global uint *src, __global uint *dst, exec_state_t *item) {
  uint x = get_global_id(0, item);
  uint y = get_global_id(1, item);

  // Calculate row starting indices.
  uint base0 = EXTENDED_WIDTH * y + x;
  uint base1 = EXTENDED_WIDTH + base0;
  uint base2 = EXTENDED_WIDTH + base1;

  // Add together 9 pixels around destination.
  uint total = src[base0] + src[base0 + 1] + src[base0 + 2] + src[base1] +
               src[base1 + 1] + src[base1 + 2] + src[base2] + src[base2 + 1] +
               src[base2 + 2];

  // Store the mean of the 9 pixeld into the destination buffer.
  dst[WIDTH * y + x] = total / 9;
}
