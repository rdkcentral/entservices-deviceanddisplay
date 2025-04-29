/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
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
 **/

#ifndef MODULE_NAME
#define MODULE_NAME COMRPCTestApp
#endif

#include <chrono>
#include <string>
#include <iostream>
#include <mutex>
#include <csignal>
#include <atomic>
#include <thread>

#include <WPEFramework/com/com.h>
#include <WPEFramework/core/core.h>
#include "WPEFramework/interfaces/IFrameRate.h"

/************************************ Wrapper for Logging *************************************/
class Logger {
    public:
        explicit Logger(const std::string& binaryName) : _binaryName(binaryName) {}

        void Log(const std::string& message) const {
            std::lock_guard<std::mutex> lock(_mutex);
            std::cout << "[" << _binaryName << "] " << message << std::endl;
        }

        void Error(const std::string& message) const {
            std::lock_guard<std::mutex> lock(_mutex);
            std::cerr << "[" << _binaryName << "] " << message << std::endl;
        }

    private:
        std::string _binaryName;
        mutable std::mutex _mutex;
};

/********************************* Test FrameRate COMRPC Impl **********************************/

using namespace WPEFramework;

// RAII Wrapper for IFrameRate
class FrameRateProxy {
    public:
        explicit FrameRateProxy(Exchange::IFrameRate* frameRate, const Logger& logger)
            : _frameRate(frameRate), _logger(logger) {}
        ~FrameRateProxy() {
            if (_frameRate != nullptr) {
                _logger.Log("Releasing FrameRate proxy...");
                _frameRate->Release();
            }
        }

        Exchange::IFrameRate* operator->() const { return _frameRate; }
        Exchange::IFrameRate* Get() const { return _frameRate; }

    private:
        Exchange::IFrameRate* _frameRate;
        const Logger& _logger;
};

/********************************* Test All IFrameRate Events **********************************/
class FrameRateEventHandler : public Exchange::IFrameRate::INotification {
    public:
        explicit FrameRateEventHandler(const Logger& logger) : _logger(logger) {}

        void OnFpsEvent(int average, int min, int max) override {
            _logger.Log("FPS Event - Average: " + std::to_string(average) +
                    ", Min: " + std::to_string(min) +
                    ", Max: " + std::to_string(max));
        }

        void OnDisplayFrameRateChanging(const std::string& displayFrameRate) override {
            _logger.Log("Display Frame Rate Changing to: " + displayFrameRate);
        }

        void OnDisplayFrameRateChanged(const std::string& displayFrameRate) override {
            _logger.Log("Display Frame Rate Changed to: " + displayFrameRate);
        }

        void AddRef() const override {}
        uint32_t Release() const override { return 0; }

        BEGIN_INTERFACE_MAP(FrameRateEventHandler)
            INTERFACE_ENTRY(Exchange::IFrameRate::INotification)
        END_INTERFACE_MAP

    private:
        const Logger& _logger;
};

/*************************************** Event Listener ****************************************/
void EventListener(FrameRateProxy& frameRate, const Logger& logger, const std::atomic<bool>& keepRunning)
{
    logger.Log("Event listener thread started. Registering for events...");

    FrameRateEventHandler eventHandler(logger);
    frameRate->Register(&eventHandler);

    while (keepRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    frameRate->Unregister(&eventHandler);
    logger.Log("Event listener thread stopped. Event handler unregistered.");
}

/*************************************** Helper Functions ***************************************/
bool HandleResult(const Logger& logger, const std::string& methodName, uint32_t result, bool success)
{
    if (result == Core::ERROR_NONE && success) {
        logger.Log(methodName + " succeeded.");
        return true;
    } else {
        logger.Error(methodName + " failed with error: " + std::to_string(result));
        return false;
    }
}

/************************************* Method Test Wrapper *************************************/
void TestFrameRateMethods(FrameRateProxy& frameRate, const Logger& logger)
{
    bool success = false;

    // Test GetDisplayFrameRate
    std::string displayFrameRate;
    HandleResult(logger, "GetDisplayFrameRate", frameRate->GetDisplayFrameRate(displayFrameRate, success), success);

    // Test GetFrmMode
    int frmmode = 0;
    HandleResult(logger, "GetFrmMode", frameRate->GetFrmMode(frmmode, success), success);

    // Test SetCollectionFrequency
    int frequency = 30;
    HandleResult(logger, "SetCollectionFrequency", frameRate->SetCollectionFrequency(frequency, success), success);

    // Test StartFpsCollection
    HandleResult(logger, "StartFpsCollection", frameRate->StartFpsCollection(success), success);

    // Wait for sometime
    std::this_thread::sleep_for(std::chrono::seconds(30*3));

    // Test StopFpsCollection
    HandleResult(logger, "StopFpsCollection", frameRate->StopFpsCollection(success), success);

    // Test SetDisplayFrameRate
    const char* newDisplayFrameRate = "60";
    HandleResult(logger, "SetDisplayFrameRate", frameRate->SetDisplayFrameRate(newDisplayFrameRate, success), success);

    // Test SetFrmMode
    int autoFrameRate = 1;
    HandleResult(logger, "SetFrmMode", frameRate->SetFrmMode(autoFrameRate, success), success);

    // Test UpdateFps
    int fps = 60;
    HandleResult(logger, "UpdateFps", frameRate->UpdateFps(fps, success), success);
}

/*************************************** Signal Handling ****************************************/

std::atomic<bool> keepRunning = true;

void SignalHandler(int signal)
{
    keepRunning = false;
}

/******************************************** Main *********************************************/
int main(int argc, char* argv[])
{
    const std::string binaryName = (argc > 0) ? argv[0] : "entServicesCOMRPC-FrameRateTest";
    Logger logger(binaryName);

    const char* thunderAccess = std::getenv("THUNDER_ACCESS");
    std::string envThunderAccess = (thunderAccess != nullptr) ? thunderAccess : "/tmp/communicator";
    logger.Log("Using THUNDER_ACCESS: " + envThunderAccess);

    Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), envThunderAccess.c_str());
    Core::ProxyType<RPC::CommunicatorClient> client = Core::ProxyType<RPC::CommunicatorClient>::Create(
            Core::NodeId(envThunderAccess.c_str()));

    if (!client.IsValid()) {
        logger.Error("Failed to create COMRPC client.");
        return 1;
    }

    Exchange::IFrameRate* rawFrameRate = client->Open<Exchange::IFrameRate>(_T("FrameRate"));
    if (rawFrameRate == nullptr) {
        logger.Error("Failed to connect to FrameRate plugin.");
        return 1;
    }
    logger.Log("Connected to FrameRate plugin.");

    FrameRateProxy frameRate(rawFrameRate, logger);

    // Start the event listener thread
    std::thread eventThread(EventListener, std::ref(frameRate), std::ref(logger), std::ref(keepRunning));

    std::signal(SIGINT, SignalHandler);
    logger.Log("Press Ctrl+C to exit...");

    TestFrameRateMethods(frameRate, logger);

    while (keepRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Wait for the event listener thread to finish
    if (eventThread.joinable()) {
        eventThread.join();
    }

    logger.Log("Exiting...");
    return 0;
}
