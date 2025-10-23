/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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
 **/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "L2Tests.h"
#include "L2TestsMock.h"
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <interfaces/IDisplayInfo.h>
#include "devicesettings.h"
#include "HostMock.h"
#include "VideoOutputPortMock.h"
#include "VideoDeviceMock.h"
#include "AudioOutputPortMock.h"
#include "DisplayInfoMock.h"
#include "EdidParserMock.h"
#include "DrmMock.h"

#define JSON_TIMEOUT (1000)
#define COM_TIMEOUT (100)
#define TEST_LOG(x, ...)                                                                                                                         \
    fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); \
    fflush(stderr);
#define DISPLAYINFO_CALLSIGN _T("DisplayInfo.1")
#define DISPLAYINFOL2TEST_CALLSIGN _T("L2tests.1")

using ::testing::NiceMock;
using namespace WPEFramework;
using testing::StrictMock;
using ::WPEFramework::Exchange::IConnectionProperties;
using ::WPEFramework::Exchange::IGraphicsProperties;
using ::WPEFramework::Exchange::IHDRProperties;
using ::WPEFramework::Exchange::IDisplayProperties;

typedef enum : uint32_t {
    DISPLAYINFOL2TEST_RESOLUTION_CHANGED = 0x00000001,
    DISPLAYINFOL2TEST_CONNECTION_CHANGED = 0x00000002,
    DISPLAYINFOL2TEST_HDCP_CHANGED = 0x00000004,
    DISPLAYINFOL2TEST_STATE_INVALID = 0x00000000
} DisplayInfoL2test_async_events_t;

class DisplayInfo_Notification : public Exchange::IConnectionProperties::INotification {
private:
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    uint32_t m_event_signalled;

    BEGIN_INTERFACE_MAP(DisplayInfo_Notification)
    INTERFACE_ENTRY(Exchange::IConnectionProperties::INotification)
    END_INTERFACE_MAP

public:
    DisplayInfo_Notification() : m_event_signalled(DISPLAYINFOL2TEST_STATE_INVALID) {}
    ~DisplayInfo_Notification() {}

    template <typename T>
    T* baseInterface()
    {
        static_assert(std::is_base_of<T, DisplayInfo_Notification>(), "base type mismatch");
        return static_cast<T*>(this);
    }

    void Updated(const Source event) override
    {
        TEST_LOG("DisplayInfo Updated event triggered: %d", event);
        std::unique_lock<std::mutex> lock(m_mutex);

        switch(event) {
            case Source::PRE_RESOLUTION_CHANGE:
                TEST_LOG("PRE_RESOLUTION_CHANGE event received");
                m_event_signalled |= DISPLAYINFOL2TEST_RESOLUTION_CHANGED;
                break;
            case Source::POST_RESOLUTION_CHANGE:
                TEST_LOG("POST_RESOLUTION_CHANGE event received");
                m_event_signalled |= DISPLAYINFOL2TEST_RESOLUTION_CHANGED;
                break;
            case Source::HDMI_CHANGE:
                TEST_LOG("HDMI_CHANGE event received");
                m_event_signalled |= DISPLAYINFOL2TEST_CONNECTION_CHANGED;
                break;
            case Source::HDCP_CHANGE:
                TEST_LOG("HDCP_CHANGE event received");
                m_event_signalled |= DISPLAYINFOL2TEST_HDCP_CHANGED;
                break;
        }
        m_condition_variable.notify_one();
    }

    uint32_t WaitForRequestStatus(uint32_t timeout_ms, DisplayInfoL2test_async_events_t expected_status)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto now = std::chrono::system_clock::now();
        std::chrono::milliseconds timeout(timeout_ms);
        uint32_t signalled = DISPLAYINFOL2TEST_STATE_INVALID;

        while (!(expected_status & m_event_signalled)) {
            if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout) {
                TEST_LOG("Timeout waiting for DisplayInfo event");
                break;
            }
        }

        signalled = m_event_signalled;
        m_event_signalled = DISPLAYINFOL2TEST_STATE_INVALID;
        return signalled;
    }
};

/* DisplayInfo L2 test class declaration */
class DisplayInfo_L2Test : public L2TestMocks {

public:
    DisplayInfo_L2Test();
    virtual ~DisplayInfo_L2Test() override;

public:
    /**
     * @brief called when DisplayInfo resolution
     * changed notification received
     */
    void OnResolutionChanged(const IConnectionProperties::INotification::Source event);

    /**
     * @brief called when DisplayInfo connection
     * changed notification received
     */
    void OnConnectionChanged(const IConnectionProperties::INotification::Source event);

    /**
     * @brief called when DisplayInfo HDCP
     * changed notification received
     */
    void OnHdcpChanged(const IConnectionProperties::INotification::Source event);

    /**
     * @brief waits for various status change on asynchronous calls
     */
    uint32_t WaitForRequestStatus(uint32_t timeout_ms, DisplayInfoL2test_async_events_t expected_status);

    void Test_ConnectionProperties(Exchange::IConnectionProperties* connectionProperties);
    void Test_GraphicsProperties(Exchange::IGraphicsProperties* graphicsProperties);
    void Test_HDRProperties(Exchange::IHDRProperties* hdrProperties);
    void Test_DisplayProperties(Exchange::IDisplayProperties* displayProperties);
    void Test_HDCPProtection(Exchange::IConnectionProperties* connectionProperties);
    void Test_EDID_Retrieval(Exchange::IConnectionProperties* connectionProperties);

    Core::Sink<DisplayInfo_Notification> mNotification;

private:
    /** @brief Mutex */
    std::mutex m_mutex;

    /** @brief Condition variable */
    std::condition_variable m_condition_variable;

    /** @brief Event signalled flag */
    uint32_t m_event_signalled;
};

/**
 * @brief Constructor for DisplayInfo L2 test class
 */
DisplayInfo_L2Test::DisplayInfo_L2Test()
    : L2TestMocks()
{
    uint32_t status = Core::ERROR_GENERAL;
    m_event_signalled = DISPLAYINFOL2TEST_STATE_INVALID;

    string videoPort(_T("HDMI0"));
    string audioPort(_T("HDMI0"));

    device::VideoOutputPort videoOutputPort;
    device::VideoDevice videoDevice;
    device::VideoResolution videoResolution;
    device::VideoOutputPortType videoOutputPortType;
    device::AudioOutputPort audioFormat;

    // Setup default mock behaviors
    ON_CALL(*p_hostImplMock, getDefaultVideoPortName())
        .WillByDefault(::testing::Return(videoPort));
    ON_CALL(*p_videoOutputPortConfigImplMock, getPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, isDisplayConnected())
        .WillByDefault(::testing::Return(true));
    ON_CALL(*p_videoOutputPortMock, getDefaultResolution())
        .WillByDefault(::testing::ReturnRef(videoResolution));
    ON_CALL(*p_hostImplMock, getVideoOutputPort(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPort));
    ON_CALL(*p_videoOutputPortMock, getName())
        .WillByDefault(::testing::ReturnRef(videoPort));
    ON_CALL(*p_audioOutputPortMock, getName())
        .WillByDefault(::testing::ReturnRef(audioPort));
    ON_CALL(*p_videoOutputPortMock, getVideoEOTF())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>({ videoDevice })));
    
    // Setup HDR capabilities mock
    ON_CALL(*p_videoDeviceMock, getHDRCapabilities(testing::_))
        .WillByDefault([](int* capabilities) {
            *capabilities = dsHDRSTANDARD_HDR10 | dsHDRSTANDARD_HLG | dsHDRSTANDARD_DolbyVision;
            return dsERR_NONE;
        });
    
    // Setup color depth capabilities mock
    ON_CALL(*p_videoOutputPortMock, getColorDepthCapabilities(testing::_))
        .WillByDefault(testing::Invoke([&](unsigned int* capabilities) {
            *capabilities = dsDISPLAY_COLORDEPTH_8BIT | dsDISPLAY_COLORDEPTH_10BIT | dsDISPLAY_COLORDEPTH_12BIT;
            return dsERR_NONE;
        }));
    
    // Setup current output settings mock
    ON_CALL(*p_videoOutputPortMock, getCurrentOutputSettings(testing::_, testing::_, testing::_, testing::_, testing::_))
        .WillByDefault([](int& videoEOTF, int& matrixCoefficients, int& colorSpace, int& colorDepth, int& quantizationRange) {
            videoEOTF = 1;
            matrixCoefficients = 1;
            colorSpace = 1;
            colorDepth = 1;
            quantizationRange = 1;
            return dsERR_NONE;
        });
    
    ON_CALL(*p_videoOutputPortConfigImplMock, getPortType(::testing::_))
        .WillByDefault(::testing::ReturnRef(videoOutputPortType));
    ON_CALL(*p_videoOutputPortMock, getType())
        .WillByDefault(::testing::ReturnRef(videoOutputPortType));

    // Setup HDCP mock methods
    ON_CALL(*p_videoOutputPortMock, getHDCPProtocol())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*p_videoOutputPortMock, GetHdmiPreference())
        .WillByDefault(::testing::Return(1));
    ON_CALL(*p_videoOutputPortMock, SetHdmiPreference(::testing::_))
        .WillByDefault(::testing::Return(true));

    // Setup EDID mock
    std::vector<uint8_t> mockEdid = {
        0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
        0x4D, 0x10, 0x34, 0x12, 0x01, 0x00, 0x00, 0x00,
        0x20, 0x1C, 0x01, 0x03, 0x80, 0x30, 0x1B, 0x78,
        0x2A, 0x35, 0x85, 0xA6, 0x56, 0x48, 0x9A, 0x24,
        0x12, 0x50, 0x54, 0xBF, 0xEF, 0x00, 0x71, 0x4F,
        0x81, 0x40, 0x81, 0x80, 0x95, 0x00, 0xA9, 0x40,
        0xB3, 0x00, 0xD1, 0xC0, 0x81, 0xC0, 0x02, 0x3A,
        0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C,
        0x45, 0x00, 0xE0, 0x0E, 0x11, 0x00, 0x00, 0x1E,
        0x00, 0x00, 0x00, 0xFD, 0x00, 0x38, 0x4B, 0x1E,
        0x51, 0x11, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x54,
        0x65, 0x73, 0x74, 0x20, 0x4D, 0x6F, 0x6E, 0x69,
        0x74, 0x6F, 0x72, 0x0A, 0x00, 0x00, 0x00, 0xFF,
        0x00, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x0A, 0x20, 0x20, 0x20, 0x00, 0xC7
    };

    ON_CALL(*p_hostImplMock, getHostEDID(::testing::_))
        .WillByDefault(::testing::Invoke([mockEdid](std::vector<uint8_t>& edid) {
            edid = mockEdid;
        }));

    // Setup supported resolutions mock
    ON_CALL(*p_videoOutputPortMock, getSupportedTvResolutions(::testing::_))
        .WillByDefault(::testing::Invoke([&](int* tvResolutions) {
            // Set supported resolutions as bit flags
            *tvResolutions = (1 << dsVIDEO_PIXELRES_1920x1080) | (1 << dsVIDEO_PIXELRES_1280x720);
        }));

    // Setup registration expectations
    EXPECT_CALL(*p_hostImplMock, Register(::testing::An<device::Host::IVideoOutputPortEvents*>()))
        .WillRepeatedly(::testing::Return(dsERR_NONE));
    EXPECT_CALL(*p_hostImplMock, UnRegister(::testing::An<device::Host::IVideoOutputPortEvents*>()))
        .WillRepeatedly(::testing::Return(dsERR_NONE));

    /* Activate plugin in constructor */
    status = ActivateService("org.rdk.DisplayInfo");
    TEST_LOG("DisplayInfo service activation status: %u", status);
    EXPECT_EQ(Core::ERROR_NONE, status);
}

/**
 * @brief Destructor for DisplayInfo L2 test class
 */
DisplayInfo_L2Test::~DisplayInfo_L2Test()
{
    uint32_t status = Core::ERROR_GENERAL;
    m_event_signalled = DISPLAYINFOL2TEST_STATE_INVALID;

    status = DeactivateService("org.rdk.DisplayInfo");
    EXPECT_EQ(Core::ERROR_NONE, status);
    TEST_LOG("DisplayInfo service deactivation status: %u", status);
}

void DisplayInfo_L2Test::OnResolutionChanged(const IConnectionProperties::INotification::Source event)
{
    TEST_LOG("OnResolutionChanged event triggered: %d", event);
    std::unique_lock<std::mutex> lock(m_mutex);

    /* Notify the requester thread. */
    m_event_signalled |= DISPLAYINFOL2TEST_RESOLUTION_CHANGED;
    m_condition_variable.notify_one();
}

void DisplayInfo_L2Test::OnConnectionChanged(const IConnectionProperties::INotification::Source event)
{
    TEST_LOG("OnConnectionChanged event triggered: %d", event);
    std::unique_lock<std::mutex> lock(m_mutex);

    /* Notify the requester thread. */
    m_event_signalled |= DISPLAYINFOL2TEST_CONNECTION_CHANGED;
    m_condition_variable.notify_one();
}

void DisplayInfo_L2Test::OnHdcpChanged(const IConnectionProperties::INotification::Source event)
{
    TEST_LOG("OnHdcpChanged event triggered: %d", event);
    std::unique_lock<std::mutex> lock(m_mutex);

    /* Notify the requester thread. */
    m_event_signalled |= DISPLAYINFOL2TEST_HDCP_CHANGED;
    m_condition_variable.notify_one();
}

/**
 * @brief waits for various status change on asynchronous calls
 *
 * @param[in] timeout_ms timeout for waiting
 */
uint32_t DisplayInfo_L2Test::WaitForRequestStatus(uint32_t timeout_ms, DisplayInfoL2test_async_events_t expected_status)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    auto now = std::chrono::system_clock::now();
    std::chrono::milliseconds timeout(timeout_ms);
    uint32_t signalled = DISPLAYINFOL2TEST_STATE_INVALID;

    while (!(expected_status & m_event_signalled))
    {
        if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout)
        {
            TEST_LOG("Timeout waiting for DisplayInfo event");
            break;
        }
    }

    signalled = m_event_signalled;
    m_event_signalled = DISPLAYINFOL2TEST_STATE_INVALID;

    return signalled;
}

/**
 * @brief Compare two request status objects
 *
 * @param[in] data Expected value
 * @return true if the argument and data match, false otherwise
 */
MATCHER_P(MatchRequestStatus, data, "")
{
    bool match = true;
    std::string expected;
    std::string actual;

    data.ToString(expected);
    arg.ToString(actual);
    TEST_LOG(" rec = %s, arg = %s", expected.c_str(), actual.c_str());
    EXPECT_STREQ(expected.c_str(), actual.c_str());

    return match;
}

/* COM-RPC tests */
void DisplayInfo_L2Test::Test_ConnectionProperties(Exchange::IConnectionProperties* connectionProperties)
{
    uint32_t status = Core::ERROR_GENERAL;

    TEST_LOG("\n############## Running Test_ConnectionProperties Test ###################\n");

    bool connected = false;
    status = connectionProperties->Connected(connected);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("Connected: %s, status: %u", connected ? "true" : "false", status);

    uint32_t width = 0;
    status = connectionProperties->Width(width);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("Width: %u, status: %u", width, status);

    uint32_t height = 0;
    status = connectionProperties->Height(height);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("Height: %u, status: %u", height, status);

    uint32_t verticalFreq = 0;
    status = connectionProperties->VerticalFreq(verticalFreq);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("VerticalFreq: %u, status: %u", verticalFreq, status);

    bool audioPassthrough = false;
    status = connectionProperties->IsAudioPassthrough(audioPassthrough);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("AudioPassthrough: %s, status: %u", audioPassthrough ? "true" : "false", status);

    string portName;
    status = connectionProperties->PortName(portName);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("PortName: %s, status: %u", portName.c_str(), status);

    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

/* COM-RPC tests */
void DisplayInfo_L2Test::Test_GraphicsProperties(Exchange::IGraphicsProperties* graphicsProperties)
{
    uint32_t status = Core::ERROR_GENERAL;

    TEST_LOG("\n############## Running Test_GraphicsProperties Test ###################\n");

    uint64_t totalGpuRam = 0;
    status = graphicsProperties->TotalGpuRam(totalGpuRam);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_GT(totalGpuRam, 0);
    TEST_LOG("TotalGpuRam: %llu, status: %u", totalGpuRam, status);

    uint64_t freeGpuRam = 0;
    status = graphicsProperties->FreeGpuRam(freeGpuRam);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_GT(freeGpuRam, 0);
    EXPECT_LE(freeGpuRam, totalGpuRam);
    TEST_LOG("FreeGpuRam: %llu, status: %u", freeGpuRam, status);

    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

/* COM-RPC tests */
void DisplayInfo_L2Test::Test_HDRProperties(Exchange::IHDRProperties* hdrProperties)
{
    uint32_t status = Core::ERROR_GENERAL;

    TEST_LOG("\n############## Running Test_HDRProperties Test ###################\n");

    Exchange::IHDRProperties::HDRType hdrType;
    status = hdrProperties->HDRSetting(hdrType);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("HDRSetting: %u, status: %u", static_cast<uint32_t>(hdrType), status);

    Exchange::IHDRProperties::IHDRIterator* tvCapabilities = nullptr;
    status = hdrProperties->TVCapabilities(tvCapabilities);
    EXPECT_EQ(status, Core::ERROR_NONE);
    if (tvCapabilities) {
        TEST_LOG("TVCapabilities retrieved successfully");
        tvCapabilities->Release();
    }

    Exchange::IHDRProperties::IHDRIterator* stbCapabilities = nullptr;
    status = hdrProperties->STBCapabilities(stbCapabilities);
    EXPECT_EQ(status, Core::ERROR_NONE);
    if (stbCapabilities) {
        TEST_LOG("STBCapabilities retrieved successfully");
        stbCapabilities->Release();
    }

    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

/* COM-RPC tests */
void DisplayInfo_L2Test::Test_DisplayProperties(Exchange::IDisplayProperties* displayProperties)
{
    uint32_t status = Core::ERROR_GENERAL;

    TEST_LOG("\n############## Running Test_DisplayProperties Test ###################\n");

    Exchange::IDisplayProperties::ColourSpaceType colorSpace;
    status = displayProperties->ColorSpace(colorSpace);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("ColorSpace: %u, status: %u", static_cast<uint32_t>(colorSpace), status);

    Exchange::IDisplayProperties::FrameRateType frameRate;
    status = displayProperties->FrameRate(frameRate);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("FrameRate: %u, status: %u", static_cast<uint32_t>(frameRate), status);

    Exchange::IDisplayProperties::ColourDepthType colorDepth;
    status = displayProperties->ColourDepth(colorDepth);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("ColourDepth: %u, status: %u", static_cast<uint32_t>(colorDepth), status);

    Exchange::IDisplayProperties::QuantizationRangeType quantizationRange;
    status = displayProperties->QuantizationRange(quantizationRange);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("QuantizationRange: %u, status: %u", static_cast<uint32_t>(quantizationRange), status);

    Exchange::IDisplayProperties::EotfType eotf;
    status = displayProperties->EOTF(eotf);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("EOTF: %u, status: %u", static_cast<uint32_t>(eotf), status);

    Exchange::IDisplayProperties::IColorimetryIterator* colorimetry = nullptr;
    status = displayProperties->Colorimetry(colorimetry);
    EXPECT_EQ(status, Core::ERROR_NONE);
    if (colorimetry) {
        TEST_LOG("Colorimetry iterator retrieved successfully");
        colorimetry->Release();
    }

    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

/* COM-RPC tests */
void DisplayInfo_L2Test::Test_HDCPProtection(Exchange::IConnectionProperties* connectionProperties)
{
    uint32_t status = Core::ERROR_GENERAL;

    TEST_LOG("\n############## Running Test_HDCPProtection Test ###################\n");

    // Test setting HDCP protection
    Exchange::IConnectionProperties::HDCPProtectionType hdcpType = Exchange::IConnectionProperties::HDCPProtectionType::HDCP_1X;
    status = connectionProperties->HDCPProtection(hdcpType);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("Set HDCPProtection to HDCP_1X, status: %u", status);

    // Test getting HDCP protection
    Exchange::IConnectionProperties::HDCPProtectionType retrievedType;
    status = connectionProperties->HDCPProtection(retrievedType);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("Retrieved HDCPProtection: %u, status: %u", static_cast<uint32_t>(retrievedType), status);

    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

/* COM-RPC tests */
void DisplayInfo_L2Test::Test_EDID_Retrieval(Exchange::IConnectionProperties* connectionProperties)
{
    uint32_t status = Core::ERROR_GENERAL;

    TEST_LOG("\n############## Running Test_EDID_Retrieval Test ###################\n");

    uint16_t edidLength = 256;
    uint8_t edidData[256];
    status = connectionProperties->EDID(edidLength, edidData);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_GT(edidLength, 0);
    EXPECT_LE(edidLength, 256);
    TEST_LOG("EDID retrieved, length: %u, status: %u", edidLength, status);

    uint8_t widthCm = 0;
    status = connectionProperties->WidthInCentimeters(widthCm);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("WidthInCentimeters: %u, status: %u", widthCm, status);

    uint8_t heightCm = 0;
    status = connectionProperties->HeightInCentimeters(heightCm);
    EXPECT_EQ(status, Core::ERROR_NONE);
    TEST_LOG("HeightInCentimeters: %u, status: %u", heightCm, status);

    if (status != Core::ERROR_NONE)
    {
        std::string errorMsg = "COM-RPC returned error " + std::to_string(status) + " (" + std::string(Core::ErrorToString(status)) + ")";
        TEST_LOG("Err: %s", errorMsg.c_str());
    }
}

/********************************************************
************Test case Details **************************
** 1. Test Connection Properties functionality
** 2. Verify all connection-related properties can be retrieved
** 3. Test notification system for connection changes
*******************************************************/

TEST_F(DisplayInfo_L2Test, DisplayInfoComRpc)
{
    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> mEngine_DisplayInfo;
    Core::ProxyType<RPC::CommunicatorClient> mClient_DisplayInfo;
    PluginHost::IShell* mController_DisplayInfo;

    TEST_LOG("Creating mEngine_DisplayInfo");
    mEngine_DisplayInfo = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    mClient_DisplayInfo = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"), Core::ProxyType<Core::IIPCServer>(mEngine_DisplayInfo));

    TEST_LOG("Creating mEngine_DisplayInfo Announcements");
#if ((THUNDER_VERSION == 2) || ((THUNDER_VERSION == 4) && (THUNDER_VERSION_MINOR == 2)))
    mEngine_DisplayInfo->Announcements(mClient_DisplayInfo->Announcement());
#endif

    if (!mClient_DisplayInfo.IsValid())
    {
        TEST_LOG("Invalid mClient_DisplayInfo");
        FAIL() << "Failed to create DisplayInfo client";
    }
    else
    {
        mController_DisplayInfo = mClient_DisplayInfo->Open<PluginHost::IShell>(_T("org.rdk.DisplayInfo"), ~0, 3000);
        if (mController_DisplayInfo)
        {
            auto connectionProperties = mController_DisplayInfo->QueryInterface<Exchange::IConnectionProperties>();
            auto graphicsProperties = mController_DisplayInfo->QueryInterface<Exchange::IGraphicsProperties>();
            auto hdrProperties = mController_DisplayInfo->QueryInterface<Exchange::IHDRProperties>();
            auto displayProperties = mController_DisplayInfo->QueryInterface<Exchange::IDisplayProperties>();

            // Register for notifications
            if (connectionProperties) {
                connectionProperties->Register(mNotification.baseInterface<Exchange::IConnectionProperties::INotification>());
            }

            if (connectionProperties)
            {
                Test_ConnectionProperties(connectionProperties);
                Test_HDCPProtection(connectionProperties);
                Test_EDID_Retrieval(connectionProperties);
            }

            if (graphicsProperties)
            {
                Test_GraphicsProperties(graphicsProperties);
            }

            if (hdrProperties)
            {
                Test_HDRProperties(hdrProperties);
            }

            if (displayProperties)
            {
                Test_DisplayProperties(displayProperties);
            }

            // Cleanup - Unregister notifications
            if (connectionProperties) {
                connectionProperties->Unregister(mNotification.baseInterface<Exchange::IConnectionProperties::INotification>());
                connectionProperties->Release();
            }
            if (graphicsProperties) {
                graphicsProperties->Release();
            }
            if (hdrProperties) {
                hdrProperties->Release();
            }
            if (displayProperties) {
                displayProperties->Release();
            }

            mController_DisplayInfo->Release();
        }
        else
        {
            TEST_LOG("mController_DisplayInfo is NULL");
            FAIL() << "Failed to open DisplayInfo controller";
        }
    }
}

TEST_F(DisplayInfo_L2Test, DisplayInfoNotification)
{
    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> mEngine_DisplayInfo;
    Core::ProxyType<RPC::CommunicatorClient> mClient_DisplayInfo;
    PluginHost::IShell* mController_DisplayInfo;
    uint32_t signalled = DISPLAYINFOL2TEST_STATE_INVALID;

    TEST_LOG("Creating mEngine_DisplayInfo");
    mEngine_DisplayInfo = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    mClient_DisplayInfo = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"), Core::ProxyType<Core::IIPCServer>(mEngine_DisplayInfo));

    TEST_LOG("Creating mEngine_DisplayInfo Announcements");
#if ((THUNDER_VERSION == 2) || ((THUNDER_VERSION == 4) && (THUNDER_VERSION_MINOR == 2)))
    mEngine_DisplayInfo->Announcements(mClient_DisplayInfo->Announcement());
#endif

    if (!mClient_DisplayInfo.IsValid())
    {
        TEST_LOG("Invalid mClient_DisplayInfo");
        FAIL() << "Failed to create DisplayInfo client";
    }
    else
    {
        mController_DisplayInfo = mClient_DisplayInfo->Open<PluginHost::IShell>(_T("org.rdk.DisplayInfo"), ~0, 3000);
        if (mController_DisplayInfo)
        {
            auto connectionProperties = mController_DisplayInfo->QueryInterface<Exchange::IConnectionProperties>();

            if (connectionProperties)
            {
                // Register for notifications
                connectionProperties->Register(mNotification.baseInterface<Exchange::IConnectionProperties::INotification>());

                // Test connection status change
                bool connected = false;
                uint32_t status = connectionProperties->Connected(connected);
                EXPECT_EQ(status, Core::ERROR_NONE);
                TEST_LOG("Connection status: %s", connected ? "connected" : "disconnected");

                // Wait for notification (may not come immediately as it depends on hardware events)
                signalled = mNotification.WaitForRequestStatus(JSON_TIMEOUT, DISPLAYINFOL2TEST_CONNECTION_CHANGED);
                TEST_LOG("Notification signalled: 0x%x", signalled);

                // Cleanup
                connectionProperties->Unregister(mNotification.baseInterface<Exchange::IConnectionProperties::INotification>());
                connectionProperties->Release();
            }

            mController_DisplayInfo->Release();
        }
        else
        {
            TEST_LOG("mController_DisplayInfo is NULL");
            FAIL() << "Failed to open DisplayInfo controller";
        }
    }
}

TEST_F(DisplayInfo_L2Test, DisplayInfoInterfaceValidation)
{
    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> mEngine_DisplayInfo;
    Core::ProxyType<RPC::CommunicatorClient> mClient_DisplayInfo;
    PluginHost::IShell* mController_DisplayInfo;

    TEST_LOG("Creating mEngine_DisplayInfo for Interface Validation");
    mEngine_DisplayInfo = Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create();
    mClient_DisplayInfo = Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId("/tmp/communicator"), Core::ProxyType<Core::IIPCServer>(mEngine_DisplayInfo));

#if ((THUNDER_VERSION == 2) || ((THUNDER_VERSION == 4) && (THUNDER_VERSION_MINOR == 2)))
    mEngine_DisplayInfo->Announcements(mClient_DisplayInfo->Announcement());
#endif

    if (!mClient_DisplayInfo.IsValid())
    {
        TEST_LOG("Invalid mClient_DisplayInfo");
        FAIL() << "Failed to create DisplayInfo client";
    }
    else
    {
        mController_DisplayInfo = mClient_DisplayInfo->Open<PluginHost::IShell>(_T("org.rdk.DisplayInfo"), ~0, 3000);
        if (mController_DisplayInfo)
        {
            // Validate all required interfaces are available
            auto connectionProperties = mController_DisplayInfo->QueryInterface<Exchange::IConnectionProperties>();
            auto graphicsProperties = mController_DisplayInfo->QueryInterface<Exchange::IGraphicsProperties>();
            auto hdrProperties = mController_DisplayInfo->QueryInterface<Exchange::IHDRProperties>();
            auto displayProperties = mController_DisplayInfo->QueryInterface<Exchange::IDisplayProperties>();

            EXPECT_TRUE(connectionProperties != nullptr) << "IConnectionProperties interface should be available";
            EXPECT_TRUE(graphicsProperties != nullptr) << "IGraphicsProperties interface should be available";
            EXPECT_TRUE(hdrProperties != nullptr) << "IHDRProperties interface should be available";
            EXPECT_TRUE(displayProperties != nullptr) << "IDisplayProperties interface should be available";

            TEST_LOG("All DisplayInfo interfaces validated successfully");

            // Cleanup
            if (connectionProperties) connectionProperties->Release();
            if (graphicsProperties) graphicsProperties->Release();
            if (hdrProperties) hdrProperties->Release();
            if (displayProperties) displayProperties->Release();

            mController_DisplayInfo->Release();
        }
        else
        {
            TEST_LOG("mController_DisplayInfo is NULL");
            FAIL() << "Failed to open DisplayInfo controller";
        }
    }
}
