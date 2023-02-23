// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <host/buffer.h>
#include <host/device.h>
#include <host/host.h>
#include <host/memory.h>
#include <mux/utils/allocator.h>

namespace host {
buffer_s::buffer_s(mux_memory_requirements_s memory_requirements)
    : data(nullptr) {
  this->memory_requirements = memory_requirements;
}
}  // namespace host

mux_result_t hostCreateBuffer(mux_device_t device, size_t size,
                              mux_allocator_info_t allocator_info,
                              mux_buffer_t *out_buffer) {
  (void)device;
  mux::allocator allocator(allocator_info);

  // TODO: Also report host::memory_s::HEAP_ANY
  mux_memory_requirements_s memory_requirements = {size, 16,
                                                   host::memory_s::HEAP_BUFFER};

  auto buffer = allocator.create<host::buffer_s>(memory_requirements);
  if (nullptr == buffer) {
    return mux_error_out_of_memory;
  }

  *out_buffer = buffer;

  return mux_success;
}

void hostDestroyBuffer(mux_device_t device, mux_buffer_t buffer,
                       mux_allocator_info_t allocator_info) {
  (void)device;
  mux::allocator allocator(allocator_info);

  allocator.destroy(static_cast<host::buffer_s *>(buffer));
}

mux_result_t hostBindBufferMemory(mux_device_t device, mux_memory_t memory,
                                  mux_buffer_t buffer, uint64_t offset) {
  (void)device;

  auto hostMemory = static_cast<host::memory_s *>(memory);

  auto hostBuffer = static_cast<host::buffer_s *>(buffer);

  hostBuffer->data = static_cast<unsigned char *>(hostMemory->data) + offset;

  return mux_success;
}
