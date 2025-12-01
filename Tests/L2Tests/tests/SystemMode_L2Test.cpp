/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FrontPanelIndicatorMock.h"
#include "L2Tests.h"
#include "L2TestsMock.h"
#include "MfrMock.h"
#include "PowerManagerHalMock.h"
#include "deepSleepMgr.h"
#include "devicesettings.h"
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <interfaces/ISystemMode.h>
#include <mutex>
#include <thread>

#define TEST_LOG(x, ...)                                                                                                                         \
    fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); \
    fflush(stderr);

#define SYSTEMMODE_CALLSIGN _T("org.rdk.SystemMode.1")
#define SYSTEMMODEL2TEST_CALLSIGN _T("L2tests.1")
#define DISPLAYSETTINGS_CALLSIGN _T("org.rdk.DisplaySettings.1")
#define SYSTEM_MODE_FILE "/tmp/SystemMode.txt"

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using ::WPEFramework::Exchange::ISystemMode;

namespace {
static void removeFile(const char* fileName)
{
    // Use sudo for protected files
    if (strcmp(fileName, "/etc/device.properties") == 0 || strcmp(fileName, "/opt/persistent/ds/cecData_2.json") == 0 || strcmp(fileName, "/opt/uimgr_settings.bin") == 0) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "sudo rm -f %s", fileName);
        int ret = system(cmd);
        if (ret != 0) {
            printf("File %s failed to remove with sudo\n", fileName);
            perror("Error deleting file");
        } else {
            printf("File %s successfully deleted with sudo\n", fileName);
        }
    } else {
        if (std::remove(fileName) != 0) {
            printf("File %s failed to remove\n", fileName);
            perror("Error deleting file");
        } else {
            printf("File %s successfully deleted\n", fileName);
        }
    }
}

static void createFile(const char* fileName, const char* fileContent)
{
    std::ofstream fileContentStream(fileName);
    fileContentStream << fileContent;
    fileContentStream << "\n";
    fileContentStream.close();
}
}

class SystemMode_L2test : public L2TestMocks {
protected:
    virtual ~SystemMode_L2test() override;
    SystemMode_L2test();

public:
    uint32_t CreateSystemModeInterfaceObject();
    void SetUp() override;
    void TearDown() override;
    bool WaitForState(const ISystemMode::SystemMode systemMode,
        const ISystemMode::State expected,
        const std::chrono::milliseconds& timeout = std::chrono::milliseconds(3000),
        const std::chrono::milliseconds& interval = std::chrono::milliseconds(100));
    bool WaitForStateJsonRpc(const std::string& modeName,
        const std::string& expectedStateName,
        const std::chrono::milliseconds& timeout = std::chrono::milliseconds(3000),
        const std::chrono::milliseconds& interval = std::chrono::milliseconds(100));

    /** @brief Safe interface validation method */
    bool ValidateInterfaces() {
        try {
            if (m_sysmodeplugin == nullptr) {
                TEST_LOG("ERROR: SystemMode plugin interface is null");
                return false;
            }
            if (m_controller_sysmode == nullptr) {
                TEST_LOG("ERROR: Controller interface is null");
                return false;
            }
            // Add small delay for interface stability
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return true;
        } catch (...) {
            TEST_LOG("EXCEPTION during interface validation");
            return false;
        }
    }

private:
    /** @brief Mutex */
    std::mutex m_mutex;

    /** @brief Condition variable */
    std::condition_variable m_condition_variable;

    /** @brief Event signalled flag */
    uint32_t m_event_signalled;

protected:
    /** @brief Pointer to the IShell interface */
    PluginHost::IShell* m_controller_sysmode;

    /** @brief Pointer to the ISystemMode interface */
    Exchange::ISystemMode* m_sysmodeplugin;
};

SystemMode_L2test::SystemMode_L2test()
    : L2TestMocks()
{
    uint32_t status = Core::ERROR_GENERAL;

    // PowerManager Mocks
    createFile("/tmp/pwrmgr_restarted", "2");
    EXPECT_CALL(*p_powerManagerHalMock, PLAT_DS_INIT())
        .WillOnce(::testing::Return(DEEPSLEEPMGR_SUCCESS));

    EXPECT_CALL(*p_powerManagerHalMock, PLAT_INIT())
        .WillRepeatedly(::testing::Return(PWRMGR_SUCCESS));

    EXPECT_CALL(*p_powerManagerHalMock, PLAT_API_SetWakeupSrc(::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return(PWRMGR_SUCCESS));

    ON_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [](char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData) {
                if (strcmp("RFC_DATA_ThermalProtection_POLL_INTERVAL", pcParameterName) == 0) {
                    strcpy(pstParamData->value, "2");
                    return WDMP_SUCCESS;
                } else if (strcmp("RFC_ENABLE_ThermalProtection", pcParameterName) == 0) {
                    strcpy(pstParamData->value, "true");
                    return WDMP_SUCCESS;
                } else if (strcmp("RFC_DATA_ThermalProtection_DEEPSLEEP_GRACE_INTERVAL", pcParameterName) == 0) {
                    strcpy(pstParamData->value, "6");
                    return WDMP_SUCCESS;
                } else {
                    /* The default threshold values will assign, if RFC call failed */
                    return WDMP_FAILURE;
                }
            }));

    EXPECT_CALL(*p_mfrMock, mfrSetTempThresholds(::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [](int high, int critical) {
                EXPECT_EQ(high, 100);
                EXPECT_EQ(critical, 110);
                return mfrERR_NONE;
            }));

    EXPECT_CALL(*p_powerManagerHalMock, PLAT_API_GetPowerState(::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [](PWRMgr_PowerState_t* powerState) {
                *powerState = PWRMGR_POWERSTATE_OFF; // by default over boot up, return PowerState OFF
                return PWRMGR_SUCCESS;
            }));

    EXPECT_CALL(*p_powerManagerHalMock, PLAT_API_SetPowerState(::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                // All tests are run without settings file
                // so default expected power state is ON
                return PWRMGR_SUCCESS;
            }));

    EXPECT_CALL(*p_mfrMock, mfrGetTemperature(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [&](mfrTemperatureState_t* curState, int* curTemperature, int* wifiTemperature) {
                *curTemperature = 90; // safe temperature
                *curState = (mfrTemperatureState_t)0;
                *wifiTemperature = 25;
                return mfrERR_NONE;
            }));

    // DisplaySettings Mocks
    string videoPort(_T("HDMI0"));
    string audioPort(_T("HDMI0"));

    device::VideoOutputPort videoOutputPort;
    device::VideoDevice videoDevice;
    device::VideoResolution videoResolution;
    device::VideoOutputPortType videoOutputPortType;
    device::VideoDFC actualVideoDFC;
    string videoDFCName(_T("FULL"));
    string videoPortSupportedResolution(_T("1080p"));

    ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
        .WillByDefault(::testing::Return(videoPort));
    ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(true));

    device::AudioOutputPort audioFormat;
    ON_CALL(*p_hostImplMock, getCurrentAudioFormat(testing::_))
        .WillByDefault(testing::Invoke(
            [](dsAudioFormat_t audioFormat) {
                audioFormat = dsAUDIO_FORMAT_NONE;
                return 0;
            }));

    ON_CALL(*p_videoOutputPortMock, getDefaultResolution())
        .WillByDefault(::testing::ReturnRef(videoResolution));

    ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));

    // ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
    //     .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(true));

    ON_CALL(*p_videoOutputPortMock, getName())
        .WillByDefault(::testing::ReturnRef(videoPort));

    ON_CALL(*p_audioOutputPortMock, getName())
        .WillByDefault(::testing::ReturnRef(audioPort));

    ON_CALL(*p_videoOutputPortMock, getVideoEOTF())
        .WillByDefault(::testing::Return(1));

    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>({ videoDevice })));

    ON_CALL(*p_videoDeviceMock, getHDRCapabilities(testing::_))
        .WillByDefault([](int* capabilities) {
            if (capabilities) {
                *capabilities = dsHDRSTANDARD_TechnicolorPrime;
            }
        });

    ON_CALL(*p_videoOutputPortMock, getColorDepthCapabilities(testing::_))
        .WillByDefault(testing::Invoke(
            [&](unsigned int* capabilities) {
                *capabilities = dsDISPLAY_COLORDEPTH_8BIT;
            }));

    ON_CALL(*p_videoOutputPortMock, setResolution(testing::_, testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            [&](std::string resolution, bool persist, bool isIgnoreEdid) {
                printf("Inside setResolution Mock\n");
                EXPECT_EQ(resolution, "1080p60");
                EXPECT_EQ(persist, true);

                std::cout << "Resolution: " << resolution
                          << ", Persist: " << persist
                          << ", Ignore EDID: " << isIgnoreEdid << std::endl;
            }));

    ON_CALL(*p_videoOutputPortMock, getCurrentOutputSettings(testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillByDefault([](int& videoEOTF, int& matrixCoefficients, int& colorSpace,
                           int& colorDepth, int& quantizationRange) {
            videoEOTF = 1; // example values
            matrixCoefficients = 0;
            colorSpace = 3;
            colorDepth = 10;
            quantizationRange = 4;

            return true;
        });

    ON_CALL(*p_videoOutputPortConfigImplMock, getPortType(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPortType));
    ON_CALL(*p_videoOutputPortMock, getType())
        .WillByDefault(::testing::ReturnRef(videoOutputPortType));
    ON_CALL(*p_videoOutputPortTypeMock, getId())
        .WillByDefault(::testing::Return(0));
    // device::VideoResolution videoResolution;
    ON_CALL(*p_videoOutputPortTypeMock, getSupportedResolutions())
        .WillByDefault(::testing::Return(device::List<device::VideoResolution>({ videoResolution })));
    ON_CALL(*p_videoResolutionMock, getName())
        .WillByDefault(::testing::ReturnRef(videoPortSupportedResolution));

    ON_CALL(*p_videoOutputPortMock, setForceHDRMode(::testing::_))
        .WillByDefault(::testing::Return(true));

    ON_CALL(*p_videoOutputPortMock, getResolution())
        .WillByDefault([]() -> const device::VideoResolution& {
            static device::VideoResolution dynamicResolution;
            return dynamicResolution;
        });

    ON_CALL(*p_videoDeviceMock, getDFC())
        .WillByDefault(::testing::Invoke([&]() -> device::VideoDFC& {
            return actualVideoDFC;
        }));

    ON_CALL(*p_videoDFCMock, getName())
        .WillByDefault(::testing::ReturnRef(videoDFCName));

    ON_CALL(*p_videoOutputPortMock, getTVHDRCapabilities(testing::_))
        .WillByDefault(testing::Invoke(
            [&](int* capabilities) {
                *capabilities = dsHDRSTANDARD_HLG | dsHDRSTANDARD_HDR10;
            }));

    /* Activate plugin in constructor */
    status = ActivateService("org.rdk.PowerManager");
    EXPECT_EQ(Core::ERROR_NONE, status);

    status = ActivateService("org.rdk.DisplaySettings");
    EXPECT_EQ(Core::ERROR_NONE, status);

   /* Activate SystemMode plugin */
    TEST_LOG("Attempting to activate SystemMode service...");
    try {
        status = ActivateService("org.rdk.SystemMode");
        if (status != Core::ERROR_NONE) {
            TEST_LOG("WARNING: SystemMode activation failed with status %u", status);
        }
        EXPECT_EQ(Core::ERROR_NONE, status);
    } catch (...) {
        TEST_LOG("EXCEPTION: SystemMode activation caused an exception");
        status = Core::ERROR_GENERAL;
    }
}

SystemMode_L2test::~SystemMode_L2test()
{
    uint32_t status = Core::ERROR_GENERAL;

    EXPECT_CALL(*p_powerManagerHalMock, PLAT_TERM())
        .WillOnce(::testing::Return(PWRMGR_SUCCESS));

    EXPECT_CALL(*p_powerManagerHalMock, PLAT_DS_TERM())
        .WillOnce(::testing::Return(DEEPSLEEPMGR_SUCCESS));

    status = DeactivateService("org.rdk.PowerManager");
    EXPECT_EQ(Core::ERROR_NONE, status);

    status = DeactivateService("org.rdk.DisplaySettings");
    EXPECT_EQ(Core::ERROR_NONE, status);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    /* Deactivate plugin in destructor */
    status = DeactivateService("org.rdk.SystemMode");
    EXPECT_EQ(Core::ERROR_NONE, status);

    removeFile("/tmp/pwrmgr_restarted");
    removeFile("/opt/uimgr_settings.bin");
}

uint32_t SystemMode_L2test::CreateSystemModeInterfaceObject()
{
    uint32_t return_value = Core::ERROR_GENERAL;
    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> SystemMode_Engine;
    Core::ProxyType<RPC::CommunicatorClient> SystemMode_Client;

    TEST_LOG("Creating SystemMode_Engine");
    try {
        // Add safety delay before interface creation
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        SystemMode_Engine = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
        if (!SystemMode_Engine.IsValid()) {
            TEST_LOG("ERROR: Failed to create SystemMode_Engine");
            return Core::ERROR_GENERAL;
        }

        // Add safety delay
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        SystemMode_Client = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"), Core::ProxyType<Core::IIPCServer>(SystemMode_Engine));
        if (!SystemMode_Client.IsValid()) {
            TEST_LOG("ERROR: Failed to create SystemMode_Client");
            return Core::ERROR_GENERAL;
        }
        
    TEST_LOG("Creating SystemMode_Engine Announcements");
#if ((THUNDER_VERSION == 2) || ((THUNDER_VERSION == 4) && (THUNDER_VERSION_MINOR == 2)))
    SystemMode_Engine->Announcements(SystemMode_Client->Announcement());
#endif
    if (!SystemMode_Client.IsValid()) {
        TEST_LOG("Invalid SystemMode_Client");
        return Core::ERROR_GENERAL;
    } else {
        // Add timeout and validation for interface opening
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        m_controller_sysmode = SystemMode_Client->Open<PluginHost::IShell>(_T("org.rdk.SystemMode"), ~0, 3000);
        if (m_controller_sysmode) {
            // Validate controller before querying interface
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            m_sysmodeplugin = m_controller_sysmode->QueryInterface<Exchange::ISystemMode>();
            if (m_sysmodeplugin) {
                // Final validation delay
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
                return_value = Core::ERROR_NONE;
                TEST_LOG("SystemMode plugin interface created successfully");
                } else {
                    TEST_LOG("ERROR: Failed to query SystemMode interface");
                }
            } else {
                TEST_LOG("ERROR: Failed to open SystemMode shell interface");
            }
        }
    } catch (const std::exception& e) {
        TEST_LOG("EXCEPTION in CreateSystemModeInterfaceObject: %s", e.what());
        // Ensure cleanup on exception
        m_sysmodeplugin = nullptr;
        m_controller_sysmode = nullptr;
        return Core::ERROR_GENERAL;
    } catch (...) {
        TEST_LOG("UNKNOWN EXCEPTION in CreateSystemModeInterfaceObject");
        // Ensure cleanup on exception
        m_sysmodeplugin = nullptr;
        m_controller_sysmode = nullptr;
        return Core::ERROR_GENERAL;    
    }
    return return_value;
}

void SystemMode_L2test::SetUp()
{
    if ((m_sysmodeplugin == nullptr) || (m_controller_sysmode == nullptr)) {
        TEST_LOG("SystemMode plugin interface not initialized, attempting to create...");
        uint32_t result = CreateSystemModeInterfaceObject();
        if (result != Core::ERROR_NONE) {
            TEST_LOG("WARNING: Failed to create SystemMode interface object, status: %u", result);
            // Mark the test as failed but don't crash
            GTEST_FAIL() << "SystemMode interface creation failed, skipping test";
            return;
        }
        EXPECT_EQ(Core::ERROR_NONE, result);
        TEST_LOG("SystemMode interface created successfully");
    }
}

void SystemMode_L2test::TearDown()
{
    if (m_sysmodeplugin) {
        m_sysmodeplugin->Release();
        m_sysmodeplugin = nullptr;
    }
    if (m_controller_sysmode) {
        m_controller_sysmode->Release();
        m_controller_sysmode = nullptr;
    }
}

bool SystemMode_L2test::WaitForState(const ISystemMode::SystemMode systemMode,
    const ISystemMode::State expected,
    const std::chrono::milliseconds& timeout,
    const std::chrono::milliseconds& interval)
{
    auto start = std::chrono::steady_clock::now();
    Exchange::ISystemMode::GetStateResult result{};
    while (std::chrono::steady_clock::now() - start < timeout) {
        if (m_sysmodeplugin) {
            Core::hresult rc = m_sysmodeplugin->GetState(systemMode, result);
            if (rc == Core::ERROR_NONE && result.state == expected) {
                return true;
            }
        }
        std::this_thread::sleep_for(interval);
    }
    return false;
}

bool SystemMode_L2test::WaitForStateJsonRpc(const std::string& modeName,
    const std::string& expectedStateName,
    const std::chrono::milliseconds& timeout,
    const std::chrono::milliseconds& interval)
{
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < timeout) {
        JsonObject params;
        JsonObject result;
        params["systemMode"] = modeName;
        uint32_t rc = InvokeServiceMethod("org.rdk.SystemMode.1", "getState", params, result);
        if (rc == Core::ERROR_NONE) {
            if (result.HasLabel("state") && result["state"].String() == expectedStateName) {
                return true;
            }
        }
        std::this_thread::sleep_for(interval);
    }
    return false;
}

// Request VIDEO state and validate via polling using GetState
TEST_F(SystemMode_L2test, RequestState_VIDEO)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    Core::hresult rc = m_sysmodeplugin->RequestState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::VIDEO);
    EXPECT_EQ(Core::ERROR_NONE, rc);
    EXPECT_TRUE(WaitForState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::VIDEO))
        << "VIDEO state not reached in time";
}

// Request GAME state and validate via polling
TEST_F(SystemMode_L2test, RequestState_GAME)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    Core::hresult rc = m_sysmodeplugin->RequestState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::GAME);
    EXPECT_EQ(Core::ERROR_NONE, rc);
    EXPECT_TRUE(WaitForState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::GAME))
        << "GAME state not reached in time";
}

// Transition VIDEO -> GAME -> VIDEO validating each step
TEST_F(SystemMode_L2test, StateTransition_VIDEO_GAME_VIDEO)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    EXPECT_EQ(Core::ERROR_NONE, m_sysmodeplugin->RequestState(Exchange::ISystemMode::DEVICE_OPTIMIZE, Exchange::ISystemMode::VIDEO));
    EXPECT_TRUE(WaitForState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::VIDEO));
    EXPECT_EQ(Core::ERROR_NONE, m_sysmodeplugin->RequestState(Exchange::ISystemMode::DEVICE_OPTIMIZE, Exchange::ISystemMode::GAME));
    EXPECT_TRUE(WaitForState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::GAME));
    EXPECT_EQ(Core::ERROR_NONE, m_sysmodeplugin->RequestState(Exchange::ISystemMode::DEVICE_OPTIMIZE, Exchange::ISystemMode::VIDEO));
    EXPECT_TRUE(WaitForState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
        Exchange::ISystemMode::VIDEO));
}

// Client activation / deactivation lifecycle
TEST_F(SystemMode_L2test, ClientActivationLifecycle)
{
    ASSERT_TRUE(ValidateInterfaces());
    
    // Add safety delay before interface operations
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // The textual name of system mode as per @text: device_optimize
    const std::string modeName = "DEVICE_OPTIMIZE";

     try {
        TEST_LOG("Starting ClientActivated call with modeName: %s", modeName.c_str());
        
        // Add validation and timing control for activation
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        Core::hresult rcAct = m_sysmodeplugin->ClientActivated(SYSTEMMODEL2TEST_CALLSIGN, modeName);
        EXPECT_EQ(Core::ERROR_NONE, rcAct);

        // Critical delay between activation and deactivation to prevent race conditions
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

         // Re-validate interfaces before deactivation
        ASSERT_TRUE(ValidateInterfaces());
        
        TEST_LOG("Starting ClientDeactivated call"); 
        Core::hresult rcDeact = m_sysmodeplugin->ClientDeactivated(SYSTEMMODEL2TEST_CALLSIGN, modeName);
        EXPECT_EQ(Core::ERROR_NONE, rcDeact);

        // Final cleanup delay
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
    } catch (const std::exception& e) {
        TEST_LOG("EXCEPTION in ClientActivationLifecycle test: %s", e.what());
        FAIL() << "Exception occurred during ClientActivationLifecycle: " << e.what();
    } catch (...) {
        TEST_LOG("UNKNOWN EXCEPTION in ClientActivationLifecycle test");
        FAIL() << "Unknown exception occurred during ClientActivationLifecycle";
    } 
}

// GetState after explicit request (ensures GetState returns latest value)
TEST_F(SystemMode_L2test, GetStateAfterRequest)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    
    try {
        // Add safety delay before operations
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        TEST_LOG("Requesting state change to VIDEO");
        EXPECT_EQ(Core::ERROR_NONE, m_sysmodeplugin->RequestState(Exchange::ISystemMode::DEVICE_OPTIMIZE, Exchange::ISystemMode::VIDEO));
        
        TEST_LOG("Waiting for state change to complete");
        ASSERT_TRUE(WaitForState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
            Exchange::ISystemMode::VIDEO));
            
        // Additional safety delay before state query
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        Exchange::ISystemMode::GetStateResult result{};
        TEST_LOG("Getting current state");
        EXPECT_EQ(Core::ERROR_NONE,
            m_sysmodeplugin->GetState(Exchange::ISystemMode::DEVICE_OPTIMIZE, result));
        EXPECT_EQ(Exchange::ISystemMode::VIDEO, result.state);
        
        TEST_LOG("GetStateAfterRequest completed successfully, state: %u", static_cast<uint32_t>(result.state));
        
    } catch (const std::exception& e) {
        TEST_LOG("EXCEPTION in GetStateAfterRequest test: %s", e.what());
        FAIL() << "Exception occurred during GetStateAfterRequest: " << e.what();
    } catch (...) {
        TEST_LOG("UNKNOWN EXCEPTION in GetStateAfterRequest test");
        FAIL() << "Unknown exception occurred during GetStateAfterRequest";
    }
}

// COM-RPC: ClientActivated with invalid system mode
TEST_F(SystemMode_L2test, ClientActivated_InvalidSystemMode)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);

    try {
        // Add safety delay before interface operation
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Test with invalid system mode - should return early (lines 260-261)
        TEST_LOG("Testing ClientActivated with invalid system mode");
        Core::hresult result = m_sysmodeplugin->ClientActivated(DISPLAYSETTINGS_CALLSIGN, "INVALID_SYSTEM_MODE");
        EXPECT_EQ(0, result);
        
        TEST_LOG("ClientActivated_InvalidSystemMode completed successfully, result: %u", result);
        
        // Safety delay after operation
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
    } catch (const std::exception& e) {
        TEST_LOG("EXCEPTION in ClientActivated_InvalidSystemMode test: %s", e.what());
        FAIL() << "Exception occurred during ClientActivated_InvalidSystemMode: " << e.what();
    } catch (...) {
        TEST_LOG("UNKNOWN EXCEPTION in ClientActivated_InvalidSystemMode test");
        FAIL() << "Unknown exception occurred during ClientActivated_InvalidSystemMode";
    }
}

// COM-RPC: ClientActivated with empty callsign
TEST_F(SystemMode_L2test, ClientActivated_EmptyCallsign)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    const std::string modeName = "DEVICE_OPTIMIZE";

    try {
        // Add safety delay before interface operation
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        TEST_LOG("Testing ClientActivated with empty callsign");
        Core::hresult result = m_sysmodeplugin->ClientActivated("", modeName);
        EXPECT_EQ(Core::ERROR_NONE, result);
        
        TEST_LOG("ClientActivated_EmptyCallsign completed successfully, result: %u", result);
        
        // Safety delay and cleanup if needed
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Attempt cleanup for empty callsign (may not be needed but safe)
        try {
            m_sysmodeplugin->ClientDeactivated("", modeName);
        } catch (...) {
            TEST_LOG("Cleanup for empty callsign not needed or failed (expected)");
        }
        
    } catch (const std::exception& e) {
        TEST_LOG("EXCEPTION in ClientActivated_EmptyCallsign test: %s", e.what());
        FAIL() << "Exception occurred during ClientActivated_EmptyCallsign: " << e.what();
    } catch (...) {
        TEST_LOG("UNKNOWN EXCEPTION in ClientActivated_EmptyCallsign test");
        FAIL() << "Unknown exception occurred during ClientActivated_EmptyCallsign";
    }
}

// COM-RPC: ClientActivated after a state has been requested
TEST_F(SystemMode_L2test, ClientActivated_AfterStateRequested)
{
    // Re-validate interfaces before deactivation
    ASSERT_TRUE(ValidateInterfaces());
    const std::string modeName = "DEVICE_OPTIMIZE";

    try {
        // Add safety delay before operations
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Step 1: Request a state to set stateRequested = true (line 185)
        Core::hresult stateResult = m_sysmodeplugin->RequestState(Exchange::ISystemMode::DEVICE_OPTIMIZE,
            Exchange::ISystemMode::GAME);
        EXPECT_EQ(Core::ERROR_NONE, stateResult);

        // Step 2: Wait for the state to be applied with timeout protection
        TEST_LOG("Waiting for state change to complete");
        EXPECT_TRUE(WaitForState(Exchange::ISystemMode::DEVICE_OPTIMIZE, Exchange::ISystemMode::GAME));
        
        // Additional safety delay and interface validation
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        ASSERT_TRUE(ValidateInterfaces());

        // The client should immediately receive the current state via Request() call
        TEST_LOG("Activating client after state change");
        Core::hresult activateResult = m_sysmodeplugin->ClientActivated(DISPLAYSETTINGS_CALLSIGN, modeName);
        EXPECT_EQ(Core::ERROR_NONE, activateResult);
        
        // Critical delay before deactivation
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        ASSERT_TRUE(ValidateInterfaces());                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                

        // Step 4: Immediate cleanup to prevent interface issues during shutdown
        TEST_LOG("Deactivating client for cleanup");
        Core::hresult cleanup = m_sysmodeplugin->ClientDeactivated(DISPLAYSETTINGS_CALLSIGN, modeName);
        EXPECT_EQ(Core::ERROR_NONE, cleanup);
        
        // Final safety delay
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
    } catch (const std::exception& e) {
        TEST_LOG("EXCEPTION in ClientActivated_AfterStateRequested test: %s", e.what());
        // Attempt cleanup on exception
        try {
            if (ValidateInterfaces()) {
            m_sysmodeplugin->ClientDeactivated(DISPLAYSETTINGS_CALLSIGN, modeName);
            }
        } catch (...) {
            TEST_LOG("Failed to cleanup client on exception");
        }
        FAIL() << "Exception occurred during ClientActivated_AfterStateRequested: " << e.what();
    } catch (...) {
        TEST_LOG("UNKNOWN EXCEPTION in ClientActivated_AfterStateRequested test");
        // Attempt cleanup on exception
        try {
            if (ValidateInterfaces()) {
            m_sysmodeplugin->ClientDeactivated(DISPLAYSETTINGS_CALLSIGN, modeName);
            }
        } catch (...) {
            TEST_LOG("Failed to cleanup client on unknown exception");
        }
        FAIL() << "Unknown exception occurred during ClientActivated_AfterStateRequested";
    }
}

// Cover case where file exists but no current state is set, should write default state VIDEO
TEST_F(SystemMode_L2test, FileExistsWithoutCurrentStateSetsDefault)
{
    if (m_sysmodeplugin) {
        m_sysmodeplugin->Release();
        m_sysmodeplugin = nullptr;
    }
    if (m_controller_sysmode) {
        m_controller_sysmode->Release();
        m_controller_sysmode = nullptr;
    }

    // Deactivate service with error handling
    TEST_LOG("Deactivating SystemMode service");
    uint32_t status = DeactivateService("org.rdk.SystemMode");
    if (status != Core::ERROR_NONE) {
        TEST_LOG("WARNING: SystemMode deactivation failed with status %u", status);
    }
    EXPECT_EQ(Core::ERROR_NONE, status);

    // File operations
    TEST_LOG("Setting up test file");
    removeFile(SYSTEM_MODE_FILE);
    createFile(SYSTEM_MODE_FILE, "DEVICE_OPTIMIZE.callsign=org.rdk.Dummy");

    // Reactivate the plugin with protection
    TEST_LOG("Reactivating SystemMode service");
    try {
        status = ActivateService("org.rdk.SystemMode");
        if (status != Core::ERROR_NONE) {
            TEST_LOG("ERROR: SystemMode reactivation failed with status %u", status);
            GTEST_FAIL() << "SystemMode reactivation failed, skipping test";
            return;
        }
        EXPECT_EQ(Core::ERROR_NONE, status);
    } catch (...) {
        TEST_LOG("EXCEPTION during SystemMode service reactivation");
        GTEST_FAIL() << "Exception during service reactivation";
        return;
    }

    // Recreate interface with protection
    TEST_LOG("Recreating SystemMode interface");
    uint32_t interfaceResult = CreateSystemModeInterfaceObject();
    if (interfaceResult != Core::ERROR_NONE) {
        TEST_LOG("ERROR: Failed to recreate SystemMode interface, status: %u", interfaceResult);
        GTEST_FAIL() << "Interface recreation failed";
        return;
    }
    ASSERT_EQ(Core::ERROR_NONE, interfaceResult);

    // Verify functionality if we have a valid interface
    if (m_sysmodeplugin) {
        TEST_LOG("Testing GetState functionality");
        try {
            Exchange::ISystemMode::GetStateResult result{};
            Core::hresult getStateResult = m_sysmodeplugin->GetState(Exchange::ISystemMode::DEVICE_OPTIMIZE, result);
            EXPECT_EQ(Core::ERROR_NONE, getStateResult);
            EXPECT_EQ(Exchange::ISystemMode::VIDEO, result.state);
        } catch (...) {
            TEST_LOG("EXCEPTION during GetState call");
            GTEST_FAIL() << "Exception during GetState";
            return;
        }
    } else {
        TEST_LOG("ERROR: m_sysmodeplugin is null after interface creation");
        GTEST_FAIL() << "Plugin interface is null";
        return;
    }

    // Cleanup with protection
    TEST_LOG("Starting test cleanup");
    try {
        if (m_sysmodeplugin) {
            m_sysmodeplugin->Release();
            m_sysmodeplugin = nullptr;
        }
        if (m_controller_sysmode) {
            m_controller_sysmode->Release();
            m_controller_sysmode = nullptr;
        }
        
        status = DeactivateService("org.rdk.SystemMode");
        if (status != Core::ERROR_NONE) {
            TEST_LOG("WARNING: Cleanup deactivation failed with status %u", status);
        }
        EXPECT_EQ(Core::ERROR_NONE, status);
        
        removeFile(SYSTEM_MODE_FILE);
        
        status = ActivateService("org.rdk.SystemMode");
        if (status != Core::ERROR_NONE) {
            TEST_LOG("WARNING: Cleanup reactivation failed with status %u", status);
        } else {
            uint32_t cleanupResult = CreateSystemModeInterfaceObject();
            if (cleanupResult != Core::ERROR_NONE) {
                TEST_LOG("WARNING: Cleanup interface recreation failed with status %u", cleanupResult);
            }
        }
    } catch (...) {
        TEST_LOG("EXCEPTION during test cleanup - continuing anyway");
    }
}

// ---------------- JSON-RPC TESTS ----------------

// JSON-RPC: requestState -> VIDEO then getState
TEST_F(SystemMode_L2test, JSONRPC_RequestState_VIDEO)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    JsonObject params;
    JsonObject result;
    uint32_t status = Core::ERROR_GENERAL;
    params["systemMode"] = "DEVICE_OPTIMIZE";
    params["state"] = "video";
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_TRUE(WaitForStateJsonRpc("DEVICE_OPTIMIZE", "video"));
}

// JSON-RPC:Invalid pstate requestState -> VIDEO then getState
TEST_F(SystemMode_L2test, JSONRPC_RequestState_InvalidState)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    JsonObject params;
    JsonObject result;
    uint32_t status = Core::ERROR_GENERAL;
    params["systemMode"] = "DEVICE_OPT";
    params["state"] = "Video";
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result);
    EXPECT_EQ(Core::ERROR_GENERAL, status);
}

// JSON-RPC: Invalid requestState
TEST_F(SystemMode_L2test, JSONRPC_RequestState_INVALID)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    JsonObject params;
    JsonObject result;
    uint32_t status = Core::ERROR_GENERAL;
    params["systemMode"] = "device_optimize";
    params["state"] = "vid";
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result);
    EXPECT_EQ(Core::ERROR_GENERAL, status);
}

// JSON-RPC: requestState -> GAME then getState
TEST_F(SystemMode_L2test, JSONRPC_RequestState_GAME)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    JsonObject params;
    JsonObject result;
    uint32_t status = Core::ERROR_GENERAL;
    params["systemMode"] = "DEVICE_OPTIMIZE";
    params["state"] = "game";
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_TRUE(WaitForStateJsonRpc("DEVICE_OPTIMIZE", "game"));
}

// JSON-RPC: getState after prior state request (ensure persistence)
TEST_F(SystemMode_L2test, JSONRPC_GetStateAfterRequest)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    // First set to video
    {
        JsonObject params;
        JsonObject result;
        params["systemMode"] = "DEVICE_OPTIMIZE";
        params["state"] = "video";
        EXPECT_EQ(Core::ERROR_NONE, InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result));
        EXPECT_TRUE(WaitForStateJsonRpc("DEVICE_OPTIMIZE", "video"));
    }
    // Now query
    JsonObject params;
    JsonObject result;
    params["systemMode"] = "DEVICE_OPTIMIZE";
    uint32_t status = InvokeServiceMethod("org.rdk.SystemMode.1", "getState", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    ASSERT_TRUE(result.HasLabel("state"));
    EXPECT_EQ(result["state"].String(), "video");
}

// JSON-RPC: getState invalid systemMode
TEST_F(SystemMode_L2test, JSONRPC_GetStateAfterRequestInvalid)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    JsonObject params;
    JsonObject result;
    params["systemMode"] = "device_opt"; // invalid systemMode
    uint32_t status = InvokeServiceMethod("org.rdk.SystemMode.1", "getState", params, result);
    EXPECT_EQ(Core::ERROR_GENERAL, status);
}

// JSON-RPC: clientActivated then clientDeactivated invalid systemMode
TEST_F(SystemMode_L2test, JSONRPC_ClientActivationLifecycle_InvalidSystemMode)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    params["callsign"] = SYSTEMMODEL2TEST_CALLSIGN;
    params["systemMode"] = "device_optimize"; // invalid systemMode
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "clientActivated", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    // reuse params for deactivation
    result.Clear();
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "clientDeactivated", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
}

// JSON-RPC: clientActivated then clientDeactivated
TEST_F(SystemMode_L2test, JSONRPC_ClientActivationLifecycle)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    uint32_t status = Core::ERROR_GENERAL;
    JsonObject params;
    JsonObject result;
    params["callsign"] = SYSTEMMODEL2TEST_CALLSIGN;
    params["systemMode"] = "DEVICE_OPTIMIZE";
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "clientActivated", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    // reuse params for deactivation
    result.Clear();
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "clientDeactivated", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
}

// JSON-RPC: Combined transition VIDEO -> GAME -> VIDEO
TEST_F(SystemMode_L2test, JSONRPC_StateTransition_VIDEO_GAME_VIDEO)
{
    JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(SYSTEMMODE_CALLSIGN, SYSTEMMODEL2TEST_CALLSIGN);
    JsonObject params;
    JsonObject result;
    uint32_t status;
    // VIDEO
    params["systemMode"] = "DEVICE_OPTIMIZE";
    params["state"] = "video";
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_TRUE(WaitForStateJsonRpc("DEVICE_OPTIMIZE", "video"));
    // GAME
    params["state"] = "game";
    result.Clear();
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_TRUE(WaitForStateJsonRpc("DEVICE_OPTIMIZE", "game"));
    // VIDEO
    params["state"] = "video";
    result.Clear();
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "requestState", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_TRUE(WaitForStateJsonRpc("DEVICE_OPTIMIZE", "video"));
}

// ClientActivated idempotency: activate twice, then deactivate once
TEST_F(SystemMode_L2test, Interface_ClientActivated_Idempotent)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    const std::string modeName = "DEVICE_OPTIMIZE";

    // First activation
    Core::hresult rcAct1 = m_sysmodeplugin->ClientActivated(DISPLAYSETTINGS_CALLSIGN, modeName);
    EXPECT_EQ(Core::ERROR_NONE, rcAct1);

    // Second activation should be handled gracefully (idempotent)
    Core::hresult rcAct2 = m_sysmodeplugin->ClientActivated(DISPLAYSETTINGS_CALLSIGN, modeName);
    EXPECT_EQ(Core::ERROR_NONE, rcAct2);

    // Cleanup: deactivate once
    Core::hresult rcDeact = m_sysmodeplugin->ClientDeactivated(DISPLAYSETTINGS_CALLSIGN, modeName);
    EXPECT_EQ(Core::ERROR_NONE, rcDeact);
}

// ClientDeactivated without prior activation should be handled gracefully
TEST_F(SystemMode_L2test, Interface_ClientDeactivated_WithoutActivation)
{
    ASSERT_TRUE(m_sysmodeplugin != nullptr);
    const std::string modeName = "DEVICE_OPTIMIZE";

    // Attempt deactivation for a callsign that was not activated
    Core::hresult rcDeact = m_sysmodeplugin->ClientDeactivated("nonexistent.callsign", modeName);
    EXPECT_EQ(Core::ERROR_NONE, rcDeact);
}

// JSON-RPC: clientActivated idempotency (activate twice) then deactivate once
TEST_F(SystemMode_L2test, JSONRPC_ClientActivation_Idempotent)
{
    JsonObject params;
    JsonObject result;
    params["callsign"] = DISPLAYSETTINGS_CALLSIGN;
    params["systemMode"] = "DEVICE_OPTIMIZE";

    // First activation
    uint32_t status = InvokeServiceMethod("org.rdk.SystemMode.1", "clientActivated", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);

    // Second activation (idempotency)
    result.Clear();
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "clientActivated", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);

    // Deactivate once
    result.Clear();
    status = InvokeServiceMethod("org.rdk.SystemMode.1", "clientDeactivated", params, result);
    EXPECT_EQ(Core::ERROR_NONE, status);
}
