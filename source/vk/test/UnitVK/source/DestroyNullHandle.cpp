// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// Tests which call the various destroy/free functions on VK_NULL_HANDLE to
// ensure such calls are silently ignored as they should be.

class DestroyNullHandle : public uvk::DescriptorPoolTest,
                          public uvk::CommandPoolTest {
 public:
  DestroyNullHandle() : DescriptorPoolTest(true), CommandPoolTest(true) {}

  // the SetUp/TearDown contain only the device test setup so we aren't
  // needlessly creating a descriptor/command pool for tests that don't need
  // them
  virtual void SetUp() { RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp()); }

  virtual void TearDown() { DeviceTest::TearDown(); }
};

TEST_F(DestroyNullHandle, Instance) {
  vkDestroyInstance(VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, Device) { vkDestroyDevice(VK_NULL_HANDLE, nullptr); }

TEST_F(DestroyNullHandle, CommandPool) {
  vkDestroyCommandPool(device, VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, CommandBuffer) {
  RETURN_ON_FATAL_FAILURE(CommandPoolTest::SetUp());

  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

  CommandPoolTest::TearDown();
}

TEST_F(DestroyNullHandle, Fence) {
  vkDestroyFence(device, VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, Semaphore) {
  vkDestroySemaphore(device, VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, Event) {
  vkDestroyEvent(device, VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, ShaderModule) {
  vkDestroyShaderModule(device, VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, Pipeline) {
  vkDestroyPipeline(device, VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, PipelineCache) {
  vkDestroyPipelineCache(device, VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, DeviceMemory) {
  vkFreeMemory(device, VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, Buffer) {
  vkDestroyBuffer(device, VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, BufferView) {
  vkDestroyBufferView(device, VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, DescriptorSetLayout) {
  vkDestroyDescriptorSetLayout(device, VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, PipelineLayout) {
  vkDestroyPipelineLayout(device, VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, DescriptorPool) {
  vkDestroyDescriptorPool(device, VK_NULL_HANDLE, nullptr);
}

TEST_F(DestroyNullHandle, DesciptorSet) {
  RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());

  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

  vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);

  DescriptorPoolTest::TearDown();
}

TEST_F(DestroyNullHandle, QueryPool) {
  vkDestroyQueryPool(device, VK_NULL_HANDLE, nullptr);
}

