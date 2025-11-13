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
#include "COMLinkMock.h"
#include <gmock/gmock.h>

#include <condition_variable>
#include <mutex>
#include <chrono>
#include <thread>

#include "FrameRate.h"

#include "FactoriesImplementation.h"
#include "HostMock.h"
#include "IarmBusMock.h"
#include "ServiceMock.h"
#include "VideoDeviceMock.h"
#include "devicesettings.h"
#include "ManagerMock.h"
#include "ThunderPortability.h"
#include "FrameRateImplementation.h"
#include "FrameRateMock.h"
#include "WorkerPoolImplementation.h"
#include "WrapsMock.h"

using namespace WPEFramework;

using ::testing::NiceMock;

typedef enum : uint32_t {
    FrameRate_OnFpsEvent = 0x00000001,
    FrameRate_OnDisplayFrameRateChanging = 0x00000002,
    FrameRate_OnDisplayFrameRateChanged = 0x00000004,
    FrameRate_StateInvalid = 0x00000000
} FrameRateEventType_t;

class L1FrameRateNotificationHandler : public Exchange::IFrameRate::INotification {
private:
    std::mutex m_mutex;
    std::condition_variable m_condition_variable;
    uint32_t m_event_signalled;
    
    // Individual event flags and parameter storage
    bool m_fpsEvent_signalled = false;
    bool m_displayFrameRateChanging_signalled = false;
    bool m_displayFrameRateChanged_signalled = false;
    
    // Parameter storage for validation
    int m_last_average;
    int m_last_min;
    int m_last_max;
    string m_last_displayFrameRate_changing;
    string m_last_displayFrameRate_changed;
    
    mutable std::atomic<uint32_t> m_refCount{1};
    
    BEGIN_INTERFACE_MAP(L1FrameRateNotificationHandler)
    INTERFACE_ENTRY(Exchange::IFrameRate::INotification)
    END_INTERFACE_MAP

public:
    L1FrameRateNotificationHandler() : m_event_signalled(0), m_last_average(0), m_last_min(0), m_last_max(0) {}
    ~L1FrameRateNotificationHandler() {}

    // Implement IReferenceCounted interface
    void AddRef() const override {
        m_refCount.fetch_add(1, std::memory_order_relaxed);
    }
    
    uint32_t Release() const override {
        uint32_t count = m_refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (count == 0) {
            delete this;
        }
        return count;
    }

    // Implement notification methods from IFrameRate::INotification interface
    void OnFpsEvent(int average, int min, int max) override {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_last_average = average;
        m_last_min = min;
        m_last_max = max;
        m_fpsEvent_signalled = true;
        m_event_signalled |= FrameRate_OnFpsEvent;
        m_condition_variable.notify_one();
    }

    void OnDisplayFrameRateChanging(const string& displayFrameRate) override {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_last_displayFrameRate_changing = displayFrameRate;
        m_displayFrameRateChanging_signalled = true;
        m_event_signalled |= FrameRate_OnDisplayFrameRateChanging;
        m_condition_variable.notify_one();
    }

    void OnDisplayFrameRateChanged(const string& displayFrameRate) override {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_last_displayFrameRate_changed = displayFrameRate;
        m_displayFrameRateChanged_signalled = true;
        m_event_signalled |= FrameRate_OnDisplayFrameRateChanged;
        m_condition_variable.notify_one();
    }

    // Wait for specific event with timeout
    bool WaitForRequestStatus(uint32_t timeout_ms, FrameRateEventType_t expected_status) {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto now = std::chrono::system_clock::now();
        std::chrono::milliseconds timeout(timeout_ms);

        while (!(expected_status & m_event_signalled)) {
            if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout) {
                return false;
            }
        }
        return true;
    }

    // Convenience method to get last frame rate (matches the pattern you showed)
    string GetLastFrameRate() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_last_displayFrameRate_changed; // Return the last changed frame rate
    }

    // Parameter getter methods for validation
    void GetLastFpsEventParams(int& average, int& min, int& max) {
        std::lock_guard<std::mutex> lock(m_mutex);
        average = m_last_average;
        min = m_last_min;
        max = m_last_max;
    }

    string GetLastDisplayFrameRateChanging() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_last_displayFrameRate_changing;
    }

    string GetLastDisplayFrameRateChanged() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_last_displayFrameRate_changed;
    }

    // Individual event check methods
    bool IsFpsEventSignalled() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_fpsEvent_signalled;
    }

    bool IsDisplayFrameRateChangingSignalled() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_displayFrameRateChanging_signalled;
    }

    bool IsDisplayFrameRateChangedSignalled() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_displayFrameRateChanged_signalled;
    }

    // Reset all events and parameters
    void Reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_event_signalled = 0;
        m_fpsEvent_signalled = false;
        m_displayFrameRateChanging_signalled = false;
        m_displayFrameRateChanged_signalled = false;
        m_last_average = 0;
        m_last_min = 0;
        m_last_max = 0;
        m_last_displayFrameRate_changing.clear();
        m_last_displayFrameRate_changed.clear();
    }

    // Reset specific event
    void ResetEvent(FrameRateEventType_t event) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_event_signalled &= ~event;
        
        switch(event) {
            case FrameRate_OnFpsEvent:
                m_fpsEvent_signalled = false;
                m_last_average = 0;
                m_last_min = 0;
                m_last_max = 0;
                break;
            case FrameRate_OnDisplayFrameRateChanging:
                m_displayFrameRateChanging_signalled = false;
                m_last_displayFrameRate_changing.clear();
                break;
            case FrameRate_OnDisplayFrameRateChanged:
                m_displayFrameRateChanged_signalled = false;
                m_last_displayFrameRate_changed.clear();
                break;
            default:
                break;
        }
    }
};

class FrameRateTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::FrameRate> plugin;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    NiceMock<ServiceMock> service;
    NiceMock<COMLinkMock> comLinkMock;
    Core::ProxyType<WorkerPoolImplementation> workerPool;
    Core::ProxyType<Plugin::FrameRateImplementation> FrameRateImplem;
    Exchange::IFrameRate::INotification *FrameRateNotification = nullptr;
    NiceMock<FactoriesImplementation> factoriesImplementation;
    PLUGINHOST_DISPATCHER *dispatcher;
    string response;
    Core::JSONRPC::Message message;
    ServiceMock  *p_serviceMock  = nullptr;
    WrapsImplMock* p_wrapsImplMock = nullptr;
    FrameRateMock* p_framerateMock = nullptr;
    HostImplMock      *p_hostImplMock = nullptr;
    VideoDeviceMock   *p_videoDeviceMock = nullptr;
    IARM_EventHandler_t _iarmDSFramerateEventHandler;
    IarmBusImplMock   *p_iarmBusImplMock = nullptr ;
    ManagerImplMock   *p_managerImplMock = nullptr ;

    FrameRateTest()
        : plugin(Core::ProxyType<Plugin::FrameRate>::Create())
        , handler(*(plugin))
        , INIT_CONX(1, 0)
	, workerPool(Core::ProxyType<WorkerPoolImplementation>::Create(
          2, Core::Thread::DefaultStackSize(), 16))
    {
	    p_serviceMock = new NiceMock <ServiceMock>;

        p_framerateMock  = new NiceMock <FrameRateMock>;

        p_wrapsImplMock = new NiceMock<WrapsImplMock>;
        Wraps::setImpl(p_wrapsImplMock);

        p_managerImplMock  = new NiceMock <ManagerImplMock>;
        device::Manager::setImpl(p_managerImplMock);

        p_hostImplMock  = new NiceMock <HostImplMock>;
        device::Host::setImpl(p_hostImplMock);

        EXPECT_CALL(*p_managerImplMock, Initialize())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return());

        PluginHost::IFactories::Assign(&factoriesImplementation);

        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
        plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
        dispatcher->Activate(&service);

        ON_CALL(*p_framerateMock, Register(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](Exchange::IFrameRate::INotification* notification) {
                    FrameRateNotification = notification;
		    return Core::ERROR_NONE;
                }));

#ifdef USE_THUNDER_R4
        ON_CALL(comLinkMock, Instantiate(::testing::_, ::testing::_, ::testing::_))
                .WillByDefault(::testing::Invoke(
                    [&](const RPC::Object& object, const uint32_t waitTime, uint32_t& connectionId) {
                        FrameRateImplem = Core::ProxyType<Plugin::FrameRateImplementation>::Create();
                        return &FrameRateImplem;
                    }));
#else
	ON_CALL(comLinkMock, Instantiate(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
	    .WillByDefault(::testing::Return(FrameRateImplem));
#endif
        p_iarmBusImplMock  = new NiceMock <IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        Core::IWorkerPool::Assign(&(*workerPool));
            workerPool->Run();

        plugin->Initialize(&service);

        device::VideoDevice videoDevice;
        p_videoDeviceMock  = new NiceMock <VideoDeviceMock>;
        device::VideoDevice::setImpl(p_videoDeviceMock);

        ON_CALL(*p_hostImplMock, getVideoDevices())
            .WillByDefault(::testing::Return(device::List<device::VideoDevice>({ videoDevice })));
    }
    virtual ~FrameRateTest()
    {
        plugin->Deinitialize(&service);
        Core::IWorkerPool::Assign(nullptr);
        workerPool.Release();

	    if (p_serviceMock != nullptr)
        {
            delete p_serviceMock;
            p_serviceMock = nullptr;
        }

        if (p_framerateMock != nullptr) {
            delete p_framerateMock;
            p_framerateMock = nullptr;
        }

        Wraps::setImpl(nullptr);
        if (p_wrapsImplMock != nullptr) {
            delete p_wrapsImplMock;
            p_wrapsImplMock = nullptr;
        }

        device::VideoDevice::setImpl(nullptr);
        if (p_videoDeviceMock != nullptr)
        {
            delete p_videoDeviceMock;
            p_videoDeviceMock = nullptr;
        }

        EXPECT_CALL(*p_managerImplMock, DeInitialize())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return());

        device::Manager::setImpl(nullptr);
        if (p_managerImplMock != nullptr)
        {
            delete p_managerImplMock;
            p_managerImplMock = nullptr;
        }

        device::Host::setImpl(nullptr);
        if (p_hostImplMock != nullptr)
        {
            delete p_hostImplMock;
            p_hostImplMock = nullptr;
        }
        dispatcher->Deactivate();
        dispatcher->Release();

        PluginHost::IFactories::Assign(nullptr);
	IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr) {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }

    }
};




TEST_F(FrameRateTest, GetDisplayFrameRate_Success)
{
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::DoAll(
            ::testing::SetArrayArgument<0>("1920x1080x60", "1920x1080x60" + 13),
            ::testing::Return(0)));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
    EXPECT_TRUE(response.find("\"success\":true") != string::npos);
    EXPECT_TRUE(response.find("\"framerate\":\"1920x1080x60\"") != string::npos);
}

TEST_F(FrameRateTest, GetDisplayFrameRate_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
}

TEST_F(FrameRateTest, GetDisplayFrameRate_DeviceError)
{
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
}

TEST_F(FrameRateTest, GetFrmMode_Success)
{
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::DoAll(
            ::testing::SetArgPointee<0>(1),
            ::testing::Return(0)));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
    EXPECT_TRUE(response.find("\"success\":true") != string::npos);
    EXPECT_TRUE(response.find("\"auto-frm-mode\":1") != string::npos);
}

TEST_F(FrameRateTest, GetFrmMode_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
}

TEST_F(FrameRateTest, GetFrmMode_DeviceError)
{
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
}

TEST_F(FrameRateTest, SetCollectionFrequency_Success)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":5000}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetCollectionFrequency_InvalidParameterTooLow)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":50}"), response));
}

TEST_F(FrameRateTest, SetCollectionFrequency_MinimumValue)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":100}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetDisplayFrameRate_Success)
{
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Return(0));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080x60\"}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetDisplayFrameRate_InvalidFormat_MissingX)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920_1080_60\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_InvalidFormat_OneX)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x108060\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_InvalidFormat_NonDigitStart)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"x1920x1080x60\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_InvalidFormat_NonDigitEnd)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080x60x\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080x60\"}"), response));
}

TEST_F(FrameRateTest, SetDisplayFrameRate_DeviceError)
{
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"1920x1080x60\"}"), response));
}

TEST_F(FrameRateTest, SetFrmMode_Success_ModeZero)
{
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Return(0));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":0}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetFrmMode_Success_ModeOne)
{
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Return(0));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":1}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, SetFrmMode_InvalidParameter_NegativeValue)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":-1}"), response));
}

TEST_F(FrameRateTest, SetFrmMode_InvalidParameter_ValueTwo)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":2}"), response));
}

TEST_F(FrameRateTest, SetFrmMode_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":1}"), response));
}

TEST_F(FrameRateTest, SetFrmMode_DeviceError)
{
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Return(1));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":1}"), response));
}

TEST_F(FrameRateTest, StartFpsCollection_Success)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, StartFpsCollection_AlreadyInProgress)
{
    handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response);
    
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startFpsCollection"), _T("{}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, StopFpsCollection_Success)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, StopFpsCollection_NotStarted)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopFpsCollection"), _T("{}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, UpdateFps_Success)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":30}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, UpdateFps_SuccessZeroValue)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":0}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, UpdateFps_InvalidParameter_NegativeValue)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":-1}"), response));
}

TEST_F(FrameRateTest, UpdateFps_HighValue)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":120}"), response));
    EXPECT_TRUE(response.find("true") != string::npos);
}

TEST_F(FrameRateTest, Information_Success)
{
    string info = plugin->Information();
    EXPECT_EQ("Plugin which exposes FrameRate related methods and notifications.", info);
}

/** Negative Test Cases **/
TEST_F(FrameRateTest, setCollectionFrequency_InvalidParameter_BelowMinimum)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setCollectionFrequency"), _T("{\"frequency\":50}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, updateFps_InvalidParameter_NegativeValue)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("updateFps"), _T("{\"newFpsValue\":-1}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, setFrmMode_InvalidParameter_InvalidMode)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":2}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, setFrmMode_InvalidParameter_NegativeMode)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":-1}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, setFrmMode_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":0}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, setFrmMode_DeviceException)
{
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int param) {
                throw device::Exception("Test exception");
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":0}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, setFrmMode_DeviceError)
{
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int param) {
                EXPECT_EQ(param, 0);
                return 1;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setFrmMode"), _T("{\"frmmode\":0}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, getFrmMode_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, getFrmMode_DeviceException)
{
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int* param) {
                throw device::Exception("Test exception");
                *param = 0;
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, getFrmMode_DeviceError)
{
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](int* param) {
                *param = 0;
                return 1;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getFrmMode"), _T("{}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, setDisplayFrameRate_InvalidParameter_InvalidFormat_NoX)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840z2160px48\"}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, setDisplayFrameRate_InvalidParameter_InvalidFormat_OneX)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840x2160z48\"}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, setDisplayFrameRate_InvalidParameter_InvalidFormat_StartsWithLetter)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"a3840x2160px48\"}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, setDisplayFrameRate_InvalidParameter_InvalidFormat_EndsWithLetter)
{
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840x2160px48a\"}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, setDisplayFrameRate_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840x2160px48\"}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, setDisplayFrameRate_DeviceException)
{
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* param) {
                throw device::Exception("Test exception");
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840x2160px48\"}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, setDisplayFrameRate_DeviceError)
{
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* param) {
                EXPECT_EQ(param, string("3840x2160px48"));
                return 1;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setDisplayFrameRate"), _T("{\"framerate\":\"3840x2160px48\"}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, getDisplayFrameRate_NoVideoDevices)
{
    ON_CALL(*p_hostImplMock, getVideoDevices())
        .WillByDefault(::testing::Return(device::List<device::VideoDevice>()));

    EXPECT_EQ(Core::ERROR_NOT_SUPPORTED, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, getDisplayFrameRate_DeviceException)
{
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](char* param) {
                throw device::Exception("Test exception");
                string framerate("3840x2160px48");
                ::memcpy(param, framerate.c_str(), framerate.length());
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, getDisplayFrameRate_DeviceError)
{
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](char* param) {
                param[0] = '\0';
                return 1;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
    EXPECT_EQ(response, "");
}

TEST_F(FrameRateTest, getDisplayFrameRate_EmptyFramerate)
{
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](char* param) {
                param[0] = '\0';
                return 0;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getDisplayFrameRate"), _T("{}"), response));
    EXPECT_EQ(response, "");
}

/**
 * @brief Comprehensive notification tests following proper L1 test patterns
 */

// ========================================
// FrameRate Plugin L1 Notification Tests
// ========================================

/**
 * @brief Test OnDisplayFrameRateChanging notification via direct OnDisplayFrameratePreChange method call
 * Uses static instance method to directly trigger the notification
 */
TEST_F(FrameRateTest, NotificationViaOnDisplayFrameratePreChange_DirectMethod_Success)
{
    L1FrameRateNotificationHandler* notificationHandler = new L1FrameRateNotificationHandler();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr)
    {
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(notificationHandler));
        
        // Direct method call to trigger OnDisplayFrameRateChanging notification
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePreChange("1920x1080x60");
        
        // Allow worker pool to process the event
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanging));
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangingSignalled());
        EXPECT_EQ("1920x1080x60", notificationHandler->GetLastDisplayFrameRateChanging());
        
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(notificationHandler));
    }
    
    notificationHandler->Release();
}

/**
 * @brief Test OnDisplayFrameRateChanged notification via direct OnDisplayFrameratePostChange method call
 * Uses static instance method to directly trigger the notification
 */
TEST_F(FrameRateTest, NotificationViaOnDisplayFrameratePostChange_DirectMethod_Success)
{
    L1FrameRateNotificationHandler* notificationHandler = new L1FrameRateNotificationHandler();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr)
    {
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(notificationHandler));
        
        // Direct method call to trigger OnDisplayFrameRateChanged notification
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePostChange("3840x2160x30");
        
        // Allow worker pool to process the event
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanged));
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangedSignalled());
        EXPECT_EQ("3840x2160x30", notificationHandler->GetLastDisplayFrameRateChanged());
        
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(notificationHandler));
    }
    
    notificationHandler->Release();
}

/**
 * @brief Test OnFpsEvent notification via direct onReportFpsTimer method call
 * Uses static instance method to directly trigger the timer callback notification
 */
TEST_F(FrameRateTest, NotificationViaOnReportFpsTimer_DirectMethod_Success)
{
    L1FrameRateNotificationHandler* notificationHandler = new L1FrameRateNotificationHandler();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr)
    {
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(notificationHandler));
        
        // Setup some FPS data first
        bool success = false;
        Plugin::FrameRateImplementation::_instance->UpdateFps(60, success);
        EXPECT_TRUE(success);
        
        Plugin::FrameRateImplementation::_instance->UpdateFps(59, success);
        EXPECT_TRUE(success);
        
        Plugin::FrameRateImplementation::_instance->UpdateFps(61, success);
        EXPECT_TRUE(success);
        
        // Direct method call to trigger OnFpsEvent notification
        Plugin::FrameRateImplementation::_instance->onReportFpsTimer();
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnFpsEvent));
        EXPECT_TRUE(notificationHandler->IsFpsEventSignalled());
        
        int average, min, max;
        notificationHandler->GetLastFpsEventParams(average, min, max);
        EXPECT_EQ(60, average);  // (60+59+61)/3 = 60
        EXPECT_EQ(59, min);
        EXPECT_EQ(61, max);
        
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(notificationHandler));
    }
    
    notificationHandler->Release();
}

/**
 * @brief Test OnFpsEvent notification via timer with no FPS data
 * Tests default values when no FPS updates have been made
 */
TEST_F(FrameRateTest, NotificationViaOnReportFpsTimer_NoFpsData_DefaultValues)
{
    L1FrameRateNotificationHandler* notificationHandler = new L1FrameRateNotificationHandler();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr)
    {
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(notificationHandler));
        
        // Direct method call without any FPS data
        Plugin::FrameRateImplementation::_instance->onReportFpsTimer();
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnFpsEvent));
        EXPECT_TRUE(notificationHandler->IsFpsEventSignalled());
        
        int average, min, max;
        notificationHandler->GetLastFpsEventParams(average, min, max);
        EXPECT_EQ(-1, average);   // No updates, so average is -1
        EXPECT_EQ(60, min);       // DEFAULT_MIN_FPS_VALUE
        EXPECT_EQ(-1, max);       // DEFAULT_MAX_FPS_VALUE
        
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(notificationHandler));
    }
    
    notificationHandler->Release();
}

/**
 * @brief Test OnDisplayFrameRateChanging notification with different frame rates
 * Tests multiple frame rate values for prechange notification
 */
TEST_F(FrameRateTest, OnDisplayFrameRateChanging_ViaPreChange_DifferentFrameRates)
{
    L1FrameRateNotificationHandler* notificationHandler = new L1FrameRateNotificationHandler();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr)
    {
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(notificationHandler));
        
        // Test with 4K 60fps
        notificationHandler->Reset();
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePreChange("3840x2160x60");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanging));
        EXPECT_EQ("3840x2160x60", notificationHandler->GetLastDisplayFrameRateChanging());
        
        // Test with HD 30fps
        notificationHandler->Reset();
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePreChange("1280x720x30");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanging));
        EXPECT_EQ("1280x720x30", notificationHandler->GetLastDisplayFrameRateChanging());
        
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(notificationHandler));
    }
    
    notificationHandler->Release();
}

/**
 * @brief Test OnDisplayFrameRateChanged notification with different frame rates
 * Tests multiple frame rate values for postchange notification
 */
TEST_F(FrameRateTest, OnDisplayFrameRateChanged_ViaPostChange_DifferentFrameRates)
{
    L1FrameRateNotificationHandler* notificationHandler = new L1FrameRateNotificationHandler();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr)
    {
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(notificationHandler));
        
        // Test with Full HD 50fps
        notificationHandler->Reset();
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePostChange("1920x1080x50");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanged));
        EXPECT_EQ("1920x1080x50", notificationHandler->GetLastDisplayFrameRateChanged());
        
        // Test with 4K 24fps
        notificationHandler->Reset();
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePostChange("3840x2160x24");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanged));
        EXPECT_EQ("3840x2160x24", notificationHandler->GetLastDisplayFrameRateChanged());
        
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(notificationHandler));
    }
    
    notificationHandler->Release();
}

/**
 * @brief Test OnFpsEvent notification with varying FPS values
 * Tests FPS event with different min/max scenarios
 */
TEST_F(FrameRateTest, OnFpsEvent_ViaTimer_VaryingFpsValues)
{
    L1FrameRateNotificationHandler* notificationHandler = new L1FrameRateNotificationHandler();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr)
    {
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(notificationHandler));
        
        // Test with wide range of FPS values
        bool success = false;
        Plugin::FrameRateImplementation::_instance->UpdateFps(30, success);
        EXPECT_TRUE(success);
        
        Plugin::FrameRateImplementation::_instance->UpdateFps(120, success);
        EXPECT_TRUE(success);
        
        Plugin::FrameRateImplementation::_instance->UpdateFps(60, success);
        EXPECT_TRUE(success);
        
        Plugin::FrameRateImplementation::_instance->UpdateFps(24, success);
        EXPECT_TRUE(success);
        
        // Trigger notification
        Plugin::FrameRateImplementation::_instance->onReportFpsTimer();
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnFpsEvent));
        
        int average, min, max;
        notificationHandler->GetLastFpsEventParams(average, min, max);
        EXPECT_EQ(58, average);  // (30+120+60+24)/4 = 58.5, truncated to 58
        EXPECT_EQ(24, min);
        EXPECT_EQ(120, max);
        
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(notificationHandler));
    }
    
    notificationHandler->Release();
}

/**
 * @brief Test OnFpsEvent notification with zero FPS value
 * Tests edge case where FPS is zero
 */
TEST_F(FrameRateTest, OnFpsEvent_ViaTimer_ZeroFpsValue)
{
    L1FrameRateNotificationHandler* notificationHandler = new L1FrameRateNotificationHandler();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr)
    {
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(notificationHandler));
        
        // Test with zero FPS
        bool success = false;
        Plugin::FrameRateImplementation::_instance->UpdateFps(0, success);
        EXPECT_TRUE(success);
        
        Plugin::FrameRateImplementation::_instance->UpdateFps(5, success);
        EXPECT_TRUE(success);
        
        // Trigger notification
        Plugin::FrameRateImplementation::_instance->onReportFpsTimer();
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnFpsEvent));
        
        int average, min, max;
        notificationHandler->GetLastFpsEventParams(average, min, max);
        EXPECT_EQ(2, average);  // (0+5)/2 = 2.5, truncated to 2
        EXPECT_EQ(0, min);
        EXPECT_EQ(5, max);
        
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(notificationHandler));
    }
    
    notificationHandler->Release();
}

/**
 * @brief Test sequence of both display frame rate notifications
 * Tests prechange followed by postchange notifications
 */
TEST_F(FrameRateTest, DisplayFrameRateNotifications_PreAndPostChange_Sequence)
{
    L1FrameRateNotificationHandler* notificationHandler = new L1FrameRateNotificationHandler();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr)
    {
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(notificationHandler));
        
        // First trigger prechange
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePreChange("1920x1080x30");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanging));
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangingSignalled());
        EXPECT_EQ("1920x1080x30", notificationHandler->GetLastDisplayFrameRateChanging());
        
        // Reset and trigger postchange
        notificationHandler->ResetEvent(FrameRate_OnDisplayFrameRateChanging);
        
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePostChange("1920x1080x30");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanged));
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangedSignalled());
        EXPECT_EQ("1920x1080x30", notificationHandler->GetLastDisplayFrameRateChanged());
        
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(notificationHandler));
    }
    
    notificationHandler->Release();
}

/**
 * @brief Test multiple notification handler registrations
 * Tests that multiple handlers receive the same notification
 */
TEST_F(FrameRateTest, MultipleNotificationHandlers_SameEvent)
{
    L1FrameRateNotificationHandler* handler1 = new L1FrameRateNotificationHandler();
    L1FrameRateNotificationHandler* handler2 = new L1FrameRateNotificationHandler();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr)
    {
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(handler1));
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(handler2));
        
        // Trigger notification that should reach both handlers
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePreChange("2560x1440x120");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Both handlers should receive the notification
        EXPECT_TRUE(handler1->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanging));
        EXPECT_TRUE(handler2->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanging));
        
        EXPECT_EQ("2560x1440x120", handler1->GetLastDisplayFrameRateChanging());
        EXPECT_EQ("2560x1440x120", handler2->GetLastDisplayFrameRateChanging());
        
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(handler1));
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(handler2));
    }
    
    handler1->Release();
    handler2->Release();
}

/**
 * @brief Test notification with empty frame rate string
 * Tests edge case with empty string parameter
 */
TEST_F(FrameRateTest, DisplayFrameRateNotification_EmptyString)
{
    L1FrameRateNotificationHandler* notificationHandler = new L1FrameRateNotificationHandler();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr)
    {
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(notificationHandler));
        
        // Test with empty string
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePreChange("");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanging));
        EXPECT_EQ("", notificationHandler->GetLastDisplayFrameRateChanging());
        
        // Test postchange with empty string
        notificationHandler->Reset();
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePostChange("");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanged));
        EXPECT_EQ("", notificationHandler->GetLastDisplayFrameRateChanged());
        
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(notificationHandler));
    }
    
    notificationHandler->Release();
}

/**
 * @brief Test FPS notification with single value update
 * Tests scenario where only one FPS value is updated
 */
TEST_F(FrameRateTest, OnFpsEvent_SingleValueUpdate)
{
    L1FrameRateNotificationHandler* notificationHandler = new L1FrameRateNotificationHandler();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr)
    {
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(notificationHandler));
        
        // Single FPS update
        bool success = false;
        Plugin::FrameRateImplementation::_instance->UpdateFps(75, success);
        EXPECT_TRUE(success);
        
        // Trigger notification
        Plugin::FrameRateImplementation::_instance->onReportFpsTimer();
        
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnFpsEvent));
        
        int average, min, max;
        notificationHandler->GetLastFpsEventParams(average, min, max);
        EXPECT_EQ(75, average);
        EXPECT_EQ(75, min);
        EXPECT_EQ(75, max);
        
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(notificationHandler));
    }
    
    notificationHandler->Release();
}

/**
 * @brief Test notification handler double registration
 * Tests that same handler can't be registered twice
 */
TEST_F(FrameRateTest, NotificationHandler_DoubleRegistration)
{
    L1FrameRateNotificationHandler* notificationHandler = new L1FrameRateNotificationHandler();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr)
    {
        // First registration should succeed
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Register(notificationHandler));
        
        // Second registration of same handler should fail
        EXPECT_EQ(Core::ERROR_ALREADY_CONNECTED, Plugin::FrameRateImplementation::_instance->Register(notificationHandler));
        
        // Unregister once
        EXPECT_EQ(Core::ERROR_NONE, Plugin::FrameRateImplementation::_instance->Unregister(notificationHandler));
    }
    
    notificationHandler->Release();
}

