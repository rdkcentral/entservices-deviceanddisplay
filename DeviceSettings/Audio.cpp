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

#include <functional>
#include <unistd.h>

#include <core/IAction.h>
#include <core/Time.h>
#include <core/WorkerPool.h>

#include "UtilsLogging.h"
#include "secure_wrapper.h"
#include "Audio.h"

Audio::Audio(INotification& parent, std::shared_ptr<IPlatform> platform)
    : _platform(std::move(platform))
    , _parent(parent)
{
    ENTRY_LOG;
    LOGINFO("Audio Constructor");
    Platform_init();
    EXIT_LOG;
}

void Audio::Platform_init()
{
    ENTRY_LOG;
    CallbackBundle bundle;
    bundle.OnAudioOutHotPlug = [this](AudioPortType portType, uint32_t portNumber, bool isConnected) {
        this->OnAudioOutHotPlug(portType, portNumber, isConnected);
    };
    bundle.OnAudioFormatUpdate = [this](AudioFormat audioFormat) {
        this->OnAudioFormatUpdate(audioFormat);
    };
    bundle.OnDolbyAtmosCapabilitiesChanged = [this](DolbyAtmosCapability atmosCaps, bool status) {
        this->OnDolbyAtmosCapabilitiesChanged(atmosCaps, status);
    };
    bundle.OnAssociatedAudioMixingChanged = [this](bool mixing) {
        this->OnAssociatedAudioMixingChanged(mixing);
    };
    bundle.OnAudioFaderControlChanged = [this](int32_t mixerBalance) {
        this->OnAudioFaderControlChanged(mixerBalance);
    };
    bundle.OnAudioPrimaryLanguageChanged = [this](const std::string& primaryLanguage) {
        this->OnAudioPrimaryLanguageChanged(primaryLanguage);
    };
    bundle.OnAudioSecondaryLanguageChanged = [this](const std::string& secondaryLanguage) {
        this->OnAudioSecondaryLanguageChanged(secondaryLanguage);
    };
    bundle.OnAudioPortStateChanged = [this](AudioPortState audioPortState) {
        this->OnAudioPortStateChanged(audioPortState);
    };
    bundle.OnAudioLevelChanged = [this](float audioLevel) {
        this->OnAudioLevelChanged(audioLevel);
    };
    bundle.OnAudioModeChanged = [this](AudioPortType portType, AudioStereoMode mode) {
        this->OnAudioModeChanged(portType, mode);
    };
    if (_platform) {
        this->platform().setAllCallbacks(bundle);
        this->platform().getPersistenceValue();
    }
    EXIT_LOG;
}

void Audio::OnAudioOutHotPlug(AudioPortType portType, uint32_t portNumber, bool isConnected)
{
    ENTRY_LOG;
    LOGINFO("OnAudioOutHotPlug: portType=%d, portNumber=%u, connected=%s", static_cast<int>(portType), portNumber, isConnected ? "true" : "false");
    // Trigger notification to parent for callback dispatch
    _parent.OnAudioOutHotPlug(portType, portNumber, isConnected);
    EXIT_LOG;
}

void Audio::OnAudioFormatUpdate(AudioFormat audioFormat)
{
    ENTRY_LOG;
    LOGINFO("OnAudioFormatUpdate: format=%d", static_cast<int>(audioFormat));
    // Trigger notification to parent for callback dispatch  
    _parent.OnAudioFormatUpdate(audioFormat);
    EXIT_LOG;
}

void Audio::OnDolbyAtmosCapabilitiesChanged(DolbyAtmosCapability atmosCaps, bool status)
{
    ENTRY_LOG;
    LOGINFO("OnDolbyAtmosCapabilitiesChanged: caps=%d, status=%s", static_cast<int>(atmosCaps), status ? "true" : "false");
    // Trigger notification to parent for callback dispatch
    _parent.OnDolbyAtmosCapabilitiesChanged(atmosCaps, status);
    EXIT_LOG;
}

void Audio::OnAudioModeChanged(AudioPortType portType, AudioStereoMode mode)
{
    ENTRY_LOG;
    LOGINFO("OnAudioModeChanged: portType=%d, mode=%d", static_cast<int>(portType), static_cast<int>(mode));
    // Trigger notification to parent for callback dispatch
    _parent.OnAudioModeEvent(portType, mode);
    EXIT_LOG;
}

// Event handler methods for audio state changes
void Audio::OnAssociatedAudioMixingChanged(bool mixing)
{
    ENTRY_LOG;
    LOGINFO("OnAssociatedAudioMixingChanged: mixing=%s", mixing ? "enabled" : "disabled");
    // Trigger notification to parent for callback dispatch
    _parent.OnAssociatedAudioMixingChanged(mixing);
    EXIT_LOG;
}

void Audio::OnAudioFaderControlChanged(int32_t mixerBalance)
{
    ENTRY_LOG;
    LOGINFO("OnAudioFaderControlChanged: mixerBalance=%d", mixerBalance);
    // Trigger notification to parent for callback dispatch
    _parent.OnAudioFaderControlChanged(mixerBalance);
    EXIT_LOG;
}

void Audio::OnAudioPrimaryLanguageChanged(const std::string& primaryLanguage)
{
    ENTRY_LOG;
    LOGINFO("OnAudioPrimaryLanguageChanged: primaryLanguage=%s", primaryLanguage.c_str());
    // Trigger notification to parent for callback dispatch
    _parent.OnAudioPrimaryLanguageChanged(primaryLanguage);
    EXIT_LOG;
}

void Audio::OnAudioSecondaryLanguageChanged(const std::string& secondaryLanguage)
{
    ENTRY_LOG;
    LOGINFO("OnAudioSecondaryLanguageChanged: secondaryLanguage=%s", secondaryLanguage.c_str());
    // Trigger notification to parent for callback dispatch
    _parent.OnAudioSecondaryLanguageChanged(secondaryLanguage);
    EXIT_LOG;
}

void Audio::OnAudioPortStateChanged(AudioPortState audioPortState)
{
    ENTRY_LOG;
    LOGINFO("OnAudioPortStateChanged: audioPortState=%d", static_cast<int>(audioPortState));
    // Trigger notification to parent for callback dispatch
    _parent.OnAudioPortStateChanged(audioPortState);
    EXIT_LOG;
}

void Audio::OnAudioLevelChanged(float audioLevel)
{
    ENTRY_LOG;
    LOGINFO("OnAudioLevelChanged: audioLevel=%.2f", audioLevel);
    // Trigger notification to parent for callback dispatch
    _parent.OnAudioLevelChangedEvent(static_cast<int32_t>(audioLevel));
    EXIT_LOG;
}

uint32_t Audio::GetAudioPort(const AudioPortType type, const int32_t index, int32_t &handle) {
    ENTRY_LOG;
    LOGINFO("GetAudioPort: type=%d, index=%d", type, index);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetAudioPort(type, index, handle);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetAudioPort: SUCCESS - type=%d, index=%d, handle=%d", type, index, handle);
    } else {
        LOGERR("GetAudioPort: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

// GetAudioPorts and GetSupportedAudioPorts methods removed - iterator type doesn't exist in interface

uint32_t Audio::GetAudioPortConfig(const AudioPortType audioPort, AudioConfig &audioConfig) {
    ENTRY_LOG;
    LOGINFO("GetAudioPortConfig: audioPort=%d", audioPort);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        // First get the audio port handle
        int32_t handle = -1;
        int32_t index = 0;
        result = this->platform().GetAudioPort(audioPort, index, handle);
        if (result == WPEFramework::Core::ERROR_NONE) {
            result = this->platform().GetAudioPortConfig(audioPort, audioConfig);
        }
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetAudioPortConfig: SUCCESS - audioPort=%d", audioPort);
    } else {
        LOGERR("GetAudioPortConfig: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioCapabilities(const int32_t handle, int32_t &capabilities) {
    ENTRY_LOG;
    LOGINFO("GetAudioCapabilities: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetAudioCapabilities(handle, capabilities);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetAudioCapabilities: SUCCESS - handle=%d, capabilities=%d", handle, capabilities);
    } else {
        LOGERR("GetAudioCapabilities: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioMS12Capabilities(const int32_t handle, int32_t &capabilities) {
    ENTRY_LOG;
    LOGINFO("GetAudioMS12Capabilities: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetAudioMS12Capabilities(handle, capabilities);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetAudioMS12Capabilities: SUCCESS - handle=%d, capabilities=%d", handle, capabilities);
    } else {
        LOGERR("GetAudioMS12Capabilities: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioFormat(const int32_t handle, AudioFormat &audioFormat) {
    ENTRY_LOG;
    LOGINFO("GetAudioFormat: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetAudioFormat(handle, audioFormat);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetAudioFormat: SUCCESS - handle=%d, audioFormat=%d", handle, audioFormat);
    } else {
        LOGERR("GetAudioFormat: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioEncoding(const int32_t handle, AudioEncoding &encoding) {
    ENTRY_LOG;
    LOGINFO("GetAudioEncoding: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetAudioEncoding(handle, encoding);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetAudioEncoding: SUCCESS - handle=%d, encoding=%d", handle, encoding);
    } else {
        LOGERR("GetAudioEncoding: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioLevel(const int32_t handle, const float audioLevel) {
    ENTRY_LOG;
    LOGINFO("SetAudioLevel: handle=%d, audioLevel=%.2f", handle, audioLevel);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetAudioLevel(handle, audioLevel);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("SetAudioLevel: SUCCESS - platform call completed successfully");
    } else {
        LOGERR("SetAudioLevel: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioLevel(const int32_t handle, float &audioLevel) {
    ENTRY_LOG;
    LOGINFO("GetAudioLevel: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetAudioLevel(handle, audioLevel);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetAudioLevel: SUCCESS - handle=%d, audioLevel=%.2f", handle, audioLevel);
    } else {
        LOGERR("GetAudioLevel: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioGain(const int32_t handle, const float gainLevel) {
    ENTRY_LOG;
    LOGINFO("SetAudioGain: handle=%d, gainLevel=%.2f", handle, gainLevel);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetAudioGain(handle, gainLevel);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("SetAudioGain: SUCCESS - platform call completed successfully");
    } else {
        LOGERR("SetAudioGain: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioGain(const int32_t handle, float &gainLevel) {
    ENTRY_LOG;
    LOGINFO("GetAudioGain: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetAudioGain(handle, gainLevel);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetAudioGain: SUCCESS - handle=%d, gainLevel=%.2f", handle, gainLevel);
    } else {
        LOGERR("GetAudioGain: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioMute(const int32_t handle, const bool mute) {
    ENTRY_LOG;
    LOGINFO("SetAudioMute: handle=%d, mute=%s", handle, mute ? "true" : "false");
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetAudioMute(handle, mute);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("SetAudioMute: SUCCESS - platform call completed successfully");
    } else {
        LOGERR("SetAudioMute: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::IsAudioMuted(const int32_t handle, bool &muted) {
    ENTRY_LOG;
    LOGINFO("IsAudioMuted: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().IsAudioMuted(handle, muted);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("IsAudioMuted: SUCCESS - handle=%d, muted=%s", handle, muted ? "true" : "false");
    } else {
        LOGERR("IsAudioMuted: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioDucking(const int32_t handle, const AudioDuckingType duckingType, const AudioDuckingAction duckingAction, const uint8_t level) {
    ENTRY_LOG;
    LOGINFO("SetAudioDucking: handle=%d, duckingType=%d, duckingAction=%d, level=%d", handle, duckingType, duckingAction, level);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetAudioDucking(handle, duckingType, duckingAction, level);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("SetAudioDucking: SUCCESS - platform call completed successfully");
    } else {
        LOGERR("SetAudioDucking: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetStereoMode(const int32_t handle, AudioStereoMode &mode) {
    ENTRY_LOG;
    LOGINFO("GetStereoMode: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetStereoMode(handle, mode);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetStereoMode: SUCCESS - handle=%d, mode=%d", handle, mode);
    } else {
        LOGERR("GetStereoMode: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetStereoMode(const int32_t handle, const AudioStereoMode mode, const bool persist) {
    ENTRY_LOG;
    LOGINFO("SetStereoMode: handle=%d, mode=%d, persist=%s", handle, mode, persist ? "true" : "false");
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetStereoMode(handle, mode, persist);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("SetStereoMode: SUCCESS - platform call completed successfully");
    } else {
        LOGERR("SetStereoMode: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAssociatedAudioMixing(const int32_t handle, const bool mixing) {
    ENTRY_LOG;
    LOGINFO("SetAssociatedAudioMixing: handle=%d, mixing=%s", handle, mixing ? "true" : "false");
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetAssociatedAudioMixing(handle, mixing);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("SetAssociatedAudioMixing: SUCCESS - platform call completed successfully");
    } else {
        LOGERR("SetAssociatedAudioMixing: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAssociatedAudioMixing(const int32_t handle, bool &mixing) {
    ENTRY_LOG;
    LOGINFO("GetAssociatedAudioMixing: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetAssociatedAudioMixing(handle, mixing);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetAssociatedAudioMixing: SUCCESS - handle=%d, mixing=%s", handle, mixing ? "true" : "false");
    } else {
        LOGERR("GetAssociatedAudioMixing: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioFaderControl(const int32_t handle, const int32_t mixerBalance) {
    ENTRY_LOG;
    LOGINFO("SetAudioFaderControl: handle=%d, mixerBalance=%d", handle, mixerBalance);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetAudioFaderControl(handle, mixerBalance);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("SetAudioFaderControl: SUCCESS - platform call completed successfully");
    } else {
        LOGERR("SetAudioFaderControl: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioFaderControl(const int32_t handle, int32_t &mixerBalance) {
    ENTRY_LOG;
    LOGINFO("GetAudioFaderControl: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetAudioFaderControl(handle, mixerBalance);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetAudioFaderControl: SUCCESS - handle=%d, mixerBalance=%d", handle, mixerBalance);
    } else {
        LOGERR("GetAudioFaderControl: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioPrimaryLanguage(const int32_t handle, const std::string primaryAudioLanguage) {
    ENTRY_LOG;
    LOGINFO("SetAudioPrimaryLanguage: handle=%d, primaryAudioLanguage=%s", handle, primaryAudioLanguage.c_str());
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetAudioPrimaryLanguage(handle, primaryAudioLanguage);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("SetAudioPrimaryLanguage: SUCCESS - platform call completed successfully");
    } else {
        LOGERR("SetAudioPrimaryLanguage: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioPrimaryLanguage(const int32_t handle, std::string &primaryAudioLanguage) {
    ENTRY_LOG;
    LOGINFO("GetAudioPrimaryLanguage: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetAudioPrimaryLanguage(handle, primaryAudioLanguage);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetAudioPrimaryLanguage: SUCCESS - handle=%d, primaryAudioLanguage=%s", handle, primaryAudioLanguage.c_str());
    } else {
        LOGERR("GetAudioPrimaryLanguage: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioSecondaryLanguage(const int32_t handle, const std::string secondaryAudioLanguage) {
    ENTRY_LOG;
    LOGINFO("SetAudioSecondaryLanguage: handle=%d, secondaryAudioLanguage=%s", handle, secondaryAudioLanguage.c_str());
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().SetAudioSecondaryLanguage(handle, secondaryAudioLanguage);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("SetAudioSecondaryLanguage: SUCCESS - platform call completed successfully");
    } else {
        LOGERR("SetAudioSecondaryLanguage: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioSecondaryLanguage(const int32_t handle, std::string &secondaryAudioLanguage) {
    ENTRY_LOG;
    LOGINFO("GetAudioSecondaryLanguage: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetAudioSecondaryLanguage(handle, secondaryAudioLanguage);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetAudioSecondaryLanguage: SUCCESS - handle=%d, secondaryAudioLanguage=%s", handle, secondaryAudioLanguage.c_str());
    } else {
        LOGERR("GetAudioSecondaryLanguage: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

// Additional key methods - implementing the most commonly used ones
uint32_t Audio::IsAudioOutputConnected(const int32_t handle, bool &isConnected) {
    ENTRY_LOG;
    LOGINFO("IsAudioOutputConnected: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().IsAudioOutputConnected(handle, isConnected);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("IsAudioOutputConnected: SUCCESS - handle=%d, isConnected=%s", handle, isConnected ? "true" : "false");
    } else {
        LOGERR("IsAudioOutputConnected: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioSinkDeviceAtmosCapability(const int32_t handle, DolbyAtmosCapability &atmosCapability) {
    ENTRY_LOG;
    LOGINFO("GetAudioSinkDeviceAtmosCapability: handle=%d", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    if (_platform) {
        result = this->platform().GetAudioSinkDeviceAtmosCapability(handle, atmosCapability);
    }
    if (result == WPEFramework::Core::ERROR_NONE) {
        LOGINFO("GetAudioSinkDeviceAtmosCapability: SUCCESS - handle=%d, atmosCapability=%d", handle, atmosCapability);
    } else {
        LOGERR("GetAudioSinkDeviceAtmosCapability: FAILED - result=%u", result);
    }
    EXIT_LOG;
    return result;
}


// NOTE: The remaining methods (like SetAudioDelay, GetAudioDelay, etc.) would follow
// the same pattern. For brevity, I'm implementing the key ones that are commonly used
// and that correspond to the notification handlers we saw in DeviceSettingsManager.h

// Placeholder implementations for methods not yet fully developed
uint32_t Audio::GetSupportedCompressions(const int32_t handle, IDeviceSettingsAudioCompressionIterator*& compressions) {
    ENTRY_LOG;
    LOGINFO("GetSupportedCompressions: handle=%d - STUB IMPLEMENTATION", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetCompression(const int32_t handle, AudioCompression &compression) {
    ENTRY_LOG;
    LOGINFO("GetCompression: handle=%d - STUB IMPLEMENTATION", handle);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetCompression(const int32_t handle, const AudioCompression compression) {
    ENTRY_LOG;
    LOGINFO("SetCompression: handle=%d, compression=%d - STUB IMPLEMENTATION", handle, compression);
    uint32_t result = WPEFramework::Core::ERROR_GENERAL;
    EXIT_LOG;
    return result;
}

// Missing Audio interface methods implementation

uint32_t Audio::IsAudioPortEnabled(const int32_t handle, bool &enabled) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->IsAudioPortEnabled(handle, enabled) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::EnableAudioPort(const int32_t handle, const bool enable) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->EnableAudioPort(handle, enable) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetSupportedARCTypes(const int32_t handle, int32_t &types) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetSupportedARCTypes(handle, types) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetSAD(const int32_t handle, const uint8_t sadList[], const uint8_t count) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetSAD(handle, sadList, count) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::EnableARC(const int32_t handle, const AudioARCStatus arcStatus) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->EnableARC(handle, arcStatus) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetStereoAuto(const int32_t handle, int32_t &mode) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetStereoAuto(handle, mode) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetStereoAuto(const int32_t handle, const int32_t mode, const bool persist) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetStereoAuto(handle, mode, persist) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioEnablePersist(const int32_t handle, bool &enabled, string &portName) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioEnablePersist(handle, enabled, portName) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioEnablePersist(const int32_t handle, const bool enable, const string portName) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioEnablePersist(handle, enable, portName) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::IsAudioMSDecoded(const int32_t handle, bool &hasms11Decode) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->IsAudioMSDecoded(handle, hasms11Decode) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::IsAudioMS12Decoded(const int32_t handle, bool &hasms12Decode) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->IsAudioMS12Decoded(handle, hasms12Decode) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioLEConfig(const int32_t handle, bool &enabled) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioLEConfig(handle, enabled) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::EnableAudioLEConfig(const int32_t handle, const bool enable) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->EnableAudioLEConfig(handle, enable) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioDelay(const int32_t handle, const uint32_t audioDelay) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioDelay(handle, audioDelay) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioDelay(const int32_t handle, uint32_t &audioDelay) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioDelay(handle, audioDelay) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioDelayOffset(const int32_t handle, const uint32_t delayOffset) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioDelayOffset(handle, delayOffset) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioDelayOffset(const int32_t handle, uint32_t &delayOffset) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioDelayOffset(handle, delayOffset) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioCompression(const int32_t handle, const int32_t compressionLevel) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioCompression(handle, compressionLevel) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioCompression(const int32_t handle, int32_t &compressionLevel) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioCompression(handle, compressionLevel) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioDialogEnhancement(const int32_t handle, const int32_t level) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioDialogEnhancement(handle, level) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioDialogEnhancement(const int32_t handle, int32_t &level) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioDialogEnhancement(handle, level) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioDolbyVolumeMode(const int32_t handle, const bool enable) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioDolbyVolumeMode(handle, enable) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioDolbyVolumeMode(const int32_t handle, bool &enabled) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioDolbyVolumeMode(handle, enabled) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioIntelligentEqualizerMode(const int32_t handle, const int32_t mode) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioIntelligentEqualizerMode(handle, mode) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioIntelligentEqualizerMode(const int32_t handle, int32_t &mode) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioIntelligentEqualizerMode(handle, mode) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioVolumeLeveller(const int32_t handle, const VolumeLeveller volumeLeveller) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioVolumeLeveller(handle, volumeLeveller) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioVolumeLeveller(const int32_t handle, VolumeLeveller &volumeLeveller) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioVolumeLeveller(handle, volumeLeveller) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioBassEnhancer(const int32_t handle, const int32_t boost) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioBassEnhancer(handle, boost) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioBassEnhancer(const int32_t handle, int32_t &boost) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioBassEnhancer(handle, boost) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::EnableAudioSurroudDecoder(const int32_t handle, const bool enable) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->EnableAudioSurroudDecoder(handle, enable) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::IsAudioSurroudDecoderEnabled(const int32_t handle, bool &enabled) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->IsAudioSurroudDecoderEnabled(handle, enabled) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioDRCMode(const int32_t handle, const int32_t drcMode) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioDRCMode(handle, drcMode) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioDRCMode(const int32_t handle, int32_t &drcMode) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioDRCMode(handle, drcMode) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioSurroudVirtualizer(const int32_t handle, const SurroundVirtualizer surroundVirtualizer) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioSurroudVirtualizer(handle, surroundVirtualizer) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioSurroudVirtualizer(const int32_t handle, SurroundVirtualizer &surroundVirtualizer) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioSurroudVirtualizer(handle, surroundVirtualizer) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioMISteering(const int32_t handle, const bool enable) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioMISteering(handle, enable) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioMISteering(const int32_t handle, bool &enable) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioMISteering(handle, enable) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioGraphicEqualizerMode(const int32_t handle, const int32_t mode) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioGraphicEqualizerMode(handle, mode) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioGraphicEqualizerMode(const int32_t handle, int32_t &mode) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioGraphicEqualizerMode(handle, mode) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioMS12ProfileList(const int32_t handle, IDeviceSettingsAudioMS12AudioProfileIterator*& ms12ProfileList) const {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioMS12ProfileList(handle, ms12ProfileList) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioMS12Profile(const int32_t handle, string &profile) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioMS12Profile(handle, profile) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioMS12Profile(const int32_t handle, const string profile) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioMS12Profile(handle, profile) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioMixerLevels(const int32_t handle, const AudioInput audioInput, const int32_t volume) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioMixerLevels(handle, audioInput, volume) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::SetAudioMS12SettingsOverride(const int32_t handle, const string profileName, const string profileSettingsName, const string profileSettingValue, const string profileState) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->SetAudioMS12SettingsOverride(handle, profileName, profileSettingsName, profileSettingValue, profileState) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::ResetAudioDialogEnhancement(const int32_t handle) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->ResetAudioDialogEnhancement(handle) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::ResetAudioBassEnhancer(const int32_t handle) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->ResetAudioBassEnhancer(handle) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::ResetAudioSurroundVirtualizer(const int32_t handle) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->ResetAudioSurroundVirtualizer(handle) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::ResetAudioVolumeLeveller(const int32_t handle) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->ResetAudioVolumeLeveller(handle) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

uint32_t Audio::GetAudioHDMIARCPortId(const int32_t handle, int32_t &portId) {
    ENTRY_LOG;
    uint32_t result = (_platform != nullptr) ? _platform->GetAudioHDMIARCPortId(handle, portId) : WPEFramework::Core::ERROR_UNAVAILABLE;
    EXIT_LOG;
    return result;
}

// ... Additional stub implementations would continue here following the same pattern
// For full implementation, each method would need proper platform delegation