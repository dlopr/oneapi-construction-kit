// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <stdio.h>

#include "clik_sync_api.h"
#include "option_parser.h"

// This header contains the kernel binary resulting from compiling
// 'device_hello.c' and turning it into a C array using the Bin2 tool.
#include "kernel_binary.h"

int main(int argc, char **argv) {
  // Process command line options.
  uint64_t local_size = 1;
  uint64_t global_size = 8;
  option_parser_t parser;
  parser.help([]() {
    fprintf(stderr, "Usage: ./hello [--local-size N] [--global-size N]\n");
  });
  parser.option('L', "local-size", 1,
                [&](const char *s) { local_size = strtoull(s, 0, 0); });
  parser.option('S', "global-size", 1,
                [&](const char *s) { global_size = strtoull(s, 0, 0); });
  parser.parse(argv);

  clik_device *device = clik_create_device();
  if (!device) {
    fprintf(stderr, "Unable to create a clik device.\n");
    return 1;
  }

  // Load the kernel program.
  clik_program *program = clik_create_program(device, hello_kernel_binary,
                                              hello_kernel_binary_size);
  if (!program) {
    fprintf(stderr, "Unable to create a program from the kernel binary.\n");
    return 2;
  }

  // Run the kernel.
  clik_ndrange ndrange;
  clik_init_ndrange_1d(&ndrange, global_size, local_size);
  printf("Running hello example (Global size: %zu, local size: %zu)\n",
         ndrange.global[0], ndrange.local[0]);
  if (!clik_run_kernel(program, "kernel_main", &ndrange, nullptr, 0)) {
    fprintf(stderr, "Could not execute the kernel.\n");
    return 3;
  }

  clik_release_program(program);
  clik_release_device(device);
  return 0;
}
