// Copyright (C) Codeplay Software Limited. All Rights Reserved.

// RUN: %oclc -execute -enqueue vector_addition_scalar_int -arg src1,1,2,3,4 -arg src2,10,20,30,40 -arg scalar,2 -compare dst,13,24,35,46 -global 4 -local 4 %s > %t
// RUN: %filecheck < %t %s

__kernel void vector_addition_scalar_int(int scalar, __global int *src1, __global int*src2,
 __global int *dst) {
	size_t gid = get_global_id(0);
	dst[gid] = scalar + src1[gid] + src2[gid];
}

// CHECK: dst - match
