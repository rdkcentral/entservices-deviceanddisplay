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
#include <chrono>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <core/Portability.h>
#include <core/Proxy.h>
#include <core/Services.h>
#include <gmock/gmock.h>

#include <interfaces/IPowerManager.h>

#include "Iarm.h"
#include "PowerManagerHalMock.h"
#include "PowerManagerImplementation.h"
#include "WorkerPoolImplementation.h"

#include "IarmBusMock.h"
#include "MfrMock.h"
#include "RfcApiMock.h"
#include "WrapsMock.h"

// utils
#include "WaitGroup.h"

using namespace WPEFramework;
using ::testing::NiceMock;

using WakeupReason  = WPEFramework::Exchange::IPowerManager::WakeupReason;
using WakeupSrcType = WPEFramework::Exchange::IPowerManager::WakeupSrcType;

class TestPowerManager : public ::testing::Test {

protected:
    WrapsImplMock* p_wrapsImplMock     = nullptr;
    RfcApiImplMock* p_rfcApiImplMock   = nullptr;
    IarmBusImplMock* p_iarmBusImplMock = nullptr;

public:
    bool wait_call = true;
    std::mutex m_mutex;
    Core::ProxyType<Plugin::PowerManagerImplementation> powerManagerImpl;
    WaitGroup setupWg; // wait group created specifically for setup / init (SetUpMocks)

    struct PowerModePreChangeEvent : public WPEFramework::Exchange::IPowerManager::IModePreChangeNotification {
        MOCK_METHOD(void, OnPowerModePreChange, (const PowerState currentState, const PowerState newState, const int transactionId, const int stateChangeAfter), (override));

        BEGIN_INTERFACE_MAP(PowerModePreChangeEvent)
        INTERFACE_ENTRY(Exchange::IPowerManager::IModePreChangeNotification)
        END_INTERFACE_MAP
    };

    struct PowerModeChangedEvent : public WPEFramework::Exchange::IPowerManager::IModeChangedNotification {
        MOCK_METHOD(void, OnPowerModeChanged, (const PowerState, const PowerState), (override));

        BEGIN_INTERFACE_MAP(PowerModeChangedEvent)
        INTERFACE_ENTRY(Exchange::IPowerManager::IModeChangedNotification)
        END_INTERFACE_MAP
    };

    struct DeepSleepWakeupEvent : public WPEFramework::Exchange::IPowerManager::IDeepSleepTimeoutNotification {
        MOCK_METHOD(void, OnDeepSleepTimeout, (const int), (override));

        BEGIN_INTERFACE_MAP(DeepSleepWakeupEvent)
        INTERFACE_ENTRY(Exchange::IPowerManager::IDeepSleepTimeoutNotification)
        END_INTERFACE_MAP
    };

    struct RebootEvent : public WPEFramework::Exchange::IPowerManager::IRebootNotification {
        MOCK_METHOD(void, OnRebootBegin, (const string&, const string&, const string&), (override));

        BEGIN_INTERFACE_MAP(RebootEvent)
        INTERFACE_ENTRY(Exchange::IPowerManager::IRebootNotification)
        END_INTERFACE_MAP
    };

    struct NetworkStandbyChangedEvent : public WPEFramework::Exchange::IPowerManager::INetworkStandbyModeChangedNotification {
        MOCK_METHOD(void, OnNetworkStandbyModeChanged, (const bool), (override));

        BEGIN_INTERFACE_MAP(NetworkStandbyChangedEvent)
        INTERFACE_ENTRY(Exchange::IPowerManager::INetworkStandbyModeChangedNotification)
        END_INTERFACE_MAP
    };

    TestPowerManager()
    {
        SetUpMocks();

        setupWg.Add(1);
        powerManagerImpl = Core::ProxyType<Plugin::PowerManagerImplementation>::Create();
        setupWg.Wait();

        // We see sync issues between Thermal pollThermalLevels & Power Manager Thermal Getter APIs
        // Safer side add a small delay (actual fix will need mutex to sync Caller then and Writer thread)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void SetUpMocks()
    {
        p_wrapsImplMock = new NiceMock<WrapsImplMock>;
        Wraps::setImpl(p_wrapsImplMock);

        p_rfcApiImplMock = new NiceMock<RfcApiImplMock>;
        RfcApi::setImpl(p_rfcApiImplMock);

        p_iarmBusImplMock = new NiceMock<IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_INIT())
            .WillOnce(::testing::Return(PWRMGR_SUCCESS));
        EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_TERM())
            .WillOnce(::testing::Return(PWRMGR_SUCCESS));

        EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_INIT())
            .WillOnce(::testing::Return(DEEPSLEEPMGR_SUCCESS));
        EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_TERM())
            .WillOnce(::testing::Return(DEEPSLEEPMGR_SUCCESS));

        ON_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [](char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData) {
                    if (strcmp("RFC_DATA_ThermalProtection_POLL_INTERVAL", pcParameterName) == 0) {
                        strcpy(pstParamData->value, "1");
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

        // called from ThermalController constructor in initializeThermalProtection
        EXPECT_CALL(mfrMock::Mock(), mfrSetTempThresholds(::testing::_, ::testing::_))
            .WillOnce(::testing::Invoke(
                [](int high, int critical) {
                    EXPECT_EQ(high, 100);
                    EXPECT_EQ(critical, 110);
                    return mfrERR_NONE;
                }));

        // called from pollThermalLevels
        EXPECT_CALL(mfrMock::Mock(), mfrGetTemperature(::testing::_, ::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Invoke(
                [this](mfrTemperatureState_t* state, int* temperatureValue, int* wifiTemp) {
                    *state            = mfrTEMPERATURE_NORMAL;
                    *temperatureValue = 40;
                    *wifiTemp         = 35;
                    setupWg.Done();
                    return mfrERR_NONE;
                }));

        // called from PowerController::init (constructor)
        EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_GetPowerState(::testing::_))
            .WillRepeatedly(::testing::Invoke(
                [](PWRMgr_PowerState_t* powerState) {
                    *powerState = PWRMGR_POWERSTATE_OFF; // by default over boot up, return PowerState OFF
                    return PWRMGR_SUCCESS;
                }));

        // called from PowerController::init (constructor)
        EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetPowerState(::testing::_))
            .WillRepeatedly(::testing::Invoke(
                [](PWRMgr_PowerState_t powerState) {
                    // All tests are run without settings file
                    // so default expected power state is ON
                    EXPECT_EQ(powerState, PWRMGR_POWERSTATE_ON);
                    return PWRMGR_SUCCESS;
                }));

        EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetWakeupSrc(::testing::_, ::testing::_))
            .Times(2)
            .WillRepeatedly(::testing::Invoke(
                [](PWRMGR_WakeupSrcType_t wakeupSrc, bool enabled) {
                    // EXPECT_EQ(wakeupSrc, PWRMGR_WAKEUPSRC_WIFI);
                    // EXPECT_EQ(enabled, true);
                    return PWRMGR_SUCCESS;
                }));
    }

    void TearDownMocks()
    {
        PowerManagerHalMock::Delete();
        mfrMock::Delete();
    }

    ~TestPowerManager() override
    {
        TEST_LOG("DTOR is called, %p", this);
        powerManagerImpl.Release();
        EXPECT_EQ(powerManagerImpl.IsValid(), false);

        Wraps::setImpl(nullptr);
        if (p_wrapsImplMock != nullptr) {
            delete p_wrapsImplMock;
            p_wrapsImplMock = nullptr;
        }

        RfcApi::setImpl(nullptr);
        if (p_rfcApiImplMock != nullptr) {
            delete p_rfcApiImplMock;
            p_rfcApiImplMock = nullptr;
        }

        IarmBus::setImpl(nullptr);
        if (p_iarmBusImplMock != nullptr) {
            delete p_iarmBusImplMock;
            p_iarmBusImplMock = nullptr;
        }

        TearDownMocks();

        system("rm /opt/uimgr_settings.bin");

        // Although this file is not created always
        // delete to avoid dependency among test cases
        system("rm -f /tmp/deepSleepTimer");
        system("rm -f /tmp/deepSleepTimerVal");
        system("rm -f /tmp/ignoredeepsleep");

        // in some rare cases we saw settings file being reused from
        // old testcase, fs sync would resolve such issues
        system("sync");
    }

    static void SetUpTestSuite()
    {
        // static WorkerPoolImplementation workerPool(4, WPEFramework::Core::Thread::DefaultStackSize(), 16);
        static WorkerPoolImplementation workerPool(4, 64 * 1024, 16);
        WPEFramework::Core::WorkerPool::Assign(&workerPool);
        workerPool.Run();
    }
};

TEST_F(TestPowerManager, GetLastWakeupReason)
{
    WakeupReason wakeupReason = WakeupReason::WAKEUP_REASON_UNKNOWN;

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_GetLastWakeupReason(::testing::_))
        .WillOnce(::testing::Invoke(
            [](DeepSleep_WakeupReason_t* wakeupReason) {
                *wakeupReason = DEEPSLEEP_WAKEUPREASON_IR;
                return DEEPSLEEPMGR_SUCCESS;
            }));

    uint32_t status = powerManagerImpl->GetLastWakeupReason(wakeupReason);

    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(wakeupReason, WakeupReason::WAKEUP_REASON_IR);
}

TEST_F(TestPowerManager, GetLastWakeupKeyCode)
{
    int wakeupKeyCode = 0;

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_GetLastWakeupKeyCode(::testing::_))
        .WillOnce(::testing::Invoke(
            [](DeepSleepMgr_WakeupKeyCode_Param_t* param) {
                // ASSERT_TRUE(param != nullptr);
                EXPECT_TRUE(param != nullptr);
                param->keyCode = 1234;
                return DEEPSLEEPMGR_SUCCESS;
            }));

    uint32_t status = powerManagerImpl->GetLastWakeupKeyCode(wakeupKeyCode);

    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(wakeupKeyCode, 1234);
}

TEST_F(TestPowerManager, SetWakeupSrcConfig)
{
    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetWakeupSrc(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](PWRMGR_WakeupSrcType_t wakeupSrc, bool enabled) {
                EXPECT_EQ(wakeupSrc, PWRMGR_WAKEUPSRC_WIFI);
                EXPECT_EQ(enabled, true);
                return PWRMGR_SUCCESS;
            }));

    int powerMode = 0;
    int config    = WakeupSrcType::WAKEUP_SRC_WIFI;

    uint32_t status = powerManagerImpl->SetWakeupSrcConfig(powerMode, WakeupSrcType::WAKEUP_SRC_WIFI, config);

    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, GetWakeupSrcConfig)
{
    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_GetWakeupSrc(::testing::_, ::testing::_))
        .Times(10)
        .WillRepeatedly(::testing::Invoke(
            [](PWRMGR_WakeupSrcType_t wakeupSrc, bool* enabled) {
                EXPECT_TRUE(enabled != nullptr);

                if (PWRMGR_WAKEUPSRC_WIFI == wakeupSrc) {
                    *enabled = true;
                } else {
                    *enabled = false;
                }
                return PWRMGR_SUCCESS;
            }));

    int powerMode = 0, config = 0;
    int src         = WakeupSrcType::WAKEUP_SRC_WIFI | WakeupSrcType::WAKEUP_SRC_IR;
    uint32_t status = powerManagerImpl->GetWakeupSrcConfig(powerMode, src, config);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(powerMode, 0);
    EXPECT_EQ(config, WakeupSrcType::WAKEUP_SRC_WIFI);
}

TEST_F(TestPowerManager, GetPowerStateBeforeReboot)
{
    PowerState powerState = PowerState::POWER_STATE_UNKNOWN;
    auto status           = powerManagerImpl->GetPowerStateBeforeReboot(powerState);
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, GetCoreTemperature)
{
    float temp  = 0;
    auto status = powerManagerImpl->GetThermalState(temp);
    EXPECT_EQ(temp, 40.0); // 40 is set in SetUpMocks
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, PowerModePreChangeAck)
{
    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetPowerState(::testing::_))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP);
                return PWRMGR_SUCCESS;
            }));

    int keyCode = 0;

    uint32_t clientId = 0;
    uint32_t status   = powerManagerImpl->AddPowerModePreChangeClient("l1-test-client", clientId);
    EXPECT_EQ(status, Core::ERROR_NONE);

    Core::ProxyType<PowerModePreChangeEvent> prechangeEvent = Core::ProxyType<PowerModePreChangeEvent>::Create();

    status = powerManagerImpl->Register(&(*prechangeEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);

    WaitGroup wg;
    wg.Add();
    EXPECT_CALL(*prechangeEvent, OnPowerModePreChange(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&](const PowerState currentState, const PowerState newState, const int transactionId, const int stateChangeAfter) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
                EXPECT_EQ(stateChangeAfter, 1);

                // Delay power mode change by 10 seconds
                auto status = powerManagerImpl->DelayPowerModeChangeBy(clientId, transactionId, 10);
                EXPECT_EQ(status, Core::ERROR_NONE);

                // Delay Change with invalid clientId
                status = powerManagerImpl->DelayPowerModeChangeBy(clientId + 10, transactionId, 10);
                EXPECT_EQ(status, Core::ERROR_INVALID_PARAMETER);
                status = powerManagerImpl->DelayPowerModeChangeBy(clientId, transactionId + 10, 10);
                EXPECT_EQ(status, Core::ERROR_INVALID_PARAMETER);

                // delay by smaller value
                status = powerManagerImpl->DelayPowerModeChangeBy(clientId, transactionId, 5);
                EXPECT_EQ(status, Core::ERROR_NONE);

                // Acknowledge - Change Complete with invalid transactionId
                status = powerManagerImpl->PowerModePreChangeComplete(clientId, transactionId + 10);
                EXPECT_EQ(status, Core::ERROR_INVALID_PARAMETER);
                // Acknowledge - Change Complete with invalid clientId
                status = powerManagerImpl->PowerModePreChangeComplete(clientId + 10, transactionId);
                EXPECT_EQ(status, Core::ERROR_INVALID_PARAMETER);

                // valid PowerModePreChangeComplete
                status = powerManagerImpl->PowerModePreChangeComplete(clientId, transactionId);
                EXPECT_EQ(status, Core::ERROR_NONE);

                wg.Done();
            }));

    // Even though same state is set multiple times only one pre change notification is invoked
    status = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);
    status = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    wg.Wait();
    // some delay to destroy AckController after IModeChanged notification
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    PowerState currentState = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState    = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(currentState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(currentState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
    EXPECT_EQ(int(prevState), int(PowerState::POWER_STATE_ON));

    status = powerManagerImpl->RemovePowerModePreChangeClient(clientId);
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->Unregister(&(*prechangeEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, PowerModePreChangeAckTimeout)
{
    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetPowerState(::testing::_))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP);
                return PWRMGR_SUCCESS;
            }));

    int keyCode = 0;

    uint32_t clientId = 0;
    uint32_t status   = powerManagerImpl->AddPowerModePreChangeClient("l1-test-client", clientId);
    EXPECT_EQ(status, Core::ERROR_NONE);

    Core::ProxyType<PowerModePreChangeEvent> prechangeEvent = Core::ProxyType<PowerModePreChangeEvent>::Create();
    Core::ProxyType<PowerModeChangedEvent> modeChangedEvent = Core::ProxyType<PowerModeChangedEvent>::Create();

    EXPECT_EQ(status, powerManagerImpl->Register(&(*prechangeEvent)));
    EXPECT_EQ(status, powerManagerImpl->Register(&(*modeChangedEvent)));

    EXPECT_CALL(*prechangeEvent, OnPowerModePreChange(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&](const PowerState currentState, const PowerState newState, const int transactionId, const int stateChangeAfter) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
                EXPECT_EQ(stateChangeAfter, 1);
            }));

    WaitGroup wg;
    wg.Add(1);
    EXPECT_CALL(*modeChangedEvent, OnPowerModeChanged(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&](const PowerState currState, const PowerState newState) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
                wg.Done();
            }));

    status = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    wg.Wait();
    // some delay to destroy AckController after IModeChanged notification
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    PowerState currentState = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState    = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(currentState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(currentState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
    EXPECT_EQ(prevState, PowerState::POWER_STATE_ON);

    status = powerManagerImpl->Unregister(&(*prechangeEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->Unregister(&(*modeChangedEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, PowerModePreChangeUnregisterBeforeAck)
{
    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetPowerState(::testing::_))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP);
                return PWRMGR_SUCCESS;
            }));

    int keyCode = 0;

    uint32_t clientId = 0;
    uint32_t status   = powerManagerImpl->AddPowerModePreChangeClient("l1-test-client", clientId);
    EXPECT_EQ(status, Core::ERROR_NONE);

    Core::ProxyType<PowerModePreChangeEvent> prechangeEvent = Core::ProxyType<PowerModePreChangeEvent>::Create();
    Core::ProxyType<PowerModeChangedEvent> modeChangedEvent = Core::ProxyType<PowerModeChangedEvent>::Create();

    EXPECT_EQ(status, powerManagerImpl->Register(&(*prechangeEvent)));
    EXPECT_EQ(status, powerManagerImpl->Register(&(*modeChangedEvent)));

    WaitGroup wg;
    wg.Add();
    EXPECT_CALL(*prechangeEvent, OnPowerModePreChange(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&](const PowerState currentState, const PowerState newState, const int transactionId, const int stateChangeAfter) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
                EXPECT_EQ(stateChangeAfter, 1);

                // Delay power mode change by 10 seconds
                auto status = powerManagerImpl->DelayPowerModeChangeBy(clientId, transactionId, 1);
                EXPECT_EQ(status, Core::ERROR_NONE);

                // Extend delay power mode change by 10 seconds
                status = powerManagerImpl->DelayPowerModeChangeBy(clientId, transactionId, 10);
                EXPECT_EQ(status, Core::ERROR_NONE);

                // acknowledge after a short delay
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                wg.Done();
            }));

    // Even though same state is set multiple times only one pre change notification is invoked
    status = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);
    status = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    wg.Wait();

    wg.Add();
    EXPECT_CALL(*modeChangedEvent, OnPowerModeChanged(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&](const PowerState currState, const PowerState newState) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
                wg.Done();
            }));

    status = powerManagerImpl->RemovePowerModePreChangeClient(clientId);
    EXPECT_EQ(status, Core::ERROR_NONE);

    wg.Wait();

    PowerState currentState = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState    = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(currentState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(currentState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
    EXPECT_EQ(prevState, PowerState::POWER_STATE_ON);

    status = powerManagerImpl->Unregister(&(*prechangeEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->Unregister(&(*modeChangedEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, DeepSleepIgnore)
{
    system("touch /tmp/ignoredeepsleep");

    uint32_t clientId = 0;
    uint32_t status   = powerManagerImpl->AddPowerModePreChangeClient("l1-test-client", clientId);
    EXPECT_EQ(status, Core::ERROR_NONE);

    Core::ProxyType<PowerModePreChangeEvent> prechangeEvent = Core::ProxyType<PowerModePreChangeEvent>::Create();

    status = powerManagerImpl->Register(&(*prechangeEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);

    WaitGroup wg;
    wg.Add();
    EXPECT_CALL(*prechangeEvent, OnPowerModePreChange(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&](const PowerState currentState, const PowerState newState, const int transactionId, const int stateChangeAfter) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
                EXPECT_EQ(stateChangeAfter, 1);

                auto status = powerManagerImpl->PowerModePreChangeComplete(clientId, transactionId);
                EXPECT_EQ(status, Core::ERROR_NONE);

                wg.Done();
            }));

    status = powerManagerImpl->SetDeepSleepTimer(10);
    EXPECT_EQ(status, Core::ERROR_NONE);

    int keyCode = 0;
    status      = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    wg.Wait();

    PowerState newState  = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(newState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_NE(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);

    status = powerManagerImpl->Unregister(&(*prechangeEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, DeepSleepUserWakeup)
{
    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetPowerState(::testing::_))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP);
                return PWRMGR_SUCCESS;
            }))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP);
                return PWRMGR_SUCCESS;
            }));

    WaitGroup wg;
    wg.Add();
    Core::ProxyType<PowerModeChangedEvent> modeChanged = Core::ProxyType<PowerModeChangedEvent>::Create();
    EXPECT_CALL(*modeChanged, OnPowerModeChanged(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
            }))
        .WillOnce(::testing::Invoke(
            [&](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(prevState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
                wg.Done();
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_SetDeepSleep(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](uint32_t deep_sleep_timeout, bool* isGPIOWakeup, bool networkStandby) {
                EXPECT_EQ(deep_sleep_timeout, 10);
                EXPECT_TRUE(nullptr != isGPIOWakeup);
                EXPECT_EQ(networkStandby, false);
                // Simulate user triggered wakeup
                *isGPIOWakeup = true;
                std::this_thread::sleep_for(std::chrono::seconds(deep_sleep_timeout / 2));
                return DEEPSLEEPMGR_SUCCESS;
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_GetLastWakeupReason(::testing::_))
        .WillOnce(::testing::Invoke(
            [](DeepSleep_WakeupReason_t* wakeupReason) {
                *wakeupReason = DEEPSLEEP_WAKEUPREASON_GPIO;
                return DEEPSLEEPMGR_SUCCESS;
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_DeepSleepWakeup())
        .WillOnce(testing::Return(DEEPSLEEPMGR_SUCCESS));

    uint32_t status = powerManagerImpl->Register(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->SetDeepSleepTimer(10);
    EXPECT_EQ(status, Core::ERROR_NONE);

    int keyCode = 0;
    status      = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    PowerState newState  = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(newState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);

    wg.Wait();

    WakeupReason wakeupReason = WakeupReason::WAKEUP_REASON_UNKNOWN;
    status                    = powerManagerImpl->GetLastWakeupReason(wakeupReason);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(wakeupReason, WakeupReason::WAKEUP_REASON_GPIO);

    status = powerManagerImpl->Unregister(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);
}

// Only difference from above test-case is a user trigger for SetPowerState ON
TEST_F(TestPowerManager, DeepSleepUserWakeupRaceCondition)
{
    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetPowerState(::testing::_))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP);
                return PWRMGR_SUCCESS;
            }))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP);
                return PWRMGR_SUCCESS;
            }))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_ON);
                return PWRMGR_SUCCESS;
            }));

    WaitGroup wg;
    wg.Add();
    Core::ProxyType<PowerModeChangedEvent> modeChanged = Core::ProxyType<PowerModeChangedEvent>::Create();
    EXPECT_CALL(*modeChanged, OnPowerModeChanged(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
            }))
        .WillOnce(::testing::Invoke(
            [&](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(prevState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
            }))
        .WillOnce(::testing::Invoke(
            [&](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(prevState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
                EXPECT_EQ(newState, PowerState::POWER_STATE_ON);
                wg.Done();
            }));

    uint32_t clientId = 0;
    uint32_t status   = powerManagerImpl->AddPowerModePreChangeClient("l1-test-client", clientId);
    EXPECT_EQ(status, Core::ERROR_NONE);

    Core::ProxyType<PowerModePreChangeEvent> prechangeEvent = Core::ProxyType<PowerModePreChangeEvent>::Create();
    EXPECT_CALL(*prechangeEvent, OnPowerModePreChange(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&](const PowerState currentState, const PowerState newState, const int transactionId, const int stateChangeAfter) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
                EXPECT_EQ(stateChangeAfter, 1);

                // valid PowerModePreChangeComplete
                auto status = powerManagerImpl->PowerModePreChangeComplete(clientId, transactionId);
                EXPECT_EQ(status, Core::ERROR_NONE);
            }))
        .WillOnce(::testing::Invoke(
            [&](const PowerState currentState, const PowerState newState, const int transactionId, const int stateChangeAfter) {
                EXPECT_EQ(currentState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
                EXPECT_EQ(stateChangeAfter, 0);

                // trigger new state change now
                wg.Done();

                // simulate a small delay (for new state change i,e ON)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // valid PowerModePreChangeComplete
                auto status = powerManagerImpl->PowerModePreChangeComplete(clientId, transactionId);
                EXPECT_EQ(status, Core::ERROR_INVALID_PARAMETER);
            }))
        .WillOnce(::testing::Invoke(
            [&](const PowerState currentState, const PowerState newState, const int transactionId, const int stateChangeAfter) {
                EXPECT_EQ(currentState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
                EXPECT_EQ(newState, PowerState::POWER_STATE_ON);
                EXPECT_EQ(stateChangeAfter, 1);

                // valid PowerModePreChangeComplete
                auto status = powerManagerImpl->PowerModePreChangeComplete(clientId, transactionId);
                EXPECT_EQ(status, Core::ERROR_NONE);
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_SetDeepSleep(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](uint32_t deep_sleep_timeout, bool* isGPIOWakeup, bool networkStandby) {
                EXPECT_EQ(deep_sleep_timeout, 10);
                EXPECT_TRUE(nullptr != isGPIOWakeup);
                EXPECT_EQ(networkStandby, false);
                // Simulate user triggered wakeup
                *isGPIOWakeup = true;
                std::this_thread::sleep_for(std::chrono::seconds(deep_sleep_timeout / 2));
                return DEEPSLEEPMGR_SUCCESS;
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_GetLastWakeupReason(::testing::_))
        .WillOnce(::testing::Invoke(
            [](DeepSleep_WakeupReason_t* wakeupReason) {
                *wakeupReason = DEEPSLEEP_WAKEUPREASON_GPIO;
                return DEEPSLEEPMGR_SUCCESS;
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_DeepSleepWakeup())
        .WillOnce(testing::Return(DEEPSLEEPMGR_SUCCESS));

    status = powerManagerImpl->Register(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->Register(&(*prechangeEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->SetDeepSleepTimer(10);
    EXPECT_EQ(status, Core::ERROR_NONE);

    int keyCode = 0;
    status      = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    wg.Wait();

    WakeupReason wakeupReason = WakeupReason::WAKEUP_REASON_UNKNOWN;
    status                    = powerManagerImpl->GetLastWakeupReason(wakeupReason);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(wakeupReason, WakeupReason::WAKEUP_REASON_GPIO);

    // ON
    wg.Add(1);
    status = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_ON, "IR-KeyPress-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    wg.Wait();

    PowerState newState  = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(newState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(prevState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
    EXPECT_EQ(newState, PowerState::POWER_STATE_ON);

    // TODO: identify whu delay is required
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    status = powerManagerImpl->Unregister(&(*prechangeEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->Unregister(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, DeepSleepTimerWakeup)
{
    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetPowerState(::testing::_))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP);
                return PWRMGR_SUCCESS;
            }))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP);
                return PWRMGR_SUCCESS;
            }));

    WaitGroup wg;
    wg.Add();
    Core::ProxyType<PowerModeChangedEvent> modeChanged = Core::ProxyType<PowerModeChangedEvent>::Create();
    EXPECT_CALL(*modeChanged, OnPowerModeChanged(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
            }))
        .WillOnce(::testing::Invoke(
            [&](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(prevState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
                wg.Done();
            }));

    Core::ProxyType<DeepSleepWakeupEvent> deepSleepTimeout = Core::ProxyType<DeepSleepWakeupEvent>::Create();
    EXPECT_CALL(*deepSleepTimeout, OnDeepSleepTimeout(::testing::_))
        .WillOnce(::testing::Invoke(
            [](const int timeout) {
                EXPECT_EQ(timeout, 10);
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_SetDeepSleep(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](uint32_t deep_sleep_timeout, bool* isGPIOWakeup, bool networkStandby) {
                EXPECT_EQ(deep_sleep_timeout, 10);
                EXPECT_TRUE(nullptr != isGPIOWakeup);
                EXPECT_EQ(networkStandby, false);
                // Simulate timer wakeup
                *isGPIOWakeup = false;
                std::this_thread::sleep_for(std::chrono::seconds(deep_sleep_timeout));
                return DEEPSLEEPMGR_SUCCESS;
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_GetLastWakeupReason(::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [](DeepSleep_WakeupReason_t* wakeupReason) {
                *wakeupReason = DEEPSLEEP_WAKEUPREASON_TIMER;
                return DEEPSLEEPMGR_SUCCESS;
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_DeepSleepWakeup())
        .WillOnce(testing::Return(DEEPSLEEPMGR_SUCCESS));

    uint32_t status = powerManagerImpl->Register(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->Register(&(*deepSleepTimeout));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->SetDeepSleepTimer(10);
    EXPECT_EQ(status, Core::ERROR_NONE);

    int keyCode = 0;
    status      = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    PowerState newState  = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(newState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);

    wg.Wait();

    WakeupReason wakeupReason = WakeupReason::WAKEUP_REASON_UNKNOWN;
    status                    = powerManagerImpl->GetLastWakeupReason(wakeupReason);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(wakeupReason, WakeupReason::WAKEUP_REASON_TIMER);

    status = powerManagerImpl->Unregister(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->Unregister(&(*deepSleepTimeout));
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, DeepSleepDelayedTimerWakeup)
{
    system("echo 1 > /tmp/deepSleepTimer");
    system("echo 2 > /tmp/deepSleepTimerVal");

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetPowerState(::testing::_))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP);
                return PWRMGR_SUCCESS;
            }))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP);
                return PWRMGR_SUCCESS;
            }));

    WaitGroup wg;
    wg.Add();
    Core::ProxyType<PowerModeChangedEvent> modeChanged = Core::ProxyType<PowerModeChangedEvent>::Create();
    EXPECT_CALL(*modeChanged, OnPowerModeChanged(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
            }))
        .WillOnce(::testing::Invoke(
            [&](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(prevState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
                wg.Done();
            }));

    Core::ProxyType<DeepSleepWakeupEvent> deepSleepTimeout = Core::ProxyType<DeepSleepWakeupEvent>::Create();
    EXPECT_CALL(*deepSleepTimeout, OnDeepSleepTimeout(::testing::_))
        .WillOnce(::testing::Invoke(
            [](const int timeout) {
                EXPECT_EQ(timeout, 2);
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_SetDeepSleep(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](uint32_t deep_sleep_timeout, bool* isGPIOWakeup, bool networkStandby) {
                EXPECT_EQ(deep_sleep_timeout, 2);
                EXPECT_TRUE(nullptr != isGPIOWakeup);
                EXPECT_EQ(networkStandby, false);
                // Simulate timer wakeup
                *isGPIOWakeup = false;
                std::this_thread::sleep_for(std::chrono::seconds(deep_sleep_timeout));
                return DEEPSLEEPMGR_SUCCESS;
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_GetLastWakeupReason(::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [](DeepSleep_WakeupReason_t* wakeupReason) {
                *wakeupReason = DEEPSLEEP_WAKEUPREASON_TIMER;
                return DEEPSLEEPMGR_SUCCESS;
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_DeepSleepWakeup())
        .WillOnce(testing::Return(DEEPSLEEPMGR_SUCCESS));

    uint32_t status = powerManagerImpl->Register(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->Register(&(*deepSleepTimeout));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->SetDeepSleepTimer(10);
    EXPECT_EQ(status, Core::ERROR_NONE);

    int keyCode = 0;
    status      = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    PowerState newState  = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(newState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);

    wg.Wait();

    WakeupReason wakeupReason = WakeupReason::WAKEUP_REASON_UNKNOWN;
    status                    = powerManagerImpl->GetLastWakeupReason(wakeupReason);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(wakeupReason, WakeupReason::WAKEUP_REASON_TIMER);

    status = powerManagerImpl->Unregister(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->Unregister(&(*deepSleepTimeout));
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, DeepSleepInvalidWakeup)
{
    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetPowerState(::testing::_))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP);
                return PWRMGR_SUCCESS;
            }))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP);
                return PWRMGR_SUCCESS;
            }));

    Core::ProxyType<PowerModeChangedEvent> modeChanged = Core::ProxyType<PowerModeChangedEvent>::Create();
    EXPECT_CALL(*modeChanged, OnPowerModeChanged(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
            }))
        .WillOnce(::testing::Invoke(
            [](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(prevState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
            }));

    Core::ProxyType<DeepSleepWakeupEvent> deepSleepTimeout = Core::ProxyType<DeepSleepWakeupEvent>::Create();
    EXPECT_CALL(*deepSleepTimeout, OnDeepSleepTimeout(::testing::_))
        .WillOnce(::testing::Invoke(
            [](const int timeout) {
                EXPECT_EQ(timeout, 10);
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_SetDeepSleep(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](uint32_t deep_sleep_timeout, bool* isGPIOWakeup, bool networkStandby) {
                EXPECT_EQ(deep_sleep_timeout, 10);
                EXPECT_TRUE(nullptr != isGPIOWakeup);
                EXPECT_EQ(networkStandby, false);
                // Simulate timer wakeup
                *isGPIOWakeup = false;
                std::this_thread::sleep_for(std::chrono::seconds(deep_sleep_timeout));
                return DEEPSLEEPMGR_SUCCESS;
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_GetLastWakeupReason(::testing::_))
        .WillOnce(::testing::Invoke(
            [](DeepSleep_WakeupReason_t* wakeupReason) {
                // Invalid wakeup reason
                return DeepSleep_Return_Status_t(-1);
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_DeepSleepWakeup())
        .WillOnce(testing::Return(DEEPSLEEPMGR_SUCCESS));

    uint32_t status = powerManagerImpl->Register(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->Register(&(*deepSleepTimeout));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->SetDeepSleepTimer(10);
    EXPECT_EQ(status, Core::ERROR_NONE);

    int keyCode = 0;
    status      = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    PowerState newState  = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(newState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);

    std::this_thread::sleep_for(std::chrono::seconds(20));

    status = powerManagerImpl->Unregister(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->Unregister(&(*deepSleepTimeout));
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, DeepSleepEarlyWakeup)
{
    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetPowerState(::testing::_))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP);
                return PWRMGR_SUCCESS;
            }))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP);
                return PWRMGR_SUCCESS;
            }));

    WaitGroup wg;
    wg.Add();
    Core::ProxyType<PowerModeChangedEvent> modeChanged = Core::ProxyType<PowerModeChangedEvent>::Create();
    EXPECT_CALL(*modeChanged, OnPowerModeChanged(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
            }))
        .WillOnce(::testing::Invoke(
            [&](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(prevState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
                wg.Done();
            }));

    Core::ProxyType<DeepSleepWakeupEvent> deepSleepTimeout = Core::ProxyType<DeepSleepWakeupEvent>::Create();
    EXPECT_CALL(*deepSleepTimeout, OnDeepSleepTimeout(::testing::_))
        .WillOnce(::testing::Invoke(
            [](const int timeout) {
                EXPECT_EQ(timeout, 10);
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_SetDeepSleep(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](uint32_t deep_sleep_timeout, bool* isGPIOWakeup, bool networkStandby) {
                EXPECT_EQ(deep_sleep_timeout, 10);
                EXPECT_TRUE(nullptr != isGPIOWakeup);
                EXPECT_EQ(networkStandby, false);
                // Simulate timer wakeup
                *isGPIOWakeup = false;
                std::this_thread::sleep_for(std::chrono::seconds(deep_sleep_timeout / 2));
                return DEEPSLEEPMGR_SUCCESS;
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_GetLastWakeupReason(::testing::_))
        .WillOnce(::testing::Invoke(
            [](DeepSleep_WakeupReason_t* wakeupReason) {
                // Invalid wakeup reason
                return DeepSleep_Return_Status_t(-1);
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_DeepSleepWakeup())
        .WillOnce(testing::Return(DEEPSLEEPMGR_SUCCESS));

    uint32_t status = powerManagerImpl->Register(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->Register(&(*deepSleepTimeout));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->SetDeepSleepTimer(10);
    EXPECT_EQ(status, Core::ERROR_NONE);

    int keyCode = 0;
    status      = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    PowerState newState  = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(newState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);

    wg.Wait();

    status = powerManagerImpl->Unregister(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->Unregister(&(*deepSleepTimeout));
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, DeepSleepFailure)
{
    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetPowerState(::testing::_))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP);
                return PWRMGR_SUCCESS;
            }))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP);
                return PWRMGR_SUCCESS;
            }));

    WaitGroup wg;
    wg.Add();
    Core::ProxyType<PowerModeChangedEvent> modeChanged = Core::ProxyType<PowerModeChangedEvent>::Create();
    EXPECT_CALL(*modeChanged, OnPowerModeChanged(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
            }))
        .WillOnce(::testing::Invoke(
            [&](const PowerState prevState, const PowerState newState) {
                EXPECT_EQ(prevState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
                wg.Done();
            }));

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_SetDeepSleep(::testing::_, ::testing::_, ::testing::_))
        .Times(5)
        .WillRepeatedly(::testing::Invoke(
            [](uint32_t deep_sleep_timeout, bool* isGPIOWakeup, bool networkStandby) {
                EXPECT_EQ(deep_sleep_timeout, 10);
                EXPECT_TRUE(nullptr != isGPIOWakeup);
                EXPECT_EQ(networkStandby, false);
                // Simulate timer wakeup
                *isGPIOWakeup = false;
                return DEEPSLEEPMGR_INVALID_ARGUMENT;
            }));

    // TODO: this is incorrect, ideally if SetDeepSleep fails, we should not call DeepSleepWakeup
    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_DeepSleepWakeup())
        .WillOnce(testing::Return(DEEPSLEEPMGR_SUCCESS));

    uint32_t status = powerManagerImpl->Register(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);

    status = powerManagerImpl->SetDeepSleepTimer(10);
    EXPECT_EQ(status, Core::ERROR_NONE);

    int keyCode = 0;
    status      = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    PowerState newState  = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(newState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);

    wg.Wait();

    status = powerManagerImpl->Unregister(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, Reboot)
{
    Core::ProxyType<RebootEvent> rebootEvent = Core::ProxyType<RebootEvent>::Create();
    EXPECT_CALL(*rebootEvent, OnRebootBegin(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&](const std::string& reasonCustom, const std::string& reasonOther, const std::string& requestor) {
                EXPECT_EQ("L1Test", requestor);
                EXPECT_EQ("L1Test-custom", reasonCustom);
                EXPECT_EQ("Unknown", reasonOther);
            }));

    WaitGroup wg;
    wg.Add(2);
    EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&](const char* command, va_list args) {
                EXPECT_EQ(string(command), string("echo 0 > /opt/.rebootFlag"));
                wg.Done();
                return Core::ERROR_NONE;
            }))
        .WillOnce(::testing::Invoke(
            [&](const char* command, va_list args) {
                EXPECT_EQ(string(command), "/lib/rdk/rebootNow.sh -s '%s' -r '%s' -o '%s'");
                wg.Done();
                return Core::ERROR_NONE;
            }));

    uint32_t status = powerManagerImpl->Register(&(*rebootEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);

    powerManagerImpl->Reboot("L1Test", "L1Test-custom", "");

    wg.Wait();

    status = powerManagerImpl->Unregister(&(*rebootEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, NetworkStandby)
{
    WaitGroup wg;
    wg.Add(1);

    Core::ProxyType<NetworkStandbyChangedEvent> nwstandbyModeChangedEvent = Core::ProxyType<NetworkStandbyChangedEvent>::Create();
    EXPECT_CALL(*nwstandbyModeChangedEvent, OnNetworkStandbyModeChanged(::testing::_))
        .WillOnce(::testing::Invoke(
            [&](const bool enabled) {
                EXPECT_EQ(enabled, true);
                wg.Done();
            }));

    auto status = powerManagerImpl->Register(&(*nwstandbyModeChangedEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);

    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetWakeupSrc(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](PWRMGR_WakeupSrcType_t wakeupSrc, bool enabled) {
                EXPECT_EQ(wakeupSrc, PWRMGR_WAKEUPSRC_WIFI);
                EXPECT_EQ(enabled, true);
                return PWRMGR_SUCCESS;
            }))
        .WillOnce(::testing::Invoke(
            [](PWRMGR_WakeupSrcType_t wakeupSrc, bool enabled) {
                EXPECT_EQ(wakeupSrc, PWRMGR_WAKEUPSRC_LAN);
                EXPECT_EQ(enabled, true);
                return PWRMGR_SUCCESS;
            }));

    powerManagerImpl->SetNetworkStandbyMode(true);

    wg.Wait();

    bool standbyMode = false;

    status = powerManagerImpl->GetNetworkStandbyMode(standbyMode);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(standbyMode, true);

    status = powerManagerImpl->Unregister(&(*nwstandbyModeChangedEvent));
    EXPECT_EQ(status, Core::ERROR_NONE);
};

TEST_F(TestPowerManager, SystemMode)
{
    // dummy test only for coverage
    auto status = powerManagerImpl->SetSystemMode(Exchange::IPowerManager::SystemMode::SYSTEM_MODE_NORMAL, Exchange::IPowerManager::SystemMode::SYSTEM_MODE_EAS);
    EXPECT_EQ(status, Core::ERROR_NONE);
}

TEST_F(TestPowerManager, TemperatureThresholds)
{
    EXPECT_CALL(mfrMock::Mock(), mfrSetTempThresholds(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](int high, int critical) {
                EXPECT_EQ(high, 90);
                EXPECT_EQ(critical, 95);
                return mfrERR_NONE;
            }));

    auto status = powerManagerImpl->SetTemperatureThresholds(90, 95);
    EXPECT_EQ(status, Core::ERROR_NONE);

    EXPECT_CALL(mfrMock::Mock(), mfrGetTempThresholds(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](int* high, int* critical) {
                *high     = 90;
                *critical = 95;
                return mfrERR_NONE;
            }));

    float high = 0, critical = 0;

    status = powerManagerImpl->GetTemperatureThresholds(high, critical);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(high, 90.00);
    EXPECT_EQ(critical, 95.00);
}

TEST_F(TestPowerManager, OverTemperatureGraceInterval)
{
    auto status = powerManagerImpl->SetOvertempGraceInterval(60);
    EXPECT_EQ(status, Core::ERROR_NONE);

    int interval = 0;
    status       = powerManagerImpl->GetOvertempGraceInterval(interval);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(interval, 60);

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
}
