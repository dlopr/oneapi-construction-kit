; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k masked_gather -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @masked_scatter(i32 addrspace(1)* %a, i32 addrspace(1)* %b, i32 addrspace(1)* %b_index) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %rem = urem i64 %call, 3
  %cmp = icmp eq i64 %rem, 0
  br i1 %cmp, label %if.else, label %if.then

if.then:                                          ; preds = %entry
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %call
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %b_index, i64 %call
  %1 = load i32, i32 addrspace(1)* %arrayidx1, align 4
  %idxprom = sext i32 %1 to i64
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %b, i64 %idxprom
  store i32 %0, i32 addrspace(1)* %arrayidx2, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  %arrayidx3 = getelementptr inbounds i32, i32 addrspace(1)* %b_index, i64 %call
  %2 = load i32, i32 addrspace(1)* %arrayidx3, align 4
  %idxprom4 = sext i32 %2 to i64
  %arrayidx5 = getelementptr inbounds i32, i32 addrspace(1)* %b, i64 %idxprom4
  store i32 42, i32 addrspace(1)* %arrayidx5, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  ret void
}

define spir_kernel void @masked_gather(i32 addrspace(1)* %a, i32 addrspace(1)* %a_index, i32 addrspace(1)* %b) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %rem = urem i64 %call, 3
  %cmp = icmp eq i64 %rem, 0
  br i1 %cmp, label %if.else, label %if.then

if.then:                                          ; preds = %entry
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %a_index, i64 %call
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %idxprom = sext i32 %0 to i64
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %idxprom
  %1 = load i32, i32 addrspace(1)* %arrayidx1, align 4
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %b, i64 %call
  store i32 %1, i32 addrspace(1)* %arrayidx2, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  %arrayidx3 = getelementptr inbounds i32, i32 addrspace(1)* %b, i64 %call
  store i32 42, i32 addrspace(1)* %arrayidx3, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; Test if the masked gather load is defined correctly
; CHECK-GE15: define <4 x i32> @__vecz_b_masked_gather_load4_Dv4_jDv4_u3ptrU3AS1Dv4_b(<4 x ptr addrspace(1)>{{( %0)?}}, <4 x i1>{{( %1)?}})
; CHECK-LT15: define <4 x i32> @__vecz_b_masked_gather_load4_Dv4_jDv4_PU3AS1jDv4_b(<4 x i32 addrspace(1)*>{{( %0)?}}, <4 x i1>{{( %1)?}})
; CHECK: entry:
; CHECK-GE15: %2 = call <4 x i32> @llvm.masked.gather.v4i32.v4p1(<4 x ptr addrspace(1)> %0, i32{{( immarg)?}} 4, <4 x i1> %1, <4 x i32> undef)
; CHECK-LT15: %2 = call <4 x i32> @llvm.masked.gather.v4i32.v4p1i32(<4 x i32 addrspace(1)*> %0, i32{{( immarg)?}} 4, <4 x i1> %1, <4 x i32> undef)
; CHECK: ret <4 x i32> %2
