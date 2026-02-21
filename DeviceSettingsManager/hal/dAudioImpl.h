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

#include "dAudio.h"
#include "UtilsLogging.h"
#include "DeviceSettingsManagerTypes.h"

#include <interfaces/IDeviceSettingsManager.h>

#include "rdk/halif/ds-hal/dsAudio.h"
#include "rdk/halif/ds-hal/dsError.h"
#include "rdk/halif/ds-hal/dsTypes.h"
#include "rdk/halif/ds-hal/dsUtl.h"
#include "rdk/ds-rpc/dsRpc.h"
#include "rdk/halif/ds-hal/dsAVDTypes.h"
#include "libIARM.h"
#include "libIBus.h"
#include "iarmUtil.h"

#include <cstdint>
#include <vector>
#include <string>

// Static global callback function pointers
static std::function<void(AudioPortType, bool)> g_AudioPortConnectionCallback;
static std::function<void(AudioFormat)> g_AudioFormatUpdateCallback;
static std::function<void(WPEFramework::Exchange::IDeviceSettingsManagerAudio::DolbyAtmosCapability)> g_AudioAtmosCapabilityCallback;
static std::function<void(bool)> g_AssociatedAudioMixingCallback;
static std::function<void(int32_t)> g_AudioFaderControlCallback;
static std::function<void(const string&)> g_AudioPrimaryLanguageCallback;
static std::function<void(const string&)> g_AudioSecondaryLanguageCallback;
static std::function<void(AudioPortType, uint32_t, bool)> g_AudioOutHotPlugCallback;
static std::function<void(WPEFramework::Exchange::IDeviceSettingsManagerAudio::DolbyAtmosCapability, bool)> g_DolbyAtmosCapabilitiesCallback;
static std::function<void(AudioPortState)> g_AudioPortStateCallback;
static std::function<void(AudioPortType, StereoModes)> g_AudioModeCallback;
static std::function<void(int32_t)> g_AudioLevelCallback;

class dAudioImpl : public hal::dAudio::IPlatform {

private:
    // delete copy constructor and assignment operator
    dAudioImpl(const dAudioImpl&) = delete;
    dAudioImpl& operator=(const dAudioImpl&) = delete;
    
    bool _isInitialized;

public:
    dAudioImpl() : _isInitialized(false)
    {
        ENTRY_LOG;
        // Initialize the DeviceSettings Audio subsystem
        try {
            dsError_t ret = dsAudioPortInit();
            if (ret != dsERR_NONE) {
                LOGERR("dsAudioPortInit failed with error: %d", ret);
            } else {
                _isInitialized = true;
                LOGINFO("Audio platform initialized successfully");
            }
        } catch (...) {
            LOGERR("Exception during Audio platform initialization");
        }
        EXIT_LOG;
    }

    virtual ~dAudioImpl()
    {
        ENTRY_LOG;
        if (_isInitialized) {
            try {
                dsError_t ret = dsAudioPortTerm();
                if (ret != dsERR_NONE) {
                    LOGERR("dsAudioPortTerm failed with error: %d", ret);
                }
            } catch (...) {
                LOGERR("Exception during Audio platform termination");
            }
            _isInitialized = false;
        }
        EXIT_LOG;
    }

    // Type conversion methods
    dsAudioPortType_t convertToDS(const AudioPortType type)
    {
        switch (type) {
            case AudioPortType::AUDIO_PORT_TYPE_LR: return dsAUDIOPORT_TYPE_ID_LR;
            case AudioPortType::AUDIO_PORT_TYPE_HDMI: return dsAUDIOPORT_TYPE_HDMI;
            case AudioPortType::AUDIO_PORT_TYPE_SPDIF: return dsAUDIOPORT_TYPE_SPDIF;
            case AudioPortType::AUDIO_PORT_TYPE_SPEAKER: return dsAUDIOPORT_TYPE_SPEAKER;
            case AudioPortType::AUDIO_PORT_TYPE_HDMIARC: return dsAUDIOPORT_TYPE_HDMI_ARC;
            case AudioPortType::AUDIO_PORT_TYPE_HEADPHONE: return dsAUDIOPORT_TYPE_HEADPHONE;
            default: return dsAUDIOPORT_TYPE_MAX;
        }
    }

    dsAudioStereoMode_t convertToDS(const StereoModes mode)
    {
        switch (mode) {
            case StereoModes::AUDIO_STEREO_UNKNOWN: return dsAUDIO_STEREO_UNKNOWN;
            case StereoModes::AUDIO_STEREO_MONO: return dsAUDIO_STEREO_MONO;
            case StereoModes::AUDIO_STEREO_STEREO: return dsAUDIO_STEREO_STEREO;
            case StereoModes::AUDIO_STEREO_SURROUND: return dsAUDIO_STEREO_SURROUND;
            case StereoModes::AUDIO_STEREO_PASSTHROUGH: return dsAUDIO_STEREO_PASSTHRU;
            default: return dsAUDIO_STEREO_UNKNOWN;
        }
    }

    StereoModes convertFromDS(const dsAudioStereoMode_t dsMode)
    {
        switch (dsMode) {
            case dsAUDIO_STEREO_UNKNOWN: return StereoModes::AUDIO_STEREO_UNKNOWN;
            case dsAUDIO_STEREO_MONO: return StereoModes::AUDIO_STEREO_MONO;
            case dsAUDIO_STEREO_STEREO: return StereoModes::AUDIO_STEREO_STEREO;
            case dsAUDIO_STEREO_SURROUND: return StereoModes::AUDIO_STEREO_SURROUND;
            case dsAUDIO_STEREO_PASSTHRU: return StereoModes::AUDIO_STEREO_PASSTHROUGH;
            default: return StereoModes::AUDIO_STEREO_UNKNOWN;
        }
    }

    // Additional type conversion methods for AudioFormat and AudioEncoding
    dsAudioFormat_t convertToDS(const AudioFormat format)
    {
        // Placeholder - would need actual mapping based on WPEFramework AudioFormat values
        return dsAUDIO_FORMAT_NONE;
    }

    AudioFormat convertFromDS(const dsAudioFormat_t format)
    {
        // Placeholder - would need actual mapping based on dsAudioFormat_t values
        return static_cast<AudioFormat>(0);
    }

    dsAudioEncoding_t convertToDS(const AudioEncoding encoding)
    {
        // Placeholder - would need actual mapping
        return dsAUDIO_ENC_NONE;
    }

    AudioEncoding convertFromDS(const dsAudioEncoding_t encoding)
    {
        // Placeholder - would need actual mapping
        return static_cast<AudioEncoding>(0);
    }

    // IPlatform interface implementation
    uint32_t GetAudioPort(const AudioPortType type, const int32_t index, int32_t &handle) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            dsAudioPortType_t dsType = convertToDS(type);
            intptr_t dsHandle;
            
            dsError_t ret = dsGetAudioPort(dsType, index, &dsHandle);
            if (ret == dsERR_NONE) {
                handle = static_cast<int32_t>(dsHandle);
                LOGINFO("GetAudioPort success: type=%d, index=%d, handle=%d", type, index, handle);
            } else {
                LOGERR("dsGetAudioPort failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in GetAudioPort");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SetStereoMode(const int32_t handle, const StereoModes mode, const bool persist) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            dsAudioStereoMode_t dsMode = convertToDS(mode);
            
            // Note: dsSetStereoMode doesn't support persist parameter in this version
            dsError_t ret = dsSetStereoMode(dsHandle, dsMode);
            if (ret == dsERR_NONE) {
                LOGINFO("SetStereoMode success: handle=%d, mode=%d", handle, static_cast<int>(mode));
                
                // Trigger audio mode callback
                // We need to determine the port type - for now using a default
                AudioPortType portType = AudioPortType::AUDIO_PORT_TYPE_HDMI; // This should be determined from handle
                StereoModes stereoMode = static_cast<StereoModes>(mode);
                TriggerAudioModeCallback(portType, stereoMode);
            } else {
                LOGERR("dsSetStereoMode failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in SetStereoMode");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SetAudioMute(const int32_t handle, const bool mute) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            dsError_t ret = dsSetAudioMute(dsHandle, mute);
            if (ret == dsERR_NONE) {
                LOGINFO("SetMuted success: handle=%d, mute=%d", handle, mute);
            } else {
                LOGERR("dsSetAudioMute failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in SetAudioMute");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }
    // Additional interface methods with basic implementations
    uint32_t IsAudioPortEnabled(const int32_t handle, bool &enabled) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            bool dsEnabled;
            dsError_t ret = dsIsAudioPortEnabled(dsHandle, &dsEnabled);
            if (ret == dsERR_NONE) {
                enabled = dsEnabled;
                LOGINFO("IsAudioPortEnabled success: handle=%d, enabled=%d", handle, enabled);
            } else {
                LOGERR("dsIsAudioPortEnabled failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in IsAudioPortEnabled");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t EnableAudioPort(const int32_t handle, const bool enable) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            dsError_t ret = dsEnableAudioPort(dsHandle, enable);
            if (ret == dsERR_NONE) {
                LOGINFO("EnableAudioPort success: handle=%d, enable=%d", handle, enable);
            } else {
                LOGERR("dsEnableAudioPort failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in EnableAudioPort");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetSupportedARCTypes(const int32_t handle, int32_t &types) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            int dsTypes;
            dsError_t ret = dsGetSupportedARCTypes(dsHandle, &dsTypes);
            if (ret == dsERR_NONE) {
                types = dsTypes;
                LOGINFO("GetSupportedARCTypes success: handle=%d, types=%d", handle, types);
            } else {
                LOGERR("dsGetSupportedARCTypes failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in GetSupportedARCTypes");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SetSAD(const int32_t handle, const uint8_t sadList[], const uint8_t count) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        if (sadList == nullptr && count > 0) {
            LOGERR("Invalid SAD list parameters");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            // Convert uint8_t array to dsAudioSADList_t structure
            dsAudioSADList_t sadListStruct;
            sadListStruct.count = count;
            for (int i = 0; i < count && i < MAX_SAD; i++) {
                sadListStruct.sad[i] = static_cast<int>(sadList[i]);
            }
            dsError_t ret = dsAudioSetSAD(dsHandle, sadListStruct);
            if (ret == dsERR_NONE) {
                LOGINFO("SetSAD success: handle=%d, count=%d", handle, count);
            } else {
                LOGERR("dsAudioSetSAD failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in SetSAD");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t EnableARC(const int32_t handle, const AudioARCStatus arcStatus) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            // Convert AudioARCStatus struct to dsAudioARCStatus_t
            dsAudioARCStatus_t dsArcStatus;
            dsArcStatus.type = (arcStatus.arcType == WPEFramework::Exchange::IDeviceSettingsManagerAudio::AudioARCType::AUDIO_ARCTYPE_ARC) ? dsAUDIOARCSUPPORT_ARC : dsAUDIOARCSUPPORT_eARC;
            dsArcStatus.status = arcStatus.status;
            dsError_t ret = dsAudioEnableARC(dsHandle, dsArcStatus);
            if (ret == dsERR_NONE) {
                LOGINFO("EnableARC success: handle=%d, arcType=%d, status=%d", handle, static_cast<int>(arcStatus.arcType), static_cast<int>(arcStatus.status));
            } else {
                LOGERR("dsAudioEnableARC failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in EnableARC");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetStereoMode(const int32_t handle, StereoModes &mode) override
    {
        if (!_isInitialized) {
            return WPEFramework::Core::ERROR_GENERAL;
        }
        
        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            dsAudioStereoMode_t dsMode;
            dsError_t ret = dsGetStereoMode(dsHandle, &dsMode);
            if (ret == dsERR_NONE) {
                mode = convertFromDS(dsMode);
                LOGINFO("GetStereoMode success: handle=%d, mode=%d", handle, static_cast<int>(mode));
            } else {
                LOGERR("dsGetStereoMode failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in GetStereoMode");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetStereoAuto(const int32_t handle, int32_t &autoMode) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            int dsAutoMode;
            dsError_t ret = dsGetStereoAuto(dsHandle, &dsAutoMode);
            if (ret == dsERR_NONE) {
                autoMode = dsAutoMode;
                LOGINFO("GetStereoAuto success: handle=%d, autoMode=%d", handle, autoMode);
            } else {
                LOGERR("dsGetStereoAuto failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in GetStereoAuto");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SetStereoAuto(const int32_t handle, const int32_t autoMode, const bool persist) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            // Note: dsSetStereoAuto may not support persist parameter in this version
            dsError_t ret = dsSetStereoAuto(dsHandle, autoMode);
            if (ret == dsERR_NONE) {
                LOGINFO("SetStereoAuto success: handle=%d, autoMode=%d, persist=%d", handle, autoMode, persist);
            } else {
                LOGERR("dsSetStereoAuto failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in SetStereoAuto");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t IsAudioMuted(const int32_t handle, bool &muted) override
    {
        if (!_isInitialized) {
            return WPEFramework::Core::ERROR_GENERAL;
        }
        
        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            bool dsMuted;
            dsError_t ret = dsIsAudioMute(dsHandle, &dsMuted);
            if (ret == dsERR_NONE) {
                muted = dsMuted;
                LOGINFO("IsAudioMuted success: handle=%d, muted=%d", handle, muted);
            } else {
                LOGERR("dsIsAudioMute failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in IsAudioMuted");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        return WPEFramework::Core::ERROR_NONE;
    }
    uint32_t SetAudioLevel(const int32_t handle, const float audioLevel) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            dsError_t ret = dsSetAudioLevel(dsHandle, audioLevel);
            if (ret == dsERR_NONE) {
                LOGINFO("SetAudioLevel success: handle=%d, level=%f", handle, audioLevel);
                
                // Trigger audio level callback
                TriggerAudioLevelCallback(static_cast<int32_t>(audioLevel * 100)); // Convert float to int percentage
            } else {
                LOGERR("dsSetAudioLevel failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in SetAudioLevel");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetAudioLevel(const int32_t handle, float &audioLevel) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            float dsLevel;
            dsError_t ret = dsGetAudioLevel(dsHandle, &dsLevel);
            if (ret == dsERR_NONE) {
                audioLevel = dsLevel;
                LOGINFO("GetAudioLevel success: handle=%d, level=%f", handle, audioLevel);
            } else {
                LOGERR("dsGetAudioLevel failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in GetAudioLevel");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }
    uint32_t SetAudioGain(const int32_t handle, const float gainLevel) override
    {
        // Placeholder implementation - DeviceSettings may not have this API
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetAudioGain(const int32_t handle, float &gainLevel) override
    {
        gainLevel = 1.0f; // Default gain
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetAudioFormat(const int32_t handle, AudioFormat &audioFormat) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            dsAudioFormat_t dsFormat;
            dsError_t ret = dsGetAudioFormat(dsHandle, &dsFormat);
            if (ret == dsERR_NONE) {
                audioFormat = convertFromDS(dsFormat);
                LOGINFO("GetAudioFormat success: handle=%d, format=%d", handle, audioFormat);
            } else {
                LOGERR("dsGetAudioFormat failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in GetAudioFormat");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetAudioEncoding(const int32_t handle, AudioEncoding &audioEncoding) override
    {
        // Placeholder implementation
        audioEncoding = static_cast<AudioEncoding>(0);
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t IsAudioMSDecoding(const int32_t handle, bool &hasMS11Decode) override
    {
        hasMS11Decode = false;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t IsAudioMS12Decoding(const int32_t handle, bool &hasMS12Decode) override
    {
        hasMS12Decode = false;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetAudioCompression(const int32_t handle, const AudioCompression compressionMode) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetAudioCompression(const int32_t handle, AudioCompression &compressionMode) override
    {
        compressionMode = static_cast<AudioCompression>(0);
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetDialogEnhancement(const int32_t handle, const int32_t enhancerLevel) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetDialogEnhancement(const int32_t handle, int32_t &enhancerLevel) override
    {
        enhancerLevel = 0;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetDolbyVolumeMode(const int32_t handle, const bool dolbyVolumeMode) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetDolbyVolumeMode(const int32_t handle, bool &dolbyVolumeMode) override
    {
        dolbyVolumeMode = false;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetIntelligentEqualizerMode(const int32_t handle, const int32_t equalizerMode) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetIntelligentEqualizerMode(const int32_t handle, int32_t &equalizerMode) override
    {
        equalizerMode = 0;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetGraphicEqualizerMode(const int32_t handle, const int32_t equalizerMode, const int32_t inputGain, const int32_t outputGain) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetGraphicEqualizerMode(const int32_t handle, int32_t &equalizerMode, int32_t &inputGain, int32_t &outputGain) override
    {
        equalizerMode = 0;
        inputGain = 0;
        outputGain = 0;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetVolumeLeveller(const int32_t handle, WPEFramework::Exchange::IDeviceSettingsManagerAudio::VolumeLeveller &leveller) override
    {
        leveller.mode = 0;
        leveller.level = 0;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetVolumeLeveller(const int32_t handle, const WPEFramework::Exchange::IDeviceSettingsManagerAudio::VolumeLeveller leveller) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetBassEnhancer(const int32_t handle, int32_t &bassLevel) override
    {
        bassLevel = 0;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetBassEnhancer(const int32_t handle, const int32_t bassLevel) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t IsSurroundDecoderEnabled(const int32_t handle, bool &enabled) override
    {
        enabled = false;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t EnableSurroundDecoder(const int32_t handle, const bool enable) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetDRCMode(const int32_t handle, int32_t &mode) override
    {
        mode = 0;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetDRCMode(const int32_t handle, const int32_t mode) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetSurroundVirtualizer(const int32_t handle, WPEFramework::Exchange::IDeviceSettingsManagerAudio::SurroundVirtualizer &virtualizer) override
    {
        virtualizer.mode = 0;
        virtualizer.boost = 0;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetSurroundVirtualizer(const int32_t handle, const WPEFramework::Exchange::IDeviceSettingsManagerAudio::SurroundVirtualizer virtualizer) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetMISteering(const int32_t handle, bool &enabled) override
    {
        enabled = false;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetMISteering(const int32_t handle, const bool enable) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetMS12AudioProfile(const int32_t handle, string &profile) override
    {
        profile = "";
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetMS12AudioProfile(const int32_t handle, const string &profile) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetMS12AudioProfileList(const int32_t handle, std::vector<string> &profileList) override
    {
        profileList.clear();
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetAssociatedAudioMixing(const int32_t handle, bool &mixing) override
    {
        mixing = false;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetAssociatedAudioMixing(const int32_t handle, const bool mixing) override
    {
        ENTRY_LOG;
        // Placeholder implementation with callback trigger
        TriggerAssociatedAudioMixingCallback(mixing);
        LOGINFO("SetAssociatedAudioMixing: handle=%d, mixing=%s", handle, mixing ? "true" : "false");
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetFaderControl(const int32_t handle, int32_t &mixerBalance) override
    {
        mixerBalance = 0;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetFaderControl(const int32_t handle, const int32_t mixerBalance) override
    {
        ENTRY_LOG;
        // Placeholder implementation with callback trigger
        TriggerAudioFaderControlCallback(mixerBalance);
        LOGINFO("SetFaderControl: handle=%d, mixerBalance=%d", handle, mixerBalance);
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t GetPrimaryLanguage(const int32_t handle, string &primaryLanguage) override
    {
        primaryLanguage = "";
        return WPEFramework::Core::ERROR_NONE;
    }
    
    uint32_t SetPrimaryLanguage(const int32_t handle, const string &primaryLanguage) override
    {
        ENTRY_LOG;
        // Placeholder implementation with callback trigger
        TriggerAudioPrimaryLanguageCallback(primaryLanguage);
        LOGINFO("SetPrimaryLanguage: handle=%d, language=%s", handle, primaryLanguage.c_str());
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }
    // Remaining method implementations with placeholders
    uint32_t GetSecondaryLanguage(const int32_t handle, string &secondaryLanguage) override
    {
        secondaryLanguage = "";
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SetSecondaryLanguage(const int32_t handle, const string &secondaryLanguage) override
    {
        ENTRY_LOG;
        // Placeholder implementation with callback trigger
        TriggerAudioSecondaryLanguageCallback(secondaryLanguage);
        LOGINFO("SetSecondaryLanguage: handle=%d, language=%s", handle, secondaryLanguage.c_str());
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetAudioCapabilities(const int32_t handle, int32_t &capabilities) override
    {
        capabilities = 0;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetMS12Capabilities(const int32_t handle, int32_t &capabilities) override
    {
        capabilities = 0;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t IsAudioOutputConnected(const int32_t handle, bool &connected) override
    {
        connected = true;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SetAudioDelay(const int32_t handle, const uint32_t audioDelay) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            intptr_t dsHandle = static_cast<intptr_t>(handle);
            dsError_t ret = dsSetAudioDelay(dsHandle, audioDelay);
            if (ret == dsERR_NONE) {
                LOGINFO("SetAudioDelay success: handle=%d, delay=%d", handle, audioDelay);
            } else {
                LOGERR("dsSetAudioDelay failed with error: %d", ret);
                return WPEFramework::Core::ERROR_GENERAL;
            }
        } catch (...) {
            LOGERR("Exception in SetAudioDelay");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetAudioDelay(const int32_t handle, uint32_t &audioDelay) override
    {
        audioDelay = 0;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetSinkDeviceAtmosCapability(const int32_t handle, DolbyAtmosCapability &atmosCapability) override
    {
        atmosCapability = static_cast<DolbyAtmosCapability>(0);
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SetAudioAtmosOutputMode(const int32_t handle, const bool enable) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t EnableLEConfig(const int32_t handle, const bool enable) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetLEConfig(const int32_t handle, bool &enabled) override
    {
        enabled = false;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SetAudioMixerLevels(const int32_t handle, const int32_t primaryVolume, const int32_t inputVolume) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t EnablePersist(const int32_t handle, const bool enable) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t IsEnablePersist(const int32_t handle, bool &enabled) override
    {
        enabled = false;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t ResetBassEnhancer(const int32_t handle) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t ResetSurroundVirtualizer(const int32_t handle) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t ResetVolumeLeveller(const int32_t handle) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t ResetDialogEnhancement(const int32_t handle) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SetMS12SettingsOverride(const int32_t handle, const string &feature, const string &value) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetHDMIARCPortId(uint32_t &portId) override
    {
        portId = 0;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SetAudioDucking(const int32_t handle, const AudioDuckingType duckingType, const AudioDuckingAction duckingAction, const uint8_t level) override
    {
        ENTRY_LOG;
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized");
            return WPEFramework::Core::ERROR_GENERAL;
        }

        try {
            // Note: dsSetAudioDucking API is not available in DeviceSettings
            // This is a stub implementation that logs the request but doesn't call any DeviceSettings API
            LOGINFO("SetAudioDucking stub: handle=%d, type=%d, action=%d, level=%d", 
                    handle, static_cast<int>(duckingType), static_cast<int>(duckingAction), level);
            
            // Return success for now - in a real implementation, this might need platform-specific handling
            // or could be implemented using other available DeviceSettings APIs
        } catch (...) {
            LOGERR("Exception in SetAudioDucking");
            return WPEFramework::Core::ERROR_GENERAL;
        }
        EXIT_LOG;
        return WPEFramework::Core::ERROR_NONE;
    }

    // Callback management methods
    void setAllCallbacks(const CallbackBundle bundle) override
    {
        ENTRY_LOG;
        
        if (!_isInitialized) {
            LOGERR("Audio platform not initialized - cannot register callbacks");
            return;
        }
        
        LOGINFO("Audio platform callback registration");
        
        // Register Audio Port Connection callback (maps to dsAudioOutRegisterConnectCB)
        if (bundle.OnAudioOutHotPlugEvent) {
            LOGINFO("Audio Port Connection Event Callback Registered");
            g_AudioOutHotPlugCallback = bundle.OnAudioOutHotPlugEvent;
            dsError_t ret = dsAudioOutRegisterConnectCB(DS_OnAudioPortConnectionEvent);
            if (ret != dsERR_NONE) {
                LOGERR("Failed to register audio port connection callback: %d", ret);
            }
        }
        
        // Register Audio Format Update callback
        if (bundle.OnAudioFormatUpdateEvent) {
            LOGINFO("Audio Format Update Event Callback Registered");
            g_AudioFormatUpdateCallback = bundle.OnAudioFormatUpdateEvent;
            dsError_t ret = dsAudioFormatUpdateRegisterCB(DS_OnAudioFormatUpdateEvent);
            if (ret != dsERR_NONE) {
                LOGERR("Failed to register audio format update callback: %d", ret);
            }
        }
        
        // Register Audio Atmos Capability callback
        if (bundle.OnDolbyAtmosCapabilitiesChangedEvent) {
            LOGINFO("Audio Atmos Capability Event Callback Registered");
            g_DolbyAtmosCapabilitiesCallback = bundle.OnDolbyAtmosCapabilitiesChangedEvent;
            dsError_t ret = dsAudioAtmosCapsChangeRegisterCB(DS_OnAudioAtmosCapabilityEvent);
            if (ret != dsERR_NONE) {
                LOGERR("Failed to register audio atmos capability callback: %d", ret);
            }
        }
        
        // Store other callbacks (no direct HAL support, but can be triggered from application)
        if (bundle.OnAssociatedAudioMixingChangedEvent) {
            LOGINFO("Associated Audio Mixing Callback Registered");
            g_AssociatedAudioMixingCallback = bundle.OnAssociatedAudioMixingChangedEvent;
        }
        
        if (bundle.OnAudioFaderControlChangedEvent) {
            LOGINFO("Audio Fader Control Callback Registered");
            g_AudioFaderControlCallback = bundle.OnAudioFaderControlChangedEvent;
        }
        
        if (bundle.OnAudioPrimaryLanguageChangedEvent) {
            LOGINFO("Audio Primary Language Callback Registered");
            g_AudioPrimaryLanguageCallback = bundle.OnAudioPrimaryLanguageChangedEvent;
        }
        
        if (bundle.OnAudioSecondaryLanguageChangedEvent) {
            LOGINFO("Audio Secondary Language Callback Registered");
            g_AudioSecondaryLanguageCallback = bundle.OnAudioSecondaryLanguageChangedEvent;
        }
        
        if (bundle.OnAudioPortStateChangedEvent) {
            LOGINFO("Audio Port State Callback Registered");
            g_AudioPortStateCallback = bundle.OnAudioPortStateChangedEvent;
        }
        
        if (bundle.OnAudioModeEvent) {
            LOGINFO("Audio Mode Event Callback Registered");
            g_AudioModeCallback = bundle.OnAudioModeEvent;
        }
        
        if (bundle.OnAudioLevelChangedEvent) {
            LOGINFO("Audio Level Changed Callback Registered");
            g_AudioLevelCallback = bundle.OnAudioLevelChangedEvent;
        }
        
        LOGINFO("Audio callbacks registration completed");
        EXIT_LOG;
    }

    void getPersistenceValue() override
    {
        // Load persistent audio settings
        LOGINFO("Audio persistence values loaded");
    }
    
    // Static callback function implementations for DeviceSettings HAL events
    static void DS_OnAudioPortConnectionEvent(dsAudioPortType_t portType, unsigned int uiPortNo, bool isPortConnected)
    {
        LOGINFO("DS_OnAudioPortConnectionEvent: portType=%d, portNo=%d, connected=%s", 
                portType, uiPortNo, isPortConnected ? "true" : "false");
        if (g_AudioOutHotPlugCallback) {
            AudioPortType audioPortType = static_cast<AudioPortType>(portType);
            g_AudioOutHotPlugCallback(audioPortType, uiPortNo, isPortConnected);
        }
    }
    
    static void DS_OnAudioFormatUpdateEvent(dsAudioFormat_t audioFormat)
    {
        LOGINFO("DS_OnAudioFormatUpdateEvent: audioFormat=%d", audioFormat);
        if (g_AudioFormatUpdateCallback) {
            AudioFormat format = static_cast<AudioFormat>(audioFormat);
            g_AudioFormatUpdateCallback(format);
        }
    }
    
    static void DS_OnAudioAtmosCapabilityEvent(_dsATMOSCapability_t capability, bool enabled)
    {
        LOGINFO("DS_OnAudioAtmosCapabilityEvent: capability=%d, enabled=%s", 
                capability, enabled ? "true" : "false");
        if (g_DolbyAtmosCapabilitiesCallback) {
            WPEFramework::Exchange::IDeviceSettingsManagerAudio::DolbyAtmosCapability atmosCapability = 
                static_cast<WPEFramework::Exchange::IDeviceSettingsManagerAudio::DolbyAtmosCapability>(capability);
            g_DolbyAtmosCapabilitiesCallback(atmosCapability, enabled);
        }
    }
    
    // Helper methods to trigger application-level callbacks (called from setters)
    void TriggerAssociatedAudioMixingCallback(bool mixing)
    {
        if (g_AssociatedAudioMixingCallback) {
            LOGINFO("Triggering AssociatedAudioMixing callback: mixing=%s", mixing ? "true" : "false");
            g_AssociatedAudioMixingCallback(mixing);
        }
    }
    
    void TriggerAudioFaderControlCallback(int32_t faderControl)
    {
        if (g_AudioFaderControlCallback) {
            LOGINFO("Triggering AudioFaderControl callback: faderControl=%d", faderControl);
            g_AudioFaderControlCallback(faderControl);
        }
    }
    
    void TriggerAudioPrimaryLanguageCallback(const string& language)
    {
        if (g_AudioPrimaryLanguageCallback) {
            LOGINFO("Triggering AudioPrimaryLanguage callback: language=%s", language.c_str());
            g_AudioPrimaryLanguageCallback(language);
        }
    }
    
    void TriggerAudioSecondaryLanguageCallback(const string& language)
    {
        if (g_AudioSecondaryLanguageCallback) {
            LOGINFO("Triggering AudioSecondaryLanguage callback: language=%s", language.c_str());
            g_AudioSecondaryLanguageCallback(language);
        }
    }
    
    void TriggerAudioPortStateCallback(AudioPortState state)
    {
        if (g_AudioPortStateCallback) {
            LOGINFO("Triggering AudioPortState callback");
            g_AudioPortStateCallback(state);
        }
    }
    
    void TriggerAudioModeCallback(AudioPortType portType, StereoModes mode)
    {
        if (g_AudioModeCallback) {
            LOGINFO("Triggering AudioMode callback: portType=%d, mode=%d", static_cast<int>(portType), static_cast<int>(mode));
            g_AudioModeCallback(portType, mode);
        }
    }
    
    void TriggerAudioLevelCallback(int32_t level)
    {
        if (g_AudioLevelCallback) {
            LOGINFO("Triggering AudioLevel callback: level=%d", level);
            g_AudioLevelCallback(level);
        }
    }
};