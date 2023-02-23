// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// REQUIRES: parameters
__kernel void compare_exchange_weak_global_local(
    volatile __global ATOMIC_TYPE *atomic_buffer,
    __global TYPE *expected_buffer, __global TYPE *desired_buffer,
    int __global *bool_output_buffer, __local TYPE *local_expected_buffer) {
  int gid = get_global_id(0);
  int lid = get_local_id(0);

  local_expected_buffer[lid] = expected_buffer[gid];

  bool_output_buffer[gid] = atomic_compare_exchange_weak_explicit(
      atomic_buffer + gid, local_expected_buffer + lid, desired_buffer[gid],
      memory_order_relaxed, memory_order_relaxed, memory_scope_work_item);

  expected_buffer[gid] = local_expected_buffer[lid];
}
