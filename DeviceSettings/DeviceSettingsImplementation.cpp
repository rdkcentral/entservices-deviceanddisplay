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
#include "DeviceSettingsFPDImplementation.h"
#include "DeviceSettingsHdmiInImplementation.h"

#include "UtilsLogging.h"
#include "UtilsSearchRDKProfile.h"
#include <syscall.h>

using namespace std;

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DeviceSettingsImp, 1, 0);

    DeviceSettingsImp* DeviceSettingsImp::_instance = nullptr;

    DeviceSettingsImp::DeviceSettingsImp()
        : _fpdSettings(DeviceSettingsFPDImpl::Create())
        , _hdmiInSettings(DeviceSettingsHdmiInImp::Create())
        , mConnectionId(0)
    {
        ENTRY_LOG;
        DeviceSettingsImp::_instance = this;
        LOGINFO("DeviceSettingsImp Constructor - Instance Address: %p", this);

        // Initialize profile type
        profileType = searchRdkProfile();
        LOGINFO("Initialized profileType: %d (0=STB, 1=TV)", profileType);

        LOGINFO("FPD implementation instance: %p", _fpdSettings);
        LOGINFO("HDMIIn implementation instance: %p", _hdmiInSettings);

        EXIT_LOG;
    }

    DeviceSettingsImp::~DeviceSettingsImp() {
        ENTRY_LOG;
        LOGINFO("DeviceSettingsImp Destructor - Instance Address: %p", this);
        
        // Clean up created implementation instances
        if (_fpdSettings != nullptr) {
            delete _fpdSettings;
            _fpdSettings = nullptr;
        }
        
        if (_hdmiInSettings != nullptr) {
            delete _hdmiInSettings;
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

        LOGINFO("DeviceSettingsImp configured successfully");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    // ============================================================================
    // IDeviceSettingsFPD interface implementation - delegate to _fpdSettings interface
    // ============================================================================
    
    Core::hresult DeviceSettingsImp::Register(Exchange::IDeviceSettingsFPD::INotification* notification) {
        ENTRY_LOG;
        Core::hresult result;
        if (_fpdSettings != nullptr) {
            result = _fpdSettings->Register(notification);
            LOGINFO("FPD Register: SUCCESS - forwarded to implementation");
        } else {
            LOGERR("FPD Register: FAILED - _fpdSettings is null");
            result = Core::ERROR_UNAVAILABLE;
        }
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::Unregister(Exchange::IDeviceSettingsFPD::INotification* notification) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->Unregister(notification) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->SetFPDTime(timeFormat, minutes, seconds) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->SetFPDScroll(scrollHoldDuration, nHorizontalScrollIterations, nVerticalScrollIterations) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->SetFPDBlink(indicator, blinkDuration, blinkIterations) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->SetFPDBrightness(indicator, brightNess, persist) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess) {
        ENTRY_LOG;
        Core::hresult result;
        if (_fpdSettings != nullptr) {
            result = _fpdSettings->GetFPDBrightness(indicator, brightNess);
            LOGINFO("GetFPDBrightness: SUCCESS - forwarded to implementation");
        } else {
            LOGERR("GetFPDBrightness: FAILED - _fpdSettings is null");
            result = Core::ERROR_UNAVAILABLE;
        }
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDState(const FPDIndicator indicator, const FPDState state) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->SetFPDState(indicator, state) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetFPDState(const FPDIndicator indicator, FPDState &state) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->GetFPDState(indicator, state) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetFPDColor(const FPDIndicator indicator, uint32_t &color) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->GetFPDColor(indicator, color) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDColor(const FPDIndicator indicator, const uint32_t color) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->SetFPDColor(indicator, color) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->SetFPDTextBrightness(textDisplay, brightNess) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->GetFPDTextBrightness(textDisplay, brightNess) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::EnableFPDClockDisplay(const bool enable) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->EnableFPDClockDisplay(enable) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->GetFPDTimeFormat(fpdTimeFormat) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->SetFPDTimeFormat(fpdTimeFormat) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetFPDMode(const FPDMode fpdMode) {
        ENTRY_LOG;
        Core::hresult result = (_fpdSettings != nullptr) ? _fpdSettings->SetFPDMode(fpdMode) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }

    // ============================================================================
    // IDeviceSettingsHDMIIn interface implementation - delegate to _hdmiInSettings interface
    // ============================================================================
    
    Core::hresult DeviceSettingsImp::Register(Exchange::IDeviceSettingsHDMIIn::INotification* notification) {
        ENTRY_LOG;
        Core::hresult result;
        if (_hdmiInSettings != nullptr) {
            result = _hdmiInSettings->Register(notification);
            LOGINFO("HDMIIn Register: SUCCESS - forwarded to implementation");
        } else {
            LOGERR("HDMIIn Register: FAILED - _hdmiInSettings is null");
            result = Core::ERROR_UNAVAILABLE;
        }
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::Unregister(Exchange::IDeviceSettingsHDMIIn::INotification* notification) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->Unregister(notification) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInNumbefOfInputs(int32_t &count) {
        ENTRY_LOG;
        Core::hresult result;
        if (_hdmiInSettings != nullptr) {
            result = _hdmiInSettings->GetHDMIInNumbefOfInputs(count);
            LOGINFO("GetHDMIInNumbefOfInputs: SUCCESS - forwarded to implementation");
        } else {
            LOGERR("GetHDMIInNumbefOfInputs: FAILED - _hdmiInSettings is null");
            result = Core::ERROR_UNAVAILABLE;
        }
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIInStatus(hdmiStatus, portConnectionStatus) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->SelectHDMIInPort(port, requestAudioMix, topMostPlane, videoPlaneType) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->ScaleHDMIInVideo(videoPosition) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->SelectHDMIZoomMode(zoomMode) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator *& gameFeatureList) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetSupportedGameFeaturesList(gameFeatureList) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIInAVLatency(videoLatency, audioLatency) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIInAllmStatus(port, allmStatus) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIInEdid2AllmSupport(port, allmSupport) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->SetHDMIInEdid2AllmSupport(port, allmSupport) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetEdidBytes(port, edidBytesLength, edidBytes) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMISPDInformation(port, spdBytesLength, spdBytes) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIEdidVersion(port, edidVersion) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->SetHDMIEdidVersion(port, edidVersion) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIVideoMode(videoPortResolution) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetHDMIVersion(port, capabilityVersion) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetVRRSupport(const HDMIInPort port, const bool vrrSupport) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->SetVRRSupport(port, vrrSupport) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetVRRSupport(const HDMIInPort port, bool &vrrSupport) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetVRRSupport(port, vrrSupport) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus) {
        ENTRY_LOG;
        Core::hresult result = (_hdmiInSettings != nullptr) ? _hdmiInSettings->GetVRRStatus(port, vrrStatus) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }

} // namespace Plugin
} // namespace WPEFramework
