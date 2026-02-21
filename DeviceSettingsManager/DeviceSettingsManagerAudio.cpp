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

#include "DeviceSettingsManagerImp.h"
#include "UtilsLogging.h"

using namespace std;

namespace WPEFramework {
namespace Plugin {

    // ================================
    // Audio Implementation Methods
    // ================================

    Core::hresult DeviceSettingsManagerImp::Register(DeviceSettingsManagerAudio::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Register(_AudioNotifications, notification);
        if (errorCode != Core::ERROR_NONE) {
            LOGERR("IAudio %p, errorCode: %u", notification, errorCode);
        } else {
            LOGINFO("IAudio %p registered successfully", notification);
        }
        EXIT_LOG;
        return errorCode;
    }

    Core::hresult DeviceSettingsManagerImp::Unregister(DeviceSettingsManagerAudio::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Unregister(_AudioNotifications, notification);
        if (errorCode != Core::ERROR_NONE) {
            LOGERR("IAudio %p, errorcode: %u", notification, errorCode);
        } else {
            LOGINFO("IAudio %p unregistered successfully", notification);
        }
        EXIT_LOG;
        return errorCode;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioPort(const AudioPortType type, const int32_t index, int32_t &handle) {
        ENTRY_LOG;
        LOGINFO("GetAudioPort: type=%d, index=%d", type, index);
        
        _apiLock.Lock();
        _audio.GetAudioPort(type, index, handle);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioPort: handle=%d", handle);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::IsAudioPortEnabled(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        LOGINFO("IsAudioPortEnabled: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.IsAudioPortEnabled(handle, enabled);
        _apiLock.Unlock();
        
        LOGINFO("IsAudioPortEnabled: handle=%d, enabled=%s", handle, enabled ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::EnableAudioPort(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        LOGINFO("EnableAudioPort: handle=%d, enable=%s", handle, enable ? "true" : "false");
        
        _apiLock.Lock();
        _audio.EnableAudioPort(handle, enable);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetStereoMode(const int32_t handle, StereoModes &mode) {
        ENTRY_LOG;
        LOGINFO("GetStereoMode: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetStereoMode(handle, mode);
        _apiLock.Unlock();
        
        LOGINFO("GetStereoMode: handle=%d, mode=%d", handle, mode);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetStereoMode(const int32_t handle, const StereoModes mode, const bool persist) {
        ENTRY_LOG;
        LOGINFO("SetStereoMode: handle=%d, mode=%d, persist=%s", handle, mode, persist ? "true" : "false");
        
        _apiLock.Lock();
        _audio.SetStereoMode(handle, mode, persist);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioMute(const int32_t handle, const bool mute) {
        ENTRY_LOG;
        LOGINFO("SetAudioMute: handle=%d, mute=%s", handle, mute ? "true" : "false");
        
        _apiLock.Lock();
        _audio.SetAudioMute(handle, mute);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::IsAudioMuted(const int32_t handle, bool &muted) {
        ENTRY_LOG;
        LOGINFO("IsAudioMuted: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.IsAudioMuted(handle, muted);
        _apiLock.Unlock();
        
        LOGINFO("IsAudioMuted: handle=%d, muted=%s", handle, muted ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetSupportedARCTypes(const int32_t handle, int32_t &types) {
        ENTRY_LOG;
        LOGINFO("GetSupportedARCTypes: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetSupportedARCTypes(handle, types);
        _apiLock.Unlock();
        
        LOGINFO("GetSupportedARCTypes: handle=%d, types=%d", handle, types);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioLevel(const int32_t handle, const float audioLevel) {
        ENTRY_LOG;
        LOGINFO("SetAudioLevel: handle=%d, audioLevel=%f", handle, audioLevel);
        
        _apiLock.Lock();
        _audio.SetAudioLevel(handle, audioLevel);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioLevel(const int32_t handle, float &audioLevel) {
        ENTRY_LOG;
        LOGINFO("GetAudioLevel: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetAudioLevel(handle, audioLevel);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioLevel: handle=%d, audioLevel=%f", handle, audioLevel);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioGain(const int32_t handle, const float gainLevel) {
        ENTRY_LOG;
        LOGINFO("SetAudioGain: handle=%d, gainLevel=%f", handle, gainLevel);
        
        _apiLock.Lock();
        _audio.SetAudioGain(handle, gainLevel);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioGain(const int32_t handle, float &gainLevel) {
        ENTRY_LOG;
        LOGINFO("GetAudioGain: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetAudioGain(handle, gainLevel);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioGain: handle=%d, gainLevel=%f", handle, gainLevel);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetSAD(const int32_t handle, const uint8_t sadList[], const uint8_t count) {
        ENTRY_LOG;
        LOGINFO("SetSAD: handle=%d, count=%d", handle, count);
        
        _apiLock.Lock();
        _audio.SetSAD(handle, sadList, count);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::EnableARC(const int32_t handle, const AudioARCStatus arcStatus) {
        ENTRY_LOG;
        LOGINFO("EnableARC: handle=%d, arcStatus=%d", handle, arcStatus.arcType);
        
        _apiLock.Lock();
        _audio.EnableARC(handle, arcStatus);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetStereoAuto(const int32_t handle, int32_t &mode) {
        ENTRY_LOG;
        LOGINFO("GetStereoAuto: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetStereoAuto(handle, mode);
        _apiLock.Unlock();
        
        LOGINFO("GetStereoAuto: handle=%d, mode=%d", handle, mode);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetStereoAuto(const int32_t handle, const int32_t mode, const bool persist) {
        ENTRY_LOG;
        LOGINFO("SetStereoAuto: handle=%d, mode=%d, persist=%s", handle, mode, persist ? "true" : "false");
        
        _apiLock.Lock();
        _audio.SetStereoAuto(handle, mode, persist);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioDucking(const int32_t handle, const AudioDuckingType duckingType, const AudioDuckingAction duckingAction, const uint8_t level) {
        ENTRY_LOG;
        LOGINFO("SetAudioDucking: handle=%d, duckingType=%d, duckingAction=%d, level=%d", handle, duckingType, duckingAction, level);
        
        _apiLock.Lock();
        _audio.SetAudioDucking(handle, duckingType, duckingAction, level);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioFormat(const int32_t handle, AudioFormat &audioFormat) {
        ENTRY_LOG;
        LOGINFO("GetAudioFormat: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetAudioFormat(handle, audioFormat);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioFormat: handle=%d, audioFormat=%d", handle, audioFormat);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioEncoding(const int32_t handle, AudioEncoding &encoding) {
        ENTRY_LOG;
        LOGINFO("GetAudioEncoding: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetAudioEncoding(handle, encoding);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioEncoding: handle=%d, encoding=%d", handle, encoding);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioEnablePersist(const int32_t handle, bool &enabled, string &portName) {
        ENTRY_LOG;
        LOGINFO("GetAudioEnablePersist: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.IsEnablePersist(handle, enabled);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioEnablePersist: handle=%d, enabled=%s", handle, enabled ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioEnablePersist(const int32_t handle, const bool enable, const string portName) {
        ENTRY_LOG;
        LOGINFO("SetAudioEnablePersist: handle=%d, enable=%s, portName=%s", handle, enable ? "true" : "false", portName.c_str());
        
        _apiLock.Lock();
        _audio.EnablePersist(handle, enable);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::IsAudioMSDecoded(const int32_t handle, bool &hasms11Decode) {
        ENTRY_LOG;
        LOGINFO("IsAudioMSDecoded: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.IsAudioMSDecoding(handle, hasms11Decode);
        _apiLock.Unlock();
        
        LOGINFO("IsAudioMSDecoded: handle=%d, hasms11Decode=%s", handle, hasms11Decode ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::IsAudioMS12Decoded(const int32_t handle, bool &hasms12Decode) {
        ENTRY_LOG;
        LOGINFO("IsAudioMS12Decoded: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.IsAudioMS12Decoding(handle, hasms12Decode);
        _apiLock.Unlock();
        
        LOGINFO("IsAudioMS12Decoded: handle=%d, hasms12Decode=%s", handle, hasms12Decode ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioLEConfig(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        LOGINFO("GetAudioLEConfig: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetLEConfig(handle, enabled);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioLEConfig: handle=%d, enabled=%s", handle, enabled ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::EnableAudioLEConfig(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        LOGINFO("EnableAudioLEConfig: handle=%d, enable=%s", handle, enable ? "true" : "false");
        
        _apiLock.Lock();
        _audio.EnableLEConfig(handle, enable);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioDelay(const int32_t handle, const uint32_t audioDelay) {
        ENTRY_LOG;
        LOGINFO("SetAudioDelay: handle=%d, audioDelay=%u", handle, audioDelay);
        
        _apiLock.Lock();
        _audio.SetAudioDelay(handle, audioDelay);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioDelay(const int32_t handle, uint32_t &audioDelay) {
        ENTRY_LOG;
        LOGINFO("GetAudioDelay: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetAudioDelay(handle, audioDelay);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioDelay: handle=%d, audioDelay=%u", handle, audioDelay);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioDelayOffset(const int32_t handle, const uint32_t delayOffset) {
        ENTRY_LOG;
        LOGINFO("SetAudioDelayOffset: handle=%d, delayOffset=%u", handle, delayOffset);
        
        // Note: Audio.cpp doesn't have SetAudioDelayOffset, using SetAudioDelay as fallback
        _apiLock.Lock();
        _audio.SetAudioDelay(handle, delayOffset);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioDelayOffset(const int32_t handle, uint32_t &delayOffset) {
        ENTRY_LOG;
        LOGINFO("GetAudioDelayOffset: handle=%d", handle);
        
        // Note: Audio.cpp doesn't have GetAudioDelayOffset, using GetAudioDelay as fallback
        _apiLock.Lock();
        _audio.GetAudioDelay(handle, delayOffset);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioDelayOffset: handle=%d, delayOffset=%u", handle, delayOffset);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioSinkDeviceAtmosCapability(const int32_t handle, DolbyAtmosCapability &atmosCapability) {
        ENTRY_LOG;
        LOGINFO("GetAudioSinkDeviceAtmosCapability: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetSinkDeviceAtmosCapability(handle, atmosCapability);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioSinkDeviceAtmosCapability: handle=%d, atmosCapability=%d", handle, atmosCapability);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioAtmosOutputMode(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        LOGINFO("SetAudioAtmosOutputMode: handle=%d, enable=%s", handle, enable ? "true" : "false");
        
        _apiLock.Lock();
        _audio.SetAudioAtmosOutputMode(handle, enable);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioCompression(const int32_t handle, const int32_t compressionLevel) {
        ENTRY_LOG;
        LOGINFO("SetAudioCompression: handle=%d, compressionLevel=%d", handle, compressionLevel);
        
        _apiLock.Lock();
        AudioCompression compressionMode = static_cast<AudioCompression>(compressionLevel);
        _audio.SetAudioCompression(handle, compressionMode);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioCompression(const int32_t handle, int32_t &compressionLevel) {
        ENTRY_LOG;
        LOGINFO("GetAudioCompression: handle=%d", handle);
        
        _apiLock.Lock();
        AudioCompression compressionMode;
        _audio.GetAudioCompression(handle, compressionMode);
        compressionLevel = static_cast<int32_t>(compressionMode);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioCompression: handle=%d, compressionLevel=%d", handle, compressionLevel);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioDialogEnhancement(const int32_t handle, const int32_t level) {
        ENTRY_LOG;
        LOGINFO("SetAudioDialogEnhancement: handle=%d, level=%d", handle, level);
        
        _apiLock.Lock();
        _audio.SetDialogEnhancement(handle, level);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioDialogEnhancement(const int32_t handle, int32_t &level) {
        ENTRY_LOG;
        LOGINFO("GetAudioDialogEnhancement: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetDialogEnhancement(handle, level);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioDialogEnhancement: handle=%d, level=%d", handle, level);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioDolbyVolumeMode(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        LOGINFO("SetAudioDolbyVolumeMode: handle=%d, enable=%s", handle, enable ? "true" : "false");
        
        _apiLock.Lock();
        _audio.SetDolbyVolumeMode(handle, enable);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioDolbyVolumeMode(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        LOGINFO("GetAudioDolbyVolumeMode: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetDolbyVolumeMode(handle, enabled);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioDolbyVolumeMode: handle=%d, enabled=%s", handle, enabled ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioIntelligentEqualizerMode(const int32_t handle, const int32_t mode) {
        ENTRY_LOG;
        LOGINFO("SetAudioIntelligentEqualizerMode: handle=%d, mode=%d", handle, mode);
        
        _apiLock.Lock();
        _audio.SetIntelligentEqualizerMode(handle, mode);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioIntelligentEqualizerMode(const int32_t handle, int32_t &mode) {
        ENTRY_LOG;
        LOGINFO("GetAudioIntelligentEqualizerMode: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetIntelligentEqualizerMode(handle, mode);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioIntelligentEqualizerMode: handle=%d, mode=%d", handle, mode);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioVolumeLeveller(const int32_t handle, const VolumeLeveller volumeLeveller) {
        ENTRY_LOG;
        LOGINFO("SetAudioVolumeLeveller: handle=%d, volumeLeveller(mode=%d, level=%d)", handle, volumeLeveller.mode, volumeLeveller.level);
        
        _apiLock.Lock();
        _audio.SetVolumeLeveller(handle, volumeLeveller);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioVolumeLeveller(const int32_t handle, VolumeLeveller &volumeLeveller) {
        ENTRY_LOG;
        LOGINFO("GetAudioVolumeLeveller: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetVolumeLeveller(handle, volumeLeveller);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioVolumeLeveller: handle=%d, volumeLeveller(mode=%d, level=%d)", handle, volumeLeveller.mode, volumeLeveller.level);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioBassEnhancer(const int32_t handle, const int32_t boost) {
        ENTRY_LOG;
        LOGINFO("SetAudioBassEnhancer: handle=%d, boost=%d", handle, boost);
        
        _apiLock.Lock();
        _audio.SetBassEnhancer(handle, boost);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioBassEnhancer(const int32_t handle, int32_t &boost) {
        ENTRY_LOG;
        LOGINFO("GetAudioBassEnhancer: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetBassEnhancer(handle, boost);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioBassEnhancer: handle=%d, boost=%d", handle, boost);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::EnableAudioSurroudDecoder(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        LOGINFO("EnableAudioSurroudDecoder: handle=%d, enable=%s", handle, enable ? "true" : "false");
        
        _apiLock.Lock();
        _audio.EnableSurroundDecoder(handle, enable);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::IsAudioSurroudDecoderEnabled(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        LOGINFO("IsAudioSurroudDecoderEnabled: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.IsSurroundDecoderEnabled(handle, enabled);
        _apiLock.Unlock();
        
        LOGINFO("IsAudioSurroudDecoderEnabled: handle=%d, enabled=%s", handle, enabled ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioDRCMode(const int32_t handle, const int32_t drcMode) {
        ENTRY_LOG;
        LOGINFO("SetAudioDRCMode: handle=%d, drcMode=%d", handle, drcMode);
        
        _apiLock.Lock();
        _audio.SetDRCMode(handle, drcMode);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioDRCMode(const int32_t handle, int32_t &drcMode) {
        ENTRY_LOG;
        LOGINFO("GetAudioDRCMode: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetDRCMode(handle, drcMode);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioDRCMode: handle=%d, drcMode=%d", handle, drcMode);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioSurroudVirtualizer(const int32_t handle, const SurroundVirtualizer surroundVirtualizer) {
        ENTRY_LOG;
        LOGINFO("SetAudioSurroudVirtualizer: handle=%d, surroundVirtualizer(mode=%d, boost=%d)", handle, surroundVirtualizer.mode, surroundVirtualizer.boost);
        
        _apiLock.Lock();
        _audio.SetSurroundVirtualizer(handle, surroundVirtualizer);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioSurroudVirtualizer(const int32_t handle, SurroundVirtualizer &surroundVirtualizer) {
        ENTRY_LOG;
        LOGINFO("GetAudioSurroudVirtualizer: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetSurroundVirtualizer(handle, surroundVirtualizer);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioSurroudVirtualizer: handle=%d, surroundVirtualizer(mode=%d, boost=%d)", handle, surroundVirtualizer.mode, surroundVirtualizer.boost);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioMISteering(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        LOGINFO("SetAudioMISteering: handle=%d, enable=%s", handle, enable ? "true" : "false");
        
        _apiLock.Lock();
        _audio.SetMISteering(handle, enable);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioMISteering(const int32_t handle, bool &enable) {
        ENTRY_LOG;
        LOGINFO("GetAudioMISteering: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetMISteering(handle, enable);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioMISteering: handle=%d, enable=%s", handle, enable ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioGraphicEqualizerMode(const int32_t handle, const int32_t mode) {
        ENTRY_LOG;
        LOGINFO("SetAudioGraphicEqualizerMode: handle=%d, mode=%d", handle, mode);
        
        _apiLock.Lock();
        // Audio.cpp SetGraphicEqualizerMode requires additional parameters, using defaults
        _audio.SetGraphicEqualizerMode(handle, mode, 0, 0);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioGraphicEqualizerMode(const int32_t handle, int32_t &mode) {
        ENTRY_LOG;
        LOGINFO("GetAudioGraphicEqualizerMode: handle=%d", handle);
        
        _apiLock.Lock();
        int32_t inputGain, outputGain;
        _audio.GetGraphicEqualizerMode(handle, mode, inputGain, outputGain);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioGraphicEqualizerMode: handle=%d, mode=%d", handle, mode);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioMS12ProfileList(const int32_t handle, WPEFramework::Exchange::IDeviceSettingsManagerAudio::IDeviceSettingsAudioMS12AudioProfileIterator*& ms12ProfileList) const {
        ENTRY_LOG;
        LOGINFO("GetAudioMS12ProfileList: handle=%d", handle);
        
        // This method requires iterator implementation - returning not implemented for now
        ms12ProfileList = nullptr;
        
        EXIT_LOG;
        return Core::ERROR_UNAVAILABLE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioMS12Profile(const int32_t handle, string &profile) {
        ENTRY_LOG;
        LOGINFO("GetAudioMS12Profile: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetMS12AudioProfile(handle, profile);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioMS12Profile: handle=%d, profile=%s", handle, profile.c_str());
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioMS12Profile(const int32_t handle, const string profile) {
        ENTRY_LOG;
        LOGINFO("SetAudioMS12Profile: handle=%d, profile=%s", handle, profile.c_str());
        
        _apiLock.Lock();
        _audio.SetMS12AudioProfile(handle, profile);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioMixerLevels(const int32_t handle, const AudioInput audioInput, const int32_t volume) {
        ENTRY_LOG;
        LOGINFO("SetAudioMixerLevels: handle=%d, audioInput=%d, volume=%d", handle, audioInput, volume);
        
        _apiLock.Lock();
        // Audio.cpp SetAudioMixerLevels has different parameters - using available method
        _audio.SetAudioMixerLevels(handle, volume, volume);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAssociatedAudioMixing(const int32_t handle, const bool mixing) {
        ENTRY_LOG;
        LOGINFO("SetAssociatedAudioMixing: handle=%d, mixing=%s", handle, mixing ? "true" : "false");
        
        _apiLock.Lock();
        _audio.SetAssociatedAudioMixing(handle, mixing);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAssociatedAudioMixing(const int32_t handle, bool &mixing) {
        ENTRY_LOG;
        LOGINFO("GetAssociatedAudioMixing: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetAssociatedAudioMixing(handle, mixing);
        _apiLock.Unlock();
        
        LOGINFO("GetAssociatedAudioMixing: handle=%d, mixing=%s", handle, mixing ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioFaderControl(const int32_t handle, const int32_t mixerBalance) {
        ENTRY_LOG;
        LOGINFO("SetAudioFaderControl: handle=%d, mixerBalance=%d", handle, mixerBalance);
        
        _apiLock.Lock();
        _audio.SetFaderControl(handle, mixerBalance);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioFaderControl(const int32_t handle, int32_t &mixerBalance) {
        ENTRY_LOG;
        LOGINFO("GetAudioFaderControl: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetFaderControl(handle, mixerBalance);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioFaderControl: handle=%d, mixerBalance=%d", handle, mixerBalance);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioPrimaryLanguage(const int32_t handle, const string primaryAudioLanguage) {
        ENTRY_LOG;
        LOGINFO("SetAudioPrimaryLanguage: handle=%d, primaryAudioLanguage=%s", handle, primaryAudioLanguage.c_str());
        
        _apiLock.Lock();
        _audio.SetPrimaryLanguage(handle, primaryAudioLanguage);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioPrimaryLanguage(const int32_t handle, string &primaryAudioLanguage) {
        ENTRY_LOG;
        LOGINFO("GetAudioPrimaryLanguage: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetPrimaryLanguage(handle, primaryAudioLanguage);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioPrimaryLanguage: handle=%d, primaryAudioLanguage=%s", handle, primaryAudioLanguage.c_str());
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioSecondaryLanguage(const int32_t handle, const string secondaryAudioLanguage) {
        ENTRY_LOG;
        LOGINFO("SetAudioSecondaryLanguage: handle=%d, secondaryAudioLanguage=%s", handle, secondaryAudioLanguage.c_str());
        
        _apiLock.Lock();
        _audio.SetSecondaryLanguage(handle, secondaryAudioLanguage);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioSecondaryLanguage(const int32_t handle, string &secondaryAudioLanguage) {
        ENTRY_LOG;
        LOGINFO("GetAudioSecondaryLanguage: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetSecondaryLanguage(handle, secondaryAudioLanguage);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioSecondaryLanguage: handle=%d, secondaryAudioLanguage=%s", handle, secondaryAudioLanguage.c_str());
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioCapabilities(const int32_t handle, int32_t &capabilities) {
        ENTRY_LOG;
        LOGINFO("GetAudioCapabilities: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetAudioCapabilities(handle, capabilities);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioCapabilities: handle=%d, capabilities=%d", handle, capabilities);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioMS12Capabilities(const int32_t handle, int32_t &capabilities) {
        ENTRY_LOG;
        LOGINFO("GetAudioMS12Capabilities: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.GetMS12Capabilities(handle, capabilities);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioMS12Capabilities: handle=%d, capabilities=%d", handle, capabilities);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioMS12SettingsOverride(const int32_t handle, const string profileName, const string profileSettingsName, const string profileSettingValue, const string profileState) {
        ENTRY_LOG;
        LOGINFO("SetAudioMS12SettingsOverride: handle=%d, profileName=%s, profileSettingsName=%s, profileSettingValue=%s, profileState=%s", 
               handle, profileName.c_str(), profileSettingsName.c_str(), profileSettingValue.c_str(), profileState.c_str());
        
        _apiLock.Lock();
        // Audio.cpp SetMS12SettingsOverride has different parameters - using available method
        _audio.SetMS12SettingsOverride(handle, profileSettingsName, profileSettingValue);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::IsAudioOutputConnected(const int32_t handle, bool &isConnected) {
        ENTRY_LOG;
        LOGINFO("IsAudioOutputConnected: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.IsAudioOutputConnected(handle, isConnected);
        _apiLock.Unlock();
        
        LOGINFO("IsAudioOutputConnected: handle=%d, isConnected=%s", handle, isConnected ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::ResetAudioDialogEnhancement(const int32_t handle) {
        ENTRY_LOG;
        LOGINFO("ResetAudioDialogEnhancement: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.ResetDialogEnhancement(handle);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::ResetAudioBassEnhancer(const int32_t handle) {
        ENTRY_LOG;
        LOGINFO("ResetAudioBassEnhancer: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.ResetBassEnhancer(handle);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::ResetAudioSurroundVirtualizer(const int32_t handle) {
        ENTRY_LOG;
        LOGINFO("ResetAudioSurroundVirtualizer: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.ResetSurroundVirtualizer(handle);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::ResetAudioVolumeLeveller(const int32_t handle) {
        ENTRY_LOG;
        LOGINFO("ResetAudioVolumeLeveller: handle=%d", handle);
        
        _apiLock.Lock();
        _audio.ResetVolumeLeveller(handle);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioHDMIARCPortId(const int32_t handle, int32_t &portId) {
        ENTRY_LOG;
        LOGINFO("GetAudioHDMIARCPortId: handle=%d", handle);
        
        _apiLock.Lock();
        uint32_t uPortId;
        _audio.GetHDMIARCPortId(uPortId);
        portId = static_cast<int32_t>(uPortId);
        _apiLock.Unlock();
        
        LOGINFO("GetAudioHDMIARCPortId: handle=%d, portId=%d", handle, portId);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    // ================================
    // Audio Event Handlers
    // ================================

    void DeviceSettingsManagerImp::OnAssociatedAudioMixingChangedNotification(bool mixing) {
        LOGINFO("OnAssociatedAudioMixingChanged: mixing=%s", mixing ? "true" : "false");
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAssociatedAudioMixingChanged, mixing);
    }

    void DeviceSettingsManagerImp::OnAudioOutHotPlugNotification(AudioPortType portType, uint32_t uiPortNumber, bool isPortConnected) {
        LOGINFO("OnAudioOutHotPlug: portType=%d, portNumber=%u, connected=%s", portType, uiPortNumber, isPortConnected ? "true" : "false");
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioOutHotPlug, portType, uiPortNumber, isPortConnected);
    }

    void DeviceSettingsManagerImp::OnAudioFormatUpdateNotification(AudioFormat audioFormat) {
        LOGINFO("OnAudioFormatUpdate: audioFormat=%d", audioFormat);
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioFormatUpdate, audioFormat);
    }

    void DeviceSettingsManagerImp::OnAudioLevelChangedEventNotification(int32_t audioLevel) {
        LOGINFO("OnAudioLevelChangedEvent: audioLevel=%d", audioLevel);
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioLevelChangedEvent, audioLevel);
    }

    void DeviceSettingsManagerImp::OnAudioFaderControlChangedNotification(int32_t mixerBalance) {
        LOGINFO("OnAudioFaderControlChanged: mixerBalance=%d", mixerBalance);
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioFaderControlChanged, mixerBalance);
    }

    void DeviceSettingsManagerImp::OnAudioPrimaryLanguageChangedNotification(const string& primaryLanguage) {
        LOGINFO("OnAudioPrimaryLanguageChanged: primaryLanguage=%s", primaryLanguage.c_str());
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioPrimaryLanguageChanged, primaryLanguage);
    }

    void DeviceSettingsManagerImp::OnAudioSecondaryLanguageChangedNotification(const string& secondaryLanguage) {
        LOGINFO("OnAudioSecondaryLanguageChanged: secondaryLanguage=%s", secondaryLanguage.c_str());
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioSecondaryLanguageChanged, secondaryLanguage);
    }

    void DeviceSettingsManagerImp::OnDolbyAtmosCapabilitiesChangedNotification(DolbyAtmosCapability atmosCapability, bool status) {
        LOGINFO("OnDolbyAtmosCapabilitiesChanged: atmosCapability=%d, status=%s", atmosCapability, status ? "true" : "false");
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnDolbyAtmosCapabilitiesChanged, atmosCapability, status);
    }

    void DeviceSettingsManagerImp::OnAudioPortStateChangedNotification(AudioPortState audioPortState) {
        LOGINFO("OnAudioPortStateChanged: audioPortState=%d", audioPortState);
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioPortStateChanged, audioPortState);
    }

    void DeviceSettingsManagerImp::OnAudioModeEventNotification(AudioPortType audioPortType, StereoModes audioMode) {
        LOGINFO("OnAudioModeEvent: audioPortType=%d, audioMode=%d", audioPortType, audioMode);
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioModeEvent, audioPortType, audioMode);
    }

} // namespace Plugin
} // namespace WPEFramework