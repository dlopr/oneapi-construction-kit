// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <stdio.h>

#include <vector>

#include "clik_async_api.h"
#include "option_parser.h"

// This header contains the kernel binary resulting from compiling
// 'device_vector_add.c' and turning it into a C array using the Bin2H tool.
#include "kernel_binary.h"

int main(int argc, char **argv) {
  // Process command line options.
  size_t local_size = 16;
  size_t global_size = 1024;
  option_parser_t parser;
  parser.help([]() {
    fprintf(stderr,
            "Usage: ./vector_add_async [--local-size L] [--global-size S]\n");
  });
  parser.option('L', "local-size", 1,
                [&](const char *s) { local_size = strtoull(s, 0, 0); });
  parser.option('S', "global-size", 1,
                [&](const char *s) { global_size = strtoull(s, 0, 0); });
  parser.parse(argv);
  if (local_size < 1) {
    fprintf(stderr, "error: local size must be positive\n");
    return 7;
  } else if (global_size < 1) {
    fprintf(stderr, "error: global size must be positive\n");
    return 7;
  } else if ((global_size % local_size) != 0) {
    fprintf(stderr,
            "error: global size (%zd) must be a multiple of local size"
            "(%zd)\n",
            global_size, local_size);
    return 7;
  }

  // Set up the device.
  clik_device *device = clik_create_device();
  if (!device) {
    fprintf(stderr, "Unable to create a clik device.\n");
    return 1;
  }
  clik_command_queue *queue = clik_get_device_queue(device);

  // Load the kernel program.
  clik_program *program =
      clik_create_program(device, vector_add_async_kernel_binary,
                          vector_add_async_kernel_binary_size);
  if (!program) {
    fprintf(stderr, "Unable to create a program from the kernel binary.\n");
    return 2;
  }

  // Initialize host data.
  size_t num_elements = global_size;
  std::vector<uint32_t> src1_data(num_elements);
  std::vector<uint32_t> src2_data(num_elements);
  std::vector<uint32_t> dst_data(num_elements);
  for (size_t j = 0; j < num_elements; j++) {
    src1_data[j] = j;
    src2_data[j] = 42 - j;
    dst_data[j] = ~0u;
  }

  // Create buffers in device memory.
  uint64_t buffer_size = num_elements * sizeof(uint32_t);
  clik_buffer *src1_buffer = clik_create_buffer(device, buffer_size);
  clik_buffer *src2_buffer = clik_create_buffer(device, buffer_size);
  clik_buffer *dst_buffer = clik_create_buffer(device, buffer_size);
  if (!src1_buffer || !src2_buffer || !dst_buffer) {
    fprintf(stderr, "Could not create buffers.\n");
    return 3;
  }

  // Write host data to device memory.
  if (!clik_enqueue_write_buffer(queue, src1_buffer, 0, &src1_data[0],
                                 buffer_size)) {
    fprintf(stderr, "Could not enqueue a write to the src1 buffer.\n");
    return 4;
  }
  if (!clik_enqueue_write_buffer(queue, src2_buffer, 0, &src2_data[0],
                                 buffer_size)) {
    fprintf(stderr, "Could not enqueue a write to the src2 buffer.\n");
    return 4;
  }
  if (!clik_enqueue_write_buffer(queue, dst_buffer, 0, &dst_data[0],
                                 buffer_size)) {
    fprintf(stderr, "Could not enqueue a write to the dst buffer.\n");
    return 4;
  }

  // Run the kernel.
  clik_ndrange ndrange;
  clik_init_ndrange_1d(&ndrange, num_elements, local_size);
  printf(
      "Running vector_add_async example (Global size: %zu, local size: %zu)\n",
      ndrange.global[0], ndrange.local[0]);

  const size_t num_args = 3;
  clik_argument args[num_args];
  clik_init_buffer_arg(&args[0], src1_buffer);
  clik_init_buffer_arg(&args[1], src2_buffer);
  clik_init_buffer_arg(&args[2], dst_buffer);
  clik_kernel *kernel =
      clik_create_kernel(program, "kernel_main", &ndrange, &args[0], num_args);
  if (!kernel) {
    fprintf(stderr, "Unable to create a kernel.\n");
    return 5;
  } else if (!clik_enqueue_kernel(queue, kernel)) {
    fprintf(stderr, "Could not enqueue the kernel.\n");
    return 5;
  }

  // Read the data produced by the kernel.
  if (!clik_enqueue_read_buffer(queue, &dst_data[0], dst_buffer, 0,
                                buffer_size)) {
    fprintf(stderr, "Could not read the output data from the kernel.\n");
    return 6;
  }

  // Start executing commands on the device.
  clik_dispatch(queue);

  // Wait for all commands to have finished executing on the device.
  clik_wait(queue);

  // Validate output buffer.
  bool validated = true;
  size_t num_errors = 0;
  const size_t max_print_errors = 10;
  for (size_t i = 0; i < num_elements; i++) {
    const uint32_t expected = 42;
    uint32_t actual = dst_data[i];
    if (expected != actual) {
      num_errors++;
      if (num_errors <= max_print_errors) {
        fprintf(stderr, "Result mismatch at %zu: expected %d, but got %d\n", i,
                expected, actual);
      }
      validated = false;
    }
  }
  if (validated) {
    fprintf(stderr, "Results validated successfully.\n");
  }

  clik_release_buffer(src1_buffer);
  clik_release_buffer(src2_buffer);
  clik_release_buffer(dst_buffer);
  clik_release_kernel(kernel);
  clik_release_program(program);
  clik_release_device(device);
  return validated ? 0 : -1;
}
