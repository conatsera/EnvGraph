#include "radio.h"

#include <experimental/coroutine>
#include <iostream>
#include <semaphore>

#include "../engine.h"
#include "../engine_defs.h"

namespace Engine
{
namespace Pipelines
{

template <const size_t buffer_size, const size_t avg_count, const int fft_shift_amount, const bool top,
          const bool max_hold, const bool prep = false>
auto AverageFunction(uint32_t currentAvgPos, fftw_complex *output, float *radioSignalData, float **radioSignalAvgBuffer)
{
    for (int i = top ? 0 : fft_shift_amount; i < (top ? fft_shift_amount : buffer_size); i++)
    {
        uint64_t sigPointIndex;
        if constexpr (top)
        {
            sigPointIndex = (i + fft_shift_amount);
        }
        else
        {
            sigPointIndex = (i - fft_shift_amount);
        }

        radioSignalAvgBuffer[currentAvgPos][sigPointIndex] =
            std::log10(std::sqrt((output[i][0] * output[i][0]) + (output[i][1] * output[i][1])));

        /*
                if constexpr (prep)
                {
                    sigPoint = std::log10(std::sqrt((output[i][0] * output[i][0]) + (output[i][1] * output[i][1])));
                }
                else if (!max_hold)
                {
                    sigPoint =
                        (sigPoint + std::log10(std::sqrt((output[i][0] * output[i][0]) + (output[i][1] *
           output[i][1])))) / 2;
                }
                else
                {
                    sigPoint = std::max(
                        sigPoint, (float)std::log10(std::sqrt((output[i][0] * output[i][0]) + (output[i][1] *
           output[i][1]))));
                }*/
    }
    return true;
};

std::binary_semaphore calcDoneSignal(0), startNextCalcSignal(1);
std::atomic<uint32_t> currentTimePos;

template<const size_t avg_count>
inline void SetLineVertices(float &leftLineEnd, float &rightLineStart, float &sigAvg,
                            uint64_t sigPointIndex, uint32_t currentAvgPos, bool overflown,
                            float ** radioSignalAvgBuffer)
{
    sigAvg = 0;
    for (size_t j = 0; j < avg_count; j++)
        {
            sigAvg += radioSignalAvgBuffer[j][sigPointIndex];
        }

        if (overflown)
        {
            leftLineEnd = (sigAvg / avg_count);
            rightLineStart = (sigAvg / avg_count);
        }
        else
        {
            leftLineEnd = (sigAvg / currentAvgPos);
            rightLineStart = (sigAvg / currentAvgPos);
        }
}

template <const size_t buffer_size, const size_t avg_count, const size_t time_depth>
void CalculateRadioSignal(bool &shutdownSignal, lms_stream_t &streamId,
                          float *radioSignalData)
{
    constexpr const int fft_shift_amount = (buffer_size / 2);

    float *buffer = (float *)malloc(kMaxBufferBytes);
    fftw_complex *input = (fftw_complex *)fftw_malloc(kMaxComplexBufferBytes);
    fftw_complex *output = (fftw_complex *)fftw_malloc(kMaxComplexBufferBytes);
    float **radioSignalAvgBuffer = (float **)malloc(sizeof(float *) * avg_count);
    for (auto i = 0; i < avg_count; i++)
        radioSignalAvgBuffer[i] = (float *)malloc(sizeof(float) * buffer_size);

    uint32_t currentAvgPos = 0;
    bool overflown = false;

    fftw_plan radioFFTPlan = fftw_plan_dft_1d(kMaxBufferSize, input, output, FFTW_FORWARD, FFTW_MEASURE);

    radioSignalData[buffer_size + 1] = 0.f;

    uint32_t timePos = 0;
    while (!shutdownSignal)
    {
        startNextCalcSignal.acquire();

        for (int i = 0; i < buffer_size * 2; i++)
            radioSignalData[i] = 0.f;
        for (int i = 0; i < avg_count; i++)
            for (int j = 0; j < buffer_size; j++)
                radioSignalAvgBuffer[i][j] = 0.f;

        currentAvgPos = 0;
        overflown = false;

        while (timePos == currentTimePos.load())
        {
            LMS_RecvStream(&streamId, buffer, buffer_size, NULL, 1000);

            for (auto i = 0; i < buffer_size / 2; i++)
            {
                input[i][0] = (double)buffer[i * 2];
            }
            for (auto i = 0; i < buffer_size / 2; i++)
                input[i][1] = (double)buffer[1 + (i * 2)];

            fftw_execute(radioFFTPlan);

            AverageFunction<buffer_size, avg_count, fft_shift_amount, true, false>(
                currentAvgPos, output, radioSignalData, radioSignalAvgBuffer);

            AverageFunction<buffer_size, avg_count, fft_shift_amount, false, false>(
                currentAvgPos, output, radioSignalData, radioSignalAvgBuffer);

            currentAvgPos++;
            if (currentAvgPos == avg_count)
            {
                currentAvgPos = 0;
                overflown = true;
            }
        }

        uint64_t vertPointIndex = (fft_shift_amount * 2);
        uint64_t sigPointIndex = (fft_shift_amount);

        float sigAvg = 0;

        SetLineVertices<avg_count>(radioSignalData[vertPointIndex], radioSignalData[vertPointIndex+1],
                                       sigAvg, sigPointIndex, currentAvgPos, overflown, radioSignalAvgBuffer);

        vertPointIndex = 0;
        sigPointIndex = 0;

        SetLineVertices<avg_count>(radioSignalData[vertPointIndex], radioSignalData[vertPointIndex+1],
                                       sigAvg, sigPointIndex, currentAvgPos, overflown, radioSignalAvgBuffer);

        for (int i = 1; i < (fft_shift_amount * 2) + 1; i+=2)
        {
            vertPointIndex = (i + (fft_shift_amount * 2));
            sigPointIndex = (((i-1)/2) + (fft_shift_amount));

            SetLineVertices<avg_count>(radioSignalData[vertPointIndex], radioSignalData[vertPointIndex+1],
                                       sigAvg, sigPointIndex, currentAvgPos, overflown, radioSignalAvgBuffer);
        }

        for (int i = (fft_shift_amount * 2) + 1; i < buffer_size * 2; i+=2)
        {
            vertPointIndex = (i - (fft_shift_amount * 2));
            sigPointIndex = (((i-1)/2) - (fft_shift_amount));

            SetLineVertices<avg_count>(radioSignalData[vertPointIndex], radioSignalData[vertPointIndex+1],
                                       sigAvg, sigPointIndex, currentAvgPos, overflown, radioSignalAvgBuffer);
        }

        timePos = currentTimePos.load();

        calcDoneSignal.release();
    }
}

void RadioPipeline::Setup(const vk::UniqueDevice &device, vk::PhysicalDeviceMemoryProperties phyDevMemProps,
                          const uint32_t graphicsQueueFamilyIndex, const uint32_t computeQueueFamilyIndex,
                          const uint32_t graphicsQueueStartIndex, const uint32_t computeQueueStartIndex,
                          const vk::UniqueRenderPass &renderPass)
{
    m_hasPreRenderStage = true;
    EnginePipeline::Setup(device, phyDevMemProps, graphicsQueueFamilyIndex, computeQueueFamilyIndex,
                          graphicsQueueStartIndex, computeQueueStartIndex, renderPass);
    CreateViewModelProjection(device);

    SetupShaders(device);
    SetupPipeline(device);

    CreateShaders(device);

    m_radioRenderMode = HIGH_RESOLUTION;

    m_mainRadioThread = std::thread(CalculateRadioSignal<HIGH_RESOLUTION, 4, 2048>, std::ref(m_shutdownSignal), std::ref(m_streamId), m_radioSignalData);

    vertexInputBindingDesc.setStride(sizeof(float));

    vertexInputAttrDesc = {{0, 0, vk::Format::eR32Sfloat, 0}};

    vertexInfoStateCI.setVertexAttributeDescriptions(vertexInputAttrDesc);

    inputAssemblyStateCI.setTopology(vk::PrimitiveTopology::eLineList);

    colorBlendAttachmentState.colorWriteMask = (vk::ColorComponentFlags)(0xf);

    vk::GraphicsPipelineCreateInfo graphicsPipelineCI{vk::PipelineCreateFlags{},
                                                      m_shaderStages,
                                                      &vertexInfoStateCI,
                                                      &inputAssemblyStateCI,
                                                      nullptr,
                                                      &viewportStateCI,
                                                      &rasterStateCI,
                                                      &multisampleStateCI,
                                                      &depthStencilStateCI,
                                                      &colorBlendStateCI,
                                                      &dynamicStateCI,
                                                      m_pipelineLayout.get(),
                                                      renderPass.get(),
                                                      0,
                                                      nullptr,
                                                      0};

    m_pipeline = device->createGraphicsPipelineUnique(m_pipelineCache.get(), graphicsPipelineCI).value;
}

void RadioPipeline::CleanupEnginePipeline(const vk::UniqueDevice &device)
{
    m_settings.currentTimePos++;
    currentTimePos++;
    m_shutdownSignal = true;
    __attribute__((unused)) auto test = m_mainRadioThread.joinable();
    if (m_mainRadioThread.joinable())
        m_mainRadioThread.join();

    fftw_cleanup();

    EnginePipelineBase::CleanupEnginePipeline(device);
}

void RadioPipeline::CreateViewModelProjection(const vk::UniqueDevice &device)
{
    auto projection = glm::perspective(glm::radians(45.0F), 1.0F, 0.1F, 100000.0F);
    auto view = glm::lookAt(glm::vec3(0, 0, -100), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0));
    auto model = glm::mat4(1.0F);
    // clang-format off
  /*auto clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f,-1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.5f, 0.0f,
                        0.0f, 0.0f, 0.5f, 1.0f);*/
    // clang-format on

    m_settings.mvp = projection * view * model;
    m_settings.fftBins = kMaxBufferSize;
    m_settings.windowWidth = kDefaultWidth;
    m_settings.strengthOffset = 30.f;
    m_settings.currentTimePos = 0;

    vk::BufferCreateInfo mvpBufferCreateInfo{vk::BufferCreateFlags{}, sizeof(m_settings),
                                             vk::BufferUsageFlagBits::eUniformBuffer};

    vk::DescriptorSetLayoutBinding mvpLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1,
                                                    vk::ShaderStageFlagBits::eVertex};

    std::array<vk::DescriptorSetLayoutBinding, 1> layoutBindings{mvpLayoutBinding};

    vk::DescriptorSetLayoutCreateInfo mvpLayoutCreateInfo{vk::DescriptorSetLayoutCreateFlags(), layoutBindings};

    DescriptorSetID mvpSetID = AddDescriptorLayoutSets(device, mvpLayoutCreateInfo);

    EngineBuffer_t mvpEngBuf =
        CreateNewBuffer(device, RadioBufferIDs::kRadioMvpBufferID, 0, mvpSetID, mvpBufferCreateInfo,
                        reinterpret_cast<void **>(&m_mvpBuffer),
                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    if (m_mvpBuffer != nullptr)
        memcpy(m_mvpBuffer, &m_settings, sizeof(m_settings));

    device->bindBufferMemory(mvpEngBuf.buffer, mvpEngBuf.devMem, 0);
}

void RadioPipeline::CreateShaders(const vk::UniqueDevice &device)
{
    vk::PipelineShaderStageCreateInfo radioVertPipelineShaderStageCI{vk::PipelineShaderStageCreateFlags{},
                                                                     vk::ShaderStageFlagBits::eVertex, nullptr, "main"};
#include "radio.vert.h"
    vk::ShaderModuleCreateInfo radioVertShaderModuleCI{vk::ShaderModuleCreateFlags{}, sizeof(radio_vert), radio_vert};

    m_radioVertShaderModule = device->createShaderModuleUnique(radioVertShaderModuleCI);

    radioVertPipelineShaderStageCI.setModule(m_radioVertShaderModule.get());

    m_shaderStages.push_back(radioVertPipelineShaderStageCI);

    vk::PipelineShaderStageCreateInfo radioFragPipelineShaderStageCI{
        vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eFragment, nullptr, "main"};
#include "control.frag.h"
    vk::ShaderModuleCreateInfo radioFragShaderModuleCI{vk::ShaderModuleCreateFlags{}, sizeof(control_frag),
                                                       control_frag};

    m_radioFragShaderModule = device->createShaderModuleUnique(radioFragShaderModuleCI);

    radioFragPipelineShaderStageCI.setModule(m_radioFragShaderModule.get());

    m_shaderStages.push_back(radioFragPipelineShaderStageCI);
}

void RadioPipeline::SetupVertexBuffer(const vk::UniqueDevice &device)
{
    vk::BufferCreateInfo vertBufferCI{vk::BufferCreateFlags{}, ((kMaxSignalBufferBytes + 64) * kMaxTimeDepth),
                                      vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                                      vk::SharingMode::eExclusive};
    vk::BufferCreateInfo fftBufferCI{vk::BufferCreateFlags{}, (kMaxSignalBufferBytes + 64),
                                     vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive};

    m_radioSignalVertEngBuf =
        CreateNewBuffer(device, kFFTVertBufferID, vertBufferCI, nullptr, vk::MemoryPropertyFlagBits::eDeviceLocal);

    m_radioSignalEngBuf =
        CreateNewBuffer(device, kFFTBufferID, fftBufferCI, reinterpret_cast<void **>(&m_radioSignalData),
                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    device->bindBufferMemory(m_radioSignalVertEngBuf.buffer, m_radioSignalVertEngBuf.devMem, 0);
    device->bindBufferMemory(m_radioSignalEngBuf.buffer, m_radioSignalEngBuf.devMem, 0);
}

void RadioPipeline::SetupFragmentBuffer(const vk::UniqueDevice &device)
{
}

void RadioPipeline::ChangeRadioRenderMode(RadioRenderMode newMode)
{
    PauseRender();

    m_radioRenderMode = newMode;

    switch (m_radioRenderMode)
    {
    case (HIGH_RESOLUTION): {
        m_mainRadioThread = std::thread(CalculateRadioSignal<HIGH_RESOLUTION, 4, 1024>, std::ref(m_shutdownSignal), std::ref(m_streamId), m_radioSignalData);
        break;
    }
    default:
        break;
    }
}

void RadioPipeline::PreRender(const vk::Device &device, vk::Extent2D windowExtents)
{
    if (!m_pauseSignal)
    {
        const auto lastTime = m_settings.currentTimePos;
        m_settings.currentTimePos++;
        if (m_settings.currentTimePos == m_timeDepth)
            m_settings.currentTimePos = 0;
        currentTimePos.store(m_settings.currentTimePos);

        calcDoneSignal.acquire();

        vk::MappedMemoryRange stageBufferRange{m_radioSignalEngBuf.devMem, 0, (m_radioRenderMode * sizeof(float)) * 2};

        device.flushMappedMemoryRanges(stageBufferRange);

        vk::BufferCopy radioSignalCopy{0, ((m_radioRenderMode * sizeof(float)) * 2) * lastTime,
                                       (m_radioRenderMode * sizeof(float)) * 2};

        vk::BufferMemoryBarrier copyBarrier{vk::AccessFlagBits::eHostWrite,
                                            vk::AccessFlagBits::eTransferRead,
                                            VK_QUEUE_FAMILY_IGNORED,
                                            VK_QUEUE_FAMILY_IGNORED,
                                            m_radioSignalEngBuf.buffer,
                                            0,
                                            radioSignalCopy.size};

        vk::BufferMemoryBarrier useBarrier{vk::AccessFlagBits::eTransferWrite,
                                           vk::AccessFlagBits::eIndexRead,
                                           VK_QUEUE_FAMILY_IGNORED,
                                           VK_QUEUE_FAMILY_IGNORED,
                                           m_radioSignalVertEngBuf.buffer,
                                           radioSignalCopy.dstOffset,
                                           radioSignalCopy.size};

        vk::CommandBufferAllocateInfo graphicsCommandBufferAI{m_graphicsCommandPool.get(),
                                                              vk::CommandBufferLevel::ePrimary, 1};

        vk::UniqueCommandBuffer cmdBuffer;
        cmdBuffer.swap(device.allocateCommandBuffersUnique(graphicsCommandBufferAI)[0]);

        vk::CommandBufferBeginInfo cmdBufBI{vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

        cmdBuffer->begin(cmdBufBI);

        cmdBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, {}, {},
                                   copyBarrier, {});

        cmdBuffer->copyBuffer(m_radioSignalEngBuf.buffer, m_radioSignalVertEngBuf.buffer, radioSignalCopy);

        cmdBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexInput, {},
                                   {}, useBarrier, {});

        cmdBuffer->end();

        vk::SubmitInfo submitInfo{{}, {}, cmdBuffer.get(), {}};

        m_graphicsQueues[0].submit(submitInfo);

        device.waitIdle();

        startNextCalcSignal.release();
    }
}

void RadioPipeline::Render(const vk::Device &device, const vk::CommandBuffer &cmdBuffer, vk::Extent2D windowExtents)
{
    if (!m_pauseSignal)
    {
        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline.get());
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout.get(), 0, m_descriptorSets,
                                     nullptr);

        memcpy(m_mvpBuffer, &m_settings, sizeof(m_settings));

        constexpr const vk::DeviceSize offsets[1]{0};
        cmdBuffer.bindVertexBuffers(0, 1, &m_radioSignalVertEngBuf.buffer, offsets);

        cmdBuffer.draw(m_radioRenderMode * m_timeDepth * 2, 1, 0, 0);
    }
    else
    {
        m_paused = true;
    }
}

} // namespace Pipelines
} // namespace Engine
