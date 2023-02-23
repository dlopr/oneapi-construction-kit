; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k noreduce2 -vecz-choices=PacketizeUniformInLoops -vecz-simd-width=4 -S < %s | %filecheck %t

; ModuleID = 'kernel.opencl'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

declare spir_func i64 @_Z12get_local_idj(i32)
declare spir_func i64 @_Z13get_global_idj(i32)
declare spir_func i64 @_Z14get_local_sizej(i32)

; Function Attrs: nounwind
define spir_kernel void @reduce(i32 addrspace(3)* %in, i32 addrspace(3)* %out) {
entry:
  %call = call spir_func i64 @_Z12get_local_idj(i32 0)
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %storemerge = phi i32 [ 1, %entry ], [ %mul6, %for.inc ]
  %conv = zext i32 %storemerge to i64
  %call1 = call spir_func i64 @_Z14get_local_sizej(i32 0)
  %cmp = icmp ult i64 %conv, %call1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %mul = mul i32 %storemerge, 3
  %conv3 = zext i32 %mul to i64
  %0 = icmp eq i32 %mul, 0
  %1 = select i1 %0, i64 1, i64 %conv3
  %rem = urem i64 %call, %1
  %cmp4 = icmp eq i64 %rem, 0
  br i1 %cmp4, label %if.then, label %for.inc

if.then:                                          ; preds = %for.body
  %arrayidx = getelementptr inbounds i32, i32 addrspace(3)* %out, i64 %call
  store i32 5, i32 addrspace(3)* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body, %if.then
  %mul6 = shl i32 %storemerge, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}

; Function Attrs: nounwind
define spir_kernel void @noreduce(i32 addrspace(3)* %in, i32 addrspace(3)* %out) {
entry:
  %call = call spir_func i64 @_Z12get_local_idj(i32 0)
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %storemerge = phi i32 [ 1, %entry ], [ %mul, %for.inc ]
  %conv = zext i32 %storemerge to i64
  %call1 = call spir_func i64 @_Z14get_local_sizej(i32 0)
  %cmp = icmp ult i64 %conv, %call1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %0 = icmp eq i32 %storemerge, 0
  %1 = select i1 %0, i32 1, i32 %storemerge
  %rem = urem i32 3, %1
  %cmp3 = icmp eq i32 %rem, 0
  br i1 %cmp3, label %if.then, label %for.inc

if.then:                                          ; preds = %for.body
  %arrayidx = getelementptr inbounds i32, i32 addrspace(3)* %out, i64 %call
  store i32 5, i32 addrspace(3)* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body, %if.then
  %mul = shl i32 %storemerge, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}

; Function Attrs: nounwind
define spir_kernel void @noreduce2(i32 addrspace(3)* %in, i32 addrspace(3)* %out) {
entry:
  %call = call spir_func i64 @_Z12get_local_idj(i32 0)
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %storemerge = phi i32 [ 1, %entry ], [ %mul, %for.inc ]
  %conv = zext i32 %storemerge to i64
  %call1 = call spir_func i64 @_Z14get_local_sizej(i32 0)
  %cmp = icmp ult i64 %conv, %call1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %0 = icmp eq i32 %storemerge, 0
  %1 = select i1 %0, i32 1, i32 %storemerge
  %rem = urem i32 3, %1
  %cmp3 = icmp eq i32 %rem, 0
  br i1 %cmp3, label %if.then, label %for.inc

if.then:                                          ; preds = %for.body
  %idxprom = zext i32 %storemerge to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(3)* %out, i64 %idxprom
  store i32 5, i32 addrspace(3)* %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body, %if.then
  %mul = shl i32 %storemerge, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}

; Function Attrs: nounwind
define spir_kernel void @conditional(i32 addrspace(1)* %in, i32 addrspace(1)* %out) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #3
  %0 = load i32, i32 addrspace(1)* %in, align 4
  %rem1 = and i32 %0, 1
  %tobool = icmp eq i32 %rem1, 0
  br i1 %tobool, label %if.end, label %if.then

if.then:                                          ; preds = %entry
  %idxprom = sext i32 %0 to i64
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %in, i64 %idxprom
  %1 = load i32, i32 addrspace(1)* %arrayidx1, align 4
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %call
  store i32 %1, i32 addrspace(1)* %arrayidx2, align 4
  br label %if.end

if.end:                                           ; preds = %entry, %if.then
  ret void
}

; This test checks if the "packetize uniform in loops" Vecz choice works on
; uniform values used by varying values in loops, but not on uniform values used
; by other uniform values only.

; CHECK-GE15: define spir_kernel void @__vecz_v4_noreduce2(ptr addrspace(3) %in, ptr addrspace(3) %out)
; CHECK-LT15: define spir_kernel void @__vecz_v4_noreduce2(i32 addrspace(3)* %in, i32 addrspace(3)* %out)
; CHECK: icmp ugt i64
; CHECK: phi i32
; CHECK: icmp eq i32
; CHECK-GE15: and i32{{.*}}, 3
; CHECK-LT15: urem i32 3
; CHECK: icmp eq i32
; CHECK: store i32 5
; CHECK: shl i32 %{{.+}}, 1
; CHECK: ret void
