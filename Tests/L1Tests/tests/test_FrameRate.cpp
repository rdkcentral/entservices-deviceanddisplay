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

class FrameRateNotificationHandler : public Exchange::IFrameRate::INotification {
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
    
    BEGIN_INTERFACE_MAP(FrameRateNotificationHandler)
    INTERFACE_ENTRY(Exchange::IFrameRate::INotification)
    END_INTERFACE_MAP

public:
    FrameRateNotificationHandler() : m_event_signalled(0), m_last_average(0), m_last_min(0), m_last_max(0) {}
    ~FrameRateNotificationHandler() {}

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
 * @brief Comprehensive notification tests covering all notification trigger points
 */

// Test notification registration and unregistration
TEST_F(FrameRateTest, NotificationHandler_RegisterAndUnregister_Success)
{
    // Get the Exchange::IFrameRate interface
    ASSERT_TRUE(FrameRateImplem.IsValid());
    Exchange::IFrameRate* frameRateInterface = &(*FrameRateImplem);
    ASSERT_NE(frameRateInterface, nullptr);

    auto notificationHandler = std::make_shared<FrameRateNotificationHandler>();
    
    // Test successful registration
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->Register(notificationHandler.get()));
    
    // Test registration of the same handler again (should return ERROR_ALREADY_CONNECTED)
    EXPECT_EQ(Core::ERROR_ALREADY_CONNECTED, frameRateInterface->Register(notificationHandler.get()));
    
    // Test successful unregistration
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->Unregister(notificationHandler.get()));
    
    // Test unregistration of already unregistered handler (should return ERROR_GENERAL)
    EXPECT_EQ(Core::ERROR_GENERAL, frameRateInterface->Unregister(notificationHandler.get()));
    
    frameRateInterface->Release();
}

// Test FPS event notification via timer trigger
TEST_F(FrameRateTest, NotificationHandler_FpsEventViaTimerTrigger_Success)
{
    ASSERT_TRUE(FrameRateImplem.IsValid());
    Exchange::IFrameRate* frameRateInterface = &(*FrameRateImplem);
    ASSERT_NE(frameRateInterface, nullptr);

    auto notificationHandler = std::make_shared<FrameRateNotificationHandler>();
    frameRateInterface->Register(notificationHandler.get());
    
    bool success = false;
    
    // Start FPS collection to enable timer
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->StartFpsCollection(success));
    EXPECT_TRUE(success);
    
    // Update FPS values to have data for the timer callback
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->UpdateFps(30, success));
    EXPECT_TRUE(success);
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->UpdateFps(60, success));
    EXPECT_TRUE(success);
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->UpdateFps(45, success));
    EXPECT_TRUE(success);
    
    // Trigger the timer callback directly via the static instance
    if (Plugin::FrameRateImplementation::_instance != nullptr) {
        Plugin::FrameRateImplementation::_instance->onReportFpsTimer();
        
        // Small delay to allow notification processing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Verify FPS event was received
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnFpsEvent));
        EXPECT_TRUE(notificationHandler->IsFpsEventSignalled());
        
        // Verify notification parameters
        int average, min, max;
        notificationHandler->GetLastFpsEventParams(average, min, max);
        EXPECT_EQ(average, 45); // (30+60+45)/3 = 45
        EXPECT_EQ(min, 30);
        EXPECT_EQ(max, 60);
    }
    
    frameRateInterface->Unregister(notificationHandler.get());
    frameRateInterface->Release();
}

// Test FPS event notification via StopFpsCollection API
TEST_F(FrameRateTest, NotificationHandler_FpsEventViaStopCollection_Success)
{
    ASSERT_TRUE(FrameRateImplem.IsValid());
    Exchange::IFrameRate* frameRateInterface = &(*FrameRateImplem);
    ASSERT_NE(frameRateInterface, nullptr);

    auto notificationHandler = std::make_shared<FrameRateNotificationHandler>();
    frameRateInterface->Register(notificationHandler.get());
    
    bool success = false;
    
    // Start FPS collection
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->StartFpsCollection(success));
    EXPECT_TRUE(success);
    
    // Update some FPS values
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->UpdateFps(25, success));
    EXPECT_TRUE(success);
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->UpdateFps(50, success));
    EXPECT_TRUE(success);
    
    // Reset notification handler before stopping collection
    notificationHandler->Reset();
    
    // Stop FPS collection - this should trigger OnFpsEvent notification
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->StopFpsCollection(success));
    EXPECT_TRUE(success);
    
    // Small delay to allow notification processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify FPS event was received
    EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnFpsEvent));
    EXPECT_TRUE(notificationHandler->IsFpsEventSignalled());
    
    // Verify notification parameters
    int average, min, max;
    notificationHandler->GetLastFpsEventParams(average, min, max);
    EXPECT_EQ(average, 37); // (25+50)/2 = 37
    EXPECT_EQ(min, 25);
    EXPECT_EQ(max, 50);
    
    frameRateInterface->Unregister(notificationHandler.get());
    frameRateInterface->Release();
}

// Test display frame rate changing notification via device event
TEST_F(FrameRateTest, NotificationHandler_DisplayFrameRateChanging_Success)
{
    ASSERT_TRUE(FrameRateImplem.IsValid());
    Exchange::IFrameRate* frameRateInterface = &(*FrameRateImplem);
    ASSERT_NE(frameRateInterface, nullptr);

    auto notificationHandler = std::make_shared<FrameRateNotificationHandler>();
    frameRateInterface->Register(notificationHandler.get());
    
    // Reset notification handler
    notificationHandler->Reset();
    
    // Trigger pre-change event directly via the static instance
    if (Plugin::FrameRateImplementation::_instance != nullptr) {
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePreChange("3840x2160px30");
        
        // Allow some time for worker pool processing
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Verify display frame rate changing event was received
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanging));
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangingSignalled());
        
        // Verify notification parameters
        string displayFrameRate = notificationHandler->GetLastDisplayFrameRateChanging();
        EXPECT_EQ(displayFrameRate, "3840x2160px30");
    }
    
    frameRateInterface->Unregister(notificationHandler.get());
    frameRateInterface->Release();
}

// Test display frame rate changed notification via device event
TEST_F(FrameRateTest, NotificationHandler_DisplayFrameRateChanged_Success)
{
    ASSERT_TRUE(FrameRateImplem.IsValid());
    Exchange::IFrameRate* frameRateInterface = &(*FrameRateImplem);
    ASSERT_NE(frameRateInterface, nullptr);

    auto notificationHandler = std::make_shared<FrameRateNotificationHandler>();
    frameRateInterface->Register(notificationHandler.get());
    
    // Reset notification handler
    notificationHandler->Reset();
    
    // Trigger post-change event directly via the static instance
    if (Plugin::FrameRateImplementation::_instance != nullptr) {
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePostChange("1920x1080px60");
        
        // Allow some time for worker pool processing
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Verify display frame rate changed event was received
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanged));
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangedSignalled());
        
        // Verify notification parameters
        string displayFrameRate = notificationHandler->GetLastDisplayFrameRateChanged();
        EXPECT_EQ(displayFrameRate, "1920x1080px60");
    }
    
    frameRateInterface->Unregister(notificationHandler.get());
    frameRateInterface->Release();
}

// Test multiple notifications and selective reset functionality
TEST_F(FrameRateTest, NotificationHandler_MultipleNotificationsAndSelectiveReset_Success)
{
    ASSERT_TRUE(FrameRateImplem.IsValid());
    Exchange::IFrameRate* frameRateInterface = &(*FrameRateImplem);
    ASSERT_NE(frameRateInterface, nullptr);

    auto notificationHandler = std::make_shared<FrameRateNotificationHandler>();
    frameRateInterface->Register(notificationHandler.get());
    
    // Reset notification handler
    notificationHandler->Reset();
    
    // Trigger multiple events
    if (Plugin::FrameRateImplementation::_instance != nullptr) {
        bool success = false;
        
        // Start and update FPS collection
        frameRateInterface->StartFpsCollection(success);
        frameRateInterface->UpdateFps(120, success);
        
        // Trigger timer event
        Plugin::FrameRateImplementation::_instance->onReportFpsTimer();
        
        // Trigger frame rate change events
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePreChange("4096x2160px24");
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePostChange("4096x2160px24");
        
        // Allow time for processing
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        // Verify all events were received
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, 
            static_cast<FrameRateEventType_t>(FrameRate_OnFpsEvent | FrameRate_OnDisplayFrameRateChanging | FrameRate_OnDisplayFrameRateChanged)));
        
        EXPECT_TRUE(notificationHandler->IsFpsEventSignalled());
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangingSignalled());
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangedSignalled());
        
        // Test selective reset - reset only FPS event
        notificationHandler->ResetEvent(FrameRate_OnFpsEvent);
        EXPECT_FALSE(notificationHandler->IsFpsEventSignalled());
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangingSignalled());
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangedSignalled());
        
        // Test full reset
        notificationHandler->Reset();
        EXPECT_FALSE(notificationHandler->IsFpsEventSignalled());
        EXPECT_FALSE(notificationHandler->IsDisplayFrameRateChangingSignalled());
        EXPECT_FALSE(notificationHandler->IsDisplayFrameRateChangedSignalled());
    }
    
    frameRateInterface->Unregister(notificationHandler.get());
    frameRateInterface->Release();
}

// Test notification handler timeout behavior
TEST_F(FrameRateTest, NotificationHandler_TimeoutBehavior_Success)
{
    ASSERT_TRUE(FrameRateImplem.IsValid());
    Exchange::IFrameRate* frameRateInterface = &(*FrameRateImplem);
    ASSERT_NE(frameRateInterface, nullptr);

    auto notificationHandler = std::make_shared<FrameRateNotificationHandler>();
    frameRateInterface->Register(notificationHandler.get());
    
    // Reset notification handler
    notificationHandler->Reset();
    
    // Test timeout when no notification is sent
    auto startTime = std::chrono::system_clock::now();
    bool result = notificationHandler->WaitForRequestStatus(500, FrameRate_OnFpsEvent);
    auto endTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_FALSE(result);
    EXPECT_GE(duration.count(), 450); // Should wait at least 450ms
    EXPECT_LE(duration.count(), 600); // Should not wait more than 600ms
    
    frameRateInterface->Unregister(notificationHandler.get());
    frameRateInterface->Release();
}

// Test notification parameter storage and getter methods
TEST_F(FrameRateTest, NotificationHandler_ParameterStorageAndGetters_Success)
{
    ASSERT_TRUE(FrameRateImplem.IsValid());
    Exchange::IFrameRate* frameRateInterface = &(*FrameRateImplem);
    ASSERT_NE(frameRateInterface, nullptr);

    auto notificationHandler = std::make_shared<FrameRateNotificationHandler>();
    frameRateInterface->Register(notificationHandler.get());
    
    // Reset notification handler
    notificationHandler->Reset();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr) {
        bool success = false;
        
        // Test FPS parameter storage
        frameRateInterface->StartFpsCollection(success);
        frameRateInterface->UpdateFps(15, success);
        frameRateInterface->UpdateFps(90, success);
        frameRateInterface->UpdateFps(75, success);
        Plugin::FrameRateImplementation::_instance->onReportFpsTimer();
        
        // Test display frame rate parameter storage
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePreChange("7680x4320px120");
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePostChange("7680x4320px120");
        
        // Allow time for processing
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        // Wait for all notifications
        notificationHandler->WaitForRequestStatus(1000, 
            static_cast<FrameRateEventType_t>(FrameRate_OnFpsEvent | FrameRate_OnDisplayFrameRateChanging | FrameRate_OnDisplayFrameRateChanged));
        
        // Verify FPS parameters
        int average, min, max;
        notificationHandler->GetLastFpsEventParams(average, min, max);
        EXPECT_EQ(average, 60); // (15+90+75)/3 = 60
        EXPECT_EQ(min, 15);
        EXPECT_EQ(max, 90);
        
        // Verify display frame rate parameters
        string changingFrameRate = notificationHandler->GetLastDisplayFrameRateChanging();
        string changedFrameRate = notificationHandler->GetLastDisplayFrameRateChanged();
        EXPECT_EQ(changingFrameRate, "7680x4320px120");
        EXPECT_EQ(changedFrameRate, "7680x4320px120");
    }
    
    frameRateInterface->Unregister(notificationHandler.get());
    frameRateInterface->Release();
}

// Test notification handler with API calls that don't trigger notifications
TEST_F(FrameRateTest, NotificationHandler_NoNotificationAPIs_Success)
{
    ASSERT_TRUE(FrameRateImplem.IsValid());
    Exchange::IFrameRate* frameRateInterface = &(*FrameRateImplem);
    ASSERT_NE(frameRateInterface, nullptr);

    auto notificationHandler = std::make_shared<FrameRateNotificationHandler>();
    frameRateInterface->Register(notificationHandler.get());
    
    // Reset notification handler
    notificationHandler->Reset();
    
    // Configure video device mock for successful operations
    ON_CALL(*p_videoDeviceMock, getCurrentDisframerate(::testing::_))
        .WillByDefault(::testing::DoAll(
            ::testing::SetArrayArgument<0>("1920x1080x60", "1920x1080x60" + 13),
            ::testing::Return(0)));
    
    ON_CALL(*p_videoDeviceMock, getFRFMode(::testing::_))
        .WillByDefault(::testing::DoAll(
            ::testing::SetArgPointee<0>(1),
            ::testing::Return(0)));
    
    ON_CALL(*p_videoDeviceMock, setDisplayframerate(::testing::_))
        .WillByDefault(::testing::Return(0));
    
    ON_CALL(*p_videoDeviceMock, setFRFMode(::testing::_))
        .WillByDefault(::testing::Return(0));
    
    bool success = false;
    string framerate;
    int frmmode;
    
    // Call APIs that typically don't trigger notifications
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->GetDisplayFrameRate(framerate, success));
    EXPECT_TRUE(success);
    
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->GetFrmMode(frmmode, success));
    EXPECT_TRUE(success);
    
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->SetCollectionFrequency(5000, success));
    EXPECT_TRUE(success);
    
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->SetDisplayFrameRate("1920x1080x60", success));
    EXPECT_TRUE(success);
    
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->SetFrmMode(1, success));
    EXPECT_TRUE(success);
    
    // UpdateFps should not trigger notification directly
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->UpdateFps(30, success));
    EXPECT_TRUE(success);
    
    // Small delay to ensure no notifications are processed
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Verify no notifications were received
    EXPECT_FALSE(notificationHandler->WaitForRequestStatus(500, 
        static_cast<FrameRateEventType_t>(FrameRate_OnFpsEvent | FrameRate_OnDisplayFrameRateChanging | FrameRate_OnDisplayFrameRateChanged)));
    
    EXPECT_FALSE(notificationHandler->IsFpsEventSignalled());
    EXPECT_FALSE(notificationHandler->IsDisplayFrameRateChangingSignalled());
    EXPECT_FALSE(notificationHandler->IsDisplayFrameRateChangedSignalled());
    
    frameRateInterface->Unregister(notificationHandler.get());
    frameRateInterface->Release();
}

// Test worker pool job dispatch mechanism for frame rate events
TEST_F(FrameRateTest, NotificationHandler_WorkerPoolJobDispatch_Success)
{
    ASSERT_TRUE(FrameRateImplem.IsValid());
    Exchange::IFrameRate* frameRateInterface = &(*FrameRateImplem);
    ASSERT_NE(frameRateInterface, nullptr);

    auto notificationHandler = std::make_shared<FrameRateNotificationHandler>();
    frameRateInterface->Register(notificationHandler.get());
    
    // Reset notification handler
    notificationHandler->Reset();
    
    if (Plugin::FrameRateImplementation::_instance != nullptr) {
        // Test pre-change and post-change events through worker pool jobs
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePreChange("1280x720px50");
        Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePostChange("1280x720px50");
        
        // Allow sufficient time for worker pool processing
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        
        // Verify both events were received
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanging));
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnDisplayFrameRateChanged));
        
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangingSignalled());
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangedSignalled());
        
        // Verify parameters match for both events
        string changingFrameRate = notificationHandler->GetLastDisplayFrameRateChanging();
        string changedFrameRate = notificationHandler->GetLastDisplayFrameRateChanged();
        EXPECT_EQ(changingFrameRate, "1280x720px50");
        EXPECT_EQ(changedFrameRate, "1280x720px50");
    }
    
    frameRateInterface->Unregister(notificationHandler.get());
    frameRateInterface->Release();
}

// Test edge case: notification when FPS collection has no updates
TEST_F(FrameRateTest, NotificationHandler_FpsEventNoUpdates_Success)
{
    ASSERT_TRUE(FrameRateImplem.IsValid());
    Exchange::IFrameRate* frameRateInterface = &(*FrameRateImplem);
    ASSERT_NE(frameRateInterface, nullptr);

    auto notificationHandler = std::make_shared<FrameRateNotificationHandler>();
    frameRateInterface->Register(notificationHandler.get());
    
    // Reset notification handler
    notificationHandler->Reset();
    
    bool success = false;
    
    // Start FPS collection but don't update any values
    EXPECT_EQ(Core::ERROR_NONE, frameRateInterface->StartFpsCollection(success));
    EXPECT_TRUE(success);
    
    // Trigger timer without any FPS updates
    if (Plugin::FrameRateImplementation::_instance != nullptr) {
        Plugin::FrameRateImplementation::_instance->onReportFpsTimer();
        
        // Small delay to allow notification processing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Verify FPS event was received even with no updates
        EXPECT_TRUE(notificationHandler->WaitForRequestStatus(1000, FrameRate_OnFpsEvent));
        EXPECT_TRUE(notificationHandler->IsFpsEventSignalled());
        
        // Verify notification parameters for no-update scenario
        int average, min, max;
        notificationHandler->GetLastFpsEventParams(average, min, max);
        EXPECT_EQ(average, -1); // No updates = -1 average
        EXPECT_EQ(min, 60);     // DEFAULT_MIN_FPS_VALUE
        EXPECT_EQ(max, -1);     // DEFAULT_MAX_FPS_VALUE
    }
    
    frameRateInterface->Unregister(notificationHandler.get());
    frameRateInterface->Release();
}

// Test notification handler robustness with rapid events
TEST_F(FrameRateTest, NotificationHandler_RapidEvents_Success)
{
    ASSERT_TRUE(FrameRateImplem.IsValid());
    Exchange::IFrameRate* frameRateInterface = &(*FrameRateImplem);
    ASSERT_NE(frameRateInterface, nullptr);

    auto notificationHandler = std::make_shared<FrameRateNotificationHandler>();
    frameRateInterface->Register(notificationHandler.get());
    
    if (Plugin::FrameRateImplementation::_instance != nullptr) {
        // Reset notification handler
        notificationHandler->Reset();
        
        // Send rapid sequence of events
        for (int i = 0; i < 5; i++) {
            Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePreChange("640x480px30");
            Plugin::FrameRateImplementation::_instance->OnDisplayFrameratePostChange("640x480px30");
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        
        // Allow time for all events to be processed
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Verify at least one of each event type was received
        // (rapid events may overwrite previous parameters but flags should be set)
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangingSignalled());
        EXPECT_TRUE(notificationHandler->IsDisplayFrameRateChangedSignalled());
        
        // Last parameters should match the final event
        string changingFrameRate = notificationHandler->GetLastDisplayFrameRateChanging();
        string changedFrameRate = notificationHandler->GetLastDisplayFrameRateChanged();
        EXPECT_EQ(changingFrameRate, "640x480px30");
        EXPECT_EQ(changedFrameRate, "640x480px30");
    }
    
    frameRateInterface->Unregister(notificationHandler.get());
    frameRateInterface->Release();
}
