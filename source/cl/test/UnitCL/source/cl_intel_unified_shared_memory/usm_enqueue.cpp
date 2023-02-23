// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <Common.h>

#include "cl_intel_unified_shared_memory.h"

struct USMCommandQueueTest : public cl_intel_unified_shared_memory_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_intel_unified_shared_memory_Test::SetUp());

    cl_device_unified_shared_memory_capabilities_intel host_capabilities;
    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
        sizeof(host_capabilities), &host_capabilities, nullptr));

    cl_int err;
    if (host_capabilities) {
      host_ptr = clHostMemAllocINTEL(context, nullptr, bytes, align, &err);
      ASSERT_SUCCESS(err);
      ASSERT_TRUE(host_ptr != nullptr);
    }

    device_ptr =
        clDeviceMemAllocINTEL(context, device, nullptr, bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(device_ptr != nullptr);

    queue = clCreateCommandQueue(context, device, 0, &err);
    ASSERT_TRUE(queue != nullptr);
    ASSERT_SUCCESS(err);
  }

  void TearDown() override {
    if (device_ptr) {
      cl_int err = clMemBlockingFreeINTEL(context, device_ptr);
      EXPECT_SUCCESS(err);
    }

    if (host_ptr) {
      cl_int err = clMemBlockingFreeINTEL(context, host_ptr);
      EXPECT_SUCCESS(err);
    }

    if (queue) {
      EXPECT_SUCCESS(clReleaseCommandQueue(queue));
    }

    cl_intel_unified_shared_memory_Test::TearDown();
  }

  static const size_t bytes = 512;
  static const cl_uint align = 4;

  void *host_ptr = nullptr;
  void *device_ptr = nullptr;

  cl_command_queue queue = nullptr;
};

// Test for invalid API usage of clEnqueueMemFillINTEL()
TEST_F(USMCommandQueueTest, MemFill_InvalidUsage) {
  const cl_int pattern[1] = {CL_INT_MAX};
  const cl_ulong16 vec_pattern[2] = {
      {{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD,
        0xE, 0xF}},
      {{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD,
        0xE, 0xF}}};

  // Null command queue argument
  cl_int err =
      clEnqueueMemFillINTEL(nullptr, device_ptr, pattern, sizeof(pattern),
                            sizeof(pattern), 0, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_COMMAND_QUEUE);

  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
  ASSERT_TRUE(other_context);
  ASSERT_SUCCESS(err);

  // Mismatch between context of event and command queue
  cl_event event = clCreateUserEvent(other_context, &err);
  err = clEnqueueMemFillINTEL(queue, device_ptr, pattern, sizeof(pattern),
                              sizeof(pattern), 1, &event, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_CONTEXT);

  EXPECT_SUCCESS(clReleaseContext(other_context));

  // Null pointer passed for dst_ptr
  err = clEnqueueMemFillINTEL(queue, nullptr, pattern, sizeof(pattern),
                              sizeof(pattern), 0, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  // Null pointer passed for pattern
  err = clEnqueueMemFillINTEL(queue, device_ptr, nullptr, sizeof(pattern),
                              sizeof(pattern), 0, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  // Pattern sizes must be powers of two, we omit 1 here since offsets
  // from it will always have valid alignment/sizes.
  const std::array<size_t, 7> valid_pattern_sizes{2, 4, 8, 16, 32, 64, 128};
  for (size_t pattern_size : valid_pattern_sizes) {
    // dst_ptr not aligned to pattern_size
    void *offset_device_ptr = getPointerOffset(device_ptr, 1);
    err = clEnqueueMemFillINTEL(queue, offset_device_ptr, pattern, pattern_size,
                                pattern_size, 0, nullptr, nullptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE)
        << "pattern size: " << pattern_size;

    // dst_ptr not aligned to pattern_size
    if (reinterpret_cast<uintptr_t>(device_ptr) % pattern_size != 0) {
      err = clEnqueueMemFillINTEL(queue, device_ptr, vec_pattern, pattern_size,
                                  pattern_size, 0, nullptr, nullptr);
      EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE)
          << "pattern size: " << pattern_size;
    }

    // size not a multiple of pattern_size
    err = clEnqueueMemFillINTEL(queue, device_ptr, pattern, pattern_size,
                                pattern_size + (pattern_size / 2), 0, nullptr,
                                nullptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE)
        << "pattern size: " << pattern_size;

    // Prefer even value when testing non power of two
    const size_t non_pow_two =
        pattern_size > 4 ? pattern_size - 2 : pattern_size + 1;
    // pattern_size not aligned to power-of-two
    err = clEnqueueMemFillINTEL(queue, device_ptr, pattern, non_pow_two,
                                non_pow_two, 0, nullptr, nullptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE)
        << "pattern size: " << pattern_size;
  }

  // pattern_size greater than the size of the largest integer supported by
  err =
      clEnqueueMemFillINTEL(queue, device_ptr, vec_pattern, sizeof(vec_pattern),
                            sizeof(vec_pattern), 0, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  // Zero value for size
  err = clEnqueueMemFillINTEL(queue, device_ptr, pattern, sizeof(pattern), 0, 0,
                              nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  // null wait list with non-zero num_events_in_wait_list
  err = clEnqueueMemFillINTEL(queue, device_ptr, pattern, sizeof(pattern),
                              sizeof(pattern), 1, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_EVENT_WAIT_LIST);

  // wait list with zero num_events_in_wait_list
  err = clEnqueueMemFillINTEL(queue, device_ptr, pattern, sizeof(pattern),
                              sizeof(pattern), 0, &event, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_EVENT_WAIT_LIST);

  EXPECT_SUCCESS(clReleaseEvent(event));
}

// Test for valid API usage of clEnqueueMemFillINTEL()
TEST_F(USMCommandQueueTest, MemFill_ValidUsage) {
  const cl_int pattern[1] = {CL_INT_MAX};
  const cl_ulong16 vec_pattern[1] = {
      {{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD,
        0xE, 0xF}}};

  cl_int err;

  // Pattern sizes must be powers of two
  const std::array<size_t, 8> valid_pattern_sizes{1, 2, 4, 8, 16, 32, 64, 128};

  if (host_ptr) {
    for (size_t pattern_size : valid_pattern_sizes) {
      // Check if allocation already aligned, or we need a new aligned
      // allocation
      if (reinterpret_cast<uintptr_t>(host_ptr) % pattern_size == 0) {
        err = clEnqueueMemFillINTEL(queue, host_ptr, vec_pattern, pattern_size,
                                    pattern_size, 0, nullptr, nullptr);
        EXPECT_SUCCESS(err) << "Pattern Size " << pattern_size;

        err = clEnqueueMemFillINTEL(queue, host_ptr, vec_pattern, pattern_size,
                                    pattern_size * 3, 0, nullptr, nullptr);
        EXPECT_SUCCESS(err) << "Pattern Size " << pattern_size;
      } else {
        void *aligned_host_ptr = clHostMemAllocINTEL(context, nullptr, bytes,
                                                     sizeof(vec_pattern), &err);
        ASSERT_SUCCESS(err) << "Pattern Size " << pattern_size;
        ASSERT_TRUE(aligned_host_ptr != nullptr)
            << "Pattern Size " << pattern_size;

        err = clEnqueueMemFillINTEL(queue, aligned_host_ptr, vec_pattern,
                                    sizeof(vec_pattern), sizeof(vec_pattern), 0,
                                    nullptr, nullptr);
        EXPECT_SUCCESS(err) << "Pattern Size " << pattern_size;

        err = clMemBlockingFreeINTEL(context, aligned_host_ptr);
        ASSERT_SUCCESS(err) << "Pattern Size " << pattern_size;
      }
    }
  }

  for (size_t pattern_size : valid_pattern_sizes) {
    // Check if allocation already aligned, or we need a new aligned allocation
    if (reinterpret_cast<uintptr_t>(device_ptr) % pattern_size == 0) {
      err = clEnqueueMemFillINTEL(queue, device_ptr, vec_pattern, pattern_size,
                                  pattern_size, 0, nullptr, nullptr);
      EXPECT_SUCCESS(err) << "Pattern Size " << pattern_size;

      err = clEnqueueMemFillINTEL(queue, device_ptr, vec_pattern, pattern_size,
                                  pattern_size * 3, 0, nullptr, nullptr);
      EXPECT_SUCCESS(err) << "Pattern Size " << pattern_size;
    } else {
      void *aligned_device_ptr = clDeviceMemAllocINTEL(
          context, device, nullptr, bytes, sizeof(vec_pattern), &err);
      ASSERT_SUCCESS(err) << "Pattern Size " << pattern_size;
      ASSERT_TRUE(aligned_device_ptr != nullptr)
          << "Pattern Size " << pattern_size;

      err = clEnqueueMemFillINTEL(queue, aligned_device_ptr, vec_pattern,
                                  sizeof(vec_pattern), sizeof(vec_pattern), 0,
                                  nullptr, nullptr);
      EXPECT_SUCCESS(err) << "Pattern Size " << pattern_size;

      err = clMemBlockingFreeINTEL(context, aligned_device_ptr);
      ASSERT_SUCCESS(err) << "Pattern Size " << pattern_size;
    }
  }

  std::array<cl_event, 2> wait_events;
  err = clEnqueueMemFillINTEL(queue, device_ptr, pattern, sizeof(pattern),
                              sizeof(pattern) * 2, 0, nullptr, &wait_events[0]);
  EXPECT_SUCCESS(err);

  if (host_ptr) {
    err =
        clEnqueueMemFillINTEL(queue, host_ptr, pattern, sizeof(pattern),
                              sizeof(pattern) * 2, 0, nullptr, &wait_events[1]);
    EXPECT_SUCCESS(err);

    void *offset_host_ptr = getPointerOffset(host_ptr, sizeof(cl_int));
    err =
        clEnqueueMemFillINTEL(queue, offset_host_ptr, pattern, sizeof(pattern),
                              sizeof(pattern), 2, wait_events.data(), nullptr);
    EXPECT_SUCCESS(err);
  }

  void *offset_device_ptr = getPointerOffset(device_ptr, sizeof(cl_int));
  err = clEnqueueMemFillINTEL(
      queue, offset_device_ptr, pattern, sizeof(pattern), sizeof(pattern),
      host_ptr == nullptr ? 1 : 2, wait_events.data(), nullptr);
  EXPECT_SUCCESS(err);

  EXPECT_SUCCESS(clReleaseEvent(wait_events[0]));
  if (host_ptr) {
    EXPECT_SUCCESS(clReleaseEvent(wait_events[1]));
  }
}

// Test for invalid API usage of clEnqueueMemcpyINTEL()
TEST_F(USMCommandQueueTest, Memcpy_InvalidUsage) {
  void *offset_device_ptr = getPointerOffset(device_ptr, sizeof(cl_int) * 4);

  // No command queue
  cl_int err =
      clEnqueueMemcpyINTEL(nullptr, CL_TRUE, device_ptr, offset_device_ptr,
                           sizeof(cl_int), 0, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_COMMAND_QUEUE);

  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
  ASSERT_TRUE(other_context);
  ASSERT_SUCCESS(err);

  // Context of event different from context of queue
  cl_event other_event = clCreateUserEvent(other_context, &err);
  ASSERT_SUCCESS(err);
  err = clEnqueueMemcpyINTEL(queue, CL_TRUE, device_ptr, offset_device_ptr,
                             sizeof(cl_int), 1, &other_event, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_CONTEXT);
  EXPECT_SUCCESS(clReleaseContext(other_context));

  // NULL dst_ptr
  err = clEnqueueMemcpyINTEL(queue, CL_TRUE, nullptr, device_ptr,
                             sizeof(cl_int), 0, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  // NULL src_ptr
  err = clEnqueueMemcpyINTEL(queue, CL_TRUE, device_ptr, nullptr,
                             sizeof(cl_int), 0, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  // Overlapping copy
  void *overlap_device_ptr = getPointerOffset(device_ptr, 1);
  err = clEnqueueMemcpyINTEL(queue, CL_TRUE, device_ptr, overlap_device_ptr,
                             sizeof(cl_int), 0, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_MEM_COPY_OVERLAP);

  // Non-zero num_wait_events with NULL wait events
  err = clEnqueueMemcpyINTEL(queue, CL_TRUE, device_ptr, offset_device_ptr,
                             sizeof(cl_int), 1, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_EVENT_WAIT_LIST);

  // Zero num_wait_events with non-NULL wait_event
  cl_event event = clCreateUserEvent(context, &err);
  ASSERT_SUCCESS(err);
  err = clEnqueueMemcpyINTEL(queue, CL_TRUE, device_ptr, offset_device_ptr,
                             sizeof(cl_int), 0, &event, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_EVENT_WAIT_LIST);

  // blocking set to CL_TRUE with an event in the wait list which has failed,
  // i.e has a negative integer event execution status
  ASSERT_SUCCESS(clSetUserEventStatus(event, -1));
  err = clEnqueueMemcpyINTEL(queue, CL_TRUE, device_ptr, offset_device_ptr,
                             sizeof(cl_int), 1, &event, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);

  // blocking set to CL_FALSE with an event in the wait list which has failed,
  // i.e has a negative integer event execution status.
  err = clEnqueueMemcpyINTEL(queue, CL_FALSE, device_ptr, offset_device_ptr,
                             sizeof(cl_int), 1, &event, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_EVENT_WAIT_LIST);

  EXPECT_SUCCESS(clReleaseEvent(event));
  EXPECT_SUCCESS(clReleaseEvent(other_event));
}

// Test for valid API usage of clEnqueueMemcpyINTEL()
TEST_F(USMCommandQueueTest, Memcpy_ValidUsage) {
  void *offset_device_ptr = getPointerOffset(device_ptr, sizeof(cl_int));
  cl_int err =
      clEnqueueMemcpyINTEL(queue, CL_TRUE, offset_device_ptr, device_ptr,
                           sizeof(cl_int), 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);

  if (host_ptr) {
    cl_event event;
    err = clEnqueueMemcpyINTEL(queue, CL_FALSE, device_ptr, host_ptr,
                               sizeof(cl_int), 0, nullptr, &event);
    EXPECT_SUCCESS(err);

    err = clEnqueueMemcpyINTEL(queue, CL_TRUE, host_ptr, device_ptr,
                               sizeof(cl_int), 1, &event, nullptr);
    EXPECT_SUCCESS(err);

    void *offset_host_ptr = getPointerOffset(host_ptr, sizeof(cl_int));
    err =
        clEnqueueMemcpyINTEL(queue, CL_TRUE, offset_host_ptr, offset_device_ptr,
                             sizeof(cl_int), 0, nullptr, nullptr);
    EXPECT_SUCCESS(err);
    err =
        clEnqueueMemcpyINTEL(queue, CL_TRUE, offset_device_ptr, offset_host_ptr,
                             sizeof(cl_int), 0, nullptr, nullptr);
    EXPECT_SUCCESS(err);

    err = clEnqueueMemcpyINTEL(queue, CL_TRUE, offset_host_ptr, host_ptr,
                               sizeof(cl_int), 0, nullptr, nullptr);
    EXPECT_SUCCESS(err);

    EXPECT_SUCCESS(clReleaseEvent(event));
  }

  // Copying from & to arbitrary host data is permitted by the spec
  cl_uchar user_data[64] = {0};
  err = clEnqueueMemcpyINTEL(queue, CL_TRUE, device_ptr, user_data,
                             sizeof(user_data), 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);

  err = clEnqueueMemcpyINTEL(queue, CL_TRUE, user_data, device_ptr,
                             sizeof(user_data), 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);

  cl_uchar *offset_user_ptr = user_data + sizeof(cl_ulong);
  err = clEnqueueMemcpyINTEL(queue, CL_TRUE, offset_user_ptr, user_data,
                             sizeof(cl_ulong), 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);
}

// Test for invalid API usage of clEnqueueMigrateMemINTEL()
TEST_F(USMCommandQueueTest, MigrateMem_InvalidUsage) {
  // Null queue
  cl_int err =
      clEnqueueMigrateMemINTEL(nullptr, device_ptr, bytes,
                               CL_MIGRATE_MEM_OBJECT_HOST, 0, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_COMMAND_QUEUE);

  // Context mismatch  between event and queue device
  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
  ASSERT_TRUE(other_context);
  ASSERT_SUCCESS(err);

  cl_event event = clCreateUserEvent(other_context, &err);
  err = clEnqueueMigrateMemINTEL(
      queue, device_ptr, bytes, CL_MIGRATE_MEM_OBJECT_HOST, 1, &event, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_CONTEXT);
  EXPECT_SUCCESS(clReleaseContext(other_context));

  // Flags is zero
  err = clEnqueueMigrateMemINTEL(queue, device_ptr, bytes, 0, 0, nullptr,
                                 nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  // Invalid Flags
  cl_mem_migration_flags bad_flags = ~cl_mem_migration_flags(0);
  err = clEnqueueMigrateMemINTEL(queue, device_ptr, bytes, bad_flags, 0,
                                 nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  // Non-zero num_wait_events with null wait_events
  err =
      clEnqueueMigrateMemINTEL(queue, device_ptr, bytes,
                               CL_MIGRATE_MEM_OBJECT_HOST, 1, nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_EVENT_WAIT_LIST);

  // Zero num_wait_events with non-null wait_events
  err = clEnqueueMigrateMemINTEL(
      queue, device_ptr, bytes, CL_MIGRATE_MEM_OBJECT_HOST, 0, &event, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_EVENT_WAIT_LIST);

  EXPECT_SUCCESS(clReleaseEvent(event));
}

// Test for valid API usage of clEnqueueMigrateMemINTEL()
TEST_F(USMCommandQueueTest, MigrateMem_ValidUsage) {
  std::array<cl_event, 2> events;
  cl_int err = clEnqueueMigrateMemINTEL(queue, device_ptr, bytes,
                                        CL_MIGRATE_MEM_OBJECT_HOST, 0, nullptr,
                                        &events[0]);
  EXPECT_SUCCESS(err);

  if (host_ptr) {
    err = clEnqueueMigrateMemINTEL(queue, host_ptr, bytes,
                                   CL_MIGRATE_MEM_OBJECT_HOST, 0, nullptr,
                                   &events[1]);
    EXPECT_SUCCESS(err);

    err = clEnqueueMigrateMemINTEL(queue, host_ptr, bytes,
                                   CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 1,
                                   &events[1], nullptr);
    EXPECT_SUCCESS(err);

    err = clEnqueueMigrateMemINTEL(
        queue, host_ptr, bytes,
        CL_MIGRATE_MEM_OBJECT_HOST | CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0,
        nullptr, nullptr);
    EXPECT_SUCCESS(err);
  }

  err = clEnqueueMigrateMemINTEL(queue, device_ptr, bytes,
                                 CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 1,
                                 &events[0], nullptr);
  EXPECT_SUCCESS(err);

  err = clEnqueueMigrateMemINTEL(
      queue, device_ptr, bytes,
      CL_MIGRATE_MEM_OBJECT_HOST | CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 0,
      nullptr, nullptr);
  EXPECT_SUCCESS(err);

  EXPECT_SUCCESS(clReleaseEvent(events[0]));
  if (host_ptr) {
    EXPECT_SUCCESS(clReleaseEvent(events[1]));
  }
}

// Test for invalid API usage of clEnqueueMemAdviseINTEL()
TEST_F(USMCommandQueueTest, MemAdvise_InvalidUsage) {
  // NULL command queue
  cl_mem_advice_intel advice = 0;
  cl_int err = clEnqueueMemAdviseINTEL(nullptr, device_ptr, bytes, advice, 0,
                                       nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_COMMAND_QUEUE);

  // Context mismatch between event and queue
  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
  ASSERT_TRUE(other_context);
  ASSERT_SUCCESS(err);

  cl_event event = clCreateUserEvent(other_context, &err);
  err = clEnqueueMemAdviseINTEL(queue, device_ptr, bytes, advice, 1, &event,
                                nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_CONTEXT);
  EXPECT_SUCCESS(clReleaseContext(other_context));

  // Non-zero num_wait_events with NULL wait_events
  err = clEnqueueMemAdviseINTEL(queue, device_ptr, bytes, advice, 1, nullptr,
                                nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_EVENT_WAIT_LIST);

  // Zero num_wait_events with non-NULL wait_events
  err = clEnqueueMemAdviseINTEL(queue, device_ptr, bytes, advice, 0, &event,
                                nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_EVENT_WAIT_LIST);

  // Advice flag not supported by device
  cl_mem_advice_intel bad_advice = 0x4208;  // values reserved but not defined
  if (host_ptr) {
    err = clEnqueueMemAdviseINTEL(queue, host_ptr, bytes, bad_advice, 0,
                                  nullptr, nullptr);
    EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  }

  err = clEnqueueMemAdviseINTEL(queue, device_ptr, bytes, bad_advice, 0,
                                nullptr, nullptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_VALUE);

  EXPECT_SUCCESS(clReleaseEvent(event));
}
