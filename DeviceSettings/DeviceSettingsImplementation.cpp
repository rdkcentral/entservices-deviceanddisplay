/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 */

#include "DeviceSettingsImplementation.h"

#include "UtilsLogging.h"
#include "UtilsSearchRDKProfile.h"
#include <syscall.h>

using namespace std;

namespace WPEFramework {
namespace Plugin {

//    SERVICE_REGISTRATION(DeviceSettingsImp, 1, 0);

    DeviceSettingsImp* DeviceSettingsImp::_instance = nullptr;

    DeviceSettingsImp::DeviceSettingsImp()
        : _audioSettings(nullptr)
        , _compositeInSettings(nullptr)
        , _displaySettings(nullptr)
        , _fpdSettings(nullptr)
        , _hdmiInSettings(nullptr)
        , _hostSettings(nullptr)
        , _videoDeviceSettings(nullptr)
        , _videoPortSettings(nullptr)
        , mConnectionId(0)
    {
        ENTRY_LOG;
        DeviceSettingsImp::_instance = this;
        LOGINFO("DeviceSettingsImp Constructor - Instance Address: %p", this);

        // Initialize profile type
        profileType = searchRdkProfile();
        LOGINFO("Initialized profileType: %d (0=STB, 1=TV)", profileType);

        EXIT_LOG;
    }

    DeviceSettingsImp::~DeviceSettingsImp() {
        ENTRY_LOG;
        LOGINFO("DeviceSettingsImp Destructor - Instance Address: %p", this);
        
        // Release all component handles
        if (_audioSettings != nullptr) {
            _audioSettings->Release();
            _audioSettings = nullptr;
        }
        if (_compositeInSettings != nullptr) {
            _compositeInSettings->Release();
            _compositeInSettings = nullptr;
        }
        if (_displaySettings != nullptr) {
            _displaySettings->Release();
            _displaySettings = nullptr;
        }
        if (_fpdSettings != nullptr) {
            _fpdSettings->Release();
            _fpdSettings = nullptr;
        }
        if (_hdmiInSettings != nullptr) {
            _hdmiInSettings->Release();
            _hdmiInSettings = nullptr;
        }
        if (_hostSettings != nullptr) {
            _hostSettings->Release();
            _hostSettings = nullptr;
        }
        if (_videoDeviceSettings != nullptr) {
            _videoDeviceSettings->Release();
            _videoDeviceSettings = nullptr;
        }
        if (_videoPortSettings != nullptr) {
            _videoPortSettings->Release();
            _videoPortSettings = nullptr;
        }
        
        EXIT_LOG;
    }

    Core::hresult DeviceSettingsImp::Configure(PluginHost::IShell* service)
    {
        ENTRY_LOG;
        LOGINFO("DeviceSettingsImp Configure called with service: %p", service);

        if (service == nullptr) {
            LOGERR("Service parameter is null");
            EXIT_LOG;
            return Core::ERROR_BAD_REQUEST;
        }

        // Initialize component handles using service->Root() and QueryInterface
        InitializeComponentHandles(service);

        LOGINFO("DeviceSettingsImp configured successfully");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    void DeviceSettingsImp::InitializeComponentHandles(PluginHost::IShell* service)
    {
        ENTRY_LOG;
        LOGINFO("Initializing component handles");

        if (service == nullptr) {
            LOGERR("Service is null, cannot initialize component handles");
            EXIT_LOG;
            return;
        }

        // Get the root interface from service
        Core::IUnknown* root = service->Root<Core::IUnknown>(mConnectionId, RPC::CommunicationTimeOut, _T("DeviceSettingsImp"));
        if (root == nullptr) {
            LOGERR("Failed to get root interface from service");
            EXIT_LOG;
            return;
        }

        // Get AudioSettings component handle
        _audioSettings = root->QueryInterface<Exchange::IDeviceSettingsAudio>();
        if (_audioSettings != nullptr) {
            LOGINFO("Successfully obtained AudioSettings handle");
        } else {
            LOGWARN("Failed to obtain AudioSettings handle");
        }

        // Get CompositeInSettings component handle
        _compositeInSettings = root->QueryInterface<Exchange::IDeviceSettingsCompositeIn>();
        if (_compositeInSettings != nullptr) {
            LOGINFO("Successfully obtained CompositeInSettings handle");
        } else {
            LOGWARN("Failed to obtain CompositeInSettings handle");
        }

        // Get DisplaySettings component handle
        _displaySettings = root->QueryInterface<Exchange::IDeviceSettingsDisplay>();
        if (_displaySettings != nullptr) {
            LOGINFO("Successfully obtained DisplaySettings handle");
        } else {
            LOGWARN("Failed to obtain DisplaySettings handle");
        }

        // Get FPDSettings component handle
        _fpdSettings = root->QueryInterface<Exchange::IDeviceSettingsFPD>();
        if (_fpdSettings != nullptr) {
            LOGINFO("Successfully obtained FPDSettings handle");
        } else {
            LOGWARN("Failed to obtain FPDSettings handle");
        }

        // Get HDMIInSettings component handle
        _hdmiInSettings = root->QueryInterface<Exchange::IDeviceSettingsHDMIIn>();
        if (_hdmiInSettings != nullptr) {
            LOGINFO("Successfully obtained HDMIInSettings handle");
        } else {
            LOGWARN("Failed to obtain HDMIInSettings handle");
        }

        // Get HostSettings component handle
        _hostSettings = root->QueryInterface<Exchange::IDeviceSettingsHost>();
        if (_hostSettings != nullptr) {
            LOGINFO("Successfully obtained HostSettings handle");
        } else {
            LOGWARN("Failed to obtain HostSettings handle");
        }

        // Get VideoDeviceSettings component handle
        _videoDeviceSettings = root->QueryInterface<Exchange::IDeviceSettingsVideoDevice>();
        if (_videoDeviceSettings != nullptr) {
            LOGINFO("Successfully obtained VideoDeviceSettings handle");
        } else {
            LOGWARN("Failed to obtain VideoDeviceSettings handle");
        }

        // Get VideoPortSettings component handle
        _videoPortSettings = root->QueryInterface<Exchange::IDeviceSettingsVideoPort>();
        if (_videoPortSettings != nullptr) {
            LOGINFO("Successfully obtained VideoPortSettings handle");
        } else {
            LOGWARN("Failed to obtain VideoPortSettings handle");
        }

        // Release the root interface reference
        root->Release();

        LOGINFO("Component handles initialization completed");
        EXIT_LOG;
    }
} // namespace Plugin
} // namespace WPEFramework
