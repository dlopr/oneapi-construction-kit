// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This test is the same as dma.09_auto_dma_loop_convolution_cond_not_global_id
// except the loop rotation done by clang-level -O1 optimizations
// has been manually applied using do..while loops.

// This one should succeed the DMA as the conditional is common across the group
__kernel void auto_dma_loop_convolution_cond_not_global_id_looprotate(
    __global uint *src, __global uint *dst, uint extraParam) {
  size_t dstYStride = get_global_size(0);
  size_t dstIndex = get_global_id(1) * dstYStride + get_global_id(0);
  size_t srcYStride = dstYStride + 16;
  size_t srcIndex = get_global_id(1) * srcYStride + get_global_id(0) + 8;
  srcIndex += srcYStride;
  uint count = 9;
  if (extraParam > 1) {
    count += 10;
  }
  size_t y = 0;
  do {
    size_t x = 0;
    do {
      count = count + src[srcYStride * y + srcIndex + x - 1];
    } while (++x < 3);
  } while (++y < 3);
  // Write data out
  dst[dstIndex] = count / 9;
}
