// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// Test Brief:
// Basic check for presence of DI entry describing kernel function.

// RUN: %oclc -cl-options '-g -cl-opt-disable' %s -enqueue getsquare -stage %stage > %t
// RUN: %filecheck < %t %s

static __constant int count = 1;
kernel void getsquare(global int *in,
                      global int *out) {
  int g_id = get_global_id(0);
  if (g_id < count)
  {
    out[g_id] = in[g_id] * in[g_id];
  }
}

// Assert the DI entry is attached to the correct IR function
// CHECK: void @__mux_host_0({{.*}} !dbg [[DI_SUBPROGRAM:![0-9]+]] {

// Assert we have debug info for the kernel
// CHECK: [[DI_SUBPROGRAM]] = distinct !DISubprogram(name: "getsquare"

// Assert kernel is associated with a file
// CHECK-SAME: file: !{{[0-9]+}}

// Assert kernel has a line number
// CHECK-SAME: line: 10

// Assert function has type opencl kernel
// CHECK-SAME: type: [[DI_SUBROUTINE_TYPE:![0-9]+]]

// Definition rather than declaration
// CHECK-SAME: spFlags: DISPFlagDefinition

// CHECK: [[DI_SUBROUTINE_TYPE]] = !DISubroutineType(cc: DW_CC_LLVM_OpenCLKernel

// Assert variables inside kernel are associated with its scope
// CHECK: !DILocalVariable(name: "in", arg: 1, scope: [[DI_SUBPROGRAM]]
// CHECK: !DILocalVariable(name: "out", arg: 2, scope: [[DI_SUBPROGRAM]]
// CHECK: !DILocalVariable(name: "g_id", scope: [[DI_SUBPROGRAM]]
