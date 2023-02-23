// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <BenchCL/error.h>
#include <BenchCL/environment.h>
#include <CL/cl.h>
#include <benchmark/benchmark.h>
#include <string>
#include <thread>

struct CreateData {
  enum { BUFFER_LENGTH = 16384, BUFFER_SIZE = BUFFER_LENGTH * sizeof(cl_int) };

  cl_platform_id platform;
  cl_device_id device;
  cl_context context;
  cl_program program;
  cl_kernel kernel;
  cl_mem out;
  cl_mem in;
  cl_command_queue queue;

  CreateData() {
    platform = benchcl::env::get()->platform;
    device = benchcl::env::get()->device;

    cl_int status = CL_SUCCESS;
    context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

    const char* str =
        "kernel void func(global int* o, global int* i) {\n"
        "  o[get_global_id(0)] = i[get_global_id(0)];\n"
        "}\n";

    program = clCreateProgramWithSource(context, 1, &str, nullptr, &status);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

    ASSERT_EQ_ERRCODE(CL_SUCCESS, clBuildProgram(program, 0, nullptr, nullptr,
                                                 nullptr, nullptr));

    kernel = clCreateKernel(program, "func", &status);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

    out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, BUFFER_SIZE, nullptr,
                         &status);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

    in = clCreateBuffer(context, CL_MEM_READ_ONLY, BUFFER_SIZE, nullptr,
                        &status);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, status);

    ASSERT_EQ_ERRCODE(CL_SUCCESS,
                      clSetKernelArg(kernel, 0, sizeof(cl_mem), &out));
    ASSERT_EQ_ERRCODE(CL_SUCCESS,
                      clSetKernelArg(kernel, 1, sizeof(cl_mem), &in));

    queue = clCreateCommandQueue(context, device, 0, &status);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, status);
  }

  ~CreateData() {
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseCommandQueue(queue));
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseMemObject(in));
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseMemObject(out));
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseKernel(kernel));
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseProgram(program));
    ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseContext(context));
  }
};

void SingleThreadOneQueueNoDependencies(benchmark::State& state) {
  CreateData cd;

  for (auto _ : state) {
    for (unsigned i = 0; i < state.range(0); i++) {
      size_t size = CreateData::BUFFER_LENGTH;
      clEnqueueNDRangeKernel(cd.queue, cd.kernel, 1, nullptr, &size, nullptr, 0,
                             nullptr, nullptr);
    }

    clFinish(cd.queue);
  }

  state.SetItemsProcessed(state.range(0));
}
BENCHMARK(SingleThreadOneQueueNoDependencies)->Arg(1)->Arg(256)->Arg(1024);

void SingleThreadOneQueue(benchmark::State& state) {
  CreateData cd;

  for (auto _ : state) {
    const size_t size = CreateData::BUFFER_LENGTH;

    cl_event event;
    clEnqueueNDRangeKernel(cd.queue, cd.kernel, 1, nullptr, &size, nullptr, 0,
                           nullptr, &event);

    for (unsigned i = 1; i < state.range(0); i++) {
      clEnqueueNDRangeKernel(cd.queue, cd.kernel, 1, nullptr, &size, nullptr, 1,
                             &event, &event);
    }

    clFinish(cd.queue);

    clReleaseEvent(event);
  }

  state.SetItemsProcessed(state.range(0));
}
BENCHMARK(SingleThreadOneQueue)->Arg(1)->Arg(256)->Arg(1024);

void MultiThreadOneQueueNoDependencies(benchmark::State& state) {
  CreateData cd;

  for (auto _ : state) {
    for (unsigned i = 0; i < state.range(0); i++) {
      size_t size = CreateData::BUFFER_LENGTH;
      clEnqueueNDRangeKernel(cd.queue, cd.kernel, 1, nullptr, &size, nullptr, 0,
                             nullptr, nullptr);
    }

    clFinish(cd.queue);
  }

  state.SetItemsProcessed(std::thread::hardware_concurrency() *
                          state.range(0));
}
BENCHMARK(MultiThreadOneQueueNoDependencies)
    ->Arg(1)
    ->Arg(256)
    ->Arg(1024)
    ->Threads(std::thread::hardware_concurrency());

void MultiThreadOneQueue(benchmark::State& state) {
  CreateData cd;

  for (auto _ : state) {
    const size_t size = CreateData::BUFFER_LENGTH;

    cl_event event;
    clEnqueueNDRangeKernel(cd.queue, cd.kernel, 1, nullptr, &size, nullptr, 0,
                           nullptr, &event);

    for (unsigned i = 1; i < state.range(0); i++) {
      clEnqueueNDRangeKernel(cd.queue, cd.kernel, 1, nullptr, &size, nullptr, 1,
                             &event, &event);
    }

    clFinish(cd.queue);

    clReleaseEvent(event);
  }

  state.SetItemsProcessed(std::thread::hardware_concurrency() *
                          state.range(0));
}
BENCHMARK(MultiThreadOneQueue)
    ->Arg(1)
    ->Arg(256)
    ->Arg(1024)
    ->Threads(std::thread::hardware_concurrency());

void MultiThreadMultiQueueNoDependencies(benchmark::State& state) {
  CreateData cd;

  cl_command_queue queue;

  if (0 == state.thread_index) {
    queue = cd.queue;
  } else {
    queue = clCreateCommandQueue(cd.context, cd.device, 0, nullptr);
  }

  for (auto _ : state) {
    for (unsigned i = 0; i < state.range(0); i++) {
      size_t size = CreateData::BUFFER_LENGTH;
      clEnqueueNDRangeKernel(queue, cd.kernel, 1, nullptr, &size, nullptr, 0,
                             nullptr, nullptr);
    }

    clFinish(queue);
  }

  if (0 != state.thread_index) {
    clReleaseCommandQueue(queue);
  }

  state.SetItemsProcessed(std::thread::hardware_concurrency() *
                          state.range(0));
}
BENCHMARK(MultiThreadMultiQueueNoDependencies)
    ->Arg(1)
    ->Arg(256)
    ->Arg(1024)
    ->Threads(std::thread::hardware_concurrency());

void MultiThreadMultiQueue(benchmark::State& state) {
  CreateData cd;

  cl_command_queue queue;

  if (0 == state.thread_index) {
    queue = cd.queue;
  } else {
    queue = clCreateCommandQueue(cd.context, cd.device, 0, nullptr);
  }

  for (auto _ : state) {
    const size_t size = CreateData::BUFFER_LENGTH;

    cl_event event;
    clEnqueueNDRangeKernel(queue, cd.kernel, 1, nullptr, &size, nullptr, 0,
                           nullptr, &event);

    for (unsigned i = 1; i < state.range(0); i++) {
      clEnqueueNDRangeKernel(queue, cd.kernel, 1, nullptr, &size, nullptr, 1,
                             &event, &event);
    }

    clFinish(queue);

    clReleaseEvent(event);
  }

  if (0 != state.thread_index) {
    clReleaseCommandQueue(queue);
  }

  state.SetItemsProcessed(std::thread::hardware_concurrency() *
                          state.range(0));
}
BENCHMARK(MultiThreadMultiQueue)
    ->Arg(1)
    ->Arg(256)
    ->Arg(1024)
    ->Threads(std::thread::hardware_concurrency());
