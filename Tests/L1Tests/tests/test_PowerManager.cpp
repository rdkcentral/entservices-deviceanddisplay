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
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <chrono>
#include <thread>

#include <core/Portability.h>
#include <core/Proxy.h>
#include <core/Services.h>
#include <gmock/gmock.h>
#include <interfaces/IPowerManager.h>

#include "PowerManagerHalMock.h"
#include "PowerManagerImplementation.h"
#include "WorkerPoolImplementation.h"

#include "IarmBusMock.h"
#include "MfrMock.h"
#include "RfcApiMock.h"
#include "WrapsMock.h"

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

    TestPowerManager()
    {
        TEST_LOG("TestPowerManager constructor is called, %p", this);
        p_wrapsImplMock = new NiceMock<WrapsImplMock>;
        Wraps::setImpl(p_wrapsImplMock);

        p_rfcApiImplMock = new NiceMock<RfcApiImplMock>;
        RfcApi::setImpl(p_rfcApiImplMock);

        p_iarmBusImplMock = new NiceMock<IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);
    }

    void SetUpMocks()
    {
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

        // called from ThermalController constructor in initializeThermalProtection
        EXPECT_CALL(mfrMock::Mock(), mfrSetTempThresholds(::testing::_, ::testing::_))
            .WillOnce(::testing::Invoke(
                [](int high, int critical) {
                    EXPECT_EQ(high, 100);
                    EXPECT_EQ(critical, 110);
                    return mfrERR_NONE;
                }));

        // called from pollThermalLevels
        // EXPECT_CALL(mfrMock::Mock(), mfrGetTemperature(::testing::_, ::testing::_, ::testing::_))
        //     .WillOnce(::testing::Invoke(
        //         [](mfrTemperatureState_t* state, int* temperatureValue, int* wifiTemp) {
        //             std::cout << "skm : hit mock " << __PRETTY_FUNCTION__ << std::endl;
        //             *state            = mfrTEMPERATURE_NORMAL;
        //             *temperatureValue = 40;
        //             *wifiTemp         = 35;
        //             return mfrERR_NONE;
        //         }));

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

    void expect_setDeepSleep()
    {

        EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetPowerState(::testing::_))
            .WillRepeatedly(::testing::Invoke(
                [](PWRMgr_PowerState_t powerState) {
                    EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_DEEP_SLEEP);
                    return PWRMGR_SUCCESS;
                }));

        EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_SetDeepSleep(::testing::_, ::testing::_, ::testing::_))
            .WillRepeatedly(::testing::Invoke(
                [](uint32_t deep_sleep_timeout, bool* isGPIOWakeup, bool networkStandby) {
                    return DEEPSLEEPMGR_SUCCESS;
                }));

        EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_DS_GetLastWakeupReason(::testing::_))
            .WillOnce(::testing::Invoke(
                [](DeepSleep_WakeupReason_t* wakeupReason) {
                    *wakeupReason = DEEPSLEEP_WAKEUPREASON_IR;
                    return DEEPSLEEPMGR_SUCCESS;
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
    }

    static void SetUpTestSuite()
    {
        static WorkerPoolImplementation workerPool(4, WPEFramework::Core::Thread::DefaultStackSize(), 16);
        WPEFramework::Core::WorkerPool::Assign(&workerPool);
        workerPool.Run();
    }
};

#if 0
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
    int config = WakeupSrcType::WAKEUP_SRC_WIFI;

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
    int src = WakeupSrcType::WAKEUP_SRC_WIFI | WakeupSrcType::WAKEUP_SRC_IR;
    uint32_t status = powerManagerImpl->GetWakeupSrcConfig(powerMode, src, config);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(powerMode, 0);
    EXPECT_EQ(config, WakeupSrcType::WAKEUP_SRC_WIFI);
}

TEST_F(TestPowerManager, SetPowerState)
{
    EXPECT_CALL(PowerManagerHalMock::Mock(), PLAT_API_SetPowerState(::testing::_))
        .WillOnce(::testing::Invoke(
            [](PWRMgr_PowerState_t powerState) {
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_STANDBY_LIGHT_SLEEP);
                return PWRMGR_SUCCESS;
            }));

    int keyCode = 0;
    uint32_t status = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP, "l1-test");

    EXPECT_EQ(status, Core::ERROR_NONE);

    PowerState currentState = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(currentState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(currentState, PowerState::POWER_STATE_STANDBY_LIGHT_SLEEP);
    EXPECT_EQ(prevState, PowerState::POWER_STATE_ON);
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
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_ON);
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
                EXPECT_EQ(newState, PowerState::POWER_STATE_ON);
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
    status = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    PowerState newState = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(newState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);

    std::this_thread::sleep_for(std::chrono::seconds(20));

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
    status = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    PowerState newState = PowerState::POWER_STATE_UNKNOWN;
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
    status = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    PowerState newState = PowerState::POWER_STATE_UNKNOWN;
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
                std::this_thread::sleep_for(std::chrono::seconds(deep_sleep_timeout/2));
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
    status = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    PowerState newState = PowerState::POWER_STATE_UNKNOWN;
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
                EXPECT_EQ(powerState, PWRMGR_POWERSTATE_ON);
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
                EXPECT_EQ(newState, PowerState::POWER_STATE_ON);
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
    status = powerManagerImpl->SetPowerState(keyCode, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP, "l1-test");
    EXPECT_EQ(status, Core::ERROR_NONE);

    PowerState newState = PowerState::POWER_STATE_UNKNOWN;
    PowerState prevState = PowerState::POWER_STATE_UNKNOWN;

    status = powerManagerImpl->GetPowerState(newState, prevState);
    EXPECT_EQ(status, Core::ERROR_NONE);
    EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);

    std::this_thread::sleep_for(std::chrono::seconds(35));

    status = powerManagerImpl->Unregister(&(*modeChanged));
    EXPECT_EQ(status, Core::ERROR_NONE);
}
#endif

TEST_F(TestPowerManager, SetTemperatureThresholdsSuccess)
{
    const float high = 199.0, critical = 99.0;

    SetUpMocks();

    powerManagerImpl = Core::ProxyType<Plugin::PowerManagerImplementation>::Create();
    EXPECT_EQ(powerManagerImpl.IsValid(), true);

    EXPECT_CALL(mfrMock::Mock(), mfrSetTempThresholds(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](int high, int critical) {
                EXPECT_EQ((int)high, 199);
                EXPECT_EQ((int)critical, 99);
                return mfrERR_NONE;
            }));

    powerManagerImpl->SetTemperatureThresholds(high, critical);

    EXPECT_CALL(mfrMock::Mock(), mfrGetTempThresholds(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](int* high, int* critical) {
                *high     = 199;
                *critical = 99;
                return mfrERR_NONE;
            }));

    float getHigh, getCritical;

    powerManagerImpl->GetTemperatureThresholds(getHigh, getCritical);

    EXPECT_EQ(getHigh, 199.0);
    EXPECT_EQ(getCritical, 99.0);
}

TEST_F(TestPowerManager, deepSleepNeedBasedOnCurrentThermal)
{
    std::condition_variable condition_variable;
    auto now = std::chrono::system_clock::now();
    std::chrono::milliseconds timeout(5000);

    wait_call = true;

    SetUpMocks();

    EXPECT_CALL(mfrMock::Mock(), mfrGetTemperature(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [&](mfrTemperatureState_t* curState, int* curTemperature, int* wifiTemperature) {
                std::cout << "skm: hit mock (exceed critical): " << __PRETTY_FUNCTION__ << std::endl;
                // If the thermal value is more than 115 (deepsleep Threshold critical), then the system will goes to deep sleep immediately
                *curTemperature  = 115;
                *curState        = (mfrTemperatureState_t)PWRMGR_TEMPERATURE_CRITICAL;
                *wifiTemperature = 0;
                wait_call        = false;
                std::lock_guard<std::mutex> lock(m_mutex);
                return mfrERR_NONE;
            }));

    expect_setDeepSleep();

    powerManagerImpl = Core::ProxyType<Plugin::PowerManagerImplementation>::Create();
    EXPECT_EQ(powerManagerImpl.IsValid(), true);

    while (wait_call) {
        // wait until getTemprature API called
        std::unique_lock<std::mutex> lock(m_mutex);
        if (condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout) {
            TEST_LOG("-------------mfrGetTemperature wait timeout--------------");
            break;
        }
    }

    // std::this_thread::sleep_for(std::chrono::seconds(2));
}

TEST_F(TestPowerManager, deepSleepGraceIntervelOnCurrentTheraml)
{
    std::condition_variable condition_variable;
    auto now = std::chrono::system_clock::now();
    std::chrono::milliseconds timeout(10000);

    SetUpMocks();

    EXPECT_CALL(mfrMock::Mock(), mfrGetTemperature(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [](mfrTemperatureState_t* curState, int* curTemperature, int* wifiTemperature) {
                // If the thermal value is 110 (deepsleep Threshold concern), then the system will wait for graceInterval time
                // If the themal value is not reduce within graceInterval time, then the system will goes to deep sleep
                *curTemperature  = 110;
                *curState        = (mfrTemperatureState_t)PWRMGR_TEMPERATURE_HIGH;
                *wifiTemperature = 0;
                return mfrERR_NONE;
            }));

    expect_setDeepSleep();

    Core::ProxyType<PowerModeChangedEvent> modeChanged = Core::ProxyType<PowerModeChangedEvent>::Create();
    EXPECT_CALL(*modeChanged, OnPowerModeChanged(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke(
            [&](const PowerState prevState, const PowerState newState) {
                std::cout << __PRETTY_FUNCTION__ << "SKM \n";
                EXPECT_EQ(prevState, PowerState::POWER_STATE_ON);
                EXPECT_EQ(newState, PowerState::POWER_STATE_STANDBY_DEEP_SLEEP);
                // wait_call = false;
                std::lock_guard<std::mutex> lock(m_mutex);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }));

    powerManagerImpl = Core::ProxyType<Plugin::PowerManagerImplementation>::Create();
    EXPECT_EQ(powerManagerImpl.IsValid(), true);

    while (wait_call) {
        // wait until getTemprature API called
        std::unique_lock<std::mutex> lock(m_mutex);
        if (condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout) {
            TEST_LOG("mfrGetTemperature wait timeout");
            break;
        }
    }
}

TEST_F(TestPowerManager, rebootNeedBasedOnCurrentTheraml)
{
    std::condition_variable condition_variable;
    auto now = std::chrono::system_clock::now();
    std::chrono::milliseconds timeout(1000);

    SetUpMocks();
    EXPECT_CALL(mfrMock::Mock(), mfrGetTemperature(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [&](mfrTemperatureState_t* curState, int* curTemperature, int* wifiTemperature) {
                // If the thermal value is more than 120 (reboot Threshold critical), then the system will reboot immediately
                *curTemperature  = 120;
                *curState        = (mfrTemperatureState_t)PWRMGR_TEMPERATURE_CRITICAL;
                *wifiTemperature = 0;
                wait_call        = false;
                std::lock_guard<std::mutex> lock(m_mutex);
                return mfrERR_NONE;
            }));

    expect_setDeepSleep();

    EXPECT_CALL(*p_wrapsImplMock, v_secure_system(::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [](const char* command, va_list args) {
                if (command[0] == 'e') {
                    EXPECT_EQ(string(command), string("echo %s > %s"));
                } else {
                    EXPECT_EQ(string(command), string("/rebootNow.sh -s Power_Thermmgr -o 'Rebooting the box due to stb temperature greater than rebootThreshold critical...'"));
                }
                return Core::ERROR_NONE;
            }));

    powerManagerImpl = Core::ProxyType<Plugin::PowerManagerImplementation>::Create();
    EXPECT_EQ(powerManagerImpl.IsValid(), true);

    while (wait_call) {
        // wait until getTemprature API called
        std::unique_lock<std::mutex> lock(m_mutex);
        if (condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout) {
            TEST_LOG("mfrGetTemperature wait timeout");
            break;
        }
    }
}

TEST_F(TestPowerManager, deClockNeededOnCurrentTheraml)
{
    std::condition_variable condition_variable;
    auto now = std::chrono::system_clock::now();
    std::chrono::milliseconds timeout(1000);

    SetUpMocks();

    EXPECT_CALL(mfrMock::Mock(), mfrGetTemperature(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [&](mfrTemperatureState_t* curState, int* curTemperature, int* wifiTemperature) {
                *curTemperature  = 110;
                *curState        = (mfrTemperatureState_t)PWRMGR_TEMPERATURE_CRITICAL;
                *wifiTemperature = 0;
                wait_call        = false;
                std::lock_guard<std::mutex> lock(m_mutex);
                return mfrERR_NONE;
            }));

    powerManagerImpl = Core::ProxyType<Plugin::PowerManagerImplementation>::Create();
    EXPECT_EQ(powerManagerImpl.IsValid(), true);

    while (wait_call) {
        // wait until getTemprature API called
        std::unique_lock<std::mutex> lock(m_mutex);
        if (condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout) {
            TEST_LOG("mfrGetTemperature wait timeout");
            break;
        }
    }
}
