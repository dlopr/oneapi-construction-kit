; Copyright (C) Codeplay Software Limited. All Rights Reserved.

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir-unknown-unknown"

declare i32 @__core_get_global_id(i32)

define spir_kernel void @kernel_main(i32 addrspace(1)* %a, i32 addrspace(1)* %b) {
entry:
  %gid = call i32 @__core_get_global_id(i32 0)
  %agep = getelementptr i32, i32 addrspace(1)* %a, i32 %gid
  %bgep = getelementptr i32, i32 addrspace(1)* %b, i32 %gid
  %load = load i32, i32 addrspace(1)* %bgep
  store i32 %load, i32 addrspace(1)* %agep
  ret void
}
