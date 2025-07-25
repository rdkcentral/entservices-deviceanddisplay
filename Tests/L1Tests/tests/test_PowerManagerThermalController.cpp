#include "gmock/gmock.h"
#include <algorithm>
#include <condition_variable>
#include <core/Portability.h>
#include <core/Proxy.h>
#include <core/Services.h>
#include <cstring>
#include <interfaces/IPowerManager.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <mutex>

#include "ThermalController.h"

// mocks
#include "IarmBusMock.h"
#include "MfrMock.h"
#include "RfcApiMock.h"
#include "WrapsMock.h"
#include "hal/ThermalImpl.h"

using namespace WPEFramework;
using ThermalTemperature = WPEFramework::Exchange::IPowerManager::ThermalTemperature;

class WaitGroup {
public:
    WaitGroup()
        : _count(0)
    {
    }

    void Add(int count = 1)
    {
        _count += count;

        if (_count <= 0)
            _cv.notify_all();
    }

    void Done() { Add(-1); }

    void Wait()
    {
        if (_count <= 0)
            return;

        std::unique_lock<std::mutex> _lock { _m };
        _cv.wait(_lock, [&]() {
            return _count == 0;
        });
    }

private:
    std::atomic<int> _count;
    std::mutex _m;
    std::condition_variable _cv;
};

class TestThermalController : public ::testing::Test, public ThermalController::INotification {

public:
    MOCK_METHOD(void, onThermalTemperatureChanged, (const ThermalTemperature, const ThermalTemperature, const float current_Temp), (override));
    MOCK_METHOD(void, onDeepSleepForThermalChange, (), (override));

    TestThermalController()
    {
        p_wrapsImplMock = new testing::NiceMock<WrapsImplMock>;
        Wraps::setImpl(p_wrapsImplMock);

        p_rfcApiImplMock = new testing::NiceMock<RfcApiImplMock>;
        RfcApi::setImpl(p_rfcApiImplMock);

        p_iarmBusImplMock = new testing::NiceMock<IarmBusImplMock>;
        IarmBus::setImpl(p_iarmBusImplMock);

        setupDefaultMocks();
    }

    void setupDefaultMocks()
    {

        ON_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [](char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData) {
                    strcpy(pstParamData->name, pcParameterName);

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
        EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(::testing::StrEq(IARM_BUS_MFRLIB_NAME), ::testing::StrEq(IARM_BUS_MFRLIB_API_SetTemperatureThresholds), ::testing::_, ::testing::_))
             .WillOnce(::testing::Invoke(
                [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                    auto* param = static_cast<IARM_Bus_MFRLib_ThermalSoCTemp_Param_t*>(arg);
                    EXPECT_EQ(param->highTemp, 100);
                    EXPECT_EQ(param->criticalTemp, 110);
                    return IARM_RESULT_SUCCESS;
                }));
    }

    ~TestThermalController() override
    {
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
    }

protected:
    WrapsImplMock* p_wrapsImplMock     = nullptr;
    RfcApiImplMock* p_rfcApiImplMock   = nullptr;
    IarmBusImplMock* p_iarmBusImplMock = nullptr;
};

TEST_F(TestThermalController, temperatureThresholds)
{
    WaitGroup wg;


    wg.Add();

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(::testing::StrEq(IARM_BUS_MFRLIB_NAME), ::testing::StrEq(IARM_BUS_MFRLIB_API_GetTemperature), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [&](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                auto* param = static_cast<IARM_Bus_MFRLib_ThermalSoCTemp_Param_t*>(arg);
                param->curSoCTemperature  = 60; // safe temperature
                param->curState        = IARM_BUS_TEMPERATURE_NORMAL;
                param->curWiFiTemperature = 25;
                wg.Done();
                return IARM_RESULT_SUCCESS;
            }));

    auto controller = ThermalController::Create(*this);

    // Set
    {
        EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(::testing::StrEq(IARM_BUS_MFRLIB_NAME), ::testing::StrEq(IARM_BUS_MFRLIB_API_SetTemperatureThresholds), ::testing::_, ::testing::_))
             .WillOnce(::testing::Invoke(
                [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                    auto* param = static_cast<IARM_Bus_MFRLib_ThermalSoCTemp_Param_t*>(arg);
                    EXPECT_EQ(param->highTemp, 90);
                    EXPECT_EQ(param->criticalTemp, 100);
                    return IARM_RESULT_SUCCESS;
                }));

        uint32_t res = controller.SetTemperatureThresholds(90, 100);
        EXPECT_EQ(res, Core::ERROR_NONE);
    }

    // Get
    {
        EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(::testing::StrEq(IARM_BUS_MFRLIB_NAME), ::testing::StrEq(IARM_BUS_MFRLIB_API_GetTemperatureThresholds), ::testing::_, ::testing::_))
             .WillOnce(::testing::Invoke(
                [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                    auto* param = static_cast<IARM_Bus_MFRLib_ThermalSoCTemp_Param_t*>(arg);
                    param->highTemp = 90;
                    param->criticalTemp = 100;
                    return IARM_RESULT_SUCCESS;
                }));

        float high, critical;

        uint32_t res = controller.GetTemperatureThresholds(high, critical);

        EXPECT_EQ(res, Core::ERROR_NONE);
        EXPECT_EQ(high, 90);
        EXPECT_EQ(critical, 100);
    }

    // wait for test to end
    wg.Wait();
}

TEST_F(TestThermalController, modeChangeHigh)
{
    WaitGroup wg;

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(::testing::StrEq(IARM_BUS_MFRLIB_NAME), ::testing::StrEq(IARM_BUS_MFRLIB_API_GetTemperature), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                auto* param = static_cast<IARM_Bus_MFRLib_ThermalSoCTemp_Param_t*>(arg);
                param->curSoCTemperature  = 100; // high temperature
                param->curState        = IARM_BUS_TEMPERATURE_HIGH;
                param->curWiFiTemperature = 25;
                return IARM_RESULT_SUCCESS;
            }));

    wg.Add();
    EXPECT_CALL(*this, onThermalTemperatureChanged(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&](const ThermalTemperature curr_mode, const ThermalTemperature new_mode, const float temp) {
            EXPECT_EQ(curr_mode, ThermalTemperature::THERMAL_TEMPERATURE_NORMAL);
            EXPECT_EQ(new_mode, ThermalTemperature::THERMAL_TEMPERATURE_HIGH);
            EXPECT_EQ(int(temp), 100);
            wg.Done();
        }));

    auto controller = ThermalController::Create(*this);

    wg.Wait();
}

TEST_F(TestThermalController, modeChangeCritical)
{
    WaitGroup wg;

    EXPECT_CALL(*p_iarmBusImplMock, IARM_Bus_Call(::testing::StrEq(IARM_BUS_MFRLIB_NAME), ::testing::StrEq(IARM_BUS_MFRLIB_API_GetTemperature), ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(
            [](const char* ownerName, const char* methodName, void* arg, size_t argLen) {
                auto* param = static_cast<IARM_Bus_MFRLib_ThermalSoCTemp_Param_t*>(arg);
                param->curSoCTemperature  = 115; // critical temperature
                param->curState        = IARM_BUS_TEMPERATURE_CRITICAL;
                param->curWiFiTemperature = 25;
                return IARM_RESULT_SUCCESS;
            }));

    wg.Add();
    EXPECT_CALL(*this, onThermalTemperatureChanged(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([&](const ThermalTemperature curr_mode, const ThermalTemperature new_mode, const float temp) {
            EXPECT_EQ(curr_mode, ThermalTemperature::THERMAL_TEMPERATURE_NORMAL);
            EXPECT_EQ(new_mode, ThermalTemperature::THERMAL_TEMPERATURE_CRITICAL);
            EXPECT_EQ(int(temp), 115);
            wg.Done();
        }));

    wg.Add();
    EXPECT_CALL(*this, onDeepSleepForThermalChange())
        .WillOnce(::testing::Invoke([&]() {
            wg.Done();
        }));

    auto controller = ThermalController::Create(*this);

    wg.Wait();
}
