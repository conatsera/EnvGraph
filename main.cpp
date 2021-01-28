#include <lime/LimeSuite.h>
#include <unistd.h>

#include <chrono>
#include <iostream>

#include "engine.h"
#include "pipelines/radio.h"
#include "pipelines/ui.h"

#include "glm/gtx/quaternion.hpp"

constexpr const auto kIdentityQuat = glm::quat_identity<float, glm::qualifier::packed_highp>();

int main(int argc, char **argv)
{
    xcb_connection_t *xConnection = xcb_connect(NULL, NULL);

    const xcb_setup_t *xSetup = xcb_get_setup(xConnection);
    xcb_screen_iterator_t xScreenIter = xcb_setup_roots_iterator(xSetup);
    xcb_screen_t *mainScreen = xScreenIter.data;

    auto xcbMask = XCB_CW_EVENT_MASK;
    xcb_event_mask_t xcbValMask[] = {
        (xcb_event_mask_t)(XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS)};

    xcb_window_t window = xcb_generate_id(xConnection);
    xcb_create_window(xConnection, XCB_COPY_FROM_PARENT, window, mainScreen->root, 0, 0, kDefaultWidth, kDefaultHeight,
                      10, XCB_WINDOW_CLASS_INPUT_OUTPUT, mainScreen->root_visual, xcbMask, xcbValMask);

    xcb_map_window(xConnection, window);

    auto flushRet = xcb_flush(xConnection);
    if (flushRet != 0)
    {
        //std::cout << flushRet << std::endl;
        //return -32;
    }

    double currentFrequency = 2437e6;
    double currentGain = 0.4;
    bool wide = false;
    lms_device_t *device = nullptr;

    int deviceCount = LMS_GetDeviceList(NULL);
    if (deviceCount < 0)
    {
        std::cout << "Failed to get device list" << std::endl;
        return -1;
    }
    else if (deviceCount < 1)
    {
        std::cout << "Failed to get device list" << std::endl;
        return -1;
    }

    std::cout << "Found " << deviceCount << " device(s)" << std::endl;

    lms_info_str_t *deviceInfoList = new lms_info_str_t[deviceCount];

    if (LMS_GetDeviceList(deviceInfoList) < 0)
    {
        std::cout << "Failed to populate device list" << std::endl;
        return -2;
    }

    if (LMS_Open(&device, deviceInfoList[0], NULL))
    {
        std::cout << "Failed to open device" << std::endl;
        return -3;
    }

    delete[] deviceInfoList;

    if (LMS_Init(device) != 0)
    {
        std::cout << "Failed to init device" << std::endl;
        LMS_Close(device);
        return -4;
    }

    if (argc == 1)
    {
        if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
        {
            std::cout << "Failed to enable channel" << std::endl;
            LMS_Close(device);
            return -5;
        }

        if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, currentFrequency) != 0)
        {
            std::cout << "Failed to set LO Frequency" << std::endl;
            LMS_Close(device);
            return -6;
        }

        double centerFrequency = nan("\0");
        if (LMS_GetLOFrequency(device, LMS_CH_RX, 0, &centerFrequency) != 0)
        {
            std::cout << "Failed to get LO Frequency" << std::endl;
            LMS_Close(device);
            return -7;
        }

        std::cout << "Center Frequency: " << centerFrequency / 1e6 << " MHz" << std::endl;

        lms_name_t antennaList[10];
        int antennaCount = LMS_GetAntennaList(device, LMS_CH_RX, 0, antennaList);
        if (antennaCount < 0)
        {
            std::cout << "Failed to get antenna list" << std::endl;
            LMS_Close(device);
            return -8;
        }

        std::cout << "Available antennas: " << std::endl;
        for (auto i = 0; i < antennaCount; i++)
            std::cout << i << ": " << antennaList[i] << std::endl;

        auto n = 0;
        if ((n = LMS_GetAntenna(device, LMS_CH_RX, 0)) < 0) // get currently selected antenna index
        {
            std::cout << "Failed to get active antenna" << std::endl;
            LMS_Close(device);
            return -9;
        };
        // print antenna index and name
        std::cout << "Automatically selected antenna: " << n << ": " << antennaList[n] << std::endl;

        if (LMS_SetAntenna(device, LMS_CH_RX, 0, LMS_PATH_LNAH) != 0) // manually select antenna
        {
            std::cout << "Failed to set active antenna" << std::endl;
            LMS_Close(device);
            return -10;
        };

        if ((n = LMS_GetAntenna(device, LMS_CH_RX, 0)) < 0) // get currently selected antenna index
        {
            std::cout << "Failed to get new active antenna" << std::endl;
            LMS_Close(device);
            return -11;
        };
        // print antenna index and name
        std::cout << "Manually selected antenna: " << n << ": " << antennaList[n] << std::endl;

        // Set sample rate to 8 MHz, preferred oversampling in RF 8x
        // This set sampling rate for all channels
        if (LMS_SetSampleRate(device, 20e6, 8) != 0)
        {
            std::cout << "Failed to set sample rate" << std::endl;
            LMS_Close(device);
            return -12;
        };
        // print resulting sampling rates (interface to host , and ADC)
        float_type rate, rf_rate;
        if (LMS_GetSampleRate(device, LMS_CH_RX, 0, &rate, &rf_rate) != 0) // NULL can be passed
        {
            std::cout << "Failed to get sample rate" << std::endl;
            LMS_Close(device);
            return -13;
        };
        std::cout << "\nHost interface sample rate: " << rate / 1e6 << " MHz\nRF ADC sample rate: " << rf_rate / 1e6
                  << "MHz\n\n";

        // Example of getting allowed parameter value range
        // There are also functions to get other parameter ranges (check LimeSuite.h)

        // Get allowed LPF bandwidth range
        lms_range_t range;
        if (LMS_GetLPFBWRange(device, LMS_CH_RX, &range) != 0)
        {
            std::cout << "Failed to get LPF bandwidth range" << std::endl;
            LMS_Close(device);
            return -14;
        };

        std::cout << "RX LPF bandwitdh range: " << range.min / 1e6 << " - " << range.max / 1e6 << " MHz\n\n";

        // Configure LPF, bandwidth 8 MHz
        if (LMS_SetLPFBW(device, LMS_CH_RX, 0, 20e6) != 0)
        {
            std::cout << "Failed to set LPF bandwidth" << std::endl;
            LMS_Close(device);
            return -15;
        };

        // Set RX gain
        if (LMS_SetNormalizedGain(device, LMS_CH_RX, 0, currentGain) != 0)
        {
            std::cout << "Failed to set norm gain" << std::endl;
            LMS_Close(device);
            return -16;
        };

        // Print RX gain
        float_type gain; // normalized gain
        if (LMS_GetNormalizedGain(device, LMS_CH_RX, 0, &gain) != 0)
        {
            std::cout << "Failed to get new norm gain" << std::endl;
            LMS_Close(device);
            return -17;
        };
        std::cout << "Normalized RX Gain: " << gain << std::endl;

        unsigned int gaindB; // gain in dB
        if (LMS_GetGaindB(device, LMS_CH_RX, 0, &gaindB) != 0)
        {
            std::cout << "Failed to get gain" << std::endl;
            LMS_Close(device);
            return -18;
        };
        std::cout << "RX Gain: " << gaindB << " dB" << std::endl;

        // Perform automatic calibration
        int calibrate_ret = false;
        calibrate_ret = LMS_Calibrate(device, LMS_CH_RX, 0, 5e6, 0);
        while (calibrate_ret != 0)
        {
            std::cout << "Failed to calibrate" << std::endl;
            currentGain += 0.01;
            if (LMS_SetNormalizedGain(device, LMS_CH_RX, 0, currentGain) != 0)
            {
                std::cout << "Failed to set norm gain" << std::endl;
                LMS_Close(device);
                return -16;
            };
            // Print RX gain
            float_type gain; // normalized gain
            if (LMS_GetNormalizedGain(device, LMS_CH_RX, 0, &gain) != 0)
            {
                std::cout << "Failed to get new norm gain" << std::endl;
                LMS_Close(device);
                return -17;
            };
            std::cout << "Normalized RX Gain: " << gain << std::endl;

            unsigned int gaindB; // gain in dB
            if (LMS_GetGaindB(device, LMS_CH_RX, 0, &gaindB) != 0)
            {
                std::cout << "Failed to get gain" << std::endl;
                LMS_Close(device);
                return -18;
            };
            std::cout << "RX Gain: " << gaindB << " dB" << std::endl;
            calibrate_ret = LMS_Calibrate(device, LMS_CH_RX, 0, 5e6, 0);
            // LMS_Close(device);
            // return -19;
        };
    }
    else
    {
        LMS_LoadConfig(device, argv[1]);
    }

    // Streaming Setup

    // Initialize stream
    lms_stream_t streamId;
    streamId.channel = 0;                         // channel number
    streamId.fifoSize = 8192 * 1024;              // fifo size in samples
    streamId.throughputVsLatency = 1.0;           // optimize for max throughput
    streamId.isTx = false;                        // RX channel
    streamId.dataFmt = lms_stream_t::LMS_FMT_F32; // 32-bit floats
    if (LMS_SetupStream(device, &streamId) != 0)
    {
        std::cout << "Failed to setup stream" << std::endl;
        return -20;
    }

    { // Anonymous scope ensures the engine and pipelines are deleted and cleaned up prior to the lms_device_t and it's
      // associated stream(s) being closed
        using Engine::Pipelines::RadioPipeline;
        using namespace Engine::UI;

        auto aEngine = std::make_unique<Engine::Engine>(xConnection, window);
        auto aEnginePtr = aEngine.get();
        auto radioPipeline = std::make_shared<RadioPipeline>();
        auto uiPipeline = std::make_shared<UIPipeline>();
        LMS_StartStream(&streamId);
        radioPipeline->ConfigureStream(streamId);

        aEngine->NewPipeline(radioPipeline);
        aEngine->NewPipeline(uiPipeline);

        bool shutdownSignal = false;

        std::thread inputThread = std::thread([&shutdownSignal, xConnection, device, &currentFrequency, &currentGain,
                                               &wide, aEnginePtr, uiPipeline] {
            auto freqLabel =
                Label{glm::vec3{0.5f, 0.125f, 0.f}, glm::vec3{2.f, 2.f, 1.f}, kIdentityQuat,
                      Engine::EngineTexture_t{Engine::EngineTextureType::SOLID_COLOR, glm::vec3{1.f, 1.f, 1.f}}, "", TextAlign::CENTER};

            std::stringstream frequencyStringFormat("");
            frequencyStringFormat << currentFrequency / 1e6 << "MHz";
            frequencyStringFormat.flush();
            freqLabel.text = frequencyStringFormat.str();

            auto freqLabelId = uiPipeline->AddLabel(freqLabel);
            double lastCalibrationFrequency = currentFrequency;

            vk::Extent2D currentWindowExtents{kDefaultWidth, kDefaultHeight};

            while (!shutdownSignal)
            {
                auto newEvent = xcb_wait_for_event(xConnection);
                if (newEvent != nullptr)
                {
                    switch (newEvent->response_type)
                    {
                    case XCB_EXPOSE: {
                        xcb_expose_event_t *exposeEvent = (xcb_expose_event_t *)newEvent;
                        currentWindowExtents.setWidth(exposeEvent->width + exposeEvent->x);
                        currentWindowExtents.setHeight(exposeEvent->height + exposeEvent->y);
                        aEnginePtr->UpdateWindowExtents(exposeEvent->width + exposeEvent->x,
                                                        exposeEvent->height + exposeEvent->y);
                        break;
                    }
                    case XCB_MOTION_NOTIFY: {
                        /*xcb_motion_notify_event_t* motionEvent = (xcb_motion_notify_event_t*) newEvent;
                        std::cout <<
                        "Motion Event:" << std::endl <<
                        "  Time: "    << motionEvent->time    << std::endl <<
                        "  Detail: "  << motionEvent->detail  << std::endl <<
                        "  State: "   << motionEvent->state   << std::endl <<
                        "  Event X: " << motionEvent->event_x << std::endl <<
                        "  Event Y: " << motionEvent->event_y << std::endl <<
                        "  Root  X: " << motionEvent->root_x  << std::endl <<
                        "  Root  Y: " << motionEvent->root_y  << std::endl;*/
                        break;
                    }/*
                    case XCB_BUTTON_RELEASE:
                    {
                      xcb_button_release_event_t *buttonEvent = (xcb_button_release_event_t *)newEvent;

                        switch (buttonEvent->detail)
                        {
                        case XCB_BUTTON_INDEX_1: {
                            leftClicked = false;
                            break;
                        }
                        default:
                          break;
                        }
                      break;
                    }*/
                    case XCB_BUTTON_PRESS: {
                        xcb_button_press_event_t *buttonEvent = (xcb_button_press_event_t *)newEvent;

                        switch (buttonEvent->detail)
                        {
                        case XCB_BUTTON_INDEX_4: // Scroll up
                        {
                            if (buttonEvent->event_x > (currentWindowExtents.width * 0.9))
                            {
                                currentGain += 0.01;
                                if (currentGain > 1.0)
                                    currentGain = 1.0;

                                // Set RX gain
                                if (LMS_SetNormalizedGain(device, LMS_CH_RX, 0, currentGain) != 0)
                                {
                                    std::cout << "Failed to set norm gain" << std::endl;
                                };

                                // Print RX gain
                                float_type gain; // normalized gain
                                if (LMS_GetNormalizedGain(device, LMS_CH_RX, 0, &gain) != 0)
                                {
                                    std::cout << "Failed to get new norm gain" << std::endl;
                                };
                                std::cout << "Normalized RX Gain: " << gain << std::endl;

                                unsigned int gaindB; // gain in dB
                                if (LMS_GetGaindB(device, LMS_CH_RX, 0, &gaindB) != 0)
                                {
                                    std::cout << "Failed to get gain" << std::endl;
                                };
                                std::cout << "RX Gain: " << gaindB << " dB" << std::endl;

                                // Perform automatic calibration
                                if (LMS_Calibrate(device, LMS_CH_RX, 0, 5e6, 0) != 0)
                                {
                                    std::cout << "Failed to calibrate" << std::endl;
                                }
                            }
                            else
                            {
                                currentFrequency += 500e3;
                                if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, currentFrequency) != 0)
                                {
                                    std::cout << "Failed to set LO Frequency" << std::endl;
                                }
                                double centerFrequency = nan("\0");
                                if (LMS_GetLOFrequency(device, LMS_CH_RX, 0, &centerFrequency) != 0)
                                {
                                    std::cout << "Failed to get LO Frequency" << std::endl;
                                }
                                std::cout << "Center Frequency: " << centerFrequency / 1e6 << " MHz" << std::endl;
                                frequencyStringFormat.str("");
                                frequencyStringFormat << currentFrequency / 1e6 << "MHz";
                                frequencyStringFormat.flush();
                                freqLabel.text = frequencyStringFormat.str();
                                uiPipeline->RemoveLabel(freqLabelId);
                                freqLabelId = uiPipeline->AddLabel(freqLabel);

                                if (std::abs(lastCalibrationFrequency - currentFrequency) > 32e6)
                                {
                                    if (LMS_Calibrate(device, LMS_CH_RX, 0, 5e6, 0) != 0)
                                    {
                                        std::cout << "Failed to calibrate" << std::endl;
                                    }
                                    lastCalibrationFrequency = currentFrequency;
                                }
                            }
                            break;
                        }
                        case XCB_BUTTON_INDEX_5: // Scroll down
                        {
                            if (buttonEvent->event_x > (currentWindowExtents.width * 0.9))
                            {
                                currentGain -= 0.01;
                                if (currentGain < 0.0)
                                    currentGain = 0.0;

                                // Set RX gain
                                if (LMS_SetNormalizedGain(device, LMS_CH_RX, 0, currentGain) != 0)
                                {
                                    std::cout << "Failed to set norm gain" << std::endl;
                                };

                                // Print RX gain
                                float_type gain; // normalized gain
                                if (LMS_GetNormalizedGain(device, LMS_CH_RX, 0, &gain) != 0)
                                {
                                    std::cout << "Failed to get new norm gain" << std::endl;
                                };
                                std::cout << "Normalized RX Gain: " << gain << std::endl;

                                unsigned int gaindB; // gain in dB
                                if (LMS_GetGaindB(device, LMS_CH_RX, 0, &gaindB) != 0)
                                {
                                    std::cout << "Failed to get gain" << std::endl;
                                };
                                std::cout << "RX Gain: " << gaindB << " dB" << std::endl;

                                // Perform automatic calibration
                                if (LMS_Calibrate(device, LMS_CH_RX, 0, 5e6, 0) != 0)
                                {
                                    std::cout << "Failed to calibrate" << std::endl;
                                }
                            }
                            else
                            {
                                currentFrequency -= 500e3;
                                if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, currentFrequency) != 0)
                                {
                                    std::cout << "Failed to set LO Frequency" << std::endl;
                                }
                                double centerFrequency = nan("\0");
                                if (LMS_GetLOFrequency(device, LMS_CH_RX, 0, &centerFrequency) != 0)
                                {
                                    std::cout << "Failed to get LO Frequency" << std::endl;
                                }
                                std::cout << "Center Frequency: " << centerFrequency / 1e6 << " MHz" << std::endl;
                                frequencyStringFormat.str("");
                                frequencyStringFormat << currentFrequency / 1e6 << "MHz";
                                frequencyStringFormat.flush();
                                freqLabel.text = frequencyStringFormat.str();
                                uiPipeline->RemoveLabel(freqLabelId);
                                freqLabelId = uiPipeline->AddLabel(freqLabel);

                                if (std::abs(lastCalibrationFrequency - currentFrequency) > 32e6)
                                {
                                    if (LMS_Calibrate(device, LMS_CH_RX, 0, 5e6, 0) != 0)
                                    {
                                        std::cout << "Failed to calibrate" << std::endl;
                                    }
                                    lastCalibrationFrequency = currentFrequency;
                                }
                            }
                            break;
                        }
                        case (8): // Mouse button backwards navigate
                        {
                            if (LMS_Calibrate(device, LMS_CH_RX, 0, 5e6, 0) != 0)
                            {
                                std::cout << "Failed to calibrate" << std::endl;
                            }
                            break;
                        }
                        case (9): {
                            wide = !wide;
                            if (wide)
                            {
                                if (LMS_SetAntenna(device, LMS_CH_RX, 0, LMS_PATH_LNAW) != 0) // manually select antenna
                                {
                                    std::cout << "Failed to set active antenna" << std::endl;
                                }
                            }
                            else
                            {
                                if (LMS_SetAntenna(device, LMS_CH_RX, 0, LMS_PATH_LNAH) != 0) // manually select antenna
                                {
                                    std::cout << "Failed to set active antenna" << std::endl;
                                }
                            }
                            break;
                        }
                        default: {
                            std::cout << buttonEvent->detail << std::endl;
                            break;
                        }
                        }
                        break;
                    }
                    default: {
                        std::cout << "New Event:" << std::endl
                                  << "  Type: " << newEvent->response_type << std::endl
                                  << "  Sequence Num: " << newEvent->sequence << std::endl
                                  << "  Data: " << std::endl;
                    }
                    }
                }
                else
                {
                    shutdownSignal = true;
                    break;
                }

                std::this_thread::sleep_for(std::chrono::microseconds(250)); // Defaults to ~4KHz poll rate
            }
            //moveThread.join();
            return 0;
        });

        auto freqLabel =
            Label{glm::vec3{1536.f, 50.f, 0.f}, glm::vec3{1.f, 1.f, 1.f}, kIdentityQuat,
                  Engine::EngineTexture_t{Engine::EngineTextureType::SOLID_COLOR, glm::vec3{1.f, 1.f, 1.f}}, "test"};

        auto freqButton = Button{
            glm::vec3{.5f, .975f, 0.f}, glm::vec3{.2f, .1f, 1.f}, kIdentityQuat,
            Engine::EngineTexture_t{Engine::EngineTextureType::SOLID_COLOR, glm::vec3{1.f, 1.f, 1.f}}, freqLabel};

        uiPipeline->AddButton(freqButton);

        aEngine->StartRender();

        std::string command;
        while (!shutdownSignal)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        inputThread.join();
    }

    LMS_Close(device);

    xcb_disconnect(xConnection);

    return 0;
}