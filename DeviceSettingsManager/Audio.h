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
#pragma once

#include <cstdint>
#include <type_traits>

#include <core/Portability.h>
#include <core/Proxy.h>
#include <core/Trace.h>

#include "UtilsLogging.h"
#include "DeviceSettingsManagerTypes.h"
#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>

#include <interfaces/IDeviceSettingsManager.h>
#include "DeviceSettingsManagerTypes.h"

#include "exception.hpp"
#include "manager.hpp"

#include "hal/dAudioImpl.h"

class Audio {
    using IPlatform = hal::dAudio::IPlatform;
    using DefaultImpl = dAudioImpl;

    std::shared_ptr<IPlatform> _platform;
public:
    class INotification {

        public:
            virtual ~INotification() = default;
            
            virtual void OnAssociatedAudioMixingChangedNotification(bool mixing) = 0;
            virtual void OnAudioFaderControlChangedNotification(int32_t mixerBalance) = 0;
            virtual void OnAudioPrimaryLanguageChangedNotification(const string& primaryLanguage) = 0;
            virtual void OnAudioSecondaryLanguageChangedNotification(const string& secondaryLanguage) = 0;
            virtual void OnAudioOutHotPlugNotification(AudioPortType portType, uint32_t uiPortNumber, bool isPortConnected) = 0;
            virtual void OnAudioFormatUpdateNotification(AudioFormat audioFormat) = 0;
            virtual void OnDolbyAtmosCapabilitiesChangedNotification(DolbyAtmosCapability atmosCapability, bool status) = 0;
            virtual void OnAudioPortStateChangedNotification(AudioPortState audioPortState) = 0;
            virtual void OnAudioModeEventNotification(AudioPortType audioPortType, StereoModes audioMode) = 0;
            virtual void OnAudioLevelChangedEventNotification(int32_t audioLevel) = 0;
    };

public:

    void Platform_init();

    uint32_t GetAudioPort(const AudioPortType type, const int32_t index, int32_t &handle);
    uint32_t IsAudioPortEnabled(const int32_t handle, bool &enabled);
    uint32_t EnableAudioPort(const int32_t handle, const bool enable);
    uint32_t GetSupportedARCTypes(const int32_t handle, int32_t &types);
    uint32_t SetSAD(const int32_t handle, const uint8_t sadList[], const uint8_t count);
    uint32_t EnableARC(const int32_t handle, const AudioARCStatus arcStatus);
    uint32_t GetStereoMode(const int32_t handle, StereoModes &mode);
    uint32_t SetStereoMode(const int32_t handle, const StereoModes mode, const bool persist);
    uint32_t GetStereoAuto(const int32_t handle, int32_t &autoMode);
    uint32_t SetStereoAuto(const int32_t handle, const int32_t autoMode, const bool persist);
    uint32_t SetAudioMute(const int32_t handle, const bool mute);
    uint32_t IsAudioMuted(const int32_t handle, bool &muted);
    uint32_t SetAudioDucking(const int32_t handle, const AudioDuckingType duckingType, const AudioDuckingAction duckingAction, const uint8_t level);
    uint32_t SetAudioLevel(const int32_t handle, const float audioLevel);
    uint32_t GetAudioLevel(const int32_t handle, float &audioLevel);
    uint32_t SetAudioGain(const int32_t handle, const float gainLevel);
    uint32_t GetAudioGain(const int32_t handle, float &gainLevel);
    uint32_t GetAudioFormat(const int32_t handle, AudioFormat &audioFormat);
    uint32_t GetAudioEncoding(const int32_t handle, AudioEncoding &audioEncoding);
    uint32_t IsAudioMSDecoding(const int32_t handle, bool &hasMS11Decode);
    uint32_t IsAudioMS12Decoding(const int32_t handle, bool &hasMS12Decode);
    uint32_t SetAudioCompression(const int32_t handle, const AudioCompression compressionMode);
    uint32_t GetAudioCompression(const int32_t handle, AudioCompression &compressionMode);
    uint32_t SetDialogEnhancement(const int32_t handle, const int32_t enhancerLevel);
    uint32_t GetDialogEnhancement(const int32_t handle, int32_t &enhancerLevel);
    uint32_t SetDolbyVolumeMode(const int32_t handle, const bool dolbyVolumeMode);
    uint32_t GetDolbyVolumeMode(const int32_t handle, bool &dolbyVolumeMode);
    uint32_t SetIntelligentEqualizerMode(const int32_t handle, const int32_t equalizerMode);
    uint32_t GetIntelligentEqualizerMode(const int32_t handle, int32_t &equalizerMode);
    uint32_t SetGraphicEqualizerMode(const int32_t handle, const int32_t equalizerMode, const int32_t inputGain, const int32_t outputGain);
    uint32_t GetGraphicEqualizerMode(const int32_t handle, int32_t &equalizerMode, int32_t &inputGain, int32_t &outputGain);
    uint32_t GetVolumeLeveller(const int32_t handle, VolumeLeveller &leveller);
    uint32_t SetVolumeLeveller(const int32_t handle, const VolumeLeveller leveller);
    uint32_t GetBassEnhancer(const int32_t handle, int32_t &bassLevel);
    uint32_t SetBassEnhancer(const int32_t handle, const int32_t bassLevel);
    uint32_t IsSurroundDecoderEnabled(const int32_t handle, bool &enabled);
    uint32_t EnableSurroundDecoder(const int32_t handle, const bool enable);
    uint32_t GetDRCMode(const int32_t handle, int32_t &mode);
    uint32_t SetDRCMode(const int32_t handle, const int32_t mode);
    uint32_t GetSurroundVirtualizer(const int32_t handle, SurroundVirtualizer &virtualizer);
    uint32_t SetSurroundVirtualizer(const int32_t handle, const SurroundVirtualizer virtualizer);
    uint32_t GetMISteering(const int32_t handle, bool &enabled);
    uint32_t SetMISteering(const int32_t handle, const bool enable);
    uint32_t GetMS12AudioProfile(const int32_t handle, string &profile);
    uint32_t SetMS12AudioProfile(const int32_t handle, const string &profile);
    uint32_t GetMS12AudioProfileList(const int32_t handle, std::vector<string> &profileList);
    uint32_t GetAssociatedAudioMixing(const int32_t handle, bool &mixing);
    uint32_t SetAssociatedAudioMixing(const int32_t handle, const bool mixing);
    uint32_t GetFaderControl(const int32_t handle, int32_t &mixerBalance);
    uint32_t SetFaderControl(const int32_t handle, const int32_t mixerBalance);
    uint32_t GetPrimaryLanguage(const int32_t handle, string &primaryLanguage);
    uint32_t SetPrimaryLanguage(const int32_t handle, const string &primaryLanguage);
    uint32_t GetSecondaryLanguage(const int32_t handle, string &secondaryLanguage);
    uint32_t SetSecondaryLanguage(const int32_t handle, const string &secondaryLanguage);
    uint32_t GetAudioCapabilities(const int32_t handle, int32_t &capabilities);
    uint32_t GetMS12Capabilities(const int32_t handle, int32_t &capabilities);
    uint32_t IsAudioOutputConnected(const int32_t handle, bool &connected);
    uint32_t SetAudioDelay(const int32_t handle, const uint32_t audioDelay);
    uint32_t GetAudioDelay(const int32_t handle, uint32_t &audioDelay);
    uint32_t GetSinkDeviceAtmosCapability(const int32_t handle, DolbyAtmosCapability &atmosCapability);
    uint32_t SetAudioAtmosOutputMode(const int32_t handle, const bool enable);
    uint32_t EnableLEConfig(const int32_t handle, const bool enable);
    uint32_t GetLEConfig(const int32_t handle, bool &enabled);
    uint32_t SetAudioMixerLevels(const int32_t handle, const int32_t primaryVolume, const int32_t inputVolume);
    uint32_t EnablePersist(const int32_t handle, const bool enable);
    uint32_t IsEnablePersist(const int32_t handle, bool &enabled);
    uint32_t ResetBassEnhancer(const int32_t handle);
    uint32_t ResetSurroundVirtualizer(const int32_t handle);
    uint32_t ResetVolumeLeveller(const int32_t handle);
    uint32_t ResetDialogEnhancement(const int32_t handle);
    uint32_t SetMS12SettingsOverride(const int32_t handle, const string &feature, const string &value);
    uint32_t GetHDMIARCPortId(uint32_t &portId);

private:
    Audio (INotification& parent, std::shared_ptr<IPlatform> platform);

    inline IPlatform& platform() const
    {
        return *_platform;
    }

    INotification& _parent;

public:
    template <typename IMPL = DefaultImpl, typename... Args>
    static Audio Create(INotification& parent, Args&&... args)
    {
        ENTRY_LOG;
        static_assert(std::is_base_of<IPlatform, IMPL>::value, "Impl must derive from hal::dAudio::IPlatform");
        auto impl = std::shared_ptr<IMPL>(new IMPL(std::forward<Args>(args)...));
        ASSERT(impl != nullptr);
        EXIT_LOG;
        return Audio(parent, std::move(impl));
    }

    void OnAssociatedAudioMixingChangedEvent(bool mixing);
    void OnAudioFaderControlChangedEvent(int32_t mixerBalance);
    void OnAudioPrimaryLanguageChangedEvent(const string& primaryLanguage);
    void OnAudioSecondaryLanguageChangedEvent(const string& secondaryLanguage);
    void OnAudioOutHotPlugEvent(AudioPortType portType, uint32_t uiPortNumber, bool isPortConnected);
    void OnAudioFormatUpdateEvent(AudioFormat audioFormat);
    void OnDolbyAtmosCapabilitiesChangedEvent(DolbyAtmosCapability atmosCapability, bool status);
    void OnAudioPortStateChangedEvent(AudioPortState audioPortState);
    void OnAudioModeEvent(AudioPortType audioPortType, StereoModes audioMode);
    void OnAudioLevelChangedEvent(int32_t audioLevel);
    ~Audio() {};

};