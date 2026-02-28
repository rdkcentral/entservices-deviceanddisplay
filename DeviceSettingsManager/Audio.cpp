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

#include "UtilsLogging.h"

#include "Audio.h"
#include "DeviceSettingsManagerTypes.h"

using IPlatform = hal::dAudio::IPlatform;
using DefaultImpl = dAudioImpl;

#include "hal/dAudio.h"
namespace hal {
namespace dAudio {
    IPlatform::~IPlatform() {}
}
}

Audio::Audio(INotification& parent, std::shared_ptr<IPlatform> platform)
    : _platform(std::move(platform))
    , _parent(parent)
{
    Platform_init();
}

void Audio::Platform_init()
{
    CallbackBundle bundle;
    bundle.OnAssociatedAudioMixingChangedEvent = [this](bool mixing) {
        this->OnAssociatedAudioMixingChangedEvent(mixing);
    };
    bundle.OnAudioFaderControlChangedEvent = [this](int32_t mixerBalance) {
        this->OnAudioFaderControlChangedEvent(mixerBalance);
    };
    bundle.OnAudioPrimaryLanguageChangedEvent = [this](const string& primaryLanguage) {
        this->OnAudioPrimaryLanguageChangedEvent(primaryLanguage);
    };
    bundle.OnAudioSecondaryLanguageChangedEvent = [this](const string& secondaryLanguage) {
        this->OnAudioSecondaryLanguageChangedEvent(secondaryLanguage);
    };
    bundle.OnAudioOutHotPlugEvent = [this](AudioPortType portType, uint32_t uiPortNumber, bool isPortConnected) {
        this->OnAudioOutHotPlugEvent(portType, uiPortNumber, isPortConnected);
    };
    bundle.OnAudioFormatUpdateEvent = [this](AudioFormat audioFormat) {
        this->OnAudioFormatUpdateEvent(audioFormat);
    };
    bundle.OnDolbyAtmosCapabilitiesChangedEvent = [this](DolbyAtmosCapability atmosCapability, bool status) {
        this->OnDolbyAtmosCapabilitiesChangedEvent(atmosCapability, status);
    };
    bundle.OnAudioPortStateChangedEvent = [this](AudioPortState audioPortState) {
        this->OnAudioPortStateChangedEvent(audioPortState);
    };
    bundle.OnAudioModeEvent = [this](AudioPortType audioPortType, StereoModes audioMode) {
        this->OnAudioModeEvent(audioPortType, audioMode);
    };
    bundle.OnAudioLevelChangedEvent = [this](int32_t audioLevel) {
        this->OnAudioLevelChangedEvent(audioLevel);
    };
    if (_platform) {
        this->platform().setAllCallbacks(bundle);
        this->platform().getPersistenceValue();
    }
}

void Audio::OnAssociatedAudioMixingChangedEvent(bool mixing)
{
    _parent.OnAssociatedAudioMixingChangedNotification(mixing);
}

void Audio::OnAudioFaderControlChangedEvent(int32_t mixerBalance)
{
    _parent.OnAudioFaderControlChangedNotification(mixerBalance);
}

void Audio::OnAudioPrimaryLanguageChangedEvent(const string& primaryLanguage)
{
    _parent.OnAudioPrimaryLanguageChangedNotification(primaryLanguage);
}

void Audio::OnAudioSecondaryLanguageChangedEvent(const string& secondaryLanguage)
{
    _parent.OnAudioSecondaryLanguageChangedNotification(secondaryLanguage);
}

void Audio::OnAudioOutHotPlugEvent(AudioPortType portType, uint32_t uiPortNumber, bool isPortConnected)
{
    _parent.OnAudioOutHotPlugNotification(portType, uiPortNumber, isPortConnected);
}

void Audio::OnAudioFormatUpdateEvent(AudioFormat audioFormat)
{
    _parent.OnAudioFormatUpdateNotification(audioFormat);
}

void Audio::OnDolbyAtmosCapabilitiesChangedEvent(DolbyAtmosCapability atmosCapability, bool status)
{
    _parent.OnDolbyAtmosCapabilitiesChangedNotification(atmosCapability, status);
}

void Audio::OnAudioPortStateChangedEvent(AudioPortState audioPortState)
{
    _parent.OnAudioPortStateChangedNotification(audioPortState);
}

void Audio::OnAudioModeEvent(AudioPortType audioPortType, StereoModes audioMode)
{
    _parent.OnAudioModeEventNotification(audioPortType, audioMode);
}

void Audio::OnAudioLevelChangedEvent(int32_t audioLevel)
{
    _parent.OnAudioLevelChangedEventNotification(audioLevel);
}

uint32_t Audio::GetAudioPort(const AudioPortType type, const int32_t index, int32_t &handle) {
    ENTRY_LOG;

    LOGINFO("GetAudioPort: type=%d, index=%d", type, index);
    this->platform().GetAudioPort(type, index, handle);

    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::IsAudioPortEnabled(const int32_t handle, bool &enabled)
{
    ENTRY_LOG;

    LOGINFO("IsAudioPortEnabled: handle=%d", handle);
    this->platform().IsAudioPortEnabled(handle, enabled);

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::EnableAudioPort(const int32_t handle, const bool enable)
{
    ENTRY_LOG;

    LOGINFO("EnableAudioPort: handle=%d, enable=%s", handle, enable ? "true" : "false");
    this->platform().EnableAudioPort(handle, enable);

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetSupportedARCTypes(const int32_t handle, int32_t &types)
{
    ENTRY_LOG;

    LOGINFO("GetSupportedARCTypes: handle=%d", handle);
    this->platform().GetSupportedARCTypes(handle, types);

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetSAD(const int32_t handle, const uint8_t sadList[], const uint8_t count)
{
    ENTRY_LOG;

    LOGINFO("SetSAD: handle=%d, count=%u", handle, count);
    this->platform().SetSAD(handle, sadList, count);

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::EnableARC(const int32_t handle, const AudioARCStatus arcStatus)
{
    ENTRY_LOG;

    LOGINFO("EnableARC: handle=%d, arcType=%d, status=%d", handle, static_cast<int>(arcStatus.arcType), static_cast<int>(arcStatus.status));
    this->platform().EnableARC(handle, arcStatus);

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetStereoMode(const int32_t handle, StereoModes &mode)
{
    ENTRY_LOG;
    LOGINFO("GetStereoMode: handle=%d", handle);
    this->platform().GetStereoMode(handle, mode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetStereoMode(const int32_t handle, const StereoModes mode, const bool persist)
{
    ENTRY_LOG;
    LOGINFO("SetStereoMode: handle=%d, mode=%d, persist=%s", handle, static_cast<int>(mode), persist ? "true" : "false");
    this->platform().SetStereoMode(handle, mode, persist);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetStereoAuto(const int32_t handle, int32_t &autoMode)
{
    ENTRY_LOG;
    LOGINFO("GetStereoAuto: handle=%d", handle);
    this->platform().GetStereoAuto(handle, autoMode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetStereoAuto(const int32_t handle, const int32_t autoMode, const bool persist)
{
    ENTRY_LOG;
    LOGINFO("SetStereoAuto: handle=%d, autoMode=%d, persist=%s", handle, autoMode, persist ? "true" : "false");
    this->platform().SetStereoAuto(handle, autoMode, persist);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetAudioMute(const int32_t handle, const bool mute)
{
    ENTRY_LOG;
    LOGINFO("SetAudioMute: handle=%d, mute=%s", handle, mute ? "true" : "false");
    this->platform().SetAudioMute(handle, mute);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::IsAudioMuted(const int32_t handle, bool &muted)
{
    ENTRY_LOG;
    LOGINFO("IsAudioMuted: handle=%d", handle);
    this->platform().IsAudioMuted(handle, muted);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetAudioDucking(const int32_t handle, const AudioDuckingType duckingType, const AudioDuckingAction duckingAction, const uint8_t level)
{
    ENTRY_LOG;
    LOGINFO("SetAudioDucking: handle=%d, duckingType=%d, duckingAction=%d, level=%d", handle, duckingType, duckingAction, level);
    this->platform().SetAudioDucking(handle, duckingType, duckingAction, level);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetAudioLevel(const int32_t handle, const float audioLevel)
{
    ENTRY_LOG;
    LOGINFO("SetAudioLevel: handle=%d, audioLevel=%.2f", handle, audioLevel);
    this->platform().SetAudioLevel(handle, audioLevel);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetAudioLevel(const int32_t handle, float &audioLevel)
{
    ENTRY_LOG;
    LOGINFO("GetAudioLevel: handle=%d", handle);
    this->platform().GetAudioLevel(handle, audioLevel);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetAudioGain(const int32_t handle, const float gainLevel)
{
    ENTRY_LOG;
    LOGINFO("SetAudioGain: handle=%d, gainLevel=%.2f", handle, gainLevel);
    this->platform().SetAudioGain(handle, gainLevel);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetAudioGain(const int32_t handle, float &gainLevel)
{
    ENTRY_LOG;
    LOGINFO("GetAudioGain: handle=%d", handle);
    this->platform().GetAudioGain(handle, gainLevel);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetAudioFormat(const int32_t handle, AudioFormat &audioFormat)
{
    ENTRY_LOG;
    LOGINFO("GetAudioFormat: handle=%d", handle);
    this->platform().GetAudioFormat(handle, audioFormat);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetAudioEncoding(const int32_t handle, AudioEncoding &audioEncoding)
{
    ENTRY_LOG;
    LOGINFO("GetAudioEncoding: handle=%d", handle);
    this->platform().GetAudioEncoding(handle, audioEncoding);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::IsAudioMSDecoding(const int32_t handle, bool &hasMS11Decode)
{
    ENTRY_LOG;
    LOGINFO("IsAudioMSDecoding: handle=%d", handle);
    this->platform().IsAudioMSDecoding(handle, hasMS11Decode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::IsAudioMS12Decoding(const int32_t handle, bool &hasMS12Decode)
{
    ENTRY_LOG;
    LOGINFO("IsAudioMS12Decoding: handle=%d", handle);
    this->platform().IsAudioMS12Decoding(handle, hasMS12Decode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetAudioCompression(const int32_t handle, const AudioCompression compressionMode)
{
    ENTRY_LOG;
    LOGINFO("SetAudioCompression: handle=%d, compressionMode=%d", handle, compressionMode);
    this->platform().SetAudioCompression(handle, compressionMode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetAudioCompression(const int32_t handle, AudioCompression &compressionMode)
{
    ENTRY_LOG;
    LOGINFO("GetAudioCompression: handle=%d", handle);
    this->platform().GetAudioCompression(handle, compressionMode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetDialogEnhancement(const int32_t handle, const int32_t enhancerLevel)
{
    ENTRY_LOG;
    LOGINFO("SetDialogEnhancement: handle=%d, enhancerLevel=%d", handle, enhancerLevel);
    this->platform().SetDialogEnhancement(handle, enhancerLevel);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetDialogEnhancement(const int32_t handle, int32_t &enhancerLevel)
{
    ENTRY_LOG;
    LOGINFO("GetDialogEnhancement: handle=%d", handle);
    this->platform().GetDialogEnhancement(handle, enhancerLevel);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetDolbyVolumeMode(const int32_t handle, const bool dolbyVolumeMode)
{
    ENTRY_LOG;
    LOGINFO("SetDolbyVolumeMode: handle=%d, dolbyVolumeMode=%s", handle, dolbyVolumeMode ? "true" : "false");
    this->platform().SetDolbyVolumeMode(handle, dolbyVolumeMode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetDolbyVolumeMode(const int32_t handle, bool &dolbyVolumeMode)
{
    ENTRY_LOG;
    LOGINFO("GetDolbyVolumeMode: handle=%d", handle);
    this->platform().GetDolbyVolumeMode(handle, dolbyVolumeMode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetIntelligentEqualizerMode(const int32_t handle, const int32_t equalizerMode)
{
    ENTRY_LOG;
    LOGINFO("SetIntelligentEqualizerMode: handle=%d, equalizerMode=%d", handle, equalizerMode);
    this->platform().SetIntelligentEqualizerMode(handle, equalizerMode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetIntelligentEqualizerMode(const int32_t handle, int32_t &equalizerMode)
{
    ENTRY_LOG;
    LOGINFO("GetIntelligentEqualizerMode: handle=%d", handle);
    this->platform().GetIntelligentEqualizerMode(handle, equalizerMode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetGraphicEqualizerMode(const int32_t handle, const int32_t equalizerMode, const int32_t inputGain, const int32_t outputGain)
{
    ENTRY_LOG;
    LOGINFO("SetGraphicEqualizerMode: handle=%d, equalizerMode=%d, inputGain=%d, outputGain=%d", handle, equalizerMode, inputGain, outputGain);
    this->platform().SetGraphicEqualizerMode(handle, equalizerMode, inputGain, outputGain);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetGraphicEqualizerMode(const int32_t handle, int32_t &equalizerMode, int32_t &inputGain, int32_t &outputGain)
{
    ENTRY_LOG;
    LOGINFO("GetGraphicEqualizerMode: handle=%d", handle);
    this->platform().GetGraphicEqualizerMode(handle, equalizerMode, inputGain, outputGain);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetVolumeLeveller(const int32_t handle, VolumeLeveller &leveller)
{
    ENTRY_LOG;
    LOGINFO("GetVolumeLeveller: handle=%d", handle);
    this->platform().GetVolumeLeveller(handle, leveller);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetVolumeLeveller(const int32_t handle, const VolumeLeveller leveller)
{
    ENTRY_LOG;
    LOGINFO("SetVolumeLeveller: handle=%d, mode=%d, level=%d", handle, leveller.mode, leveller.level);
    this->platform().SetVolumeLeveller(handle, leveller);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetBassEnhancer(const int32_t handle, int32_t &bassLevel)
{
    ENTRY_LOG;
    LOGINFO("GetBassEnhancer: handle=%d", handle);
    this->platform().GetBassEnhancer(handle, bassLevel);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetBassEnhancer(const int32_t handle, const int32_t bassLevel)
{
    ENTRY_LOG;
    LOGINFO("SetBassEnhancer: handle=%d, bassLevel=%d", handle, bassLevel);
    this->platform().SetBassEnhancer(handle, bassLevel);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::IsSurroundDecoderEnabled(const int32_t handle, bool &enabled)
{
    ENTRY_LOG;
    LOGINFO("IsSurroundDecoderEnabled: handle=%d", handle);
    this->platform().IsSurroundDecoderEnabled(handle, enabled);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::EnableSurroundDecoder(const int32_t handle, const bool enable)
{
    ENTRY_LOG;
    LOGINFO("EnableSurroundDecoder: handle=%d, enable=%s", handle, enable ? "true" : "false");
    this->platform().EnableSurroundDecoder(handle, enable);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetDRCMode(const int32_t handle, int32_t &mode)
{
    ENTRY_LOG;
    LOGINFO("GetDRCMode: handle=%d", handle);
    this->platform().GetDRCMode(handle, mode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetDRCMode(const int32_t handle, const int32_t mode)
{
    ENTRY_LOG;
    LOGINFO("SetDRCMode: handle=%d, mode=%d", handle, mode);
    this->platform().SetDRCMode(handle, mode);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetSurroundVirtualizer(const int32_t handle, SurroundVirtualizer &virtualizer)
{
    ENTRY_LOG;
    LOGINFO("GetSurroundVirtualizer: handle=%d", handle);
    this->platform().GetSurroundVirtualizer(handle, virtualizer);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetSurroundVirtualizer(const int32_t handle, const SurroundVirtualizer virtualizer)
{
    ENTRY_LOG;
    LOGINFO("SetSurroundVirtualizer: handle=%d, mode=%d, boost=%d", handle, virtualizer.mode, virtualizer.boost);
    this->platform().SetSurroundVirtualizer(handle, virtualizer);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetMISteering(const int32_t handle, bool &enabled)
{
    ENTRY_LOG;
    LOGINFO("GetMISteering: handle=%d", handle);
    this->platform().GetMISteering(handle, enabled);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetMISteering(const int32_t handle, const bool enable)
{
    ENTRY_LOG;
    LOGINFO("SetMISteering: handle=%d, enable=%s", handle, enable ? "true" : "false");
    this->platform().SetMISteering(handle, enable);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetMS12AudioProfile(const int32_t handle, string &profile)
{
    ENTRY_LOG;
    LOGINFO("GetMS12AudioProfile: handle=%d", handle);
    this->platform().GetMS12AudioProfile(handle, profile);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetMS12AudioProfile(const int32_t handle, const string &profile)
{
    ENTRY_LOG;
    LOGINFO("SetMS12AudioProfile: handle=%d, profile=%s", handle, profile.c_str());
    this->platform().SetMS12AudioProfile(handle, profile);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetMS12AudioProfileList(const int32_t handle, std::vector<string> &profileList)
{
    ENTRY_LOG;
    LOGINFO("GetMS12AudioProfileList: handle=%d", handle);
    this->platform().GetMS12AudioProfileList(handle, profileList);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetAssociatedAudioMixing(const int32_t handle, bool &mixing)
{
    ENTRY_LOG;
    LOGINFO("GetAssociatedAudioMixing: handle=%d", handle);
    this->platform().GetAssociatedAudioMixing(handle, mixing);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetAssociatedAudioMixing(const int32_t handle, const bool mixing)
{
    ENTRY_LOG;
    LOGINFO("SetAssociatedAudioMixing: handle=%d, mixing=%s", handle, mixing ? "true" : "false");
    this->platform().SetAssociatedAudioMixing(handle, mixing);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetFaderControl(const int32_t handle, int32_t &mixerBalance)
{
    ENTRY_LOG;
    LOGINFO("GetFaderControl: handle=%d", handle);
    this->platform().GetFaderControl(handle, mixerBalance);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetFaderControl(const int32_t handle, const int32_t mixerBalance)
{
    ENTRY_LOG;
    LOGINFO("SetFaderControl: handle=%d, mixerBalance=%d", handle, mixerBalance);
    this->platform().SetFaderControl(handle, mixerBalance);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetPrimaryLanguage(const int32_t handle, string &primaryLanguage)
{
    ENTRY_LOG;
    LOGINFO("GetPrimaryLanguage: handle=%d", handle);
    this->platform().GetPrimaryLanguage(handle, primaryLanguage);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetPrimaryLanguage(const int32_t handle, const string &primaryLanguage)
{
    ENTRY_LOG;
    LOGINFO("SetPrimaryLanguage: handle=%d, primaryLanguage=%s", handle, primaryLanguage.c_str());
    this->platform().SetPrimaryLanguage(handle, primaryLanguage);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetSecondaryLanguage(const int32_t handle, string &secondaryLanguage)
{
    ENTRY_LOG;
    LOGINFO("GetSecondaryLanguage: handle=%d", handle);
    this->platform().GetSecondaryLanguage(handle, secondaryLanguage);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetSecondaryLanguage(const int32_t handle, const string &secondaryLanguage)
{
    ENTRY_LOG;
    LOGINFO("SetSecondaryLanguage: handle=%d, secondaryLanguage=%s", handle, secondaryLanguage.c_str());
    this->platform().SetSecondaryLanguage(handle, secondaryLanguage);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetAudioCapabilities(const int32_t handle, int32_t &capabilities)
{
    ENTRY_LOG;
    LOGINFO("GetAudioCapabilities: handle=%d", handle);
    this->platform().GetAudioCapabilities(handle, capabilities);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetMS12Capabilities(const int32_t handle, int32_t &capabilities)
{
    ENTRY_LOG;
    LOGINFO("GetMS12Capabilities: handle=%d", handle);
    this->platform().GetMS12Capabilities(handle, capabilities);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::IsAudioOutputConnected(const int32_t handle, bool &connected)
{
    ENTRY_LOG;
    LOGINFO("IsAudioOutputConnected: handle=%d", handle);
    this->platform().IsAudioOutputConnected(handle, connected);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetAudioDelay(const int32_t handle, const uint32_t audioDelay)
{
    ENTRY_LOG;
    LOGINFO("SetAudioDelay: handle=%d, audioDelay=%u", handle, audioDelay);
    this->platform().SetAudioDelay(handle, audioDelay);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetAudioDelay(const int32_t handle, uint32_t &audioDelay)
{
    ENTRY_LOG;
    LOGINFO("GetAudioDelay: handle=%d", handle);
    this->platform().GetAudioDelay(handle, audioDelay);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetSinkDeviceAtmosCapability(const int32_t handle, DolbyAtmosCapability &atmosCapability)
{
    ENTRY_LOG;
    LOGINFO("GetSinkDeviceAtmosCapability: handle=%d", handle);
    this->platform().GetSinkDeviceAtmosCapability(handle, atmosCapability);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetAudioAtmosOutputMode(const int32_t handle, const bool enable)
{
    ENTRY_LOG;
    LOGINFO("SetAudioAtmosOutputMode: handle=%d, enable=%s", handle, enable ? "true" : "false");
    this->platform().SetAudioAtmosOutputMode(handle, enable);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::EnableLEConfig(const int32_t handle, const bool enable)
{
    ENTRY_LOG;
    LOGINFO("EnableLEConfig: handle=%d, enable=%s", handle, enable ? "true" : "false");
    this->platform().EnableLEConfig(handle, enable);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetLEConfig(const int32_t handle, bool &enabled)
{
    ENTRY_LOG;
    LOGINFO("GetLEConfig: handle=%d", handle);
    this->platform().GetLEConfig(handle, enabled);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetAudioMixerLevels(const int32_t handle, const int32_t primaryVolume, const int32_t inputVolume)
{
    ENTRY_LOG;
    LOGINFO("SetAudioMixerLevels: handle=%d, primaryVolume=%d, inputVolume=%d", handle, primaryVolume, inputVolume);
    this->platform().SetAudioMixerLevels(handle, primaryVolume, inputVolume);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::EnablePersist(const int32_t handle, const bool enable)
{
    ENTRY_LOG;
    LOGINFO("EnablePersist: handle=%d, enable=%s", handle, enable ? "true" : "false");
    this->platform().EnablePersist(handle, enable);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::IsEnablePersist(const int32_t handle, bool &enabled)
{
    ENTRY_LOG;
    LOGINFO("IsEnablePersist: handle=%d", handle);
    this->platform().IsEnablePersist(handle, enabled);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::ResetBassEnhancer(const int32_t handle)
{
    ENTRY_LOG;
    LOGINFO("ResetBassEnhancer: handle=%d", handle);
    this->platform().ResetBassEnhancer(handle);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::ResetSurroundVirtualizer(const int32_t handle)
{
    ENTRY_LOG;
    LOGINFO("ResetSurroundVirtualizer: handle=%d", handle);
    this->platform().ResetSurroundVirtualizer(handle);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::ResetVolumeLeveller(const int32_t handle)
{
    ENTRY_LOG;
    LOGINFO("ResetVolumeLeveller: handle=%d", handle);
    this->platform().ResetVolumeLeveller(handle);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::ResetDialogEnhancement(const int32_t handle)
{
    ENTRY_LOG;
    LOGINFO("ResetDialogEnhancement: handle=%d", handle);
    this->platform().ResetDialogEnhancement(handle);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::SetMS12SettingsOverride(const int32_t handle, const string &feature, const string &value)
{
    ENTRY_LOG;
    LOGINFO("SetMS12SettingsOverride: handle=%d, feature=%s, value=%s", handle, feature.c_str(), value.c_str());
    this->platform().SetMS12SettingsOverride(handle, feature, value);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t Audio::GetHDMIARCPortId(uint32_t &portId)
{
    ENTRY_LOG;
    LOGINFO("GetHDMIARCPortId");
    this->platform().GetHDMIARCPortId(portId);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}