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
#include "DeviceSettingsAudioImplementation.h"

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
        , _audioSettings(DeviceSettingsAudioImpl::Create())
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
        LOGINFO("Audio implementation instance: %p", _audioSettings);

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
        
        if (_audioSettings != nullptr) {
            delete _audioSettings;
            _audioSettings = nullptr;
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

    // ============================================================================
    // IDeviceSettingsAudio interface implementation - delegate to _audioSettings interface
    // ============================================================================
    
    Core::hresult DeviceSettingsImp::Register(Exchange::IDeviceSettingsAudio::INotification* notification) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->Register(notification) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::Unregister(Exchange::IDeviceSettingsAudio::INotification* notification) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->Unregister(notification) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioPort(const AudioPortType type, const int32_t index, int32_t &handle) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioPort(type, index, handle) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    // GetAudioPorts and GetSupportedAudioPorts methods removed - iterator type doesn't exist

    Core::hresult DeviceSettingsImp::GetAudioPortConfig(const AudioPortType audioPort, AudioConfig &audioConfig) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioPortConfig(audioPort, audioConfig) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsImp::SetAudioPortConfig(const AudioPortType audioPort, const AudioConfig audioConfig) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioPortConfig(audioPort, audioConfig) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioCapabilities(const int32_t handle, int32_t &capabilities) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioCapabilities(handle, capabilities) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioMS12Capabilities(const int32_t handle, int32_t &capabilities) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioMS12Capabilities(handle, capabilities) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioFormat(const int32_t handle, AudioFormat &audioFormat) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioFormat(handle, audioFormat) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioEncoding(const int32_t handle, AudioEncoding &encoding) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioEncoding(handle, encoding) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetSupportedCompressions(const int32_t handle, IDeviceSettingsAudioCompressionIterator*& compressions) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetSupportedCompressions(handle, compressions) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetCompression(const int32_t handle, AudioCompression &compression) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetCompression(handle, compression) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetCompression(const int32_t handle, const AudioCompression compression) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetCompression(handle, compression) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioLevel(const int32_t handle, const float audioLevel) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioLevel(handle, audioLevel) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioLevel(const int32_t handle, float &audioLevel) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioLevel(handle, audioLevel) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioGain(const int32_t handle, const float gainLevel) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioGain(handle, gainLevel) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioGain(const int32_t handle, float &gainLevel) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioGain(handle, gainLevel) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioMute(const int32_t handle, const bool mute) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioMute(handle, mute) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::IsAudioMuted(const int32_t handle, bool &muted) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->IsAudioMuted(handle, muted) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioDucking(const int32_t handle, const AudioDuckingType duckingType, const AudioDuckingAction duckingAction, const uint8_t level) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioDucking(handle, duckingType, duckingAction, level) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetStereoMode(const int32_t handle, AudioStereoMode &mode) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetStereoMode(handle, mode) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetStereoMode(const int32_t handle, const AudioStereoMode mode, const bool persist) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetStereoMode(handle, mode, persist) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAssociatedAudioMixing(const int32_t handle, const bool mixing) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAssociatedAudioMixing(handle, mixing) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAssociatedAudioMixing(const int32_t handle, bool &mixing) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAssociatedAudioMixing(handle, mixing) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioFaderControl(const int32_t handle, const int32_t mixerBalance) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioFaderControl(handle, mixerBalance) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioFaderControl(const int32_t handle, int32_t &mixerBalance) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioFaderControl(handle, mixerBalance) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioPrimaryLanguage(const int32_t handle, const string primaryAudioLanguage) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioPrimaryLanguage(handle, primaryAudioLanguage) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioPrimaryLanguage(const int32_t handle, string &primaryAudioLanguage) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioPrimaryLanguage(handle, primaryAudioLanguage) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioSecondaryLanguage(const int32_t handle, const string secondaryAudioLanguage) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioSecondaryLanguage(handle, secondaryAudioLanguage) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioSecondaryLanguage(const int32_t handle, string &secondaryAudioLanguage) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioSecondaryLanguage(handle, secondaryAudioLanguage) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::IsAudioOutputConnected(const int32_t handle, bool &isConnected) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->IsAudioOutputConnected(handle, isConnected) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioSinkDeviceAtmosCapability(const int32_t handle, DolbyAtmosCapability &atmosCapability) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioSinkDeviceAtmosCapability(handle, atmosCapability) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioAtmosOutputMode(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioAtmosOutputMode(handle, enable) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }

    // Missing Audio interface delegation methods
    
    Core::hresult DeviceSettingsImp::IsAudioPortEnabled(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->IsAudioPortEnabled(handle, enabled) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::EnableAudioPort(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->EnableAudioPort(handle, enable) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetSupportedARCTypes(const int32_t handle, int32_t &types) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetSupportedARCTypes(handle, types) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetSAD(const int32_t handle, const uint8_t sadList[], const uint8_t count) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetSAD(handle, sadList, count) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::EnableARC(const int32_t handle, const AudioARCStatus arcStatus) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->EnableARC(handle, arcStatus) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetStereoAuto(const int32_t handle, int32_t &mode) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetStereoAuto(handle, mode) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetStereoAuto(const int32_t handle, const int32_t mode, const bool persist) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetStereoAuto(handle, mode, persist) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioEnablePersist(const int32_t handle, bool &enabled, string &portName) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioEnablePersist(handle, enabled, portName) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioEnablePersist(const int32_t handle, const bool enable, const string portName) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioEnablePersist(handle, enable, portName) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::IsAudioMSDecoded(const int32_t handle, bool &hasms11Decode) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->IsAudioMSDecoded(handle, hasms11Decode) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::IsAudioMS12Decoded(const int32_t handle, bool &hasms12Decode) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->IsAudioMS12Decoded(handle, hasms12Decode) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioLEConfig(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioLEConfig(handle, enabled) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::EnableAudioLEConfig(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->EnableAudioLEConfig(handle, enable) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioDelay(const int32_t handle, const uint32_t audioDelay) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioDelay(handle, audioDelay) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioDelay(const int32_t handle, uint32_t &audioDelay) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioDelay(handle, audioDelay) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioDelayOffset(const int32_t handle, const uint32_t delayOffset) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioDelayOffset(handle, delayOffset) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioDelayOffset(const int32_t handle, uint32_t &delayOffset) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioDelayOffset(handle, delayOffset) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioCompression(const int32_t handle, const int32_t compressionLevel) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioCompression(handle, compressionLevel) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioCompression(const int32_t handle, int32_t &compressionLevel) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioCompression(handle, compressionLevel) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioDialogEnhancement(const int32_t handle, const int32_t level) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioDialogEnhancement(handle, level) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioDialogEnhancement(const int32_t handle, int32_t &level) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioDialogEnhancement(handle, level) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioDolbyVolumeMode(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioDolbyVolumeMode(handle, enable) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioDolbyVolumeMode(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioDolbyVolumeMode(handle, enabled) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioIntelligentEqualizerMode(const int32_t handle, const int32_t mode) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioIntelligentEqualizerMode(handle, mode) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioIntelligentEqualizerMode(const int32_t handle, int32_t &mode) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioIntelligentEqualizerMode(handle, mode) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioVolumeLeveller(const int32_t handle, const VolumeLeveller volumeLeveller) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioVolumeLeveller(handle, volumeLeveller) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioVolumeLeveller(const int32_t handle, VolumeLeveller &volumeLeveller) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioVolumeLeveller(handle, volumeLeveller) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioBassEnhancer(const int32_t handle, const int32_t boost) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioBassEnhancer(handle, boost) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioBassEnhancer(const int32_t handle, int32_t &boost) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioBassEnhancer(handle, boost) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::EnableAudioSurroudDecoder(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->EnableAudioSurroudDecoder(handle, enable) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::IsAudioSurroudDecoderEnabled(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->IsAudioSurroudDecoderEnabled(handle, enabled) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioDRCMode(const int32_t handle, const int32_t drcMode) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioDRCMode(handle, drcMode) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioDRCMode(const int32_t handle, int32_t &drcMode) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioDRCMode(handle, drcMode) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioSurroudVirtualizer(const int32_t handle, const SurroundVirtualizer surroundVirtualizer) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioSurroudVirtualizer(handle, surroundVirtualizer) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioSurroudVirtualizer(const int32_t handle, SurroundVirtualizer &surroundVirtualizer) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioSurroudVirtualizer(handle, surroundVirtualizer) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioMISteering(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioMISteering(handle, enable) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioMISteering(const int32_t handle, bool &enable) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioMISteering(handle, enable) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioGraphicEqualizerMode(const int32_t handle, const int32_t mode) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioGraphicEqualizerMode(handle, mode) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioGraphicEqualizerMode(const int32_t handle, int32_t &mode) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioGraphicEqualizerMode(handle, mode) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioMS12ProfileList(const int32_t handle, IDeviceSettingsAudioMS12AudioProfileIterator*& ms12ProfileList) const {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioMS12ProfileList(handle, ms12ProfileList) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioMS12Profile(const int32_t handle, string &profile) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioMS12Profile(handle, profile) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioMS12Profile(const int32_t handle, const string profile) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioMS12Profile(handle, profile) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioMixerLevels(const int32_t handle, const AudioInput audioInput, const int32_t volume) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioMixerLevels(handle, audioInput, volume) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::SetAudioMS12SettingsOverride(const int32_t handle, const string profileName, const string profileSettingsName, const string profileSettingValue, const string profileState) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->SetAudioMS12SettingsOverride(handle, profileName, profileSettingsName, profileSettingValue, profileState) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::ResetAudioDialogEnhancement(const int32_t handle) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->ResetAudioDialogEnhancement(handle) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::ResetAudioBassEnhancer(const int32_t handle) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->ResetAudioBassEnhancer(handle) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::ResetAudioSurroundVirtualizer(const int32_t handle) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->ResetAudioSurroundVirtualizer(handle) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::ResetAudioVolumeLeveller(const int32_t handle) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->ResetAudioVolumeLeveller(handle) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsImp::GetAudioHDMIARCPortId(const int32_t handle, int32_t &portId) {
        ENTRY_LOG;
        Core::hresult result = (_audioSettings != nullptr) ? _audioSettings->GetAudioHDMIARCPortId(handle, portId) : Core::ERROR_UNAVAILABLE;
        EXIT_LOG;
        return result;
    }

} // namespace Plugin
} // namespace WPEFramework
