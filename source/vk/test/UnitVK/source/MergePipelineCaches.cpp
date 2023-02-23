// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkMergePipelineCaches

class MergePipelineCaches : public uvk::PipelineLayoutTest {
 public:
  MergePipelineCaches()
      : srcCacheCount(2),
        srcPipelineCaches(srcCacheCount),
        dstPipelineCache(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(PipelineLayoutTest::SetUp());

    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr,
                          &dstPipelineCache);

    for (VkPipelineCache& pipelineCache : srcPipelineCaches) {
      ASSERT_EQ_RESULT(VK_SUCCESS,
                       vkCreatePipelineCache(device, &pipelineCacheCreateInfo,
                                             nullptr, &pipelineCache));
    }

    uvk::ShaderCode shaderCode = uvk::getShader(uvk::Shader::nop);

    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shaderCode.size;
    shaderModuleCreateInfo.pCode =
        reinterpret_cast<const uint32_t*>(shaderCode.code);

    vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr,
                         &shaderModule);

    VkPipelineShaderStageCreateInfo stage = {};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.module = shaderModule;
    stage.pName = "main";
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;

    VkComputePipelineCreateInfo computePipelineCreateInfo = {};
    computePipelineCreateInfo.sType =
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.stage = stage;
    computePipelineCreateInfo.layout = pipelineLayout;

    std::vector<VkPipeline> pipelines(srcCacheCount);

    for (int pipelineIndex = 0; pipelineIndex < srcCacheCount; pipelineIndex++) {
      vkCreateComputePipelines(device, srcPipelineCaches[pipelineIndex], 1,
                               &computePipelineCreateInfo, nullptr,
                               &pipelines[pipelineIndex]);
      vkDestroyPipeline(device, pipelines[pipelineIndex], nullptr);
    }

    vkDestroyShaderModule(device, shaderModule, nullptr);
  }

  virtual void TearDown() override {
    vkDestroyPipelineCache(device, dstPipelineCache, nullptr);
    for (VkPipelineCache& cache : srcPipelineCaches) {
      vkDestroyPipelineCache(device, cache, nullptr);
    }

    PipelineLayoutTest::TearDown();
  }

  int srcCacheCount;
  std::vector<VkPipelineCache> srcPipelineCaches;
  VkPipelineCache dstPipelineCache;
};

TEST_F(MergePipelineCaches, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkMergePipelineCaches(device, dstPipelineCache,
                                                     srcPipelineCaches.size(),
                                                     srcPipelineCaches.data()));
}
