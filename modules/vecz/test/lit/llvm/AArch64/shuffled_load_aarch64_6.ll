; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -vecz-target-triple=aarch64-unknown-unknown -S < %s | %filecheck %s

target triple = "spir64-unknown-unknown"
target datalayout = "e-p:64:64:64-m:e-i64:64-f80:128-n8:16:32:64-S128"

define spir_kernel void @load16(i32 addrspace(1)* %out, i32 addrspace(1)* %in, i32 %stride) {
entry:
  %call = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %conv = trunc i64 %call to i32
  %call1 = tail call spir_func i64 @_Z13get_global_idj(i32 1)
  %conv2 = trunc i64 %call1 to i32
  %mul = mul nsw i32 %conv2, %stride
  %add = add nsw i32 %mul, %conv
  %mul3 = shl nsw i32 %add, 1
  %add4 = add nsw i32 %mul3, 3
  %idxprom = sext i32 %add4 to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %idxprom
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %shl = shl i32 %0, 1
  %add8 = add nsw i32 %mul3, 2
  %idxprom9 = sext i32 %add8 to i64
  %arrayidx10 = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %idxprom9
  %1 = load i32, i32 addrspace(1)* %arrayidx10, align 4
  %sub = sub nsw i32 %shl, %1
  %idxprom13 = sext i32 %add to i64
  %arrayidx14 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idxprom13
  store i32 %sub, i32 addrspace(1)* %arrayidx14, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; CHECK: define {{(dso_local )?}}spir_kernel void @load16
; CHECK: [[LOAD:%.+]] = call { <4 x i32>, <4 x i32> } @llvm.aarch64.neon.ld2.v4i32.p1
; CHECK-NOT: load <4 x i32>
; CHECK-NOT: call { <4 x i32>, <4 x i32> } @llvm.aarch64.neon.ld2
; CHECK-NOT: call <4 x i32> @__vecz_b_interleaved_load
; CHECK-NOT: call <4 x i32> @__vecz_b_gather_load
; CHECK: extractvalue { <4 x i32>, <4 x i32> } [[LOAD]], 0
; CHECK: extractvalue { <4 x i32>, <4 x i32> } [[LOAD]], 1
; CHECK-NOT: extractvalue
; CHECK-NOT: shufflevector
; CHECK: ret void
