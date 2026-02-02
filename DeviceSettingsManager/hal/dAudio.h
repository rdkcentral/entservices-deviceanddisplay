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

#include "dsAudio.h"
#include "dsError.h"
#include "dsUtl.h"
#include "dsTypes.h"

#include "rfcapi.h"

#include <core/Portability.h>
#include <WPEFramework/interfaces/IDeviceSettingsManager.h>
#include "DeviceSettingsManagerTypes.h"
#include "Module.h"

#include "UtilsLogging.h"
#include <cstdint>

namespace hal {
namespace dAudio {

    class IPlatform {

    public:
        virtual ~IPlatform();
        void InitialiseHAL();
        void DeInitialiseHAL();

        // Audio Platform interface methods - all pure virtual
        virtual uint32_t GetAudioPort(const AudioPortType type, const int32_t index, int32_t &handle) = 0;
        virtual uint32_t IsAudioPortEnabled(const int32_t handle, bool &enabled) = 0;
        virtual uint32_t EnableAudioPort(const int32_t handle, const bool enable) = 0;
        virtual uint32_t GetSupportedARCTypes(const int32_t handle, int32_t &types) = 0;
        virtual uint32_t SetSAD(const int32_t handle, const uint8_t sadList[], const uint8_t count) = 0;
        virtual uint32_t EnableARC(const int32_t handle, const AudioARCStatus arcStatus) = 0;
        virtual uint32_t GetStereoMode(const int32_t handle, StereoModes &mode) = 0;
        virtual uint32_t SetStereoMode(const int32_t handle, const StereoModes mode, const bool persist) = 0;
        virtual uint32_t GetStereoAuto(const int32_t handle, int32_t &mode) = 0;
        virtual uint32_t SetStereoAuto(const int32_t handle, const int32_t mode, const bool persist) = 0;
        virtual uint32_t SetAudioMute(const int32_t handle, const bool mute) = 0;
        virtual uint32_t IsAudioMuted(const int32_t handle, bool &muted) = 0;
        virtual uint32_t SetAudioDucking(const int32_t handle, const AudioDuckingType duckingType, const AudioDuckingAction duckingAction, const uint8_t level) = 0;
        virtual uint32_t SetAudioLevel(const int32_t handle, const float audioLevel) = 0;
        virtual uint32_t GetAudioLevel(const int32_t handle, float &audioLevel) = 0;
        virtual uint32_t SetAudioGain(const int32_t handle, const float gainLevel) = 0;
        virtual uint32_t GetAudioGain(const int32_t handle, float &gainLevel) = 0;
        virtual uint32_t GetAudioFormat(const int32_t handle, AudioFormat &audioFormat) = 0;
        virtual uint32_t GetAudioEncoding(const int32_t handle, AudioEncoding &audioEncoding) = 0;
        virtual uint32_t IsAudioMSDecoding(const int32_t handle, bool &hasMS11Decode) = 0;
        virtual uint32_t IsAudioMS12Decoding(const int32_t handle, bool &hasMS12Decode) = 0;
        virtual uint32_t SetAudioCompression(const int32_t handle, const AudioCompression compressionMode) = 0;
        virtual uint32_t GetAudioCompression(const int32_t handle, AudioCompression &compressionMode) = 0;
        virtual uint32_t SetDialogEnhancement(const int32_t handle, const int32_t enhancerLevel) = 0;
        virtual uint32_t GetDialogEnhancement(const int32_t handle, int32_t &enhancerLevel) = 0;
        virtual uint32_t SetDolbyVolumeMode(const int32_t handle, const bool dolbyVolumeMode) = 0;
        virtual uint32_t GetDolbyVolumeMode(const int32_t handle, bool &dolbyVolumeMode) = 0;
        virtual uint32_t SetIntelligentEqualizerMode(const int32_t handle, const int32_t equalizerMode) = 0;
        virtual uint32_t GetIntelligentEqualizerMode(const int32_t handle, int32_t &equalizerMode) = 0;
        virtual uint32_t SetGraphicEqualizerMode(const int32_t handle, const int32_t equalizerMode, const int32_t inputGain, const int32_t outputGain) = 0;
        virtual uint32_t GetGraphicEqualizerMode(const int32_t handle, int32_t &equalizerMode, int32_t &inputGain, int32_t &outputGain) = 0;
        virtual uint32_t GetVolumeLeveller(const int32_t handle, WPEFramework::Exchange::IDeviceSettingsManagerAudio::VolumeLeveller &leveller) = 0;
        virtual uint32_t SetVolumeLeveller(const int32_t handle, const WPEFramework::Exchange::IDeviceSettingsManagerAudio::VolumeLeveller leveller) = 0;
        virtual uint32_t GetBassEnhancer(const int32_t handle, int32_t &bassLevel) = 0;
        virtual uint32_t SetBassEnhancer(const int32_t handle, const int32_t bassLevel) = 0;
        virtual uint32_t IsSurroundDecoderEnabled(const int32_t handle, bool &enabled) = 0;
        virtual uint32_t EnableSurroundDecoder(const int32_t handle, const bool enable) = 0;
        virtual uint32_t GetDRCMode(const int32_t handle, int32_t &mode) = 0;
        virtual uint32_t SetDRCMode(const int32_t handle, const int32_t mode) = 0;
        virtual uint32_t GetSurroundVirtualizer(const int32_t handle, WPEFramework::Exchange::IDeviceSettingsManagerAudio::SurroundVirtualizer &virtualizer) = 0;
        virtual uint32_t SetSurroundVirtualizer(const int32_t handle, const WPEFramework::Exchange::IDeviceSettingsManagerAudio::SurroundVirtualizer virtualizer) = 0;
        virtual uint32_t GetMISteering(const int32_t handle, bool &enabled) = 0;
        virtual uint32_t SetMISteering(const int32_t handle, const bool enable) = 0;
        virtual uint32_t GetMS12AudioProfile(const int32_t handle, string &profile) = 0;
        virtual uint32_t SetMS12AudioProfile(const int32_t handle, const string &profile) = 0;
        virtual uint32_t GetMS12AudioProfileList(const int32_t handle, std::vector<string> &profileList) = 0;
        virtual uint32_t GetAssociatedAudioMixing(const int32_t handle, bool &mixing) = 0;
        virtual uint32_t SetAssociatedAudioMixing(const int32_t handle, const bool mixing) = 0;
        virtual uint32_t GetFaderControl(const int32_t handle, int32_t &mixerBalance) = 0;
        virtual uint32_t SetFaderControl(const int32_t handle, const int32_t mixerBalance) = 0;
        virtual uint32_t GetPrimaryLanguage(const int32_t handle, string &primaryLanguage) = 0;
        virtual uint32_t SetPrimaryLanguage(const int32_t handle, const string &primaryLanguage) = 0;
        virtual uint32_t GetSecondaryLanguage(const int32_t handle, string &secondaryLanguage) = 0;
        virtual uint32_t SetSecondaryLanguage(const int32_t handle, const string &secondaryLanguage) = 0;
        virtual uint32_t GetAudioCapabilities(const int32_t handle, int32_t &capabilities) = 0;
        virtual uint32_t GetMS12Capabilities(const int32_t handle, int32_t &capabilities) = 0;
        virtual uint32_t IsAudioOutputConnected(const int32_t handle, bool &connected) = 0;
        virtual uint32_t SetAudioDelay(const int32_t handle, const uint32_t audioDelay) = 0;
        virtual uint32_t GetAudioDelay(const int32_t handle, uint32_t &audioDelay) = 0;
        virtual uint32_t GetSinkDeviceAtmosCapability(const int32_t handle, DolbyAtmosCapability &atmosCapability) = 0;
        virtual uint32_t SetAudioAtmosOutputMode(const int32_t handle, const bool enable) = 0;
        virtual uint32_t EnableLEConfig(const int32_t handle, const bool enable) = 0;
        virtual uint32_t GetLEConfig(const int32_t handle, bool &enabled) = 0;
        virtual uint32_t SetAudioMixerLevels(const int32_t handle, const int32_t primaryVolume, const int32_t inputVolume) = 0;
        virtual uint32_t EnablePersist(const int32_t handle, const bool enable) = 0;
        virtual uint32_t IsEnablePersist(const int32_t handle, bool &enabled) = 0;
        virtual uint32_t ResetBassEnhancer(const int32_t handle) = 0;
        virtual uint32_t ResetSurroundVirtualizer(const int32_t handle) = 0;
        virtual uint32_t ResetVolumeLeveller(const int32_t handle) = 0;
        virtual uint32_t ResetDialogEnhancement(const int32_t handle) = 0;
        virtual uint32_t SetMS12SettingsOverride(const int32_t handle, const string &feature, const string &value) = 0;
        virtual uint32_t GetHDMIARCPortId(uint32_t &portId) = 0;

        // Callback management
        virtual void setAllCallbacks(const CallbackBundle bundle) = 0;
        virtual void getPersistenceValue() = 0;
    };

}
}