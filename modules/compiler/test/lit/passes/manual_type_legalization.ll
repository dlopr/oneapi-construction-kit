; Copyright (C) Codeplay Software Limited
;
; Licensed under the Apache License, Version 2.0 (the "License") with LLVM
; Exceptions; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

; RUN: muxc --passes manual-type-legalization,verify -S %s | FileCheck %s

; Make sure we use a triple that does not have half as a legal type.
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define half @f
; CHECK-DAG: [[AEXT:%.*]] = fpext half %a to float
; CHECK-DAG: [[BEXT:%.*]] = fpext half %b to float
; CHECK-DAG: [[CEXT:%.*]] = fpext half %c to float
; CHECK-DAG: [[DADD:%.*]] = fadd float [[AEXT]], [[BEXT]]
; CHECK-DAG: [[DTRUNC:%.*]] = fptrunc float [[DADD]] to half
; CHECK-DAG: [[DEXT:%.*]] = fpext half [[DTRUNC]] to float
; CHECK-DAG: [[EADD:%.*]] = fadd float [[DEXT]], [[CEXT]]
; CHECK-DAG: [[ETRUNC:%.*]] = fptrunc float [[EADD]] to half
; CHECK: ret half [[ETRUNC]]
define half @f(half %a, half %b, half %c) {
entry:
  %d = fadd half %a, %b
  %e = fadd half %d, %c
  ret half %e
}
