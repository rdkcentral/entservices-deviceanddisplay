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

#include <core/Portability.h>
#include <core/Proxy.h>
#include <core/Services.h>
#include <interfaces/IPowerManager.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "PowerManagerHalMock.h"
#include "PowerManagerImplementation.h"
#include "WorkerPoolImplementation.h"

using namespace WPEFramework;

using WakeupReason = WPEFramework::Exchange::IPowerManager::WakeupReason;
using WakeupSrcType = WPEFramework::Exchange::IPowerManager::WakeupSrcType;

class TestPowerManager : public ::testing::Test {
public:
    Core::ProxyType<Plugin::PowerManagerImplementation> powerManagerImpl;

    TestPowerManager()
    {
        SetUpMocks();

        powerManagerImpl = Core::ProxyType<Plugin::PowerManagerImplementation>::Create();
        EXPECT_EQ(powerManagerImpl.IsValid(), true);
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
    }

    void TearDownMocks()
    {
        PowerManagerHalMock::Delete();
    }

    ~TestPowerManager() override
    {
        powerManagerImpl.Release();
        EXPECT_EQ(powerManagerImpl.IsValid(), false);
        TearDownMocks();
    }

    static void SetUpTestSuite()
    {
        static WorkerPoolImplementation workerPool(4, WPEFramework::Core::Thread::DefaultStackSize(), 16);
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
