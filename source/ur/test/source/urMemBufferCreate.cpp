// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "uur/fixtures.h"

using urMemBufferCreateTest = uur::ContextTest;
UUR_INSTANTIATE_DEVICE_TEST_SUITE_P(urMemBufferCreateTest);

TEST_P(urMemBufferCreateTest, Success) {
  ur_mem_handle_t buffer = nullptr;
  ASSERT_SUCCESS(urMemBufferCreate(context, UR_MEM_FLAG_READ_WRITE, 4096,
                                   nullptr, &buffer));
  ASSERT_NE(nullptr, buffer);
  ASSERT_SUCCESS(urMemRelease(buffer));
}

TEST_P(urMemBufferCreateTest, InvalidNullHandleContext) {
  ur_mem_handle_t buffer = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                   urMemBufferCreate(nullptr, UR_MEM_FLAG_READ_WRITE, 4096,
                                     nullptr, &buffer));
}

TEST_P(urMemBufferCreateTest, InvalidEnumerationFlags) {
  ur_mem_handle_t buffer = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_ENUMERATION,
                   urMemBufferCreate(context, UR_MEM_FLAG_FORCE_UINT32, 4096,
                                     nullptr, &buffer));
}

// TODO: The spec states that hostPtr is non-optional but this seems like a bug,
// the user shouldn't have to pass a host pointer if they don't intend to use
// one. This test has been disabled until this is resolved.
TEST_P(urMemBufferCreateTest, DISABLED_InvalidNullPointerHostPtr) {
  ur_mem_handle_t buffer = nullptr;
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urMemBufferCreate(context, UR_MEM_FLAG_READ_WRITE, 4096,
                                     nullptr, &buffer));
}

TEST_P(urMemBufferCreateTest, InvalidNullPointerBuffer) {
  ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_POINTER,
                   urMemBufferCreate(context, UR_MEM_FLAG_READ_WRITE, 4096,
                                     nullptr, nullptr));
}
