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

    SERVICE_REGISTRATION(DeviceSettingsImp, 1, 0);

    DeviceSettingsImp* DeviceSettingsImp::_instance = nullptr;

    DeviceSettingsImp::DeviceSettingsImp()
        : _fpdSettings(nullptr)
        , _hdmiInSettings(nullptr)
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
        
        // Release interface pointers obtained through QueryInterface on this instance
        if (_fpdSettings != nullptr) {
            _fpdSettings->Release();
            _fpdSettings = nullptr;
        }
        
        if (_hdmiInSettings != nullptr) {
            _hdmiInSettings->Release();
            _hdmiInSettings = nullptr;
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

        // Initialize component handles using QueryInterface on this instance
        InitializeComponentHandles(service);

        LOGINFO("DeviceSettingsImp configured successfully");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    void DeviceSettingsImp::InitializeComponentHandles(PluginHost::IShell* service)
    {
        ENTRY_LOG;
        LOGINFO("Initializing component handles using QueryInterface on self");

#if 0
        if (service == nullptr) {
            LOGERR("Service is null, cannot initialize component handles");
            EXIT_LOG;
            return;
        }

        // Since DeviceSettingsImp implements IDeviceSettingsFPD, get the interface handle via QueryInterface
        void* fpdInterface = this->QueryInterface(Exchange::IDeviceSettingsFPD::ID);
        if (fpdInterface != nullptr) {
            _fpdSettings = reinterpret_cast<Exchange::IDeviceSettingsFPD*>(fpdInterface);
            LOGINFO("Successfully obtained FPD interface handle via QueryInterface: %p", _fpdSettings);
        } else {
            LOGERR("Failed to get IDeviceSettingsFPD interface via QueryInterface - this should not happen");
        }

        // Since DeviceSettingsImp implements IDeviceSettingsHDMIIn, get the interface handle via QueryInterface
        void* hdmiInInterface = this->QueryInterface(Exchange::IDeviceSettingsHDMIIn::ID);
        if (hdmiInInterface != nullptr) {
            _hdmiInSettings = reinterpret_cast<Exchange::IDeviceSettingsHDMIIn*>(hdmiInInterface);
            LOGINFO("Successfully obtained HDMIIn interface handle via QueryInterface: %p", _hdmiInSettings);
        } else {
            LOGERR("Failed to get IDeviceSettingsHDMIIn interface via QueryInterface - this should not happen");
        }

        // TODO: Add QueryInterface calls for other interfaces as they are implemented
        // _audioSettings = this->QueryInterface<Exchange::IDeviceSettingsAudio>();
        // _compositeInSettings = this->QueryInterface<Exchange::IDeviceSettingsCompositeIn>();
        // _displaySettings = this->QueryInterface<Exchange::IDeviceSettingsDisplay>();

        LOGINFO("Component handles initialization completed using QueryInterface");
#endif
        EXIT_LOG;
    }

    // ============================================================================
    // IDeviceSettingsFPD interface implementation - delegate to _fpdSettings interface
    // ============================================================================
    
    Core::hresult DeviceSettingsImp::Register(Exchange::IDeviceSettingsFPD::INotification* notification) {
        return (_fpdSettings != nullptr) ? _fpdSettings->Register(notification) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::Unregister(Exchange::IDeviceSettingsFPD::INotification* notification) {
        return (_fpdSettings != nullptr) ? _fpdSettings->Unregister(notification) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) {
        return (_fpdSettings != nullptr) ? _fpdSettings->SetFPDTime(timeFormat, minutes, seconds) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) {
        return (_fpdSettings != nullptr) ? _fpdSettings->SetFPDScroll(scrollHoldDuration, nHorizontalScrollIterations, nVerticalScrollIterations) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) {
        return (_fpdSettings != nullptr) ? _fpdSettings->SetFPDBlink(indicator, blinkDuration, blinkIterations) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) {
        return (_fpdSettings != nullptr) ? _fpdSettings->SetFPDBrightness(indicator, brightNess, persist) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess) {
        return (_fpdSettings != nullptr) ? _fpdSettings->GetFPDBrightness(indicator, brightNess) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDState(const FPDIndicator indicator, const FPDState state) {
        return (_fpdSettings != nullptr) ? _fpdSettings->SetFPDState(indicator, state) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetFPDState(const FPDIndicator indicator, FPDState &state) {
        return (_fpdSettings != nullptr) ? _fpdSettings->GetFPDState(indicator, state) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetFPDColor(const FPDIndicator indicator, uint32_t &color) {
        return (_fpdSettings != nullptr) ? _fpdSettings->GetFPDColor(indicator, color) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDColor(const FPDIndicator indicator, const uint32_t color) {
        return (_fpdSettings != nullptr) ? _fpdSettings->SetFPDColor(indicator, color) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) {
        return (_fpdSettings != nullptr) ? _fpdSettings->SetFPDTextBrightness(textDisplay, brightNess) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) {
        return (_fpdSettings != nullptr) ? _fpdSettings->GetFPDTextBrightness(textDisplay, brightNess) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::EnableFPDClockDisplay(const bool enable) {
        return (_fpdSettings != nullptr) ? _fpdSettings->EnableFPDClockDisplay(enable) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) {
        return (_fpdSettings != nullptr) ? _fpdSettings->GetFPDTimeFormat(fpdTimeFormat) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) {
        return (_fpdSettings != nullptr) ? _fpdSettings->SetFPDTimeFormat(fpdTimeFormat) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDMode(const FPDMode fpdMode) {
        return (_fpdSettings != nullptr) ? _fpdSettings->SetFPDMode(fpdMode) : Core::ERROR_UNAVAILABLE;
    }

    // ============================================================================
    // IDeviceSettingsHDMIIn interface implementation - delegate to _hdmiInSettings interface
    // ============================================================================
    
    Core::hresult DeviceSettingsImp::Register(Exchange::IDeviceSettingsHDMIIn::INotification* notification) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->Register(notification) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::Unregister(Exchange::IDeviceSettingsHDMIIn::INotification* notification) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->Unregister(notification) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInNumbefOfInputs(int32_t &count) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIInNumbefOfInputs(count) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIInStatus(hdmiStatus, portConnectionStatus) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->SelectHDMIInPort(port, requestAudioMix, topMostPlane, videoPlaneType) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->ScaleHDMIInVideo(videoPosition) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->SelectHDMIZoomMode(zoomMode) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator *& gameFeatureList) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetSupportedGameFeaturesList(gameFeatureList) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIInAVLatency(videoLatency, audioLatency) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIInAllmStatus(port, allmStatus) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIInEdid2AllmSupport(port, allmSupport) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->SetHDMIInEdid2AllmSupport(port, allmSupport) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetEdidBytes(port, edidBytesLength, edidBytes) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMISPDInformation(port, spdBytesLength, spdBytes) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIEdidVersion(port, edidVersion) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->SetHDMIEdidVersion(port, edidVersion) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIVideoMode(videoPortResolution) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIVersion(port, capabilityVersion) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::SetVRRSupport(const HDMIInPort port, const bool vrrSupport) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->SetVRRSupport(port, vrrSupport) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetVRRSupport(const HDMIInPort port, bool &vrrSupport) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetVRRSupport(port, vrrSupport) : Core::ERROR_UNAVAILABLE;
    }
    
    Core::hresult DeviceSettingsImp::GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus) {
        return (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetVRRStatus(port, vrrStatus) : Core::ERROR_UNAVAILABLE;
    }
    
    // ============================================================================
    // Stub implementations for other interfaces - return ERROR_NOT_SUPPORTED for now
    // ============================================================================
    
    // Note: Based on the interface inheritance, DeviceSettingsImp must implement
    // ALL pure virtual methods from the inherited interfaces. Since we only have
    // FPD and HDMIIn implementations ready, we provide stub implementations
    // for other interfaces that return ERROR_NOT_SUPPORTED.
    //
    // These stub implementations prevent the class from being abstract while
    // clearly indicating that the functionality is not yet implemented.
} // namespace Plugin
} // namespace WPEFramework
