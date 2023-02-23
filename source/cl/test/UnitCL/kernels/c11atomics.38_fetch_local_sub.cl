// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void fetch_local_sub(__global TYPE *input_buffer,
                              __global TYPE *output_buffer,
                              __global TYPE *initial_values,
                              volatile __local ATOMIC_TYPE *local_buffer) {
  uint gid = get_global_id(0);
  uint lid = get_local_id(0);
  uint wgid = get_group_id(0);

  if (lid == 0) {
    atomic_init(local_buffer + lid, initial_values[wgid]);
  }
  work_group_barrier(CLK_LOCAL_MEM_FENCE);
  atomic_fetch_sub_explicit(local_buffer, input_buffer[gid],
                            memory_order_relaxed, memory_scope_work_item);
  work_group_barrier(CLK_LOCAL_MEM_FENCE);
  if (lid == 0) {
    output_buffer[wgid] = atomic_load_explicit(
        local_buffer, memory_order_relaxed, memory_scope_work_item);
  }
}
