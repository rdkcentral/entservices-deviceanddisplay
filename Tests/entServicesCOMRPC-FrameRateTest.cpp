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
#include <WPEFramework/com/com.h>
#include <WPEFramework/core/core.h>
#include "WPEFramework/interfaces/IFrameRate.h"

/************************************ Wrapper for Logging *************************************/
class Logger {
public:
    Logger(const std::string& binaryName) : _binaryName(binaryName) {}

    void Log(const std::string& message) const {
        std::cout << "[" << _binaryName << "] " << message << std::endl;
    }

    void Error(const std::string& message) const {
        std::cerr << "[" << _binaryName << "] " << message << std::endl;
    }

private:
    std::string _binaryName;
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

    // Handle FPS event
    void OnFpsEvent(int average, int min, int max) override {
        _logger.Log("FPS Event - Average: " + std::to_string(average) +
                    ", Min: " + std::to_string(min) +
                    ", Max: " + std::to_string(max));
    }

    // Handle display frame rate changing event
    void OnDisplayFrameRateChanging(const std::string& displayFrameRate) override {
        _logger.Log("Display Frame Rate Changing to: " + displayFrameRate);
    }

    // Handle display frame rate changed event
    void OnDisplayFrameRateChanged(const std::string& displayFrameRate) override {
        _logger.Log("Display Frame Rate Changed to: " + displayFrameRate);
    }

    BEGIN_INTERFACE_MAP(FrameRateEventHandler)
        INTERFACE_ENTRY(Exchange::IFrameRate::INotification)
    END_INTERFACE_MAP

private:
    const Logger& _logger;
};

int main(int argc, char* argv[])
{
	/*************************************** Logger Setup ****************************************/
    const std::string binaryName = (argc > 0) ? argv[0] : "entServicesCOMRPC-FrameRateTest";
    Logger logger(binaryName);

    /******************************************* Init *******************************************/
    const char* thunderAccess = std::getenv("THUNDER_ACCESS");
    std::string envThunderAccess = (thunderAccess != nullptr) ? thunderAccess : "/tmp/communicator";
    logger.Log("Using THUNDER_ACCESS: " + envThunderAccess);

    Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), envThunderAccess.c_str());
    Core::ProxyType<RPC::CommunicatorClient> client = Core::ProxyType<RPC::CommunicatorClient>::Create(
        Core::NodeId(envThunderAccess.c_str()));

    if (client.IsValid() == false) {
        logger.Error("Failed to create COMRPC client.");
        return 1;
    }

    /************************************* Plugin Connector **************************************/
    Exchange::IFrameRate* rawFrameRate = client->Open<Exchange::IFrameRate>(_T("FrameRate"));
    if (rawFrameRate == nullptr) {
        logger.Error("Failed to connect to FrameRate plugin.");
        return 1;
    }
    logger.Log("Connected to FrameRate plugin.");

    // Use RAII wrapper for FrameRate proxy
    FrameRateProxy frameRate(rawFrameRate, logger);

    /************************************ Subscribe to Events ************************************/
    FrameRateEventHandler eventHandler(logger);
    frameRate->Register(&eventHandler);
    logger.Log("Event handler registered.");

    /************************************* Test All Methods **************************************/
    bool success = false;
    int frmmode = 0;

    // Gets the Display Frame Rate - GetDisplayFrameRate method
    uint32_t result = frameRate->GetDisplayFrameRate(frmmode, success);
    if (result == Core::ERROR_NONE && success) {
        logger.Log("GetDisplayFrameRate response: " + std::to_string(frmmode));
    } else {
        logger.Error("GetDisplayFrameRate error: " + std::to_string(result));
    }

    // Gets framerate mode - GetFrmMode method
    result = frameRate->GetFrmMode(frmmode, success);
    if (result == Core::ERROR_NONE && success) {
        logger.Log("GetFrmMode response: " + std::to_string(frmmode));
    } else {
        logger.Error("GetFrmMode error: " + std::to_string(result));
    }

    // Sets the FPS data collection interval - SetCollectionFrequency method
    int frequency = 30;
    result = frameRate->SetCollectionFrequency(frequency, success);
    if (result == Core::ERROR_NONE && success) {
        logger.Log("SetCollectionFrequency response: " + std::to_string(frequency));
    } else {
        logger.Error("SetCollectionFrequency error: " + std::to_string(result));
    }

    // Sets the display framerate values - SetDisplayFrameRate method
    const char* displayFrameRate = "60";
    result = frameRate->SetDisplayFrameRate(displayFrameRate, success);
    if (result == Core::ERROR_NONE && success) {
        logger.Log("SetDisplayFrameRate response: " + std::string(displayFrameRate));
    } else {
        logger.Error("SetDisplayFrameRate error: " + std::to_string(result));
    }

    // Sets the auto framerate mode - SetFrmMode method
    int autoFrameRate = 1;
    result = frameRate->SetFrmMode(autoFrameRate, success);
    if (result == Core::ERROR_NONE && success) {
        logger.Log("SetFrmMode response: " + std::to_string(autoFrameRate));
    } else {
        logger.Error("SetFrmMode error: " + std::to_string(result));
    }

    // Starts the FPS data collection - StartFpsCollection method
    result = frameRate->StartFpsCollection(success);
    if (result == Core::ERROR_NONE && success) {
        logger.Log("StartFpsCollection response: " + std::to_string(success));
    } else {
        logger.Error("StartFpsCollection error: " + std::to_string(result));
    }

    // Stops the FPS data collection - StopFpsCollection method
    result = frameRate->StopFpsCollection(success);
    if (result == Core::ERROR_NONE && success) {
        logger.Log("StopFpsCollection response: " + std::to_string(success));
    } else {
        logger.Error("StopFpsCollection error: " + std::to_string(result));
    }

    // Update the FPS values - UpdateFps method
    int fps = 60;
    result = frameRate->UpdateFps(fps, success);
    if (result == Core::ERROR_NONE && success) {
        logger.Log("UpdateFps response: " + std::to_string(success));
    } else {
        logger.Error("UpdateFps error: " + std::to_string(result));
    }

    /************************************ Wait for Notifications ************************************/
    logger.Log("Waiting for events... Press Ctrl+C to exit.");
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }

    /******************************************* Clean-Up *******************************************/
    // FrameRateProxy destructor will automatically release the proxy
    logger.Log("Exiting...");
    frameRate->Unregister(&eventHandler);
    return 0;
}
