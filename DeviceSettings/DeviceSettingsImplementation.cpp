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

#define DELEGATE_TO_COMPONENT(component, method, ...) \
    ENTRY_LOG; \
    Core::hresult result = (component != nullptr) ? component->method(__VA_ARGS__) : Core::ERROR_UNAVAILABLE; \
    EXIT_LOG; \
    return result;

// Macro for methods that don't take parameters
#define DELEGATE_TO_COMPONENT_NO_PARAMS(component, method) \
    ENTRY_LOG; \
    Core::hresult result = (component != nullptr) ? component->method() : Core::ERROR_UNAVAILABLE; \
    EXIT_LOG; \
    return result;

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
        DELEGATE_TO_COMPONENT(_fpdSettings, Unregister, notification)
    }
    
    Core::hresult DeviceSettingsImp::SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) {
        DELEGATE_TO_COMPONENT(_fpdSettings, SetFPDTime, timeFormat, minutes, seconds)
    }
    
    Core::hresult DeviceSettingsImp::SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) {
        DELEGATE_TO_COMPONENT(_fpdSettings, SetFPDScroll, scrollHoldDuration, nHorizontalScrollIterations, nVerticalScrollIterations)
    }
    
    Core::hresult DeviceSettingsImp::SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) {
        DELEGATE_TO_COMPONENT(_fpdSettings, SetFPDBlink, indicator, blinkDuration, blinkIterations)
    }
    
    Core::hresult DeviceSettingsImp::SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) {
        DELEGATE_TO_COMPONENT(_fpdSettings, SetFPDBrightness, indicator, brightNess, persist)
    }
    
    Core::hresult DeviceSettingsImp::GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess) {
        DELEGATE_TO_COMPONENT(_fpdSettings, GetFPDBrightness, indicator, brightNess)
    }
    
    Core::hresult DeviceSettingsImp::SetFPDState(const FPDIndicator indicator, const FPDState state) {
        DELEGATE_TO_COMPONENT(_fpdSettings, SetFPDState, indicator, state)
    }
    
    Core::hresult DeviceSettingsImp::GetFPDState(const FPDIndicator indicator, FPDState &state) {
        DELEGATE_TO_COMPONENT(_fpdSettings, GetFPDState, indicator, state)
    }
    
    Core::hresult DeviceSettingsImp::GetFPDColor(const FPDIndicator indicator, uint32_t &color) {
        DELEGATE_TO_COMPONENT(_fpdSettings, GetFPDColor, indicator, color)
    }
    
    Core::hresult DeviceSettingsImp::SetFPDColor(const FPDIndicator indicator, const uint32_t color) {
        DELEGATE_TO_COMPONENT(_fpdSettings, SetFPDColor, indicator, color)
    }
    
    Core::hresult DeviceSettingsImp::SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) {
        DELEGATE_TO_COMPONENT(_fpdSettings, SetFPDTextBrightness, textDisplay, brightNess)
    }
    
    Core::hresult DeviceSettingsImp::GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) {
        DELEGATE_TO_COMPONENT(_fpdSettings, GetFPDTextBrightness, textDisplay, brightNess)
    }
    
    Core::hresult DeviceSettingsImp::EnableFPDClockDisplay(const bool enable) {
        DELEGATE_TO_COMPONENT(_fpdSettings, EnableFPDClockDisplay, enable)
    }
    
    Core::hresult DeviceSettingsImp::GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) {
        DELEGATE_TO_COMPONENT(_fpdSettings, GetFPDTimeFormat, fpdTimeFormat)
    }
    
    Core::hresult DeviceSettingsImp::SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) {
        DELEGATE_TO_COMPONENT(_fpdSettings, SetFPDTimeFormat, fpdTimeFormat)
    }
    
    Core::hresult DeviceSettingsImp::SetFPDMode(const FPDMode fpdMode) {
        DELEGATE_TO_COMPONENT(_fpdSettings, SetFPDMode, fpdMode)
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
        DELEGATE_TO_COMPONENT(_hdmiInSettings, Unregister, notification)
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInNumbefOfInputs(int32_t &count) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, GetHDMIInNumbefOfInputs, count)
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, GetHDMIInStatus, hdmiStatus, portConnectionStatus)
    }
    
    Core::hresult DeviceSettingsImp::SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, SelectHDMIInPort, port, requestAudioMix, topMostPlane, videoPlaneType)
    }
    
    Core::hresult DeviceSettingsImp::ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, ScaleHDMIInVideo, videoPosition)
    }
    
    Core::hresult DeviceSettingsImp::SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, SelectHDMIZoomMode, zoomMode)
    }
    
    Core::hresult DeviceSettingsImp::GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator *& gameFeatureList) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, GetSupportedGameFeaturesList, gameFeatureList)
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, GetHDMIInAVLatency, videoLatency, audioLatency)
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, GetHDMIInAllmStatus, port, allmStatus)
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, GetHDMIInEdid2AllmSupport, port, allmSupport)
    }
    
    Core::hresult DeviceSettingsImp::SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, SetHDMIInEdid2AllmSupport, port, allmSupport)
    }
    
    Core::hresult DeviceSettingsImp::GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, GetEdidBytes, port, edidBytesLength, edidBytes)
    }
    
    Core::hresult DeviceSettingsImp::GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, GetHDMISPDInformation, port, spdBytesLength, spdBytes)
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, GetHDMIEdidVersion, port, edidVersion)
    }
    
    Core::hresult DeviceSettingsImp::SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, SetHDMIEdidVersion, port, edidVersion)
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, GetHDMIVideoMode, videoPortResolution)
    }
    
    Core::hresult DeviceSettingsImp::GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, GetHDMIVersion, port, capabilityVersion)
    }
    
    Core::hresult DeviceSettingsImp::SetVRRSupport(const HDMIInPort port, const bool vrrSupport) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, SetVRRSupport, port, vrrSupport)
    }
    
    Core::hresult DeviceSettingsImp::GetVRRSupport(const HDMIInPort port, bool &vrrSupport) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, GetVRRSupport, port, vrrSupport)
    }
    
    Core::hresult DeviceSettingsImp::GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus) {
        DELEGATE_TO_COMPONENT(_hdmiInSettings, GetVRRStatus, port, vrrStatus)
    }

    // ============================================================================
    // IDeviceSettingsAudio interface implementation - delegate to _audioSettings interface
    // ============================================================================
    
    Core::hresult DeviceSettingsImp::Register(Exchange::IDeviceSettingsAudio::INotification* notification) {
        DELEGATE_TO_COMPONENT(_audioSettings, Register, notification)
    }
    
    Core::hresult DeviceSettingsImp::Unregister(Exchange::IDeviceSettingsAudio::INotification* notification) {
        DELEGATE_TO_COMPONENT(_audioSettings, Unregister, notification)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioPort(const AudioPortType type, const int32_t index, int32_t &handle) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioPort, type, index, handle)
    }
    
    // GetAudioPorts and GetSupportedAudioPorts methods removed - iterator type doesn't exist

    Core::hresult DeviceSettingsImp::GetAudioPortConfig(const AudioPortType audioPort, AudioConfig &audioConfig) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioPortConfig, audioPort, audioConfig)
    }

    Core::hresult DeviceSettingsImp::SetAudioPortConfig(const AudioPortType audioPort, const AudioConfig audioConfig) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioPortConfig, audioPort, audioConfig)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioCapabilities(const int32_t handle, int32_t &capabilities) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioCapabilities, handle, capabilities)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioMS12Capabilities(const int32_t handle, int32_t &capabilities) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioMS12Capabilities, handle, capabilities)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioFormat(const int32_t handle, AudioFormat &audioFormat) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioFormat, handle, audioFormat)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioEncoding(const int32_t handle, AudioEncoding &encoding) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioEncoding, handle, encoding)
    }
    
    Core::hresult DeviceSettingsImp::GetSupportedCompressions(const int32_t handle, IDeviceSettingsAudioCompressionIterator*& compressions) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetSupportedCompressions, handle, compressions)
    }
    
    Core::hresult DeviceSettingsImp::GetCompression(const int32_t handle, AudioCompression &compression) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetCompression, handle, compression)
    }
    
    Core::hresult DeviceSettingsImp::SetCompression(const int32_t handle, const AudioCompression compression) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetCompression, handle, compression)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioLevel(const int32_t handle, const float audioLevel) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioLevel, handle, audioLevel)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioLevel(const int32_t handle, float &audioLevel) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioLevel, handle, audioLevel)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioGain(const int32_t handle, const float gainLevel) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioGain, handle, gainLevel)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioGain(const int32_t handle, float &gainLevel) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioGain, handle, gainLevel)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioMute(const int32_t handle, const bool mute) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioMute, handle, mute)
    }
    
    Core::hresult DeviceSettingsImp::IsAudioMuted(const int32_t handle, bool &muted) {
        DELEGATE_TO_COMPONENT(_audioSettings, IsAudioMuted, handle, muted)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioDucking(const int32_t handle, const AudioDuckingType duckingType, const AudioDuckingAction duckingAction, const uint8_t level) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioDucking, handle, duckingType, duckingAction, level)
    }
    
    Core::hresult DeviceSettingsImp::GetStereoMode(const int32_t handle, AudioStereoMode &mode) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetStereoMode, handle, mode)
    }
    
    Core::hresult DeviceSettingsImp::SetStereoMode(const int32_t handle, const AudioStereoMode mode, const bool persist) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetStereoMode, handle, mode, persist)
    }
    
    Core::hresult DeviceSettingsImp::SetAssociatedAudioMixing(const int32_t handle, const bool mixing) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAssociatedAudioMixing, handle, mixing)
    }
    
    Core::hresult DeviceSettingsImp::GetAssociatedAudioMixing(const int32_t handle, bool &mixing) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAssociatedAudioMixing, handle, mixing)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioFaderControl(const int32_t handle, const int32_t mixerBalance) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioFaderControl, handle, mixerBalance)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioFaderControl(const int32_t handle, int32_t &mixerBalance) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioFaderControl, handle, mixerBalance)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioPrimaryLanguage(const int32_t handle, const string primaryAudioLanguage) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioPrimaryLanguage, handle, primaryAudioLanguage)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioPrimaryLanguage(const int32_t handle, string &primaryAudioLanguage) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioPrimaryLanguage, handle, primaryAudioLanguage)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioSecondaryLanguage(const int32_t handle, const string secondaryAudioLanguage) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioSecondaryLanguage, handle, secondaryAudioLanguage)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioSecondaryLanguage(const int32_t handle, string &secondaryAudioLanguage) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioSecondaryLanguage, handle, secondaryAudioLanguage)
    }
    
    Core::hresult DeviceSettingsImp::IsAudioOutputConnected(const int32_t handle, bool &isConnected) {
        DELEGATE_TO_COMPONENT(_audioSettings, IsAudioOutputConnected, handle, isConnected)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioSinkDeviceAtmosCapability(const int32_t handle, DolbyAtmosCapability &atmosCapability) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioSinkDeviceAtmosCapability, handle, atmosCapability)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioAtmosOutputMode(const int32_t handle, const bool enable) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioAtmosOutputMode, handle, enable)
    }

    // Missing Audio interface delegation methods
    Core::hresult DeviceSettingsImp::IsAudioPortEnabled(const int32_t handle, bool &enabled) {
        DELEGATE_TO_COMPONENT(_audioSettings, IsAudioPortEnabled, handle, enabled)
    }

    Core::hresult DeviceSettingsImp::EnableAudioPort(const int32_t handle, const bool enable) {
        DELEGATE_TO_COMPONENT(_audioSettings, EnableAudioPort, handle, enable)
    }

    Core::hresult DeviceSettingsImp::GetSupportedARCTypes(const int32_t handle, int32_t &types) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetSupportedARCTypes, handle, types)
    }

    Core::hresult DeviceSettingsImp::SetSAD(const int32_t handle, const uint8_t sadList[], const uint8_t count) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetSAD, handle, sadList, count)
    }
    
    Core::hresult DeviceSettingsImp::EnableARC(const int32_t handle, const AudioARCStatus arcStatus) {
        DELEGATE_TO_COMPONENT(_audioSettings, EnableARC, handle, arcStatus)
    }
    
    Core::hresult DeviceSettingsImp::GetStereoAuto(const int32_t handle, int32_t &mode) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetStereoAuto, handle, mode)
    }
    
    Core::hresult DeviceSettingsImp::SetStereoAuto(const int32_t handle, const int32_t mode, const bool persist) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetStereoAuto, handle, mode, persist)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioEnablePersist(const int32_t handle, bool &enabled, string &portName) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioEnablePersist, handle, enabled, portName)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioEnablePersist(const int32_t handle, const bool enable, const string portName) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioEnablePersist, handle, enable, portName)
    }
    
    Core::hresult DeviceSettingsImp::IsAudioMSDecoded(const int32_t handle, bool &hasms11Decode) {
        DELEGATE_TO_COMPONENT(_audioSettings, IsAudioMSDecoded, handle, hasms11Decode)
    }
    
    Core::hresult DeviceSettingsImp::IsAudioMS12Decoded(const int32_t handle, bool &hasms12Decode) {
        DELEGATE_TO_COMPONENT(_audioSettings, IsAudioMS12Decoded, handle, hasms12Decode)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioLEConfig(const int32_t handle, bool &enabled) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioLEConfig, handle, enabled)
    }
    
    Core::hresult DeviceSettingsImp::EnableAudioLEConfig(const int32_t handle, const bool enable) {
        DELEGATE_TO_COMPONENT(_audioSettings, EnableAudioLEConfig, handle, enable)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioDelay(const int32_t handle, const uint32_t audioDelay) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioDelay, handle, audioDelay)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioDelay(const int32_t handle, uint32_t &audioDelay) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioDelay, handle, audioDelay)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioDelayOffset(const int32_t handle, const uint32_t delayOffset) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioDelayOffset, handle, delayOffset)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioDelayOffset(const int32_t handle, uint32_t &delayOffset) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioDelayOffset, handle, delayOffset)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioCompression(const int32_t handle, const int32_t compressionLevel) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioCompression, handle, compressionLevel)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioCompression(const int32_t handle, int32_t &compressionLevel) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioCompression, handle, compressionLevel)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioDialogEnhancement(const int32_t handle, const int32_t level) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioDialogEnhancement, handle, level)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioDialogEnhancement(const int32_t handle, int32_t &level) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioDialogEnhancement, handle, level)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioDolbyVolumeMode(const int32_t handle, const bool enable) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioDolbyVolumeMode, handle, enable)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioDolbyVolumeMode(const int32_t handle, bool &enabled) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioDolbyVolumeMode, handle, enabled)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioIntelligentEqualizerMode(const int32_t handle, const int32_t mode) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioIntelligentEqualizerMode, handle, mode)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioIntelligentEqualizerMode(const int32_t handle, int32_t &mode) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioIntelligentEqualizerMode, handle, mode)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioVolumeLeveller(const int32_t handle, const VolumeLeveller volumeLeveller) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioVolumeLeveller, handle, volumeLeveller)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioVolumeLeveller(const int32_t handle, VolumeLeveller &volumeLeveller) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioVolumeLeveller, handle, volumeLeveller)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioBassEnhancer(const int32_t handle, const int32_t boost) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioBassEnhancer, handle, boost)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioBassEnhancer(const int32_t handle, int32_t &boost) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioBassEnhancer, handle, boost)
    }
    
    Core::hresult DeviceSettingsImp::EnableAudioSurroudDecoder(const int32_t handle, const bool enable) {
        DELEGATE_TO_COMPONENT(_audioSettings, EnableAudioSurroudDecoder, handle, enable)
    }
    
    Core::hresult DeviceSettingsImp::IsAudioSurroudDecoderEnabled(const int32_t handle, bool &enabled) {
        DELEGATE_TO_COMPONENT(_audioSettings, IsAudioSurroudDecoderEnabled, handle, enabled)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioDRCMode(const int32_t handle, const int32_t drcMode) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioDRCMode, handle, drcMode)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioDRCMode(const int32_t handle, int32_t &drcMode) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioDRCMode, handle, drcMode)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioSurroudVirtualizer(const int32_t handle, const SurroundVirtualizer surroundVirtualizer) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioSurroudVirtualizer, handle, surroundVirtualizer)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioSurroudVirtualizer(const int32_t handle, SurroundVirtualizer &surroundVirtualizer) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioSurroudVirtualizer, handle, surroundVirtualizer)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioMISteering(const int32_t handle, const bool enable) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioMISteering, handle, enable)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioMISteering(const int32_t handle, bool &enable) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioMISteering, handle, enable)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioGraphicEqualizerMode(const int32_t handle, const int32_t mode) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioGraphicEqualizerMode, handle, mode)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioGraphicEqualizerMode(const int32_t handle, int32_t &mode) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioGraphicEqualizerMode, handle, mode)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioMS12ProfileList(const int32_t handle, IDeviceSettingsAudioMS12AudioProfileIterator*& ms12ProfileList) const {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioMS12ProfileList, handle, ms12ProfileList)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioMS12Profile(const int32_t handle, string &profile) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioMS12Profile, handle, profile)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioMS12Profile(const int32_t handle, const string profile) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioMS12Profile, handle, profile)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioMixerLevels(const int32_t handle, const AudioInput audioInput, const int32_t volume) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioMixerLevels, handle, audioInput, volume)
    }
    
    Core::hresult DeviceSettingsImp::SetAudioMS12SettingsOverride(const int32_t handle, const string profileName, const string profileSettingsName, const string profileSettingValue, const string profileState) {
        DELEGATE_TO_COMPONENT(_audioSettings, SetAudioMS12SettingsOverride, handle, profileName, profileSettingsName, profileSettingValue, profileState)
    }
    
    Core::hresult DeviceSettingsImp::ResetAudioDialogEnhancement(const int32_t handle) {
        DELEGATE_TO_COMPONENT(_audioSettings, ResetAudioDialogEnhancement, handle)
    }
    
    Core::hresult DeviceSettingsImp::ResetAudioBassEnhancer(const int32_t handle) {
        DELEGATE_TO_COMPONENT(_audioSettings, ResetAudioBassEnhancer, handle)
    }
    
    Core::hresult DeviceSettingsImp::ResetAudioSurroundVirtualizer(const int32_t handle) {
        DELEGATE_TO_COMPONENT(_audioSettings, ResetAudioSurroundVirtualizer, handle)
    }
    
    Core::hresult DeviceSettingsImp::ResetAudioVolumeLeveller(const int32_t handle) {
        DELEGATE_TO_COMPONENT(_audioSettings, ResetAudioVolumeLeveller, handle)
    }
    
    Core::hresult DeviceSettingsImp::GetAudioHDMIARCPortId(const int32_t handle, int32_t &portId) {
        DELEGATE_TO_COMPONENT(_audioSettings, GetAudioHDMIARCPortId, handle, portId)
    }

} // namespace Plugin
} // namespace WPEFramework
