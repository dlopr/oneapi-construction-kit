; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; Check that veczc can vectorize a kernel then vectorize the vectorized kernel,
; with base mappings from 1->2 and 2->3 and derived mappings back from 2->1 and
; 3->2.
; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k add:2 -S < %s > %t2
; RUN: %veczc -k __vecz_v2_add:4 -S < %t2 | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @add(i32 addrspace(1)* %in1, i32 addrspace(1)* %in2, i32 addrspace(1)* %out) {
entry:
  %tid = call spir_func i64 @_Z13get_global_idj(i32 0) #3
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %in1, i64 %tid
  %i1 = load i32, i32 addrspace(1)* %arrayidx, align 16
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %in2, i64 %tid
  %i2 = load i32, i32 addrspace(1)* %arrayidx1, align 16
  %add = add nsw i32 %i1, %i2
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %tid
  store i32 %add, i32 addrspace(1)* %arrayidx2, align 16
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32) #2

; CHECK-GE15: define spir_kernel void @add(ptr addrspace(1) %in1, ptr addrspace(1) %in2, ptr addrspace(1) %out){{.*}} !codeplay_ca_vecz.base ![[BASE_1:[0-9]+]]
; CHECK-LT15: define spir_kernel void @add(i32 addrspace(1)* %in1, i32 addrspace(1)* %in2, i32 addrspace(1)* %out){{.*}} !codeplay_ca_vecz.base ![[BASE_1:[0-9]+]]
; CHECK-GE15: define spir_kernel void @__vecz_v2_add(ptr addrspace(1) %in1, ptr addrspace(1) %in2, ptr addrspace(1) %out){{.*}} !codeplay_ca_vecz.base ![[BASE_2:[0-9]+]] !codeplay_ca_vecz.derived ![[DERIVED_1:[0-9]+]] {
; CHECK-LT15: define spir_kernel void @__vecz_v2_add(i32 addrspace(1)* %in1, i32 addrspace(1)* %in2, i32 addrspace(1)* %out){{.*}} !codeplay_ca_vecz.base ![[BASE_2:[0-9]+]] !codeplay_ca_vecz.derived ![[DERIVED_1:[0-9]+]] {
  ; CHECK-GE15: define spir_kernel void @__vecz_v4___vecz_v2_add(ptr addrspace(1) %in1, ptr addrspace(1) %in2, ptr addrspace(1) %out){{.*}} !codeplay_ca_vecz.derived ![[DERIVED_2:[0-9]+]] {
  ; CHECK-LT15: define spir_kernel void @__vecz_v4___vecz_v2_add(i32 addrspace(1)* %in1, i32 addrspace(1)* %in2, i32 addrspace(1)* %out){{.*}} !codeplay_ca_vecz.derived ![[DERIVED_2:[0-9]+]] {

; CHECK: ![[BASE_1]] = !{![[VMD_1:[0-9]+]], {{.*}} @__vecz_v2_add}
; CHECK: ![[VMD_1]] = !{i32 2, i32 0, i32 0, i32 0}
; CHECK: ![[BASE_2]] = !{![[VMD_2:[0-9]+]], {{.*}} @__vecz_v4___vecz_v2_add}
; CHECK: ![[VMD_2]] = !{i32 4, i32 0, i32 0, i32 0}
; CHECK: ![[DERIVED_1]] = !{![[VMD_1]], {{.*}} @add}
; CHECK: ![[DERIVED_2]] = !{![[VMD_2]], {{.*}} @__vecz_v2_add}
