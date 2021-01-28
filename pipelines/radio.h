#pragma once
#ifndef GRAPHICS_PIPELINES_RADIO_H
#define GRAPHICS_PIPELINES_RADIO_H

#include <fftw3.h>
#include <lime/LimeSuite.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

#include "../pipelines.h"

constexpr const int kMaxBufferSize = 512; // complex samples per buffer

namespace Engine
{
namespace Pipelines
{

constexpr const size_t kMaxBufferBytes = sizeof(float) * kMaxBufferSize * 2;
constexpr const size_t kMaxSignalBufferBytes = sizeof(float) * kMaxBufferSize * 2;
constexpr const size_t kMaxComplexBufferBytes = sizeof(fftw_complex) * kMaxBufferSize;

constexpr const size_t kMaxTimeDepth = 2048;

enum RadioBufferIDs : BufferID_t
{
    kRadioMvpBufferID = 0,
    kFFTVertBufferID = 1,
    kFFTBufferID = 2
};

enum RadioImageIDs : ImageID_t
{
    kRadioFFTImage = 0
};

enum RadioRenderMode : size_t
{
    HIGH_RESOLUTION = kMaxBufferSize,
    MAX_THROUGHPUT
};

template<const size_t buffer_size>
struct RadioRenderSizes
{
    const size_t bufferSize = buffer_size;
    const size_t complexBufferBytes = sizeof(fftw_complex) * buffer_size;
    const size_t signalBufferBytes = sizeof(float) * buffer_size;
};

template<const size_t buffer_size, const size_t avg_count>
struct AverageBuffer
{
    float ab[avg_count][buffer_size];
};

class RadioPipeline : public EnginePipeline<1, 1, 3, 0, 1, 0>
{
  public:
    virtual void Setup(const vk::UniqueDevice &device, vk::PhysicalDeviceMemoryProperties phyDevMemProps,
                       const uint32_t graphicsQueueFamilyIndex, const uint32_t computeQueueFamilyIndex,
                       const uint32_t graphicsQueueStartIndex, const uint32_t computeQueueStartIndex,
                       const vk::UniqueRenderPass &renderPass) override;

    virtual void CleanupEnginePipeline(const vk::UniqueDevice& device) override;

    void SetupShaders(const vk::UniqueDevice &device)
    {
        SetupVertexBuffer(device);
        // SetupFragmentBuffer(device);
    };

    virtual void PreRender(const vk::Device &device, vk::Extent2D windowExtents) override;

    virtual void Render(const vk::Device &device, const vk::CommandBuffer &cmdBuffer,
                        vk::Extent2D windowExtents) override;

    void ConfigureStream(lms_stream_t streamId)
    {
        m_streamId = streamId;
    };

    void SetFrequency(const double newFrequency)
    {
        m_frequency = newFrequency;
    };

    const double GetBandwidth()
    {
        return m_bandwidth;
    };

    void SetBandwidth(const double newBandwidth)
    {
        m_bandwidth = newBandwidth;
    };

    const double GetLowPassFilter()
    {
        return m_lpFilter;
    };

    void SetLowPassFilter(const double newLPFilter)
    {
        m_lpFilter = newLPFilter;
    };

    const double GetOversamplingRatio()
    {
        return m_oversampleRatio;
    };

    void SetOversamplingRatio(const double newOR)
    {
        m_oversampleRatio = newOR;
    };

    void ChangeRadioRenderMode(RadioRenderMode newMode);

    void ChangeView(glm::highp_mat4 newMvp)
    {
        m_settings.mvp = newMvp;
    }

  private:
    void CreateViewModelProjection(const vk::UniqueDevice &device);
    void CreateShaders(const vk::UniqueDevice &device);

    void SetupVertexBuffer(const vk::UniqueDevice &device);
    void SetupFragmentBuffer(const vk::UniqueDevice &device);

  private:
    //size_t m_avgCount = 4;
    size_t m_timeDepth = 2048;

    bool m_shutdownSignal = false;
    //std::mutex m_fftCalcLock;

    std::thread m_mainRadioThread;
    std::thread m_averageThreadTop;
    std::thread m_averageThreadBot;

    RadioRenderMode m_radioRenderMode = HIGH_RESOLUTION;
    struct
    {
        glm::highp_mat4 mvp;
        float fftBins;
        float windowWidth;
        float strengthOffset;
        uint32_t currentTimePos;
    } m_settings;
    uint8_t *m_mvpBuffer = nullptr;

    std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;

    vk::UniqueShaderModule m_radioVertShaderModule;
    vk::UniqueShaderModule m_radioFragShaderModule;

    EngineBuffer_t m_radioSignalVertEngBuf;
    EngineBuffer_t m_radioSignalEngBuf;

    float *m_radioSignalData;

    lms_stream_t m_streamId;

    double m_frequency = 0;
    double m_bandwidth = 0;
    double m_lpFilter = 0;
    uint32_t m_oversampleRatio = 0;
};

} // namespace Pipelines
} // namespace Engine

#endif