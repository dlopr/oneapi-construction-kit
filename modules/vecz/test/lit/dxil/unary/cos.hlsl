// Copyright (C) Codeplay Software Limited. All Rights Reserved.
// Build command: dxc -T cs_6_0 -E CSMain cos.hlsl -Fo cos.bc

// RUN: %veczc -o %t.bc %S/cos.bc -k CSMain -w 2
// RUN: %llvm-dis -o %t.ll %t.bc
// RUN: %filecheck < %t.ll %s

RWStructuredBuffer<float4x1> a;

[numthreads(128, 1, 1)]
void CSMain(uint3 gid : SV_DispatchThreadID) {
  a[gid.x] = cos(a[gid.x]);
}

// CHECK: define void @__vecz_v2_CSMain
// CHECK: call <2 x float> @llvm.cos.v2f32(<2 x float>
// CHECK: call <2 x float> @llvm.cos.v2f32(<2 x float>
// CHECK: call <2 x float> @llvm.cos.v2f32(<2 x float>
// CHECK: call <2 x float> @llvm.cos.v2f32(<2 x float>
