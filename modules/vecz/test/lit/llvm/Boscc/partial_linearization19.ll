; Copyright (C) Codeplay Software Limited. All Rights Reserved.

; RUN: %veczc -k partial_linearization19 -vecz-passes="function(simplifycfg),mergereturn,vecz-loop-rotate,cfg-convert,cleanup-divergence" -vecz-choices=LinearizeBOSCC -S < %s | %filecheck %s

; The CFG of the following kernel is:
;
;       a
;       |
;       b <----.
;      / \     |
;     c   \    |
;    / \   \   |
;   d   e   f -'
;   |   |   |
;    \  \   g
;     \  \ / \
;      \  h   i <,
;       \  \ /  /
;        \  j  /
;         \   /
;          `-'
;
; * where nodes b, c, and g are uniform branches, and node f is a varying
;   branch.
; * where nodes g, h, i and j are divergent.
;
; With BOSCC, it will be transformed as follows:
;
;       a
;       |
;       b <----. .---> b' <----.
;      / \     | |    / \      |
;     c   \    | |   c'  \     |
;    / \   \   | |  / \   \    |
;   d   e   f -' | d'  e'  f' -'
;   |   |   |\___' |   |   |
;    \  \   g       \  |  /
;     \  \ / \       \ | /
;      \  h   i <,    \|/
;       \  \ /  /      g'
;        \  j  /       |
;         \ | /        i'
;          `-'         |
;           |          h'
;           |          |
;            `--> & <- j'
;
; where '&' represents merge blocks of BOSCC regions.
;
; The uniform branch `g` has been linearized because both its successors are
; divergent. Not linearizing `g`  would mean that only one of both
; successors could be executed in addition to the other, pending a uniform
; condition evaluates to true, whereas what we want is to possibly execute both
; no matter what the uniform condition evaluates to.
;
; __kernel void partial_linearization19(__global int *out, int n) {
;   int id = get_global_id(0);
;   int ret = 0;
;   int i = 0;
;
;   while (1) {
;     if (n > 5) {
;       if (n == 6) {
;         goto d;
;       } else {
;         goto e;
;       }
;     }
;     if (++i + id > 3) {
;       break;
;     }
;   }
;
;   // g
;   if (n == 3) {
;     goto h;
;   } else {
;     goto i;
;   }
;
; d:
;   for (int i = 0; i < n + 5; i++) ret += 2;
;   goto i;
;
; e:
;   for (int i = 1; i < n * 2; i++) ret += i;
;   goto h;
;
; i:
;   for (int i = 0; i < n + 5; i++) ret++;
;   goto j;
;
; h:
;   for (int i = 0; i < n; i++) ret++;
;   goto j;
;
; j:
;   out[id] = ret;
; }

; ModuleID = 'Unknown buffer'
source_filename = "kernel.opencl"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @partial_linearization19(i32 addrspace(1)* %out, i32 %n) #0 {
entry:
  %call = call spir_func i64 @_Z13get_global_idj(i32 0) #2
  %conv = trunc i64 %call to i32
  br label %while.body

while.body:                                       ; preds = %if.end, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %if.end ]
  %cmp = icmp sgt i32 %n, 5
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %while.body
  %cmp2 = icmp eq i32 %n, 6
  br i1 %cmp2, label %for.cond, label %for.cond20

if.end:                                           ; preds = %while.body
  %inc = add nuw nsw i32 %i.0, 1
  %add = add nsw i32 %inc, %conv
  %cmp5 = icmp sgt i32 %add, 3
  br i1 %cmp5, label %while.end, label %while.body

while.end:                                        ; preds = %if.end
  %cmp9 = icmp eq i32 %n, 3
  br i1 %cmp9, label %h, label %i28

for.cond:                                         ; preds = %for.body, %if.then
  %ret.0 = phi i32 [ %add17, %for.body ], [ 0, %if.then ]
  %storemerge3 = phi i32 [ %inc18, %for.body ], [ 0, %if.then ]
  %add14 = add nsw i32 %n, 5
  %cmp15 = icmp slt i32 %storemerge3, %add14
  br i1 %cmp15, label %for.body, label %i28

for.body:                                         ; preds = %for.cond
  %add17 = add nuw nsw i32 %ret.0, 2
  %inc18 = add nuw nsw i32 %storemerge3, 1
  br label %for.cond

for.cond20:                                       ; preds = %for.body23, %if.then
  %ret.1 = phi i32 [ %add24, %for.body23 ], [ 0, %if.then ]
  %storemerge2 = phi i32 [ %inc26, %for.body23 ], [ 1, %if.then ]
  %mul = shl nsw i32 %n, 1
  %cmp21 = icmp slt i32 %storemerge2, %mul
  br i1 %cmp21, label %for.body23, label %h

for.body23:                                       ; preds = %for.cond20
  %add24 = add nuw nsw i32 %storemerge2, %ret.1
  %inc26 = add nuw nsw i32 %storemerge2, 1
  br label %for.cond20

i28:                                              ; preds = %for.cond, %while.end
  %ret.2 = phi i32 [ 0, %while.end ], [ %ret.0, %for.cond ]
  br label %for.cond30

for.cond30:                                       ; preds = %for.body34, %i28
  %ret.3 = phi i32 [ %ret.2, %i28 ], [ %inc35, %for.body34 ]
  %storemerge = phi i32 [ 0, %i28 ], [ %inc37, %for.body34 ]
  %add31 = add nsw i32 %n, 5
  %cmp32 = icmp slt i32 %storemerge, %add31
  br i1 %cmp32, label %for.body34, label %j

for.body34:                                       ; preds = %for.cond30
  %inc35 = add nuw nsw i32 %ret.3, 1
  %inc37 = add nuw nsw i32 %storemerge, 1
  br label %for.cond30

h:                                                ; preds = %for.cond20, %while.end
  %ret.4 = phi i32 [ 0, %while.end ], [ %ret.1, %for.cond20 ]
  br label %for.cond40

for.cond40:                                       ; preds = %for.body43, %h
  %ret.5 = phi i32 [ %ret.4, %h ], [ %inc44, %for.body43 ]
  %storemerge1 = phi i32 [ 0, %h ], [ %inc46, %for.body43 ]
  %cmp41 = icmp slt i32 %storemerge1, %n
  br i1 %cmp41, label %for.body43, label %j

for.body43:                                       ; preds = %for.cond40
  %inc44 = add nsw i32 %ret.5, 1
  %inc46 = add nuw nsw i32 %storemerge1, 1
  br label %for.cond40

j:                                                ; preds = %for.cond40, %for.cond30
  %ret.6 = phi i32 [ %ret.3, %for.cond30 ], [ %ret.5, %for.cond40 ]
  %idxprom = sext i32 %conv to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %idxprom
  store i32 %ret.6, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: convergent nounwind readonly
declare spir_func i64 @_Z13get_global_idj(i32) #1

attributes #0 = { convergent nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="0" "stackrealign" "uniform-work-group-size"="true" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { convergent nounwind readonly "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="0" "stackrealign" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { convergent nobuiltin nounwind readonly }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!opencl.kernels = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}
!2 = !{void (i32 addrspace(1)*, i32)* @partial_linearization19, !3, !4, !5, !6, !7, !8}
!3 = !{!"kernel_arg_addr_space", i32 1, i32 0}
!4 = !{!"kernel_arg_access_qual", !"none", !"none"}
!5 = !{!"kernel_arg_type", !"int*", !"int"}
!6 = !{!"kernel_arg_base_type", !"int*", !"int"}
!7 = !{!"kernel_arg_type_qual", !"", !""}
!8 = !{!"kernel_arg_name", !"out", !"n"}

; CHECK: spir_kernel void @__vecz_v4_partial_linearization19
; CHECK: br i1 true, label %[[WHILEBODYUNIFORM:.+]], label %[[WHILEBODY:.+]]

; CHECK: [[WHILEBODY]]:
; CHECK: %[[CMP:.+]] = icmp
; CHECK: br i1 %[[CMP]], label %[[IFTHEN:.+]], label %[[IFEND:.+]]

; CHECK: [[IFTHEN]]:
; CHECK: %[[CMP2:.+]] = icmp
; CHECK: br label %[[WHILEBODYPUREEXIT:.+]]

; CHECK: [[IFTHENELSE:.+]]:
; CHECK: br label %[[H:.+]]

; CHECK: [[IFTHENSPLIT:.+]]:
; CHECK: br i1 %[[CMP2MERGE:.+]], label %[[FORCONDPREHEADER:.+]], label %[[FORCOND20PREHEADER:.+]]

; CHECK: [[FORCOND20PREHEADER]]:
; CHECK: br label %[[FORCOND20:.+]]

; CHECK: [[FORCONDPREHEADER]]:
; CHECK: br label %[[FORCOND:.+]]

; CHECK: [[IFEND]]:
; CHECK: br i1 %{{.+}}, label %[[WHILEBODY]], label %[[WHILEBODYPUREEXIT]]

; CHECK: [[WHILEBODYPUREEXIT]]:
; CHECK: %[[CMP2MERGE]] = phi i1 [ %[[CMP2]], %[[IFTHEN]] ], [ false, %[[IFEND]] ]
; CHECK: br label %[[WHILEEND:.+]]

; CHECK: [[WHILEBODYUNIFORM]]:
; CHECK: %[[CMPUNIFORM:.+]] = icmp
; CHECK: br i1 %[[CMPUNIFORM]], label %[[IFTHENUNIFORM:.+]], label %[[IFENDUNIFORM:.+]]

; CHECK: [[IFENDUNIFORM]]:
; CHECK: br i1 %{{.+}}, label %[[WHILEENDUNIFORM:.+]], label %[[IFENDUNIFORMBOSCCINDIR:.+]]

; CHECK: [[WHILEENDUNIFORM]]:
; CHECK: %[[CMP9UNIFORM:.+]] = icmp
; CHECK: br i1 %[[CMP9UNIFORM]], label %[[HUNIFORM:.+]], label %[[I28UNIFORM:.+]]

; CHECK: [[IFENDUNIFORMBOSCCINDIR]]:
; CHECK: br i1 %{{.+}}, label %[[WHILEBODYUNIFORM]], label %[[IFENDUNIFORMBOSCCSTORE:.+]]

; CHECK: [[IFENDUNIFORMBOSCCSTORE]]:
; CHECK: br label %[[WHILEBODY]]

; CHECK: [[IFTHENUNIFORM]]:
; CHECK: %[[CMP2UNIFORM:.+]] = icmp
; CHECK: br i1 %[[CMP2UNIFORM]], label %[[FORCONDPREHEADERUNIFORM:.+]], label %[[FORCOND20PREHEADERUNIFORM:.+]]

; CHECK: [[FORCOND20PREHEADERUNIFORM]]:
; CHECK: br label %[[FORCOND20UNIFORM:.+]]

; CHECK: [[FORCOND20UNIFORM]]:
; CHECK: br i1 {{(%[0-9A-Za-z\.]+)|(false)}}, label %[[FORBODY23UNIFORM:.+]], label %[[HLOOPEXITUNIFORM:.+]]

; CHECK: [[FORBODY23UNIFORM]]:
; CHECK: br label %[[FORCOND20UNIFORM]]

; CHECK: [[HLOOPEXITUNIFORM]]:
; CHECK: br label %[[HUNIFORM]]

; CHECK: [[FORCONDPREHEADERUNIFORM]]:
; CHECK: br label %[[FORCONDUNIFORM:.+]]

; CHECK: [[FORCONDUNIFORM]]:
; CHECK: br i1 {{(%[0-9A-Za-z\.]+)|(false)}}, label %[[FORBODYUNIFORM:.+]], label %[[I28LOOPEXITUNIFORM:.+]]

; CHECK: [[FORBODYUNIFORM]]:
; CHECK: br label %[[FORCONDUNIFORM]]

; CHECK: [[I28LOOPEXITUNIFORM]]:
; CHECK: br label %[[I28UNIFORM]]

; CHECK: [[HUNIFORM]]:
; CHECK: br label %[[FORCOND40UNIFORM:.+]]

; CHECK: [[FORCOND40UNIFORM]]:
; CHECK: br i1 {{(%[0-9A-Za-z\.]+)|(false)}}, label %[[FORBODY43UNIFORM:.+]], label %[[JLOOPEXIT1UNIFORM:.+]]

; CHECK: [[FORBODY43UNIFORM]]:
; CHECK: br label %[[FORCOND40UNIFORM]]

; CHECK: [[JLOOPEXIT1UNIFORM]]:
; CHECK: br label %[[JUNIFORM:.+]]

; CHECK: [[I28UNIFORM]]:
; CHECK: br label %[[FORCOND30UNIFORM:.+]]

; CHECK: [[FORCOND30UNIFORM]]:
; CHECK: br i1 {{(%[0-9A-Za-z\.]+)|(false)}}, label %[[FORBODY34UNIFORM:.+]], label %[[JLOOPEXITUNIFORM:.+]]

; CHECK: [[FORBODY34UNIFORM]]:
; CHECK: br label %[[FORCOND30UNIFORM]]

; CHECK: [[JLOOPEXITUNIFORM]]:
; CHECK: br label %[[J:.+]]

; CHECK: [[WHILEEND]]:
; CHECK: br label %[[WHILEENDELSE:.+]]

; CHECK: [[WHILEENDELSE]]:
; CHECK: br i1 %{{.+}}, label %[[IFTHENELSE]], label %[[IFTHENSPLIT]]

; CHECK: [[FORCOND]]:
; CHECK: br i1 {{(%[0-9A-Za-z\.]+)|(false)}}, label %[[FORBODY:.+]], label %[[I28LOOPEXIT:.+]]

; CHECK: [[FORBODY]]:
; CHECK: br label %[[FORCOND]]

; CHECK: [[FORCOND20]]:
; CHECK: br i1 {{(%[0-9A-Za-z\.]+)|(false)}}, label %[[FORBODY23:.+]], label %[[HLOOPEXIT:.+]]

; CHECK: [[FORBODY23]]:
; CHECK: br label %[[FORCOND20]]

; CHECK: [[I28LOOPEXIT]]:
; CHECK: br label %[[H:.+]]

; CHECK: [[I28:.+]]:
; CHECK: br label %[[FORCOND30:.+]]

; CHECK: [[FORCOND30]]:
; CHECK: br i1 {{(%[0-9A-Za-z\.]+)|(false)}}, label %[[FORBODY34:.+]], label %[[JLOOPEXIT:.+]]

; CHECK: [[FORBODY34]]:
; CHECK: br label %[[FORCOND30]]

; CHECK: [[HLOOPEXIT]]:
; CHECK: br label %[[H]]

; CHECK: [[H]]:
; CHECK: br label %[[FORCOND40:.+]]

; CHECK: [[FORCOND40]]:
; CHECK: br i1 {{(%[0-9A-Za-z\.]+)|(false)}}, label %[[FORBODY43:.+]], label %[[JLOOPEXIT2:.+]]

; CHECK: [[FORBODY43]]:
; CHECK: br label %[[FORCOND40]]

; CHECK: [[JLOOPEXIT]]:
; CHECK: br label %[[J]]

; CHECK: [[JLOOPEXIT2]]:
; CHECK: br label %[[I28]]

; CHECK: [[J]]:
; CHECK: ret void
