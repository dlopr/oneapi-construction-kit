; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %pp-llvm-ver -o %t < %s --llvm-ver %LLVMVER
; RUN: %veczc -k k_controlflow_loop_if -S < %s | %filecheck %t

; ModuleID = 'test.cl'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind uwtable
define void @k_controlflow_loop_if(float* nocapture %out, float* nocapture readonly %in1, i32* nocapture readnone %in2) #0 {
entry:
  %call = tail call i64 @get_global_id(i32 0) #2
  %sext = shl i64 %call, 32
  %idxprom = ashr exact i64 %sext, 32
  %arrayidx = getelementptr inbounds float, float* %in1, i64 %idxprom
  %0 = bitcast float* %arrayidx to i32*
  %1 = load i32, i32* %0, align 4, !tbaa !7
  %arrayidx2 = getelementptr inbounds float, float* %out, i64 %idxprom
  %2 = bitcast float* %arrayidx2 to i32*
  store i32 %1, i32* %2, align 4, !tbaa !7
  ret void
}

declare i64 @get_global_id(i32) #1

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nobuiltin nounwind }

!opencl.kernels = !{!0}
!llvm.ident = !{!6}

!0 = !{void (float*, float*, i32*)* @k_controlflow_loop_if, !1, !2, !3, !4, !5}
!1 = !{!"kernel_arg_addr_space", i32 0, i32 0, i32 0}
!2 = !{!"kernel_arg_access_qual", !"none", !"none", !"none"}
!3 = !{!"kernel_arg_type", !"float*", !"float*", !"int*"}
!4 = !{!"kernel_arg_base_type", !"float*", !"float*", !"int*"}
!5 = !{!"kernel_arg_type_qual", !"", !"", !""}
!6 = !{!"clang version 3.8.0 "}
!7 = !{!8, !8, i64 0}
!8 = !{!"float", !9, i64 0}
!9 = !{!"omnipotent char", !10, i64 0}
!10 = !{!"Simple C/C++ TBAA"}

; The vectorized function
; CHECK: define void @__vecz_v[[WIDTH:[0-9]+]]_k_controlflow_loop_if(

; The unmangled get_global_id call
; CHECK: tail call i64 @get_global_id(i32 0)

; The vectorized loads and stores
; CHECK-GE15: load <4 x i32>, ptr %arrayidx, align 4
; CHECK-LT15: load <4 x i32>, <4 x i32>* %0, align 4
; CHECK-GE15: store <4 x i32> %0, ptr %arrayidx2, align 4
; CHECK-LT15: store <4 x i32> %1, <4 x i32>* %2, align 4
