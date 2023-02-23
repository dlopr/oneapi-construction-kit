; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k multiple_exit_blocks -vecz-passes="function(simplifycfg,dce),mergereturn,cfg-convert" -S < %s | %filecheck %s

; ModuleID = 'Unknown buffer'
source_filename = "Unknown buffer"
target datalayout = "e-m:e-i64:64-f80:128-n8:1:32:64-S128"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind readnone
declare spir_func i64 @_Z12get_local_idj(i32)
declare spir_func i64 @_Z13get_global_idj(i32)

define spir_kernel void @multiple_exit_blocks(i64 %n) {
entry:
  %gid = tail call spir_func i64 @_Z13get_global_idj(i32 0)
  %lid = tail call spir_func i64 @_Z12get_local_idj(i32 0)
  %cmp1 = icmp slt i64 %lid, %n
  %cmp2 = icmp slt i64 %gid, %n
  br i1 %cmp2, label %if.then, label %if.end

if.then:                                             ; preds = %entry
  %cmp3 = and i1 %cmp1, %cmp2
  br i1 %cmp3, label %if.then2, label %if.else2

if.then2:                                             ; preds = %if.then
  br label %if.else2

if.else2:                                             ; preds = %if.then, %if.then2
  br i1 %cmp1, label %if.then3, label %if.end

if.then3:                                             ; preds = %if.else2
  br label %if.end

if.end:                                             ; preds = %entry, %if.else2, %if.then3
  ret void
}

; The purpose of this test is to make sure we do not have a kernel that has more
; than one exit block after following the preparation pass.

; CHECK: define spir_kernel void @__vecz_v4_multiple_exit_blocks

; We don't want to generate any ROSCC branches:
; CHECK-NOT: entry.ROSCC:

; Only one return statement:
; CHECK: ret void
; CHECK-NOT: ret void
