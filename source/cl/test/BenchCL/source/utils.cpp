// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <BenchCL/utils.h>
#include <BenchCL/environment.h>
#include <cassert>
#include <vector>
#include <string>

#define CHECK(EXPR)        \
  err = EXPR;              \
  if (err != CL_SUCCESS) { \
    return err;            \
  }                        \
  (void)0

cl_uint benchcl::get_device(cargo::string_view device_name,
                            cl_platform_id &platform, cl_device_id &device) {
  cl_uint num_platforms = 0;
  cl_uint err;
  CHECK(clGetPlatformIDs(1, &platform, &num_platforms));
  assert(1 == num_platforms);

  cl_uint num_devices = 0;
  CHECK(clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices));

  // if there's only one device and we don't have a name return the device
  if (device_name == nullptr && num_devices == 1) {
    CHECK(clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device,
                         &num_devices));
    return CL_SUCCESS;
  }

  // otherwise try to find a device with the given name
  std::vector<cl_device_id> devices(num_devices);
  CHECK(clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num_devices,
                       devices.data(), nullptr));

  std::vector<std::string> names;
  for (auto dev : devices) {
    size_t size = 0;
    CHECK(clGetDeviceInfo(dev, CL_DEVICE_NAME, 0, nullptr, &size));

    std::string name(size, '\0');
    CHECK(
        clGetDeviceInfo(dev, CL_DEVICE_NAME, name.length(), &name[0], nullptr));

    names.push_back(name);
    if (name == device_name) {
      device = dev;
      return CL_SUCCESS;
    }
  }

  if (device_name == nullptr) {
    fprintf(stderr,
            "error: multiple devices found but no device name specified, "
            "please use '--benchcl_device='\n");
  } else {
    fprintf(stderr, "error: device '%s' not found\n", device_name.data());
  }

  fprintf(stderr, "Available devices:\n");
  for (std::string &name : names) {
    fprintf(stderr, "  - '%s'\n", name.c_str());
  }

  return CL_DEVICE_NOT_FOUND;
}
