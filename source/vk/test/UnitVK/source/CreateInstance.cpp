// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateInstance

class CreateInstance : public ::testing::Test {
 public:
  CreateInstance()
      : applicationInfo(), createInfo(), instance(VK_NULL_HANDLE) {}

  virtual void SetUp() {
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "UnitVK";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    applicationInfo.pEngineName = "Codeplay Vulkan Compute Test Suite";
    applicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 11);

    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &applicationInfo;
  }

  virtual void TearDown() {
    if (instance) {
      vkDestroyInstance(instance, nullptr);
    }
  }

  VkApplicationInfo applicationInfo;
  VkInstanceCreateInfo createInfo;
  VkInstance instance;
};

TEST_F(CreateInstance, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateInstance(&createInfo, nullptr, &instance));
}

TEST_F(CreateInstance, DefaultAllocator) {
  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkCreateInstance(&createInfo, uvk::defaultAllocator(), &instance));
  vkDestroyInstance(instance, uvk::defaultAllocator());
  instance = VK_NULL_HANDLE;
}

TEST_F(CreateInstance, DefaultLayer) {
  uint32_t layerCount;
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

  if (0 < layerCount) {
    std::vector<VkLayerProperties> layerProperties(layerCount);
    ASSERT_EQ_RESULT(VK_SUCCESS, vkEnumerateInstanceLayerProperties(
                                     &layerCount, layerProperties.data()));

    createInfo.enabledLayerCount = 1;
    const char *enabledLayerNames[] = {layerProperties[0].layerName};
    createInfo.ppEnabledLayerNames = enabledLayerNames;

    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkCreateInstance(&createInfo, nullptr, &instance));
  }
}

TEST_F(CreateInstance, DefaultExtension) {
  uint32_t extensionCount;
  ASSERT_EQ_RESULT(VK_SUCCESS, vkEnumerateInstanceExtensionProperties(
                                   nullptr, &extensionCount, nullptr));

  if (0 < extensionCount) {
    std::vector<VkExtensionProperties> extensionProperties(extensionCount);
    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkEnumerateInstanceExtensionProperties(
                         nullptr, &extensionCount, extensionProperties.data()));

    createInfo.enabledExtensionCount = 1;
    const char *enabledExtensionNames[] = {
        extensionProperties[0].extensionName};
    createInfo.ppEnabledExtensionNames = enabledExtensionNames;

    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkCreateInstance(&createInfo, nullptr, &instance));
  }
}

TEST_F(CreateInstance, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(
      VK_ERROR_OUT_OF_HOST_MEMORY,
      vkCreateInstance(&createInfo, uvk::nullAllocator(), &instance));
}

TEST_F(CreateInstance, LayerNotPresent) {
  // Set enabledLayerCount to 1 and give it a non-existant layer
  const char *dummyLayer = "not really a layer name";

  createInfo.enabledLayerCount = 1;
  createInfo.ppEnabledLayerNames = &dummyLayer;

  ASSERT_EQ_RESULT(VK_ERROR_LAYER_NOT_PRESENT,
                   vkCreateInstance(&createInfo, nullptr, &instance));
}

TEST_F(CreateInstance, ExtensionNotPresent) {
  // Set enabledExtensionCount to 1 and give it a non-existant extension
  const char *dummyExtension = "not really an extension name";

  createInfo.enabledExtensionCount = 1;
  createInfo.ppEnabledExtensionNames = &dummyExtension;

  ASSERT_EQ_RESULT(VK_ERROR_EXTENSION_NOT_PRESENT,
                   vkCreateInstance(&createInfo, nullptr, &instance));
}

#ifndef UNITVK_USE_LOADER
TEST_F(CreateInstance, IncompatibleDriver) {
  // Set apiVersion to something nonsensical
  applicationInfo.apiVersion = VK_MAKE_VERSION(1023, 0, 0);

  ASSERT_EQ_RESULT(VK_ERROR_INCOMPATIBLE_DRIVER,
                   vkCreateInstance(&createInfo, nullptr, &instance));
}
#endif

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable due to the fact
// that we can't currently access device memory  allocators to mess with.
//
// VK_ERROR_INITIALIZATION_FAILED
// Is a possible return from this function, but is untestable because it can't
// actually be generated using only api calls.
