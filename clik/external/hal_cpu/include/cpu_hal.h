// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _CLIK_RUNTIME_CPU_HAL_H
#define _CLIK_RUNTIME_CPU_HAL_H

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "hal.h"

using elf_program = void *;
struct exec_state;

struct cpu_barrier {
  std::mutex mutex;
  std::condition_variable entry;
  int threads_entered = 0;
  std::condition_variable exit;
  int sequence_id = 0;

  void wait(int num_threads);
};

class cpu_hal : public hal::hal_device_t {
 public:
  cpu_hal(hal::hal_device_info_t *info, std::mutex &hal_lock);
  virtual ~cpu_hal();

  size_t get_word_size() const { return sizeof(uintptr_t); }

  // find a specific kernel function in a compiled program
  // returns `hal_invalid_kernel` if no symbol could be found
  hal::hal_kernel_t program_find_kernel(hal::hal_program_t program,
                                        const char *name) override;

  // load an ELF file into target memory
  // returns `hal_invalid_program` if the program could not be loaded
  hal::hal_program_t program_load(const void *data, hal::hal_size_t size);

  // execute a kernel on the target
  bool kernel_exec(hal::hal_program_t program, hal::hal_kernel_t kernel,
                   const hal::hal_ndrange_t *nd_range,
                   const hal::hal_arg_t *args, uint32_t num_args,
                   uint32_t work_dim) override;

  // unload a program from the target
  bool program_free(hal::hal_program_t program);

  // return available target memory estimate
  hal::hal_size_t mem_avail();

  // allocate a memory range on the target
  // will return `hal_nullptr` if the operation was unsuccessful
  hal::hal_addr_t mem_alloc(hal::hal_size_t size,
                            hal::hal_size_t alignment) override;

  // copy memory between target buffers
  bool mem_copy(hal::hal_addr_t dst, hal::hal_addr_t src,
                hal::hal_size_t size) override;

  // free a memory range on the target
  bool mem_free(hal::hal_addr_t addr) override;

  // fill memory with a pattern
  bool mem_fill(hal::hal_addr_t dst, const void *pattern,
                hal::hal_size_t pattern_size, hal::hal_size_t size) override;

  // read memory from the target to the host
  bool mem_read(void *dst, hal::hal_addr_t src, hal::hal_size_t size) override;

  // write host memory to the target
  bool mem_write(hal::hal_addr_t dst, const void *src,
                 hal::hal_size_t size) override;

 private:
  bool pack_args(std::vector<uint8_t> &packed_data, const hal::hal_arg_t *args,
                 uint32_t num_args, elf_program program, uint32_t hal_flags);
  void pack_arg(std::vector<uint8_t> &packed_data, const void *value,
                size_t size, size_t align = 0);
  void pack_word_arg(std::vector<uint8_t> &packed_data, uint64_t value,
                     uint32_t hal_flags);
  void pack_uint32_arg(std::vector<uint8_t> &packed_data, uint32_t value,
                       size_t align = 0);
  void pack_uint64_arg(std::vector<uint8_t> &packed_data, uint64_t value,
                       size_t align = 0);

  void kernel_entry(exec_state *state);

  bool hal_debug() const { return debug; }

  std::mutex &hal_lock;
  hal::hal_device_info_t *info;
  bool debug = false;
  uint32_t local_mem_size = 8 << 20;
  uint8_t *local_mem = nullptr;
  cpu_barrier barrier;
  std::map<hal::hal_program_t, std::string> binary_files;
};

#endif
