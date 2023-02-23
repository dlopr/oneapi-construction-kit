// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// This kernel is a clone of Task_13_01_magic_square using shuffled order of
// get_global_id index. get_global_size is adjusted accordingly.
__kernel void magic_square(const __global float* src, __global float *tmp,
                           __global float *dst) {
  uint x = get_global_id(2);
  uint y = get_global_id(1);
  uint z = get_global_id(0);

  const size_t size1 = get_global_size(1);
  const size_t size2 = get_global_size(0);

  // distributed sum of 16 elements over z dimension
  size_t index_tmp = x * size2 * size1 + y * size2 + z;
  for(uint i = 0; i < 4; i++) {
    // i * 4 + z % 4 stays within range [0; 15]
    size_t index_in = x * size2 * size1 + y * size2 + i * 4 + z % 4;
    tmp[index_tmp] += src[index_in];
  }

  barrier(CLK_GLOBAL_MEM_FENCE);
  // reduce
  if(z == 0) {
    float accumulate = 0;
    for (uint k = 0; k < size2 / 4; k++) {
      size_t index_tmp_2 = x * size2 * size1 + y * size2 + k;
      accumulate += tmp[index_tmp_2];
    }
    size_t index_out = x * size1 + y;
    dst[index_out] = accumulate;
  }
  return;
}
