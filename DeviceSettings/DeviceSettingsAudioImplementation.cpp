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

#include "DeviceSettingsAudioImplementation.h"

#include "UtilsLogging.h"
#include <syscall.h>

using namespace std;

namespace WPEFramework {
namespace Plugin {

    DeviceSettingsAudioImpl::DeviceSettingsAudioImpl()
        : _audio(Audio::Create(*this))
    {
        ENTRY_LOG;
        LOGINFO("DeviceSettingsAudioImpl Constructor - Instance Address: %p", this);
        EXIT_LOG;
    }

    DeviceSettingsAudioImpl::~DeviceSettingsAudioImpl() {
        ENTRY_LOG;
        LOGINFO("DeviceSettingsAudioImpl Destructor - Instance Address: %p", this);
        EXIT_LOG;
    }

    template<typename Func, typename... Args>
    void DeviceSettingsAudioImpl::dispatchAudioEvent(Func notifyFunc, Args&&... args) {
        LOGINFO(">>");
        _callbackLock.Lock();
        for (auto& notification : _AudioNotifications) {
            auto start = std::chrono::steady_clock::now();
            (notification->*notifyFunc)(std::forward<Args>(args)...);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("client %p took %" PRId64 "ms to process IAudio event", notification, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }
        _callbackLock.Unlock();
        LOGINFO("<<");
    }

    template <typename T>
    Core::hresult DeviceSettingsAudioImpl::Register(std::list<T*>& list, T* notification)
    {
        uint32_t status = Core::ERROR_GENERAL;
        ENTRY_LOG;
        ASSERT(nullptr != notification);

        _callbackLock.Lock();
        // Make sure we can't register the same notification callback multiple times
        if (std::find(list.begin(), list.end(), notification) == list.end()) {
            list.push_back(notification);
            notification->AddRef();
            status = Core::ERROR_NONE;
        } else {
            LOGWARN("Notification %p already registered - skipping", notification);
        }
        _callbackLock.Unlock();

        EXIT_LOG;
        return status;
    }

    template <typename T>
    Core::hresult DeviceSettingsAudioImpl::Unregister(std::list<T*>& list, const T* notification)
    {
        uint32_t status = Core::ERROR_GENERAL;
        ENTRY_LOG;
        ASSERT(nullptr != notification);
        _callbackLock.Lock();

        // Make sure we can't unregister the same notification callback multiple times
        auto itr = std::find(list.begin(), list.end(), notification);
        if (itr != list.end()) {
            (*itr)->Release();
            list.erase(itr);
            status = Core::ERROR_NONE;
        }

        _callbackLock.Unlock();
        EXIT_LOG;
        return status;
    }

    Core::hresult DeviceSettingsAudioImpl::Register(DeviceSettingsAudio::INotification* notification)
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

    Core::hresult DeviceSettingsAudioImpl::Unregister(DeviceSettingsAudio::INotification* notification)
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

    // Audio notification implementations - hardware callbacks
    void DeviceSettingsAudioImpl::OnAssociatedAudioMixingChanged(bool mixing)
    {
        ENTRY_LOG;
        LOGINFO("OnAssociatedAudioMixingChanged event Received: mixing=%s", mixing ? "true" : "false");
        dispatchAudioEvent(&DeviceSettingsAudio::INotification::OnAssociatedAudioMixingChanged, mixing);
        EXIT_LOG;
    }

    void DeviceSettingsAudioImpl::OnAudioFaderControlChanged(int32_t mixerBalance)
    {
        ENTRY_LOG;
        LOGINFO("OnAudioFaderControlChanged event Received: mixerBalance=%d", mixerBalance);
        dispatchAudioEvent(&DeviceSettingsAudio::INotification::OnAudioFaderControlChanged, mixerBalance);
        EXIT_LOG;
    }

    void DeviceSettingsAudioImpl::OnAudioPrimaryLanguageChanged(const std::string& primaryLanguage)
    {
        ENTRY_LOG;
        LOGINFO("OnAudioPrimaryLanguageChanged event Received: primaryLanguage=%s", primaryLanguage.c_str());
        dispatchAudioEvent(&DeviceSettingsAudio::INotification::OnAudioPrimaryLanguageChanged, primaryLanguage);
        EXIT_LOG;
    }

    void DeviceSettingsAudioImpl::OnAudioSecondaryLanguageChanged(const std::string& secondaryLanguage)
    {
        ENTRY_LOG;
        LOGINFO("OnAudioSecondaryLanguageChanged event Received: secondaryLanguage=%s", secondaryLanguage.c_str());
        dispatchAudioEvent(&DeviceSettingsAudio::INotification::OnAudioSecondaryLanguageChanged, secondaryLanguage);
        EXIT_LOG;
    }

    void DeviceSettingsAudioImpl::OnAudioOutHotPlug(AudioPortType portType, uint32_t uiPortNumber, bool isPortConnected)
    {
        ENTRY_LOG;
        LOGINFO("OnAudioOutHotPlug event Received: portType=%d, port=%u, connected=%s", portType, uiPortNumber, isPortConnected ? "true" : "false");
        dispatchAudioEvent(&DeviceSettingsAudio::INotification::OnAudioOutHotPlug, portType, uiPortNumber, isPortConnected);
        EXIT_LOG;
    }

    void DeviceSettingsAudioImpl::OnAudioFormatUpdate(AudioFormat audioFormat)
    {
        ENTRY_LOG;
        LOGINFO("OnAudioFormatUpdate event Received: audioFormat=%d", audioFormat);
        dispatchAudioEvent(&DeviceSettingsAudio::INotification::OnAudioFormatUpdate, audioFormat);
        EXIT_LOG;
    }

    void DeviceSettingsAudioImpl::OnDolbyAtmosCapabilitiesChanged(DolbyAtmosCapability atmosCapability, bool status)
    {
        ENTRY_LOG;
        LOGINFO("OnDolbyAtmosCapabilitiesChanged event Received: capability=%d, status=%s", atmosCapability, status ? "true" : "false");
        dispatchAudioEvent(&DeviceSettingsAudio::INotification::OnDolbyAtmosCapabilitiesChanged, atmosCapability, status);
        EXIT_LOG;
    }

    void DeviceSettingsAudioImpl::OnAudioPortStateChanged(AudioPortState audioPortState)
    {
        ENTRY_LOG;
        LOGINFO("OnAudioPortStateChanged event Received: audioPortState=%d", audioPortState);
        dispatchAudioEvent(&DeviceSettingsAudio::INotification::OnAudioPortStateChanged, audioPortState);
        EXIT_LOG;
    }

    void DeviceSettingsAudioImpl::OnAudioLevelChangedEvent(int32_t audioLevel)
    {
        ENTRY_LOG;
        LOGINFO("OnAudioLevelChangedEvent event Received: audioLevel=%d", audioLevel);
        dispatchAudioEvent(&DeviceSettingsAudio::INotification::OnAudioLevelChangedEvent, audioLevel);
        EXIT_LOG;
    }

    void DeviceSettingsAudioImpl::OnAudioModeEvent(AudioPortType audioPortType, AudioStereoMode audioMode)
    {
        ENTRY_LOG;
        LOGINFO("OnAudioModeEvent event Received: portType=%d, mode=%d", audioPortType, audioMode);
        dispatchAudioEvent(&DeviceSettingsAudio::INotification::OnAudioModeEvent, audioPortType, audioMode);
        EXIT_LOG;
    }

    // Audio port management
    Core::hresult DeviceSettingsAudioImpl::GetAudioPort(const AudioPortType type, const int32_t index, int32_t &handle) {
        ENTRY_LOG;
        LOGINFO("GetAudioPort: type=%d, index=%d", type, index);
        uint32_t result = _audio.GetAudioPort(type, index, handle);
        EXIT_LOG;
        return result;
    }

    // GetAudioPorts and GetSupportedAudioPorts methods removed - iterator type doesn't exist

    Core::hresult DeviceSettingsAudioImpl::GetAudioPortConfig(const AudioPortType audioPort, AudioConfig &audioConfig) {
        ENTRY_LOG;
        LOGINFO("GetAudioPortConfig: audioPort=%d", audioPort);
        uint32_t result = _audio.GetAudioPortConfig(audioPort, audioConfig);
        EXIT_LOG;
        return result;
    }

    // Audio capabilities
    Core::hresult DeviceSettingsAudioImpl::GetAudioCapabilities(const int32_t handle, int32_t &capabilities) {
        ENTRY_LOG;
        LOGINFO("GetAudioCapabilities: handle=%d", handle);
        uint32_t result = _audio.GetAudioCapabilities(handle, capabilities);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::GetAudioMS12Capabilities(const int32_t handle, int32_t &capabilities) {
        ENTRY_LOG;
        LOGINFO("GetAudioMS12Capabilities: handle=%d", handle);
        uint32_t result = _audio.GetAudioMS12Capabilities(handle, capabilities);
        EXIT_LOG;
        return result;
    }

    // Audio format and encoding
    Core::hresult DeviceSettingsAudioImpl::GetAudioFormat(const int32_t handle, AudioFormat &audioFormat) {
        ENTRY_LOG;
        LOGINFO("GetAudioFormat: handle=%d", handle);
        uint32_t result = _audio.GetAudioFormat(handle, audioFormat);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::GetAudioEncoding(const int32_t handle, AudioEncoding &encoding) {
        ENTRY_LOG;
        LOGINFO("GetAudioEncoding: handle=%d", handle);
        uint32_t result = _audio.GetAudioEncoding(handle, encoding);
        EXIT_LOG;
        return result;
    }

    // Audio level and volume control
    Core::hresult DeviceSettingsAudioImpl::SetAudioLevel(const int32_t handle, const float audioLevel) {
        ENTRY_LOG;
        LOGINFO("SetAudioLevel: handle=%d, audioLevel=%.2f", handle, audioLevel);
        uint32_t result = _audio.SetAudioLevel(handle, audioLevel);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::GetAudioLevel(const int32_t handle, float &audioLevel) {
        ENTRY_LOG;
        LOGINFO("GetAudioLevel: handle=%d", handle);
        uint32_t result = _audio.GetAudioLevel(handle, audioLevel);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::SetAudioGain(const int32_t handle, const float gainLevel) {
        ENTRY_LOG;
        LOGINFO("SetAudioGain: handle=%d, gainLevel=%.2f", handle, gainLevel);
        uint32_t result = _audio.SetAudioGain(handle, gainLevel);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::GetAudioGain(const int32_t handle, float &gainLevel) {
        ENTRY_LOG;
        LOGINFO("GetAudioGain: handle=%d", handle);
        uint32_t result = _audio.GetAudioGain(handle, gainLevel);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::SetAudioMute(const int32_t handle, const bool mute) {
        ENTRY_LOG;
        LOGINFO("SetAudioMute: handle=%d, mute=%s", handle, mute ? "true" : "false");
        uint32_t result = _audio.SetAudioMute(handle, mute);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::IsAudioMuted(const int32_t handle, bool &muted) {
        ENTRY_LOG;
        LOGINFO("IsAudioMuted: handle=%d", handle);
        uint32_t result = _audio.IsAudioMuted(handle, muted);
        EXIT_LOG;
        return result;
    }

    // Audio ducking
    Core::hresult DeviceSettingsAudioImpl::SetAudioDucking(const int32_t handle, const AudioDuckingType duckingType, const AudioDuckingAction duckingAction, const uint8_t level) {
        ENTRY_LOG;
        LOGINFO("SetAudioDucking: handle=%d, duckingType=%d, duckingAction=%d, level=%d", handle, duckingType, duckingAction, level);
        uint32_t result = _audio.SetAudioDucking(handle, duckingType, duckingAction, level);
        EXIT_LOG;
        return result;
    }

    // Stereo mode
    Core::hresult DeviceSettingsAudioImpl::GetStereoMode(const int32_t handle, AudioStereoMode &mode) {
        ENTRY_LOG;
        LOGINFO("GetStereoMode: handle=%d", handle);
        uint32_t result = _audio.GetStereoMode(handle, mode);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::SetStereoMode(const int32_t handle, const AudioStereoMode mode, const bool persist) {
        ENTRY_LOG;
        LOGINFO("SetStereoMode: handle=%d, mode=%d, persist=%s", handle, mode, persist ? "true" : "false");
        uint32_t result = _audio.SetStereoMode(handle, mode, persist);
        EXIT_LOG;
        return result;
    }

    // Associated audio mixing
    Core::hresult DeviceSettingsAudioImpl::SetAssociatedAudioMixing(const int32_t handle, const bool mixing) {
        ENTRY_LOG;
        LOGINFO("SetAssociatedAudioMixing: handle=%d, mixing=%s", handle, mixing ? "true" : "false");
        uint32_t result = _audio.SetAssociatedAudioMixing(handle, mixing);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::GetAssociatedAudioMixing(const int32_t handle, bool &mixing) {
        ENTRY_LOG;
        LOGINFO("GetAssociatedAudioMixing: handle=%d", handle);
        uint32_t result = _audio.GetAssociatedAudioMixing(handle, mixing);
        EXIT_LOG;
        return result;
    }

    // Audio fader control
    Core::hresult DeviceSettingsAudioImpl::SetAudioFaderControl(const int32_t handle, const int32_t mixerBalance) {
        ENTRY_LOG;
        LOGINFO("SetAudioFaderControl: handle=%d, mixerBalance=%d", handle, mixerBalance);
        uint32_t result = _audio.SetAudioFaderControl(handle, mixerBalance);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::GetAudioFaderControl(const int32_t handle, int32_t &mixerBalance) {
        ENTRY_LOG;
        LOGINFO("GetAudioFaderControl: handle=%d", handle);
        uint32_t result = _audio.GetAudioFaderControl(handle, mixerBalance);
        EXIT_LOG;
        return result;
    }

    // Audio language settings
    Core::hresult DeviceSettingsAudioImpl::SetAudioPrimaryLanguage(const int32_t handle, const std::string primaryAudioLanguage) {
        ENTRY_LOG;
        LOGINFO("SetAudioPrimaryLanguage: handle=%d, primaryAudioLanguage=%s", handle, primaryAudioLanguage.c_str());
        uint32_t result = _audio.SetAudioPrimaryLanguage(handle, primaryAudioLanguage);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::GetAudioPrimaryLanguage(const int32_t handle, std::string &primaryAudioLanguage) {
        ENTRY_LOG;
        LOGINFO("GetAudioPrimaryLanguage: handle=%d", handle);
        uint32_t result = _audio.GetAudioPrimaryLanguage(handle, primaryAudioLanguage);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::SetAudioSecondaryLanguage(const int32_t handle, const std::string secondaryAudioLanguage) {
        ENTRY_LOG;
        LOGINFO("SetAudioSecondaryLanguage: handle=%d, secondaryAudioLanguage=%s", handle, secondaryAudioLanguage.c_str());
        uint32_t result = _audio.SetAudioSecondaryLanguage(handle, secondaryAudioLanguage);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::GetAudioSecondaryLanguage(const int32_t handle, std::string &secondaryAudioLanguage) {
        ENTRY_LOG;
        LOGINFO("GetAudioSecondaryLanguage: handle=%d", handle);
        uint32_t result = _audio.GetAudioSecondaryLanguage(handle, secondaryAudioLanguage);
        EXIT_LOG;
        return result;
    }

    // Output connection status
    Core::hresult DeviceSettingsAudioImpl::IsAudioOutputConnected(const int32_t handle, bool &isConnected) {
        ENTRY_LOG;
        LOGINFO("IsAudioOutputConnected: handle=%d", handle);
        uint32_t result = _audio.IsAudioOutputConnected(handle, isConnected);
        EXIT_LOG;
        return result;
    }

    // Dolby Atmos
    Core::hresult DeviceSettingsAudioImpl::GetAudioSinkDeviceAtmosCapability(const int32_t handle, DolbyAtmosCapability &atmosCapability) {
        ENTRY_LOG;
        LOGINFO("GetAudioSinkDeviceAtmosCapability: handle=%d", handle);
        uint32_t result = _audio.GetAudioSinkDeviceAtmosCapability(handle, atmosCapability);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::SetAudioAtmosOutputMode(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        LOGINFO("SetAudioAtmosOutputMode: handle=%d, enable=%s", handle, enable ? "true" : "false");
        uint32_t result = _audio.SetAudioAtmosOutputMode(handle, enable);
        EXIT_LOG;
        return result;
    }

    // Stub implementations for compression methods
    Core::hresult DeviceSettingsAudioImpl::GetSupportedCompressions(const int32_t handle, IDeviceSettingsAudioCompressionIterator*& compressions) {
        ENTRY_LOG;
        LOGINFO("GetSupportedCompressions: handle=%d - STUB IMPLEMENTATION", handle);
        uint32_t result = _audio.GetSupportedCompressions(handle, compressions);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::GetCompression(const int32_t handle, AudioCompression &compression) {
        ENTRY_LOG;
        LOGINFO("GetCompression: handle=%d - STUB IMPLEMENTATION", handle);
        uint32_t result = _audio.GetCompression(handle, compression);
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::SetCompression(const int32_t handle, const AudioCompression compression) {
        ENTRY_LOG;
        LOGINFO("SetCompression: handle=%d, compression=%d - STUB IMPLEMENTATION", handle, compression);
        uint32_t result = _audio.SetCompression(handle, compression);
        EXIT_LOG;
        return result;
    }

    // Additional stub implementations for other methods would go here
    Core::hresult DeviceSettingsAudioImpl::GetMS12Capabilities(const int32_t handle, IDeviceSettingsAudioCompressionIterator*& compressions) {
        ENTRY_LOG;
        LOGINFO("GetMS12Capabilities: handle=%d - STUB IMPLEMENTATION", handle);
        uint32_t result = WPEFramework::Core::ERROR_GENERAL;
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::GetStereoAuto(const int32_t handle, int32_t &mode) {
        ENTRY_LOG;
        LOGINFO("GetStereoAuto: handle=%d - STUB IMPLEMENTATION", handle);
        uint32_t result = WPEFramework::Core::ERROR_GENERAL;
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsAudioImpl::SetStereoAuto(const int32_t handle, const int32_t mode, const bool persist) {
        ENTRY_LOG;
        LOGINFO("SetStereoAuto: handle=%d, mode=%d, persist=%s - STUB IMPLEMENTATION", handle, mode, persist ? "true" : "false");
        uint32_t result = WPEFramework::Core::ERROR_GENERAL;
        EXIT_LOG;
        return result;
    }

    // Missing Audio interface methods implementation
    
    Core::hresult DeviceSettingsAudioImpl::IsAudioPortEnabled(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        uint32_t result = _audio.IsAudioPortEnabled(handle, enabled);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::EnableAudioPort(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        uint32_t result = _audio.EnableAudioPort(handle, enable);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetSupportedARCTypes(const int32_t handle, int32_t &types) {
        ENTRY_LOG;
        uint32_t result = _audio.GetSupportedARCTypes(handle, types);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetSAD(const int32_t handle, const uint8_t sadList[], const uint8_t count) {
        ENTRY_LOG;
        uint32_t result = _audio.SetSAD(handle, sadList, count);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::EnableARC(const int32_t handle, const AudioARCStatus arcStatus) {
        ENTRY_LOG;
        uint32_t result = _audio.EnableARC(handle, arcStatus);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioEnablePersist(const int32_t handle, bool &enabled, string &portName) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioEnablePersist(handle, enabled, portName);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioEnablePersist(const int32_t handle, const bool enable, const string portName) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioEnablePersist(handle, enable, portName);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::IsAudioMSDecoded(const int32_t handle, bool &hasms11Decode) {
        ENTRY_LOG;
        uint32_t result = _audio.IsAudioMSDecoded(handle, hasms11Decode);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::IsAudioMS12Decoded(const int32_t handle, bool &hasms12Decode) {
        ENTRY_LOG;
        uint32_t result = _audio.IsAudioMS12Decoded(handle, hasms12Decode);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioLEConfig(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioLEConfig(handle, enabled);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::EnableAudioLEConfig(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        uint32_t result = _audio.EnableAudioLEConfig(handle, enable);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioDelay(const int32_t handle, const uint32_t audioDelay) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioDelay(handle, audioDelay);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioDelay(const int32_t handle, uint32_t &audioDelay) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioDelay(handle, audioDelay);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioDelayOffset(const int32_t handle, const uint32_t delayOffset) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioDelayOffset(handle, delayOffset);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioDelayOffset(const int32_t handle, uint32_t &delayOffset) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioDelayOffset(handle, delayOffset);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioCompression(const int32_t handle, const int32_t compressionLevel) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioCompression(handle, compressionLevel);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioCompression(const int32_t handle, int32_t &compressionLevel) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioCompression(handle, compressionLevel);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioDialogEnhancement(const int32_t handle, const int32_t level) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioDialogEnhancement(handle, level);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioDialogEnhancement(const int32_t handle, int32_t &level) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioDialogEnhancement(handle, level);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioDolbyVolumeMode(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioDolbyVolumeMode(handle, enable);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioDolbyVolumeMode(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioDolbyVolumeMode(handle, enabled);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioIntelligentEqualizerMode(const int32_t handle, const int32_t mode) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioIntelligentEqualizerMode(handle, mode);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioIntelligentEqualizerMode(const int32_t handle, int32_t &mode) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioIntelligentEqualizerMode(handle, mode);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioVolumeLeveller(const int32_t handle, const VolumeLeveller volumeLeveller) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioVolumeLeveller(handle, volumeLeveller);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioVolumeLeveller(const int32_t handle, VolumeLeveller &volumeLeveller) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioVolumeLeveller(handle, volumeLeveller);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioBassEnhancer(const int32_t handle, const int32_t boost) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioBassEnhancer(handle, boost);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioBassEnhancer(const int32_t handle, int32_t &boost) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioBassEnhancer(handle, boost);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::EnableAudioSurroudDecoder(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        uint32_t result = _audio.EnableAudioSurroudDecoder(handle, enable);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::IsAudioSurroudDecoderEnabled(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        uint32_t result = _audio.IsAudioSurroudDecoderEnabled(handle, enabled);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioDRCMode(const int32_t handle, const int32_t drcMode) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioDRCMode(handle, drcMode);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioDRCMode(const int32_t handle, int32_t &drcMode) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioDRCMode(handle, drcMode);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioSurroudVirtualizer(const int32_t handle, const SurroundVirtualizer surroundVirtualizer) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioSurroudVirtualizer(handle, surroundVirtualizer);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioSurroudVirtualizer(const int32_t handle, SurroundVirtualizer &surroundVirtualizer) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioSurroudVirtualizer(handle, surroundVirtualizer);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioMISteering(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioMISteering(handle, enable);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioMISteering(const int32_t handle, bool &enable) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioMISteering(handle, enable);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioGraphicEqualizerMode(const int32_t handle, const int32_t mode) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioGraphicEqualizerMode(handle, mode);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioGraphicEqualizerMode(const int32_t handle, int32_t &mode) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioGraphicEqualizerMode(handle, mode);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioMS12ProfileList(const int32_t handle, IDeviceSettingsAudioMS12AudioProfileIterator*& ms12ProfileList) const {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioMS12ProfileList(handle, ms12ProfileList);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioMS12Profile(const int32_t handle, string &profile) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioMS12Profile(handle, profile);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioMS12Profile(const int32_t handle, const string profile) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioMS12Profile(handle, profile);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioMixerLevels(const int32_t handle, const AudioInput audioInput, const int32_t volume) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioMixerLevels(handle, audioInput, volume);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::SetAudioMS12SettingsOverride(const int32_t handle, const string profileName, const string profileSettingsName, const string profileSettingValue, const string profileState) {
        ENTRY_LOG;
        uint32_t result = _audio.SetAudioMS12SettingsOverride(handle, profileName, profileSettingsName, profileSettingValue, profileState);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::ResetAudioDialogEnhancement(const int32_t handle) {
        ENTRY_LOG;
        uint32_t result = _audio.ResetAudioDialogEnhancement(handle);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::ResetAudioBassEnhancer(const int32_t handle) {
        ENTRY_LOG;
        uint32_t result = _audio.ResetAudioBassEnhancer(handle);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::ResetAudioSurroundVirtualizer(const int32_t handle) {
        ENTRY_LOG;
        uint32_t result = _audio.ResetAudioSurroundVirtualizer(handle);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::ResetAudioVolumeLeveller(const int32_t handle) {
        ENTRY_LOG;
        uint32_t result = _audio.ResetAudioVolumeLeveller(handle);
        EXIT_LOG;
        return result;
    }
    
    Core::hresult DeviceSettingsAudioImpl::GetAudioHDMIARCPortId(const int32_t handle, int32_t &portId) {
        ENTRY_LOG;
        uint32_t result = _audio.GetAudioHDMIARCPortId(handle, portId);
        EXIT_LOG;
        return result;
    }

} // namespace Plugin
} // namespace WPEFramework