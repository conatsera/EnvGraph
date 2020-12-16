#pragma once
#ifndef ENGINE_PIPELINES_H
#define ENGINE_PIPELINES_H

#include <cinttypes>
#include <vector>
#include <map>

#include <vulkan/vulkan.hpp>

#include "engine_defs.h"

namespace Engine {
namespace Pipelines {

typedef struct QueueRequirements {
  uint32_t graphics;
  uint32_t compute;
} QueueRequirements_t;

class EnginePipelineBase {
 public:
  virtual void Setup(const vk::UniqueDevice& device,
                     vk::PhysicalDeviceMemoryProperties phyDevMemProps,
                     const uint32_t graphicsQueueFamilyIndex,
                     const uint32_t computeQueueFamilyIndex,
                     const uint32_t graphicsQueueStartIndex,
                     const uint32_t computeQueueStartIndex) {}

  virtual void CleanupEnginePipeline(const vk::UniqueDevice& device) {}

  virtual const QueueRequirements_t GetQueueRequirements() const {
    return {0, 0};
  }

  const bool IsSetup() const { return m_setupComplete; }

 protected:
  vk::PhysicalDeviceMemoryProperties m_phyDevMemProps;
  bool m_setupComplete = false;
};

template <uint32_t graphics_queue_count>
class GraphicsEnginePipelineBase : public EnginePipelineBase {
 protected:
  uint32_t m_graphicsQueueFamilyIndex;
  uint32_t m_graphicsQueueStartIndex;
  vk::Queue m_graphicsQueues[graphics_queue_count];

  void CreateGraphicsQueues(const vk::UniqueDevice& device) {
    for (int i = 0; i < graphics_queue_count; i++) {
      m_graphicsQueues[i] = device.get().getQueue(
          m_graphicsQueueFamilyIndex, m_graphicsQueueStartIndex + i);
    }
  }
};
template <uint32_t compute_queue_count>
class ComputeEnginePipelineBase : public EnginePipelineBase {
 protected:
  uint32_t m_computeQueueFamilyIndex;
  uint32_t m_computeQueueStartIndex;
  vk::Queue m_computeQueues[compute_queue_count];

  void CreateComputeQueues(const vk::UniqueDevice& device) {
    for (int i = 0; i < compute_queue_count; i++) {
      m_computeQueues[i] = device.get().getQueue(m_computeQueueFamilyIndex,
                                                 m_computeQueueStartIndex + i);
    }
  }
};

template <uint32_t descriptor_set_count, uint32_t image_count,
          uint32_t buffer_count, uint32_t graphics_queue_count,
          uint32_t compute_queue_count>
class EnginePipeline
    : public EnginePipelineBase,
      public std::enable_if<(graphics_queue_count > 0),
                            GraphicsEnginePipelineBase<graphics_queue_count>>,
      public std::enable_if<(compute_queue_count > 0),
                            ComputeEnginePipelineBase<compute_queue_count>> {
public:
  const QueueRequirements_t GetQueueRequirements() const override {
    return QueueRequirements{graphics_queue_count, compute_queue_count};
  }

 protected:
  vk::DeviceMemory CreateMemory(
      const vk::UniqueDevice& device, const vk::MemoryRequirements memReqs, 

                    const vk::UniqueDeviceMemory& deviceMemory) const {
    vk::MemoryAllocateInfo newImageMemAlloc{
        memReqs.size,
        };

    __attribute__((unused)) vk::DeviceMemory newImageDevMem =
        device->allocateMemory(newImageMemAlloc);
  }

  void CleanupEnginePipeline(const vk::UniqueDevice& device) override {
    for (const auto& image : m_images) device->destroyImage(image);

    for (const auto& bufferDescriptor : m_bufferDescriptors)
        device->destroyBuffer(bufferDescriptor.buffer);

    for (const auto& devMem : m_devMemBufferAllocs) {
      device->unmapMemory(devMem);
      device->freeMemory(devMem);
    }

    for (const auto& devMem : m_devMemAllocs) {
      device->freeMemory(devMem);
    }
  };

  uint32_t GetMemoryType(uint32_t typeBits,
                                 vk::MemoryPropertyFlags requirements_mask) {
    // Search memtypes to find first index with those properties
    for (uint32_t i = 0; i < m_phyDevMemProps.memoryTypeCount; i++) {
      if ((typeBits & 1) == 1) {
        // Type is available, does it match user properties?
        if ((m_phyDevMemProps.memoryTypes[i].propertyFlags &
             requirements_mask) == requirements_mask) {
          return i;
        }
      }
      typeBits >>= 1;
    }
    // No memory types matched, return failure
    return false;
  }
   
  const DescriptorSetID AddDescriptorLayoutSets(
      const vk::UniqueDevice& device,
      vk::DescriptorSetLayoutCreateInfo& newDescriptorSetLayoutCreateInfo) {
    const uint32_t setID = m_currentDescriptorSet++;

    m_descriptorSetLayouts[setID] =
        device->createDescriptorSetLayout(newDescriptorSetLayoutCreateInfo);

    for (int i = 0; i < newDescriptorSetLayoutCreateInfo.bindingCount; i++) {
      vk::DescriptorType descType =
          newDescriptorSetLayoutCreateInfo.pBindings[i].descriptorType;

      if (m_descriptorPoolSizes.count(descType) == 0)
        m_descriptorPoolSizes.emplace(descType,
                                      vk::DescriptorPoolSize(descType));

      m_descriptorPoolSizes.at(descType).descriptorCount++;
    }

    return setID;
  }

  vk::ImageView CreateNewImage(
      const vk::UniqueDevice& device, uint32_t imageID,
      std::vector<DescriptorSetID> descSetIDs,
      vk::ImageCreateInfo& newImageCreateInfo,
      vk::ImageSubresourceRange newImageSubresourceRange) {
    vk::Image newImage = device->createImage(newImageCreateInfo);

    m_images[imageID] = newImage;
    for (const auto& descSetID : descSetIDs)
      m_imageMapping[descSetID][m_imageMapCount[descSetID]++] = imageID;

    vk::MemoryRequirements memReqs =
        device->getImageMemoryRequirements(newImage);
    vk::MemoryAllocateInfo newImageMemAlloc{
        memReqs.size, GetMemoryType(memReqs.memoryTypeBits,
                                    vk::MemoryPropertyFlagBits::eDeviceLocal)};

    vk::DeviceMemory newImageDevMem = device->allocateMemory(newImageMemAlloc);

    m_devMemAllocs.push_back(newImageDevMem);

    device->bindImageMemory(newImage, newImageDevMem, 0);

    vk::ImageViewType newImageViewType;

    switch (newImageCreateInfo.imageType) {
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

    vk::ImageViewCreateInfo newImageViewCreateInfo{
        vk::ImageViewCreateFlags{},
        newImage,
        newImageViewType,
        newImageCreateInfo.format,
        vk::ComponentMapping{vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                             vk::ComponentSwizzle::eB,
                             vk::ComponentSwizzle::eA},
        newImageSubresourceRange};

    vk::ImageView newImageView =
        device->createImageView(newImageViewCreateInfo);

    m_imageViews.push_back(newImageView);

    return newImageView;
  }
  vk::ImageView CreateNewImage(
      const vk::UniqueDevice& device, uint32_t imageID,
      DescriptorSetID descSetID, vk::ImageCreateInfo& newImageCreateInfo,
      vk::ImageSubresourceRange newImageSubresourceRange) {
    return CreateNewImage(device, imageID,
        std::vector<DescriptorSetID>{descSetID},
                          newImageCreateInfo, newImageSubresourceRange);
  }

  void CreateNewBuffer(const vk::UniqueDevice& device, uint32_t bufferID,
      std::vector<DescriptorSetID> descSetIDs,
      vk::BufferCreateInfo& newBufferCreateInfo,
      void* hostBuffer, bool requiresBarrier = false) {
    vk::Buffer newBuffer = device->createBuffer(newBufferCreateInfo);

    vk::MemoryRequirements memReqs =
        device->getBufferMemoryRequirements(newBuffer);

    m_bufferDescriptors[bufferID] =
      vk::DescriptorBufferInfo(newBuffer, 0, newBufferCreateInfo.size);
    for (const auto& descSetID : descSetIDs)
      m_bufferMapping[descSetID][m_bufferMapCount[descSetID]++] = bufferID;

    vk::MemoryAllocateInfo newBufferMemAlloc{
        memReqs.size,
        GetMemoryType(memReqs.memoryTypeBits,
                      vk::MemoryPropertyFlagBits::eHostVisible |
                          vk::MemoryPropertyFlagBits::eHostCoherent)};

    vk::DeviceMemory newBufferDevMem =
        device->allocateMemory(newBufferMemAlloc);
    m_devMemBufferAllocs.push_back(newBufferDevMem);

    vk::Result memMapResult = device->mapMemory(newBufferDevMem, 0, memReqs.size, vk::MemoryMapFlags(),
                      &hostBuffer);

    if (memMapResult != vk::Result::eSuccess)
    {
      //addLogEntry(OMRLogLevel::OMR_ERROR, OMRLogOrigin::RENDERER, "device mapMemory failed: %s ", vk::to_string(memMapResult).c_str());
      printf("device mapMemory failed: %s ", vk::to_string(memMapResult).c_str());
    }

    device->bindBufferMemory(newBuffer, newBufferDevMem, 0);
  }
  void CreateNewBuffer(const vk::UniqueDevice& device, uint32_t bufferID,
                       DescriptorSetID descSetID,
                       vk::BufferCreateInfo& newBufferCreateInfo,
                       void* hostBuffer, bool requiresBarrier = false) {
    CreateNewBuffer(device, bufferID, std::vector<DescriptorSetID>{descSetID},
                    newBufferCreateInfo, hostBuffer, requiresBarrier);
  }
  void CreateNewBuffer(const vk::UniqueDevice& device, uint32_t bufferID,
                       vk::BufferCreateInfo& newBufferCreateInfo,
                       void* hostBuffer, bool requiresBarrier = false) {
    CreateNewBuffer(device, bufferID,
                    std::vector<DescriptorSetID>{INVALID_DESCRIPTOR_SET_ID},
                    newBufferCreateInfo, hostBuffer, requiresBarrier);
  }

  void SetupPipeline(const vk::UniqueDevice& device) {
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(
        vk::PipelineLayoutCreateFlags(), m_descriptorSetCount,
        m_descriptorSetLayouts);

    m_pipelineLayout = device->createPipelineLayout(pipelineLayoutCreateInfo);

    std::vector<vk::DescriptorPoolSize> descPoolSizes;

    for (std::pair<vk::DescriptorType, vk::DescriptorPoolSize> descPoolEntry :
         m_descriptorPoolSizes) {
      descPoolSizes.push_back(descPoolEntry.second);
    }

    vk::DescriptorPoolCreateInfo descPoolCreateInfo{
        vk::DescriptorPoolCreateFlags(), (uint32_t)m_descriptorSetCount,
        (uint32_t)descPoolSizes.size(), descPoolSizes.data()};

    m_descriptorPool = device->createDescriptorPool(descPoolCreateInfo);

    vk::DescriptorSetAllocateInfo descSetAllocInfo{
        m_descriptorPool, m_descriptorSetCount, m_descriptorSetLayouts};
    m_descriptorSets = device->allocateDescriptorSets(descSetAllocInfo);

    std::vector<vk::WriteDescriptorSet> writeDescriptorSets;

    for (uint32_t setIndex = 0; setIndex < m_descriptorSetCount; setIndex++) {
      const auto& descriptorSet = m_descriptorSets.at(setIndex);
      uint32_t setSize = m_bufferMapCount[setIndex];
      for (uint32_t i = 0; i < setSize; i++) {
        writeDescriptorSets.push_back(vk::WriteDescriptorSet{
            descriptorSet,
            i,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &m_bufferDescriptors[m_bufferMapping[setIndex][i]],
            nullptr,
        });
      }
    }

    device->updateDescriptorSets(writeDescriptorSets, nullptr);
  }

 private:
  vk::CommandPool m_cmdPool;
  vk::CommandBuffer m_cmdBuffer;

  const uint32_t m_descriptorSetCount = descriptor_set_count;
  uint32_t m_currentDescriptorSet = 0;

  std::vector<vk::DeviceMemory> m_devMemBufferAllocs;
  std::vector<vk::DeviceMemory> m_devMemAllocs;

  std::vector<vk::ImageView> m_imageViews;

  vk::DescriptorBufferInfo m_bufferDescriptors[buffer_count];
  vk::Image m_images[image_count];

  // TODO: Benchmark large image and buffer counts. These "maps" create a lot
  //       of wasted entries, but it saves from runtime vector allocation.
  //       More importantly the code to handle mapping is simplified.
  uint32_t m_bufferMapCount[descriptor_set_count];
  uint32_t m_imageMapCount[descriptor_set_count];
  uint32_t m_bufferMapping[descriptor_set_count][image_count];
  uint32_t m_imageMapping[descriptor_set_count][buffer_count];

  vk::PipelineLayout m_pipelineLayout;

  vk::DescriptorSetLayout m_descriptorSetLayouts[descriptor_set_count];

  std::map<vk::DescriptorType, vk::DescriptorPoolSize> m_descriptorPoolSizes;
  vk::DescriptorPool m_descriptorPool;

  std::vector<vk::DescriptorSet> m_descriptorSets;
};

} // namespace Pipelines
} // namespace Engine

#endif