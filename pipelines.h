#pragma once
#ifndef ENGINE_PIPELINES_H
#define ENGINE_PIPELINES_H

#include <chrono>
#include <cinttypes>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "engine_defs.h"

namespace Engine
{
namespace Pipelines
{

typedef struct QueueRequirements
{
    uint32_t graphics;
    uint32_t compute;
} QueueRequirements_t;

typedef uint32_t BufferID_t;
typedef uint32_t ImageID_t;
typedef uint32_t SamplerID_t;

typedef struct DescriptorInfo
{
    vk::DescriptorType type;
    uint32_t bindLoc = UINT32_MAX;
    vk::DescriptorBufferInfo buffer;
    vk::DescriptorImageInfo image;
} DescriptorInfo_t;

class EnginePipelineBase
{
  public:
    virtual ~EnginePipelineBase()
    {
    }

    virtual void Setup(const vk::UniqueDevice &device, vk::PhysicalDeviceMemoryProperties phyDevMemProps,
                       const uint32_t graphicsQueueFamilyIndex, const uint32_t computeQueueFamilyIndex,
                       const uint32_t graphicsQueueStartIndex, const uint32_t computeQueueStartIndex,
                       const vk::UniqueRenderPass &renderPass)
    {
        m_phyDevMemProps = phyDevMemProps;
    }

    virtual void CleanupEnginePipeline(const vk::UniqueDevice &device)
    {
    }

    virtual const QueueRequirements_t GetQueueRequirements() const
    {
        return {0, 0};
    }

    const bool IsSetup() const
    {
        return m_setupComplete;
    }

    const bool HasPreRenderStage() const
    {
        return m_hasPreRenderStage;
    }

    virtual void PreRender(const vk::Device &device, vk::Extent2D windowExtents){};

    virtual void Render(const vk::Device &device, const vk::CommandBuffer &cmdBuffer, vk::Extent2D windowExtents){};

    virtual void Resized(vk::Extent2D windowExtents){};

    virtual void Compute(const vk::Device &device){};

  protected:
    vk::PhysicalDeviceMemoryProperties m_phyDevMemProps;

    bool m_hasPreRenderStage = false;
    bool m_setupComplete = false;
};

template <uint32_t graphics_queue_count> class GraphicsEnginePipelineBase
{
  protected:
    uint32_t m_graphicsQueueFamilyIndex;
    uint32_t m_graphicsQueueStartIndex;
    vk::Queue m_graphicsQueues[graphics_queue_count];

    vk::UniqueCommandPool m_graphicsCommandPool;

    void SetupGraphics(const vk::UniqueDevice &device, vk::PhysicalDeviceMemoryProperties phyDevMemProps,
                       const uint32_t graphicsQueueFamilyIndex, const uint32_t graphicsQueueStartIndex)
    {
        m_graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
        m_graphicsQueueStartIndex = graphicsQueueStartIndex;

        CreateGraphicsQueues(device);

        vk::CommandPoolCreateInfo graphicsCmdPoolCreateInfo(vk::CommandPoolCreateFlags(), m_graphicsQueueFamilyIndex);

        m_graphicsCommandPool = device->createCommandPoolUnique(graphicsCmdPoolCreateInfo);
    }

    void CreateGraphicsQueues(const vk::UniqueDevice &device)
    {
        for (uint32_t i = 0; i < graphics_queue_count; i++)
        {
            m_graphicsQueues[i] = device->getQueue(m_graphicsQueueFamilyIndex, m_graphicsQueueStartIndex + i);
        }
    }
};
template <uint32_t compute_queue_count> class ComputeEnginePipelineBase
{
  protected:
    uint32_t m_computeQueueFamilyIndex;
    uint32_t m_computeQueueStartIndex;
    vk::Queue m_computeQueues[compute_queue_count];

    void SetupCompute(const vk::UniqueDevice &device, vk::PhysicalDeviceMemoryProperties phyDevMemProps,
                      const uint32_t computeQueueFamilyIndex, const uint32_t computeQueueStartIndex)
    {
        m_computeQueueFamilyIndex = computeQueueFamilyIndex;
        m_computeQueueStartIndex = computeQueueStartIndex;
    }

    void CreateComputeQueues(const vk::UniqueDevice &device)
    {
        for (int i = 0; i < compute_queue_count; i++)
        {
            m_computeQueues[i] = device.get().getQueue(m_computeQueueFamilyIndex, m_computeQueueStartIndex + i);
        }
    }
};

template <uint32_t descriptor_set_count, uint32_t image_count, uint32_t buffer_count, uint32_t sampler_count,
          uint32_t graphics_queue_count, uint32_t compute_queue_count, uint32_t push_constant_count = 0>
class EnginePipeline : public EnginePipelineBase,
                       public GraphicsEnginePipelineBase<graphics_queue_count>,
                       public ComputeEnginePipelineBase<compute_queue_count>
{
  public:
    virtual void Setup(const vk::UniqueDevice &device, vk::PhysicalDeviceMemoryProperties phyDevMemProps,
                       const uint32_t graphicsQueueFamilyIndex, const uint32_t computeQueueFamilyIndex,
                       const uint32_t graphicsQueueStartIndex, const uint32_t computeQueueStartIndex,
                       const vk::UniqueRenderPass &renderPass) override
    {
        if constexpr (graphics_queue_count > 0)
            GraphicsEnginePipelineBase<graphics_queue_count>::SetupGraphics(
                device, phyDevMemProps, graphicsQueueFamilyIndex, graphicsQueueStartIndex);

        if constexpr (compute_queue_count > 0)
            ComputeEnginePipelineBase<compute_queue_count>::SetupCompute(
                device, phyDevMemProps, computeQueueFamilyIndex, computeQueueStartIndex);

        EnginePipelineBase::Setup(device, phyDevMemProps, graphicsQueueFamilyIndex, computeQueueFamilyIndex,
                                  graphicsQueueStartIndex, computeQueueStartIndex, renderPass);
    }

    constexpr const QueueRequirements_t GetQueueRequirements() const override
    {
        return QueueRequirements{graphics_queue_count, compute_queue_count};
    }

    void PauseRender()
    {
        m_pauseSignal = true;
        while (m_paused == false)
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    const bool IsPaused() const
    {
        return m_paused;
    };

  protected:
    bool m_pauseSignal;
    bool m_paused;

  protected:
    void CleanupEnginePipeline(const vk::UniqueDevice &device) override
    {
        for (const auto &descriptorSetLayout : m_descriptorSetLayouts)
            device->destroyDescriptorSetLayout(descriptorSetLayout);

        device->destroyDescriptorPool(m_descriptorPool);

        for (const auto &samplerDesc : m_samplerDescriptors)
            device->destroySampler(samplerDesc.sampler);

        for (const auto &imageView : m_imageViews)
            device->destroyImageView(imageView);

        for (const auto &image : m_images)
            device->destroyImage(image);

        for (const auto &bufferDescriptor : m_bufferDescriptors)
            device->destroyBuffer(bufferDescriptor.buffer);

        for (const auto &devMem : m_devMemBufferAllocs)
        {
            device->unmapMemory(devMem);
            device->freeMemory(devMem);
        }

        for (const auto &devMem : m_devMemAllocs)
        {
            device->freeMemory(devMem);
        }
    };

    uint32_t GetMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags requirements_mask)
    {
        // Search memtypes to find first index with those properties
        for (uint32_t i = 0; i < m_phyDevMemProps.memoryTypeCount; i++)
        {
            if ((typeBits & 1) == 1)
            {
                // Type is available, does it match user properties?
                if ((m_phyDevMemProps.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
                {
                    return i;
                }
            }
            typeBits >>= 1;
        }
        // No memory types matched, return failure
        return false;
    }

    const DescriptorSetID AddDescriptorLayoutSets(const vk::UniqueDevice &device,
                                                  vk::DescriptorSetLayoutCreateInfo &newDescriptorSetLayoutCreateInfo)
    {
        const uint32_t setID = m_currentDescriptorSet++;

        m_descriptorSetLayouts[setID] = device->createDescriptorSetLayout(newDescriptorSetLayoutCreateInfo);

        for (uint32_t i = 0; i < newDescriptorSetLayoutCreateInfo.bindingCount; i++)
        {
            vk::DescriptorType descType = newDescriptorSetLayoutCreateInfo.pBindings[i].descriptorType;

            if (m_descriptorPoolSizes.count(descType) == 0)
                m_descriptorPoolSizes.emplace(descType, vk::DescriptorPoolSize(descType));

            m_descriptorPoolSizes.at(descType).descriptorCount++;
        }

        return setID;
    }

    vk::ImageView CreateNewImage(const vk::UniqueDevice &device, ImageID_t imageID, uint32_t bindingLocation,
                                 std::vector<DescriptorSetID> descSetIDs, vk::ImageCreateInfo &newImageCreateInfo,
                                 vk::ImageSubresourceRange newImageSubresourceRange, vk::ImageLayout finalLayout,
                                 vk::Sampler imageSampler)
    {
        vk::Image newImage = device->createImage(newImageCreateInfo);

        m_images[imageID] = newImage;

        vk::MemoryRequirements memReqs = device->getImageMemoryRequirements(newImage);
        vk::MemoryAllocateInfo newImageMemAlloc{
            memReqs.size, GetMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)};

        vk::DeviceMemory newImageDevMem = device->allocateMemory(newImageMemAlloc);

        m_devMemAllocs.push_back(newImageDevMem);

        device->bindImageMemory(newImage, newImageDevMem, 0);

        vk::ImageViewType newImageViewType;

        switch (newImageCreateInfo.imageType)
        {
        case (vk::ImageType::e1D): {
            newImageViewType = vk::ImageViewType::e1D;
            break;
        }
        case (vk::ImageType::e2D): {
            newImageViewType = vk::ImageViewType::e2D;
            break;
        }
        case (vk::ImageType::e3D): {
            newImageViewType = vk::ImageViewType::e3D;
            break;
        }
        }

        vk::ImageViewCreateInfo newImageViewCreateInfo{vk::ImageViewCreateFlags{},
                                                       newImage,
                                                       newImageViewType,
                                                       newImageCreateInfo.format,
                                                       {} /*vk::ComponentMapping{vk::ComponentSwizzle::eR,
                                                             vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB,
                                                                             vk::ComponentSwizzle::eA}*/
                                                       ,
                                                       newImageSubresourceRange};

        vk::ImageView newImageView = device->createImageView(newImageViewCreateInfo);

        m_imageViews.push_back(newImageView);

        if (descSetIDs[0] != INVALID_DESCRIPTOR_SET_ID)
        {
            for (const auto &descSetID : descSetIDs)
            {
                vk::DescriptorType type;
                if (imageSampler)
                    type = vk::DescriptorType::eCombinedImageSampler;
                else
                    type = vk::DescriptorType::eStorageImage;
                DescriptorInfo_t imageDescInfo{
                    type, bindingLocation, {}, vk::DescriptorImageInfo{imageSampler, newImageView, finalLayout}};
                m_descriptorSetObjects[descSetID][bindingLocation] = imageDescInfo;
            }
        }

        return newImageView;
    }
    vk::ImageView CreateNewImage(const vk::UniqueDevice &device, uint32_t imageID, uint32_t bindingLocation,
                                 DescriptorSetID descSetID, vk::ImageCreateInfo &newImageCreateInfo,
                                 vk::ImageSubresourceRange newImageSubresourceRange, vk::ImageLayout finalLayout,
                                 vk::Sampler imageSampler = {})
    {
        return CreateNewImage(device, imageID, bindingLocation, std::vector<DescriptorSetID>{descSetID},
                              newImageCreateInfo, newImageSubresourceRange, finalLayout, imageSampler);
    }

    EngineBuffer_t CreateNewBuffer(const vk::UniqueDevice &device, BufferID_t bufferID, uint32_t bindingLocation,
                                   std::vector<DescriptorSetID> descSetIDs, vk::BufferCreateInfo &newBufferCreateInfo,
                                   void **hostBuffer, vk::MemoryPropertyFlags memProps, bool requiresBarrier = false)
    {
        vk::Buffer newBuffer = device->createBuffer(newBufferCreateInfo);

        vk::MemoryRequirements memReqs = device->getBufferMemoryRequirements(newBuffer);

        // auto memProps = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

        vk::MemoryAllocateInfo newBufferMemAlloc{memReqs.size, GetMemoryType(memReqs.memoryTypeBits, memProps)};

        vk::DeviceMemory newBufferDevMem = device->allocateMemory(newBufferMemAlloc);
        m_devMemBufferAllocs.push_back(newBufferDevMem);

        if (vk::MemoryPropertyFlagBits::eHostVisible & memProps)
        {
            auto memMapResult = device->mapMemory(newBufferDevMem, 0, VK_WHOLE_SIZE, vk::MemoryMapFlags(), hostBuffer);

            if (memMapResult != vk::Result::eSuccess)
            {
                printf("device mapMemory failed: %s ", vk::to_string(memMapResult).c_str());
            }
        }

        m_bufferDescriptors[bufferID] = vk::DescriptorBufferInfo(newBuffer, 0, memReqs.size);
        if (descSetIDs[0] != INVALID_DESCRIPTOR_SET_ID)
        {
            for (const auto &descSetID : descSetIDs)
            {
                DescriptorInfo_t bufferDescInfo{vk::DescriptorType::eUniformBuffer, bindingLocation,
                                                vk::DescriptorBufferInfo{newBuffer, 0, newBufferCreateInfo.size}};
                m_descriptorSetObjects[descSetID][bindingLocation] = bufferDescInfo;
            }
        }

        return EngineBuffer_t{newBuffer, newBufferDevMem};
    }
    EngineBuffer_t CreateNewBuffer(const vk::UniqueDevice &device, BufferID_t bufferID, uint32_t bindingLocation,
                                   DescriptorSetID descSetID, vk::BufferCreateInfo &newBufferCreateInfo,
                                   void **hostBuffer, vk::MemoryPropertyFlags memProps, bool requiresBarrier = false)
    {
        return CreateNewBuffer(device, bufferID, bindingLocation, std::vector<DescriptorSetID>{descSetID},
                               newBufferCreateInfo, hostBuffer, memProps, requiresBarrier);
    }
    EngineBuffer_t CreateNewBuffer(const vk::UniqueDevice &device, BufferID_t bufferID,
                                   vk::BufferCreateInfo &newBufferCreateInfo, void **hostBuffer,
                                   vk::MemoryPropertyFlags memProps, bool requiresBarrier = false)
    {
        return CreateNewBuffer(device, bufferID, UINT32_MAX, std::vector<DescriptorSetID>{INVALID_DESCRIPTOR_SET_ID},
                               newBufferCreateInfo, hostBuffer, memProps, requiresBarrier);
    }

    vk::Sampler CreateNewSampler(const vk::UniqueDevice &device, SamplerID_t samplerID, uint32_t bindingLocation,
                                 std::vector<DescriptorSetID> descSetIDs, vk::SamplerCreateInfo &newSamplerCreateInfo,
                                 bool requiresBarrier = false)
    {

        vk::Sampler sampler = device->createSampler(newSamplerCreateInfo);
        m_samplerDescriptors[samplerID] = vk::DescriptorImageInfo{sampler};

        if (descSetIDs[0] != INVALID_DESCRIPTOR_SET_ID)
        {
            for (const auto &descSetID : descSetIDs)
            {
                DescriptorInfo_t samplerDescInfo{
                    vk::DescriptorType::eCombinedImageSampler, bindingLocation, {}, vk::DescriptorImageInfo{sampler}};
                m_descriptorSetObjects[descSetID][bindingLocation] = samplerDescInfo;
            }
        }

        return sampler;
    }

    vk::Sampler CreateNewSampler(const vk::UniqueDevice &device, SamplerID_t samplerID, uint32_t bindingLocation,
                                 DescriptorSetID descSetID, vk::SamplerCreateInfo &newSamplerCreateInfo,
                                 bool requiresBarrier = false)
    {
        return CreateNewSampler(device, samplerID, bindingLocation, std::vector<DescriptorSetID>{descSetID},
                                newSamplerCreateInfo, requiresBarrier);
    }
    vk::Sampler CreateNewSampler(const vk::UniqueDevice &device, SamplerID_t samplerID,
                                 vk::SamplerCreateInfo &newSamplerCreateInfo, bool requiresBarrier = false)
    {
        return CreateNewSampler(device, samplerID, UINT32_MAX, std::vector<DescriptorSetID>{INVALID_DESCRIPTOR_SET_ID},
                                newSamplerCreateInfo, requiresBarrier);
    }

    void SetupPipeline(const vk::UniqueDevice &device)
    {

        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), m_descriptorSetLayouts,
                                                              m_pushConstantRanges);

        m_pipelineLayout = device->createPipelineLayoutUnique(pipelineLayoutCreateInfo);

        std::vector<vk::DescriptorPoolSize> descPoolSizes;

        for (std::pair<vk::DescriptorType, vk::DescriptorPoolSize> descPoolEntry : m_descriptorPoolSizes)
        {
            descPoolSizes.push_back(descPoolEntry.second);
        }

        vk::DescriptorPoolCreateInfo descPoolCreateInfo{vk::DescriptorPoolCreateFlags(), (uint32_t)m_descriptorSetCount,
                                                        (uint32_t)descPoolSizes.size(), descPoolSizes.data()};

        m_descriptorPool = device->createDescriptorPool(descPoolCreateInfo);

        vk::DescriptorSetAllocateInfo descSetAllocInfo{m_descriptorPool, m_descriptorSetLayouts};
        m_descriptorSets = device->allocateDescriptorSets(descSetAllocInfo);

        std::vector<vk::WriteDescriptorSet> writeDescriptorSets;

        for (uint32_t descSetID = 0; descSetID < descriptor_set_count; descSetID++)
        {
            auto const &descriptorSetObjects = m_descriptorSetObjects[descSetID];
            for (auto const &descriptorSetObject : descriptorSetObjects)
            {
                if (descriptorSetObject.bindLoc != UINT32_MAX)
                {
                    const vk::DescriptorImageInfo *imageInfo =
                        (descriptorSetObject.type == vk::DescriptorType::eCombinedImageSampler) ||
                                (descriptorSetObject.type == vk::DescriptorType::eStorageImage)
                            ? &descriptorSetObject.image
                            : nullptr;
                    const vk::DescriptorBufferInfo *buffInfo =
                        (descriptorSetObject.type == vk::DescriptorType::eUniformBuffer) ? &descriptorSetObject.buffer
                                                                                         : nullptr;
                    writeDescriptorSets.push_back(
                        vk::WriteDescriptorSet{m_descriptorSets[descSetID], descriptorSetObject.bindLoc, 0, 1,
                                               descriptorSetObject.type, imageInfo, buffInfo, nullptr});
                }
            }
        }

        device->updateDescriptorSets(writeDescriptorSets, nullptr);

        vk::PipelineCacheCreateInfo pipelineCacheCreateInfo{vk::PipelineCacheCreateFlags{}, 0, nullptr};

        m_pipelineCache = device->createPipelineCacheUnique(pipelineCacheCreateInfo);
    }

  protected:
    vk::CommandPool m_cmdPool;
    vk::CommandBuffer m_cmdBuffer;

    const uint32_t m_descriptorSetCount = descriptor_set_count;
    uint32_t m_currentDescriptorSet = 0;

    std::vector<vk::DeviceMemory> m_devMemBufferAllocs;
    std::vector<vk::DeviceMemory> m_devMemAllocs;

    std::array<vk::Image, image_count> m_images;
    std::vector<vk::ImageView> m_imageViews;

    std::array<vk::DescriptorBufferInfo, buffer_count + image_count> m_bufferDescriptors;
    std::array<vk::DescriptorImageInfo, sampler_count> m_samplerDescriptors;

    std::array<std::array<DescriptorInfo_t, image_count + buffer_count + sampler_count>, descriptor_set_count>
        m_descriptorSetObjects;

    vk::UniquePipelineLayout m_pipelineLayout;
    vk::UniquePipelineCache m_pipelineCache;
    vk::UniquePipeline m_pipeline;
    std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;

    std::array<vk::DescriptorSetLayout, descriptor_set_count> m_descriptorSetLayouts;
    std::array<vk::PushConstantRange, push_constant_count> m_pushConstantRanges;

    std::map<vk::DescriptorType, vk::DescriptorPoolSize> m_descriptorPoolSizes;
    vk::DescriptorPool m_descriptorPool;

    std::vector<vk::DescriptorSet> m_descriptorSets;

    // Pipeline creation variables start
    vk::VertexInputBindingDescription vertexInputBindingDesc{0, 0, vk::VertexInputRate::eVertex};

    std::vector<vk::VertexInputAttributeDescription> vertexInputAttrDesc;

    std::array<vk::DynamicState, 2> dynamicStateEnables{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicStateCI{vk::PipelineDynamicStateCreateFlags{}, dynamicStateEnables};

    vk::PipelineVertexInputStateCreateInfo vertexInfoStateCI{vk::PipelineVertexInputStateCreateFlags{},
                                                             vertexInputBindingDesc, vertexInputAttrDesc};

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{vk::PipelineInputAssemblyStateCreateFlags{},
                                                                  vk::PrimitiveTopology::eTriangleList, false};

    vk::PipelineRasterizationStateCreateInfo rasterStateCI{vk::PipelineRasterizationStateCreateFlags{},
                                                           false,
                                                           false,
                                                           vk::PolygonMode::eFill,
                                                           vk::CullModeFlagBits::eBack,
                                                           vk::FrontFace::eClockwise,
                                                           false,
                                                           0,
                                                           0,
                                                           0,
                                                           1.f};

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState();

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCI{vk::PipelineColorBlendStateCreateFlags{}, false,
                                                            vk::LogicOp::eClear, colorBlendAttachmentState,
                                                            std::array<float, 4>{1.f, 1.f, 1.f, 1.f}};

    vk::PipelineViewportStateCreateInfo viewportStateCI{vk::PipelineViewportStateCreateFlags{}, 1, nullptr, 1, nullptr};

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCI{
        vk::PipelineDepthStencilStateCreateFlags{},
        VK_TRUE,
        VK_TRUE,
        vk::CompareOp::eLessOrEqual,
        VK_FALSE,
        VK_FALSE,
        vk::StencilOpState{vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways, 0,
                           0, 0},
        vk::StencilOpState{vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways, 0,
                           0, 0}};

    vk::PipelineMultisampleStateCreateInfo multisampleStateCI{vk::PipelineMultisampleStateCreateFlags{},
                                                              vk::SampleCountFlagBits::e1,
                                                              VK_FALSE,
                                                              0.0,
                                                              nullptr,
                                                              VK_FALSE,
                                                              VK_FALSE};

    // Pipeline creation variables end
};

} // namespace Pipelines
} // namespace Engine

#endif