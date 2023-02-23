; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; REQUIRES: llvm-14+

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -vecz-target-triple="riscv64-unknown-unknown" -vecz-target-features=+v -vecz-scalable -vecz-simd-width=4 -vecz-choices=VectorPredication -S < %s | %filecheck %t

target triple = "spir64-unknown-unknown"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @load_add_store(i32* %aptr, i32* %bptr, i32* %zptr) {
entry:
  %idx = call spir_func i64 @_Z13get_global_idj(i32 0)
  %arrayidxa = getelementptr inbounds i32, i32* %aptr, i64 %idx
  %arrayidxb = getelementptr inbounds i32, i32* %bptr, i64 %idx
  %arrayidxz = getelementptr inbounds i32, i32* %zptr, i64 %idx
  %a = load i32, i32* %arrayidxa, align 4
  %b = load i32, i32* %arrayidxb, align 4
  %sum = add i32 %a, %b
  store i32 %sum, i32* %arrayidxz, align 4
  ret void
}

; CHECK: define spir_kernel void @__vecz_nxv4_vp_load_add_store
; CHECK: %local.id = call i64 @__mux_get_local_id(i32 0)
; CHECK: %local.size = call i64 @__mux_get_local_size(i32 0)
; CHECK: %work.remaining = sub nuw nsw i64 %local.size, %local.id
; CHECK: %[[vli64:.+]] = call i64 @llvm.riscv.vsetvli.opt.i64(i64 %work.remaining, i64 2, i64 1)
; CHECK: %[[vl:.+]] = trunc i64 %[[vli64]] to i32
; CHECK-GE15: %[[lhs:.+]] = call <vscale x 4 x i32> @llvm.vp.load.nxv4i32.p0({{.*}}, i32 %[[vl]])
; CHECK-LT15: %[[lhs:.+]] = call <vscale x 4 x i32> @llvm.vp.load.nxv4i32.p0nxv4i32({{.*}}, i32 %[[vl]])
; CHECK-GE15: %[[rhs:.+]] = call <vscale x 4 x i32> @llvm.vp.load.nxv4i32.p0({{.*}}, i32 %[[vl]])
; CHECK-LT15: %[[rhs:.+]] = call <vscale x 4 x i32> @llvm.vp.load.nxv4i32.p0nxv4i32({{.*}}, i32 %[[vl]])
; CHECK: %[[sum:.+]] = call <vscale x 4 x i32> @llvm.vp.add.nxv4i32(<vscale x 4 x i32> %[[lhs]], <vscale x 4 x i32> %[[rhs]], {{.*}}, i32 %[[vl]])
; CHECK-GE15: call void @llvm.vp.store.nxv4i32.p0(<vscale x 4 x i32> %[[sum]], {{.*}}, i32 %[[vl]])
; CHECK-LT15: call void @llvm.vp.store.nxv4i32.p0nxv4i32(<vscale x 4 x i32> %[[sum]], {{.*}}, i32 %[[vl]])
