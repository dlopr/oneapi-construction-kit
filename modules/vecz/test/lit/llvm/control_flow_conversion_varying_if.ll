; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k test_varying_if -vecz-passes=cfg-convert -vecz-simd-width=4 -S < %s | %filecheck %t

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_uniform_if(i32 %a, i32* %b) {
entry:
  %cmp = icmp eq i32 %a, 1
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %idxprom = sext i32 %a to i64
  %arrayidx = getelementptr inbounds i32, i32* %b, i64 %idxprom
  store i32 11, i32* %arrayidx, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  %arrayidx1 = getelementptr inbounds i32, i32* %b, i64 42
  store i32 13, i32* %arrayidx1, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  ret void
}

define spir_kernel void @test_varying_if(i32 %a, i32* %b) {
entry:
  %conv = sext i32 %a to i64
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %cmp = icmp eq i64 %conv, %call
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %idxprom = sext i32 %a to i64
  %arrayidx = getelementptr inbounds i32, i32* %b, i64 %idxprom
  store i32 11, i32* %arrayidx, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  %arrayidx2 = getelementptr inbounds i32, i32* %b, i64 42
  store i32 13, i32* %arrayidx2, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  ret void
}

define spir_kernel void @test_uniform_loop(i32 %a, i32* %b)  {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %conv = trunc i64 %call to i32
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  %storemerge = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %cmp = icmp slt i32 %storemerge, 16
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = add nsw i32 %storemerge, %a
  %add2 = add nsw i32 %storemerge, %conv
  %idxprom = sext i32 %add2 to i64
  %arrayidx = getelementptr inbounds i32, i32* %b, i64 %idxprom
  store i32 %add, i32* %arrayidx, align 4
  %inc = add nsw i32 %storemerge, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}

define spir_kernel void @test_varying_loop(i32 %a, i32* %b) {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %conv = trunc i64 %call to i32
  %sub = sub nsw i32 16, %conv
  br label %for.cond

for.cond:                                         ; preds = %for.body, %entry
  %storemerge = phi i32 [ %sub, %entry ], [ %inc, %for.body ]
  %cmp = icmp slt i32 %storemerge, 16
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = add nsw i32 %storemerge, %a
  %add2 = add nsw i32 %storemerge, %conv
  %idxprom = sext i32 %add2 to i64
  %arrayidx = getelementptr inbounds i32, i32* %b, i64 %idxprom
  store i32 %add, i32* %arrayidx, align 4
  %inc = add nsw i32 %storemerge, 1
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}

define spir_kernel void @test_nested_loops(i32* %a, i32* %b)  {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0)
  %conv = trunc i64 %call to i32
  %sub = sub nsw i32 16, %conv
  br label %for.cond

for.cond:                                         ; preds = %for.inc12, %entry
  %storemerge = phi i32 [ %sub, %entry ], [ %inc13, %for.inc12 ]
  %cmp = icmp slt i32 %storemerge, 16
  br i1 %cmp, label %for.body, label %for.end14

for.body:                                         ; preds = %for.cond
  %sub2 = sub nsw i32 24, %conv
  br label %for.cond3

for.cond3:                                        ; preds = %for.body6, %for.body
  %storemerge1 = phi i32 [ %sub2, %for.body ], [ %inc, %for.body6 ]
  %cmp4 = icmp slt i32 %storemerge, 24
  br i1 %cmp4, label %for.body6, label %for.inc12

for.body6:                                        ; preds = %for.cond3
  %add = add nsw i32 %storemerge1, %conv
  %idxprom = sext i32 %add to i64
  %arrayidx = getelementptr inbounds i32, i32* %a, i64 %idxprom
  %0 = load i32, i32* %arrayidx, align 4
  %add7 = add i32 %storemerge1, %storemerge
  %add8 = add i32 %add7, %0
  %add9 = add nsw i32 %storemerge, %conv
  %idxprom10 = sext i32 %add9 to i64
  %arrayidx11 = getelementptr inbounds i32, i32* %b, i64 %idxprom10
  store i32 %add8, i32* %arrayidx11, align 4
  %inc = add nsw i32 %storemerge1, 1
  br label %for.cond3

for.inc12:                                        ; preds = %for.cond3
  %inc13 = add nsw i32 %storemerge, 1
  br label %for.cond

for.end14:                                        ; preds = %for.cond
  ret void
}

declare spir_func i64 @_Z13get_global_idj(i32)

; Check for a varying that needs masked operations
; CHECK-GE15: define spir_kernel void @__vecz_v4_test_varying_if(i32 %a, ptr %b)
; CHECK-LT15: define spir_kernel void @__vecz_v4_test_varying_if(i32 %a, i32* %b)
; CHECK: %cmp = icmp eq i64 %conv, %call
; CHECK: %cmp.not = xor i1 %cmp, true
; CHECK-GE15: call void @__vecz_b_masked_store4_ju3ptrb(i32 11, ptr %arrayidx, i1 %cmp)
; CHECK-LT15: call void @__vecz_b_masked_store4_jPjb(i32 11, i32* %arrayidx, i1 %cmp)
; CHECK-GE15: call void @__vecz_b_masked_store4_ju3ptrb(i32 13, ptr %arrayidx2, i1 %cmp.not)
; CHECK-LT15: call void @__vecz_b_masked_store4_jPjb(i32 13, i32* %arrayidx2, i1 %cmp.not)

; Note that the entry mask would be removed by any DCE pass
; CHECK: ret void
