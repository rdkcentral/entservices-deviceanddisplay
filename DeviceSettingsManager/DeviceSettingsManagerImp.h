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

#pragma once

#include "Module.h"

#include <memory>
#include <unordered_map>
#include <chrono>
#include <cstdint>
#include <type_traits>

#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>

#include <interfaces/IDeviceSettingsManager.h>

#include "fpd.h"
#include "HdmiIn.h"
#include "Audio.h"

#include "list.hpp"
#include "DeviceSettingsManagerTypes.h"

// Forward declaration for static assertions
namespace WPEFramework {
namespace Plugin {
    class DeviceSettingsManagerImp;
}
}

namespace WPEFramework {
namespace Plugin {
    class DeviceSettingsManagerImp :
                                     public DeviceSettingsManagerFPD
#ifndef USE_LEGACY_INTERFACE
                                     , public Exchange::IDeviceSettingsManager
#endif
                                   , public DeviceSettingsManagerHDMIIn
                                   , public DeviceSettingsManagerAudio
                                   , public HdmiIn::INotification
                                   , public FPD::INotification
                                   , public Audio::INotification
                                   , public Exchange::IDeviceSettingsManagerHDMIIn::INotification
                                   , public Exchange::IDeviceSettingsManagerAudio::INotification
                                   , public Exchange::IDeviceSettingsManagerFPD::INotification
    {
    public:
        // Minimal implementations to satisfy IReferenceCounted
        uint32_t AddRef() const override { return 1; }
        uint32_t Release() const override { return 1; }

        // We do not allow this plugin to be copied !!
        DeviceSettingsManagerImp();
        ~DeviceSettingsManagerImp() override;

        static DeviceSettingsManagerImp* instance(DeviceSettingsManagerImp* DeviceSettingsManagerImpl = nullptr);

        // We do not allow this plugin to be copied !!
        DeviceSettingsManagerImp(const DeviceSettingsManagerImp&)            = delete;
        DeviceSettingsManagerImp& operator=(const DeviceSettingsManagerImp&) = delete;

        BEGIN_INTERFACE_MAP(DeviceSettingsManagerImp)
#ifndef USE_LEGACY_INTERFACE
        INTERFACE_ENTRY(Exchange::IDeviceSettingsManager)
#endif
        INTERFACE_ENTRY(DeviceSettingsManagerFPD)
        INTERFACE_ENTRY(DeviceSettingsManagerHDMIIn)
        INTERFACE_ENTRY(DeviceSettingsManagerAudio)
        END_INTERFACE_MAP

    public:
        class EXTERNAL LambdaJob : public Core::IDispatch {
        protected:
            LambdaJob(DeviceSettingsManagerImp* impl, std::function<void()> lambda)
                : _impl(impl)
                , _lambda(std::move(lambda))
            {
            }

        public:
            LambdaJob()                            = delete;
            LambdaJob(const LambdaJob&)            = delete;
            LambdaJob& operator=(const LambdaJob&) = delete;
            ~LambdaJob() {}

            static Core::ProxyType<Core::IDispatch> Create(DeviceSettingsManagerImp* impl, std::function<void()> lambda)
            {
                return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<LambdaJob>::Create(impl, std::move(lambda))));
            }

            virtual void Dispatch()
            {
                _lambda();
            }

        private:
            DeviceSettingsManagerImp* _impl;
            std::function<void()> _lambda;
        };

    public:
        void DeviceManager_Init();
        void InitializeIARM();

        // FPD methods
        virtual Core::hresult Register(DeviceSettingsManagerFPD::INotification* notification) override;
        virtual Core::hresult Unregister(DeviceSettingsManagerFPD::INotification* notification) override;
        Core::hresult SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) override;
        Core::hresult SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) override;
        Core::hresult SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) override;
        Core::hresult SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) override;
        Core::hresult GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess) override;
        Core::hresult SetFPDState(const FPDIndicator indicator, const FPDState state) override;
        Core::hresult GetFPDState(const FPDIndicator indicator, FPDState &state) override;
        Core::hresult GetFPDColor(const FPDIndicator indicator, uint32_t &color) override;
        Core::hresult SetFPDColor(const FPDIndicator indicator, const uint32_t color) override;
        Core::hresult SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) override;
        Core::hresult GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) override;
        Core::hresult EnableFPDClockDisplay(const bool enable) override;
        Core::hresult GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) override;
        Core::hresult SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) override;
        Core::hresult SetFPDMode(const FPDMode fpdMode) override;

        // HDMIIn methods
        virtual Core::hresult Register(DeviceSettingsManagerHDMIIn::INotification* notification) override;
        virtual Core::hresult Unregister(DeviceSettingsManagerHDMIIn::INotification* notification) override;
        Core::hresult GetHDMIInNumbefOfInputs(int32_t &count) override;
        Core::hresult GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) override;
        Core::hresult SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) override;
        Core::hresult ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) override;
        Core::hresult SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) override;
        Core::hresult GetSupportedGameFeaturesList(DeviceSettingsManagerHDMIIn::IHDMIInGameFeatureListIterator *& gameFeatureList) override;
        Core::hresult GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency) override;
        Core::hresult GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus) override;
        Core::hresult GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) override;
        Core::hresult SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) override;
        Core::hresult GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]) override;
        Core::hresult GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) override;
        Core::hresult GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion) override;
        Core::hresult SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) override;
        Core::hresult GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution) override;
        Core::hresult GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion) override;
        Core::hresult SetVRRSupport(const HDMIInPort port, const bool vrrSupport) override;
        Core::hresult GetVRRSupport(const HDMIInPort port, bool &vrrSupport) override;
        Core::hresult GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus) override;

        // Audio methods
        virtual Core::hresult Register(DeviceSettingsManagerAudio::INotification* notification) override;
        virtual Core::hresult Unregister(DeviceSettingsManagerAudio::INotification* notification) override;
        Core::hresult GetAudioPort(const AudioPortType type, const int32_t index, int32_t &handle) override;
        Core::hresult IsAudioPortEnabled(const int32_t handle, bool &enabled) override;
        Core::hresult EnableAudioPort(const int32_t handle, const bool enable) override;
        Core::hresult GetStereoMode(const int32_t handle, StereoModes &mode) override;
        Core::hresult SetStereoMode(const int32_t handle, const StereoModes mode, const bool persist) override;
        Core::hresult SetAudioMute(const int32_t handle, const bool mute) override;
        Core::hresult IsAudioMuted(const int32_t handle, bool &muted) override;
        Core::hresult GetSupportedARCTypes(const int32_t handle, int32_t &types) override;
        Core::hresult SetSAD(const int32_t handle, const uint8_t sadList[], const uint8_t count) override;
        Core::hresult EnableARC(const int32_t handle, const AudioARCStatus arcStatus) override;
        Core::hresult GetStereoAuto(const int32_t handle, int32_t &mode) override;
        Core::hresult SetStereoAuto(const int32_t handle, const int32_t mode, const bool persist) override;
        Core::hresult SetAudioDucking(const int32_t handle, const AudioDuckingType duckingType, const AudioDuckingAction duckingAction, const uint8_t level) override;
        Core::hresult SetAudioLevel(const int32_t handle, const float audioLevel) override;
        Core::hresult GetAudioLevel(const int32_t handle, float &audioLevel) override;
        Core::hresult SetAudioGain(const int32_t handle, const float gainLevel) override;
        Core::hresult GetAudioGain(const int32_t handle, float &gainLevel) override;
        Core::hresult GetAudioFormat(const int32_t handle, AudioFormat &audioFormat) override;
        Core::hresult GetAudioEncoding(const int32_t handle, AudioEncoding &encoding) override;
        Core::hresult GetAudioEnablePersist(const int32_t handle, bool &enabled, string &portName) override;
        Core::hresult SetAudioEnablePersist(const int32_t handle, const bool enable, const string portName) override;
        Core::hresult IsAudioMSDecoded(const int32_t handle, bool &hasms11Decode) override;
        Core::hresult IsAudioMS12Decoded(const int32_t handle, bool &hasms12Decode) override;
        Core::hresult GetAudioLEConfig(const int32_t handle, bool &enabled) override;
        Core::hresult EnableAudioLEConfig(const int32_t handle, const bool enable) override;
        Core::hresult SetAudioDelay(const int32_t handle, const uint32_t audioDelay) override;
        Core::hresult GetAudioDelay(const int32_t handle, uint32_t &audioDelay) override;
        Core::hresult SetAudioDelayOffset(const int32_t handle, const uint32_t delayOffset) override;
        Core::hresult GetAudioDelayOffset(const int32_t handle, uint32_t &delayOffset) override;
        Core::hresult GetAudioSinkDeviceAtmosCapability(const int32_t handle, DolbyAtmosCapability &atmosCapability) override;
        Core::hresult SetAudioAtmosOutputMode(const int32_t handle, const bool enable) override;
        Core::hresult SetAudioCompression(const int32_t handle, const int32_t compressionLevel) override;
        Core::hresult GetAudioCompression(const int32_t handle, int32_t &compressionLevel) override;
        Core::hresult SetAudioDialogEnhancement(const int32_t handle, const int32_t level) override;
        Core::hresult GetAudioDialogEnhancement(const int32_t handle, int32_t &level) override;
        Core::hresult SetAudioDolbyVolumeMode(const int32_t handle, const bool enable) override;
        Core::hresult GetAudioDolbyVolumeMode(const int32_t handle, bool &enabled) override;
        Core::hresult SetAudioIntelligentEqualizerMode(const int32_t handle, const int32_t mode) override;
        Core::hresult GetAudioIntelligentEqualizerMode(const int32_t handle, int32_t &mode) override;
        Core::hresult SetAudioVolumeLeveller(const int32_t handle, const VolumeLeveller volumeLeveller) override;
        Core::hresult GetAudioVolumeLeveller(const int32_t handle, VolumeLeveller &volumeLeveller) override;
        Core::hresult SetAudioBassEnhancer(const int32_t handle, const int32_t boost) override;
        Core::hresult GetAudioBassEnhancer(const int32_t handle, int32_t &boost) override;
        Core::hresult EnableAudioSurroudDecoder(const int32_t handle, const bool enable) override;
        Core::hresult IsAudioSurroudDecoderEnabled(const int32_t handle, bool &enabled) override;
        Core::hresult SetAudioDRCMode(const int32_t handle, const int32_t drcMode) override;
        Core::hresult GetAudioDRCMode(const int32_t handle, int32_t &drcMode) override;
        Core::hresult SetAudioSurroudVirtualizer(const int32_t handle, const SurroundVirtualizer surroundVirtualizer) override;
        Core::hresult GetAudioSurroudVirtualizer(const int32_t handle, SurroundVirtualizer &surroundVirtualizer) override;
        Core::hresult SetAudioMISteering(const int32_t handle, const bool enable) override;
        Core::hresult GetAudioMISteering(const int32_t handle, bool &enable) override;
        Core::hresult SetAudioGraphicEqualizerMode(const int32_t handle, const int32_t mode) override;
        Core::hresult GetAudioGraphicEqualizerMode(const int32_t handle, int32_t &mode) override;
        Core::hresult GetAudioMS12ProfileList(const int32_t handle, WPEFramework::Exchange::IDeviceSettingsManagerAudio::IDeviceSettingsAudioMS12AudioProfileIterator*& ms12ProfileList) const override;
        Core::hresult GetAudioMS12Profile(const int32_t handle, string &profile) override;
        Core::hresult SetAudioMS12Profile(const int32_t handle, const string profile) override;
        Core::hresult SetAudioMixerLevels(const int32_t handle, const AudioInput audioInput, const int32_t volume) override;
        Core::hresult SetAssociatedAudioMixing(const int32_t handle, const bool mixing) override;
        Core::hresult GetAssociatedAudioMixing(const int32_t handle, bool &mixing) override;
        Core::hresult SetAudioFaderControl(const int32_t handle, const int32_t mixerBalance) override;
        Core::hresult GetAudioFaderControl(const int32_t handle, int32_t &mixerBalance) override;
        Core::hresult SetAudioPrimaryLanguage(const int32_t handle, const string primaryAudioLanguage) override;
        Core::hresult GetAudioPrimaryLanguage(const int32_t handle, string &primaryAudioLanguage) override;
        Core::hresult SetAudioSecondaryLanguage(const int32_t handle, const string secondaryAudioLanguage) override;
        Core::hresult GetAudioSecondaryLanguage(const int32_t handle, string &secondaryAudioLanguage) override;
        Core::hresult GetAudioCapabilities(const int32_t handle, int32_t &capabilities) override;
        Core::hresult GetAudioMS12Capabilities(const int32_t handle, int32_t &capabilities) override;
        Core::hresult SetAudioMS12SettingsOverride(const int32_t handle, const string profileName, const string profileSettingsName, const string profileSettingValue, const string profileState) override;
        Core::hresult IsAudioOutputConnected(const int32_t handle, bool &isConnected) override;
        Core::hresult ResetAudioDialogEnhancement(const int32_t handle) override;
        Core::hresult ResetAudioBassEnhancer(const int32_t handle) override;
        Core::hresult ResetAudioSurroundVirtualizer(const int32_t handle) override;
        Core::hresult ResetAudioVolumeLeveller(const int32_t handle) override;
        Core::hresult GetAudioHDMIARCPortId(const int32_t handle, int32_t &portId) override;

        static DeviceSettingsManagerImp* _instance;

    private:
        std::list<DeviceSettingsManagerFPD::INotification*> _FPDNotifications;
        std::list<DeviceSettingsManagerHDMIIn::INotification*> _HDMIInNotifications;
        std::list<DeviceSettingsManagerAudio::INotification*> _AudioNotifications;

        // lock to guard all apis of DeviceSettingsManager
        mutable Core::CriticalSection _apiLock;
        // lock to guard all notification from DeviceSettingsManager to clients and also their callback register & unregister
        mutable Core::CriticalSection _callbackLock;

        template <typename T>
        Core::hresult Register(std::list<T*>& list, T* notification);
        template <typename T>
        Core::hresult Unregister(std::list<T*>& list, const T* notification);

        template<typename Func, typename... Args>
        void dispatchHDMIInEvent(Func notifyFunc, Args&&... args);

        template<typename Func, typename... Args>
        void dispatchFPDEvent(Func notifyFunc, Args&&... args);

        template<typename Func, typename... Args>
        void dispatchAudioEvent(Func notifyFunc, Args&&... args);

        virtual void OnHDMIInEventHotPlugNotification(const HDMIInPort port, const bool isConnected) override;
        virtual void OnHDMIInEventSignalStatusNotification(const HDMIInPort port, const HDMIInSignalStatus signalStatus) override;
        virtual void OnHDMIInEventStatusNotification(const HDMIInPort activePort, const bool isPresented) override;
        virtual void OnHDMIInVideoModeUpdateNotification(const HDMIInPort port, const HDMIVideoPortResolution videoPortResolution) override;
        virtual void OnHDMIInAllmStatusNotification(const HDMIInPort port, const bool allmStatus) override;
        virtual void OnHDMIInAVIContentTypeNotification(const HDMIInPort port, const HDMIInAviContentType aviContentType) override;
        virtual void OnHDMIInAVLatencyNotification(const int32_t audioDelay, const int32_t videoDelay) override;
        virtual void OnHDMIInVRRStatusNotification(const HDMIInPort port, const HDMIInVRRType vrrType) override;

        // FPD notification method
        virtual void OnFPDTimeFormatChanged(const FPDTimeFormat timeFormat) override;

        // Audio notification methods from Audio::INotification interface
        virtual void OnAssociatedAudioMixingChangedNotification(bool mixing) override;
        virtual void OnAudioFaderControlChangedNotification(int32_t mixerBalance) override;
        virtual void OnAudioPrimaryLanguageChangedNotification(const string& primaryLanguage) override;
        virtual void OnAudioSecondaryLanguageChangedNotification(const string& secondaryLanguage) override;
        virtual void OnAudioOutHotPlugNotification(AudioPortType portType, uint32_t uiPortNumber, bool isPortConnected) override;
        virtual void OnAudioFormatUpdateNotification(AudioFormat audioFormat) override;
        virtual void OnDolbyAtmosCapabilitiesChangedNotification(DolbyAtmosCapability atmosCapability, bool status) override;
        virtual void OnAudioPortStateChangedNotification(AudioPortState audioPortState) override;
        virtual void OnAudioModeEventNotification(AudioPortType audioPortType, StereoModes audioMode) override;
        virtual void OnAudioLevelChangedEventNotification(int32_t audioLevel) override;

        FPD _fpd;
        HdmiIn _hdmiIn;
        Audio _audio;
    };

} // namespace Plugin
} // namespace WPEFramework
