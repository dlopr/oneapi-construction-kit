// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _REFSIDRV_KERNEL_IF_H
#define _REFSIDRV_KERNEL_IF_H

#include "device_if.h"

typedef uint32_t uint;

#define __kernel

#define __global
#define __constant
#define __local
// This can only be used on variable declarations, whether at function level
// or in the global scope.
#define __local_variable static __attribute__((section(".local")))
#define __private

int print(exec_state_t *e, const char *fmt, ...);
void barrier(struct exec_state *state) __attribute__((noinline));
uintptr_t start_dma(void *dst, const void *src, size_t size_in_bytes,
                    struct exec_state *state);
void wait_dma(uintptr_t xfer_id, struct exec_state *state);

// Retrieve a pointer to the current hart's execution context.
inline exec_state_t *get_context(wg_info_t *wg) { return (exec_state_t *)wg; }

inline uint32_t get_work_dim(exec_state_t *e) { return e->wg.num_dim; }

inline uint32_t get_global_id(uint32_t rank, exec_state_t *e) {
  return (e->wg.group_id[rank] * e->wg.local_size[rank]) + e->local_id[rank] +
         e->wg.global_offset[rank];
}

inline uint32_t get_local_id(uint32_t rank, exec_state_t *e) {
  return e->local_id[rank];
}

inline uint32_t get_group_id(uint32_t rank, exec_state_t *e) {
  return e->wg.group_id[rank];
}

inline uint32_t get_global_offset(uint32_t rank, exec_state_t *e) {
  return e->wg.global_offset[rank];
}

inline uint32_t get_local_size(uint32_t rank, exec_state_t *e) {
  return e->wg.local_size[rank];
}

inline uint32_t get_global_size(uint32_t rank, exec_state_t *e) {
  return e->wg.local_size[rank] * e->wg.num_groups[rank];
}

#endif  // _REFSIDRV_KERNEL_IF_H
