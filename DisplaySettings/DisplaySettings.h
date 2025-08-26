/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2019 RDK Management
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
**/

#pragma once

#include <mutex>
#include <condition_variable>
#include "Module.h"
#include "dsTypes.h"
#include "tptimer.h"
#include "libIARM.h"
#include "rfcapi.h"
#include <interfaces/ISystemMode.h>
#include <interfaces/IDeviceOptimizeStateActivator.h>
#include <iostream>
#include <fstream>
#include <interfaces/IPowerManager.h>
#include "PowerManagerInterface.h"

#if 1 /* Display Events from libds Library */
#include "dsTypes.h"
#include "host.hpp"
#endif

using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;
using ThermalTemperature = WPEFramework::Exchange::IPowerManager::ThermalTemperature;
namespace WPEFramework {

    namespace Plugin {

		// This is a server for a JSONRPC communication channel.
		// For a plugin to be capable to handle JSONRPC, inherit from PluginHost::JSONRPC.
		// By inheriting from this class, the plugin realizes the interface PluginHost::IDispatcher.
		// This realization of this interface implements, by default, the following methods on this plugin
		// - exists
		// - register
		// - unregister
		// Any other methood to be handled by this plugin  can be added can be added by using the
		// templated methods Register on the PluginHost::JSONRPC class.
		// As the registration/unregistration of notifications is realized by the class PluginHost::JSONRPC,
		// this class exposes a public method called, Notify(), using this methods, all subscribed clients
		// will receive a JSONRPC message as a notification, in case this method is called.
        class DisplaySettings : public PluginHost::IPlugin, public PluginHost::JSONRPC,Exchange::IDeviceOptimizeStateActivator {
        private:
            typedef Core::JSON::String JString;
            typedef Core::JSON::ArrayType<JString> JStringArray;
            typedef Core::JSON::Boolean JBool;
            class PowerManagerNotification : public Exchange::IPowerManager::IModeChangedNotification {
            private:
                PowerManagerNotification(const PowerManagerNotification&) = delete;
                PowerManagerNotification& operator=(const PowerManagerNotification&) = delete;
            
            public:
                explicit PowerManagerNotification(DisplaySettings& parent)
                    : _parent(parent)
                {
                }
                ~PowerManagerNotification() override = default;

            public:
                void OnPowerModeChanged(const PowerState currentState, const PowerState newState) override
                {
                    _parent.onPowerModeChanged(currentState, newState);
                }

                template <typename T>
                T* baseInterface()
                {
                    static_assert(std::is_base_of<T, PowerManagerNotification>(), "base type mismatch");
                    return static_cast<T*>(this);
                }

                BEGIN_INTERFACE_MAP(PowerManagerNotification)
                INTERFACE_ENTRY(Exchange::IPowerManager::IModeChangedNotification)
                END_INTERFACE_MAP
            
            private:
                DisplaySettings& _parent;
            };

#if 1 /* Display Events from libds Library */
            class DisplayEventNotification : public device::Host::IDisplayEvent {
            private:
                DisplayEventNotification(const DisplayEventNotification&) = delete;
                DisplayEventNotification& operator=(const DisplayEventNotification&) = delete;

            public:
                explicit DisplayEventNotification(DisplaySettings& parent)
                    : _parent(parent)
                {
                }
                ~DisplayEventNotification() override = default;

            public:
                void OnDisplayRxSense(dsDisplayEvent_t displayEvent) override
                {
                    _parent.OnDisplayRxSense(displayEvent); //TODO
                }

                BEGIN_INTERFACE_MAP(DisplayEventNotification)
                INTERFACE_ENTRY(device::Host::IDisplayEvent)
                END_INTERFACE_MAP

            private:
                DisplaySettings& _parent;
            };

            class AudioOutputPortEventsNotification : public device::Host::IAudioOutputPortEvents {
            private:
                AudioOutputPortEventsNotification(const AudioOutputPortEventsNotification&) = delete;
                AudioOutputPortEventsNotification& operator=(const AudioOutputPortEventsNotification&) = delete;

            public:
                explicit AudioOutputPortEventsNotification(DisplaySettings& parent)
                    : _parent(parent)
                {
                }
                ~AudioOutputPortEventsNotification() override = default;

                void OnAudioOutHotPlug(dsAudioPortType_t audioPortType, int uiPortNumber, bool isPortConnected) override
                {
                    _parent.OnAudioOutHotPlug(audioPortType, uiPortNumber, isPortConnected);
                }
                void OnAudioFormatUpdate(dsAudioFormat_t audioFormat) override
                {
                    _parent.OnAudioFormatUpdate(audioFormat); //TODO
                }
                void OnDolbyAtmosCapabilitiesChanged(dsATMOSCapability_t atmosCapability, bool status)  override
                {
                    _parent.OnDolbyAtmosCapabilitiesChanged(atmosCapability, status); //TODO
                }
                    void OnAudioPortStateChanged(dsAudioPortState_t audioPortState) override
                {
                    _parent.OnAudioPortStateChanged(audioPortState); //TODO
                }
                    void OnAssociatedAudioMixingChanged(bool mixing) override
                {
                    _parent.OnAssociatedAudioMixingChanged(mixing); //TODO
                }
                    void OnAudioFaderControlChanged(int mixerBalance) override
                {
                    _parent.OnAudioFaderControlChanged(mixerBalance); //TODO
                }
                void OnAudioPrimaryLanguageChanged(const std::string& primaryLanguage) override
                {
                    _parent.OnAudioPrimaryLanguageChanged(primaryLanguage); //TODO
                }
                void OnAudioSecondaryLanguageChanged(const std::string& secondaryLanguage) override
                {
                    _parent.OnAudioSecondaryLanguageChanged(secondaryLanguage); //TODO
                }

                BEGIN_INTERFACE_MAP(AudioOutputPortEventsNotification)
                INTERFACE_ENTRY(device::Host::IAudioOutputPortEvents)
                END_INTERFACE_MAP

            private:
                DisplaySettings& _parent;
            };

            class DisplayHDMIHotPlugNotification : public device::Host::IDisplayHDMIHotPlugEvents {
            private:
                DisplayHDMIHotPlugNotification(const DisplayHDMIHotPlugNotification&) = delete;
                DisplayHDMIHotPlugNotification& operator=(const DisplayHDMIHotPlugNotification&) = delete;

            public:
                explicit DisplayHDMIHotPlugNotification(DisplaySettings& parent)
                    : _parent(parent)
                {
                }
                ~DisplayHDMIHotPlugNotification() override = default;

            public:
                void void OnDisplayHDMIHotPlug(dsDisplayEvent_t displayEvent) override
                {
                    _parent.OnDisplayHDMIHotPlug(displayEvent); //TODO
                }

                BEGIN_INTERFACE_MAP(DisplayHDMIHotPlugNotification)
                INTERFACE_ENTRY(device::Host::IDisplayHDMIHotPlugEvents)
                END_INTERFACE_MAP

            private:
                DisplaySettings& _parent;
            };

            class HDMIInEventsNotification : public device::Host::IHDMIInEvents {
            private:
                HDMIInEventsNotification(const HDMIInEventsNotification&) = delete;
                HDMIInEventsNotification& operator=(const HDMIInEventsNotification&) = delete;

            public:
                explicit HDMIInEventsNotification(DisplaySettings& parent)
                    : _parent(parent)
                {
                }
                ~HDMIInEventsNotification() override = default;

            public:
                void OnHDMIInEventHotPlug(dsHdmiInPort_t port, bool isConnected) override
                {
                    _parent.OnHDMIInEventHotPlug(port, isConnected); //TODO
                }

                BEGIN_INTERFACE_MAP(HDMIInEventsNotification)
                INTERFACE_ENTRY(device::Host::IHDMIInEvents)
                END_INTERFACE_MAP

            private:
                DisplaySettings& _parent;
            };

            class VideoDeviceEventsNotification : public device::Host::IVideoDeviceEvents {
            private:
                VideoDeviceEventsNotification(const VideoDeviceEventsNotification&) = delete;
                VideoDeviceEventsNotification& operator=(const VideoDeviceEventsNotification&) = delete;

            public:
                explicit VideoDeviceEventsNotification(DisplaySettings& parent)
                    : _parent(parent)
                {
                }
                ~VideoDeviceEventsNotification() override = default;

            public:
                void OnZoomSettingsChanged(dsVideoZoom_t zoomSetting) override
                {
                    _parent.OnZoomSettingsChanged(zoomSetting); //TODO
                }

                BEGIN_INTERFACE_MAP(VideoDeviceEventsNotification)
                INTERFACE_ENTRY(device::Host::IVideoDeviceEvents)
                END_INTERFACE_MAP

            private:
                DisplaySettings& _parent;
            };

            class VideoOutputPortEventsNotification : public device::Host::IVideoOutputPortEvents {
            private:
                VideoOutputPortEventsNotification(const VideoOutputPortEventsNotification&) = delete;
                VideoOutputPortEventsNotification& operator=(const VideoOutputPortEventsNotification&) = delete;

            public:
                explicit VideoOutputPortEventsNotification(DisplaySettings& parent)
                    : _parent(parent)
                {
                }
                ~VideoOutputPortEventsNotification() override = default;

            public:
                void OnResolutionPreChange(const int width, const int height) override
                {
                    _parent.OnResolutionPreChange(width, height); //TODO
                }
                void OnResolutionPostChange(const int width, const int height) override
                {
                    _parent.OnResolutionPostChange(width, height); //TODO
                }
                void OnVideoFormatUpdate(dsHDRStandard_t videoFormatHDR) override
                {
                    _parent.OnVideoFormatUpdate(videoFormatHDR); //TODO
                }

                BEGIN_INTERFACE_MAP(VideoOutputPortEventsNotification)
                INTERFACE_ENTRY(device::Host::IVideoOutputPortEvents)
                END_INTERFACE_MAP

            private:
                DisplaySettings& _parent;
            };

#endif

            // We do not allow this plugin to be copied !!
            DisplaySettings(const DisplaySettings&) = delete;
            DisplaySettings& operator=(const DisplaySettings&) = delete;

            //Begin methods
            uint32_t getConnectedVideoDisplays(const JsonObject& parameters, JsonObject& response);
            uint32_t getConnectedAudioPorts(const JsonObject& parameters, JsonObject& response);
	    uint32_t setEnableAudioPort (const JsonObject& parameters, JsonObject& response);
            uint32_t getSupportedResolutions(const JsonObject& parameters, JsonObject& response);
            uint32_t getSupportedVideoDisplays(const JsonObject& parameters, JsonObject& response);
            uint32_t getSupportedTvResolutions(const JsonObject& parameters, JsonObject& response);
            uint32_t getSupportedSettopResolutions(const JsonObject& parameters, JsonObject& response);
            uint32_t getSupportedAudioPorts(const JsonObject& parameters, JsonObject& response);
            uint32_t getSupportedAudioModes(const JsonObject& parameters, JsonObject& response);
            uint32_t getZoomSetting(const JsonObject& parameters, JsonObject& response);
            uint32_t setZoomSetting(const JsonObject& parameters, JsonObject& response);
            uint32_t getCurrentResolution(const JsonObject& parameters, JsonObject& response);
            uint32_t setCurrentResolution(const JsonObject& parameters, JsonObject& response);
            uint32_t getSoundMode(const JsonObject& parameters, JsonObject& response);
            uint32_t setSoundMode(const JsonObject& parameters, JsonObject& response);
            uint32_t readEDID(const JsonObject& parameters, JsonObject& response);
            uint32_t readHostEDID(const JsonObject& parameters, JsonObject& response);
            uint32_t getActiveInput(const JsonObject& parameters, JsonObject& response);
            uint32_t getTvHDRSupport(const JsonObject& parameters, JsonObject& response);
            uint32_t getSettopHDRSupport(const JsonObject& parameters, JsonObject& response);
            uint32_t setVideoPortStatusInStandby(const JsonObject& parameters, JsonObject& response);
            uint32_t getVideoPortStatusInStandby(const JsonObject& parameters, JsonObject& response);
            uint32_t getCurrentOutputSettings(const JsonObject& parameters, JsonObject& response);
            uint32_t setForceHDRMode(const JsonObject& parameters, JsonObject& response);
            //End methods
            uint32_t setMS12AudioCompression(const JsonObject& parameters, JsonObject& response);
            uint32_t getMS12AudioCompression(const JsonObject& parameters, JsonObject& response);
            uint32_t setDolbyVolumeMode(const JsonObject& parameters, JsonObject& response);
            uint32_t getDolbyVolumeMode(const JsonObject& parameters, JsonObject& response);
            uint32_t setDialogEnhancement(const JsonObject& parameters, JsonObject& response);
            uint32_t getDialogEnhancement(const JsonObject& parameters, JsonObject& response);
            uint32_t setIntelligentEqualizerMode(const JsonObject& parameters, JsonObject& response);
            uint32_t getIntelligentEqualizerMode(const JsonObject& parameters, JsonObject& response);
            uint32_t setGraphicEqualizerMode(const JsonObject& parameters, JsonObject& response);
            uint32_t getGraphicEqualizerMode(const JsonObject& parameters, JsonObject& response);
	    uint32_t setMS12AudioProfile(const JsonObject& parameters, JsonObject& response);
	    uint32_t getMS12AudioProfile(const JsonObject& parameters, JsonObject& response);
	    uint32_t getSupportedMS12AudioProfiles(const JsonObject& parameters, JsonObject& response);
            uint32_t getAudioDelay(const JsonObject& parameters, JsonObject& response);
            uint32_t setAudioDelay(const JsonObject& parameters, JsonObject& response);
            uint32_t getSinkAtmosCapability(const JsonObject& parameters, JsonObject& response);
            uint32_t setAudioAtmosOutputMode(const JsonObject& parameters, JsonObject& response);
            uint32_t getTVHDRCapabilities(const JsonObject& parameters, JsonObject& response);
            uint32_t isConnectedDeviceRepeater(const JsonObject& parameters, JsonObject& response);
            uint32_t getDefaultResolution(const JsonObject& parameters, JsonObject& response);
            uint32_t setScartParameter(const JsonObject& parameters, JsonObject& response);
            uint32_t getVolumeLeveller(const JsonObject& parameters, JsonObject& response);
            uint32_t getBassEnhancer(const JsonObject& parameters, JsonObject& response);
            uint32_t isSurroundDecoderEnabled(const JsonObject& parameters, JsonObject& response);
            uint32_t getDRCMode(const JsonObject& parameters, JsonObject& response);
            uint32_t getSurroundVirtualizer(const JsonObject& parameters, JsonObject& response);
            uint32_t getMISteering(const JsonObject& parameters, JsonObject& response);
            uint32_t setVolumeLeveller(const JsonObject& parameters, JsonObject& response);
            uint32_t setBassEnhancer(const JsonObject& parameters, JsonObject& response);
            uint32_t enableSurroundDecoder(const JsonObject& parameters, JsonObject& response);
            uint32_t setSurroundVirtualizer(const JsonObject& parameters, JsonObject& response);
            uint32_t setMISteering(const JsonObject& parameters, JsonObject& response);
            uint32_t setGain(const JsonObject& parameters, JsonObject& response);
            uint32_t getGain(const JsonObject& parameters, JsonObject& response);
            uint32_t setMuted(const JsonObject& parameters, JsonObject& response);
            uint32_t getMuted(const JsonObject& parameters, JsonObject& response);
            uint32_t setVolumeLevel(const JsonObject& parameters, JsonObject& response);
            uint32_t getVolumeLevel(const JsonObject& parameters, JsonObject& response);
            uint32_t setDRCMode(const JsonObject& parameters, JsonObject& response);
            uint32_t getSettopMS12Capabilities(const JsonObject& parameters, JsonObject& response);
            uint32_t getSettopAudioCapabilities(const JsonObject& parameters, JsonObject& response);
            uint32_t getEnableAudioPort(const JsonObject& parameters, JsonObject& response);

	    uint32_t setAssociatedAudioMixing(const JsonObject& parameters, JsonObject& response);
            uint32_t getAssociatedAudioMixing(const JsonObject& parameters, JsonObject& response);
            uint32_t setFaderControl(const JsonObject& parameters, JsonObject& response);
            uint32_t getFaderControl(const JsonObject& parameters, JsonObject& response);
            uint32_t setPrimaryLanguage(const JsonObject& parameters, JsonObject& response);
            uint32_t getPrimaryLanguage(const JsonObject& parameters, JsonObject& response);
            uint32_t setSecondaryLanguage(const JsonObject& parameters, JsonObject& response);
            uint32_t getSecondaryLanguage(const JsonObject& parameters, JsonObject& response);

	    uint32_t getAudioFormat(const JsonObject& parameters, JsonObject& response);
	    uint32_t getVolumeLeveller2(const JsonObject& parameters, JsonObject& response);
	    uint32_t setVolumeLeveller2(const JsonObject& parameters, JsonObject& response);
	    uint32_t getSurroundVirtualizer2(const JsonObject& parameters, JsonObject& response);
	    uint32_t setSurroundVirtualizer2(const JsonObject& parameters, JsonObject& response);
            uint32_t resetDialogEnhancement(const JsonObject& parameters, JsonObject& response);
            uint32_t resetBassEnhancer(const JsonObject& parameters, JsonObject& response);
            uint32_t resetSurroundVirtualizer(const JsonObject& parameters, JsonObject& response);
            uint32_t resetVolumeLeveller(const JsonObject& parameters, JsonObject& response);
            uint32_t getVideoFormat(const JsonObject& parameters, JsonObject& response);
            uint32_t setMS12ProfileSettingsOverride(const JsonObject& parameters, JsonObject& response);

            uint32_t setPreferredColorDepth(const JsonObject& parameters, JsonObject& response);
            uint32_t getPreferredColorDepth(const JsonObject& parameters, JsonObject& response);
            uint32_t getColorDepthCapabilities(const JsonObject& parameters, JsonObject& response);
	    uint32_t getSupportedMS12Config(const JsonObject& parameters, JsonObject& response);

            void InitAudioPorts();
            void AudioPortsReInitialize();
            static void initAudioPortsWorker(void);
            //End methods

            //Begin events
            void resolutionPreChange();
            void resolutionChanged(int width, int height);
            void zoomSettingUpdated(const string& zoomSetting);
            void activeInputChanged(bool activeInput);
            void connectedVideoDisplaysUpdated(int hdmiHotPlugEvent);
            void connectedAudioPortUpdated (int iAudioPortType, bool isPortConnected);
	    void notifyAudioFormatChange(dsAudioFormat_t audioFormat);
		void notifyAtmosCapabilityChange(dsATMOSCapability_t atmoCaps);
            void notifyAssociatedAudioMixingChange(bool mixing);
            void notifyFaderControlChange(bool mixerbalance);
            void notifyPrimaryLanguageChange(std::string pLang);
            void notifySecondaryLanguageChange(std::string sLang);
	    void notifyVideoFormatChange(dsHDRStandard_t videoFormat);
	    void onARCInitiationEventHandler(const JsonObject& parameters);
            void onARCTerminationEventHandler(const JsonObject& parameters);
	    void onShortAudioDescriptorEventHandler(const JsonObject& parameters);
	    void onSystemAudioModeEventHandler(const JsonObject& parameters);
            void onArcAudioStatusEventHandler(const JsonObject& parameters);
	    void onAudioDeviceConnectedStatusEventHandler(const JsonObject& parameters);
	    void onCecEnabledEventHandler(const JsonObject& parameters);
            void onAudioDevicePowerStatusEventHandler(const JsonObject& parameters);
	    bool isDisplayConnected (std::string port);
            //End events
        public:
            DisplaySettings();
            virtual ~DisplaySettings();
            //IPlugin methods
            virtual const string Initialize(PluginHost::IShell* service) override;
            virtual void Deinitialize(PluginHost::IShell* service) override;
            virtual string Information() const override { return {}; }
            void onPowerModeChanged(const PowerState currentState, const PowerState newState);
            void registerEventHandlers();
            BEGIN_INTERFACE_MAP(DisplaySettings)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
	    INTERFACE_ENTRY(Exchange::IDeviceOptimizeStateActivator)
            END_INTERFACE_MAP

	    Core::hresult Request(const string& newState);

        private:
            void InitializeIARM();
            void DeinitializeIARM();
            static void ResolutionPreChange(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void ResolutionPostChange(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void DisplResolutionHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsHdmiEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
	    static void formatUpdateEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
        static void checkAtmosCapsEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void powerEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void audioPortStateEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void dsSettingsChangeEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            void getConnectedVideoDisplaysHelper(std::vector<string>& connectedDisplays);
	    void audioFormatToString(dsAudioFormat_t audioFormat, JsonObject &response);
            const char *getVideoFormatTypeToString(dsHDRStandard_t format);
            dsHDRStandard_t getVideoFormatTypeFromString(const char *mode);
            JsonArray getSupportedVideoFormats();
            bool checkPortName(std::string& name) const;
            PowerState getSystemPowerState();

	    void getHdmiCecSinkPlugin();
	    WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>* m_client;
	    std::vector<std::string> m_clientRegisteredEventNames;
	    uint32_t subscribeForHdmiCecSinkEvent(const char* eventName);
	    bool setUpHdmiCecSinkArcRouting (bool arcEnable);
	    bool requestShortAudioDescriptor();
            bool requestAudioDevicePowerStatus();
	    bool sendHdmiCecSinkAudioDevicePowerOn();
	    bool getHdmiCecSinkCecEnableStatus();
	    bool getHdmiCecSinkAudioDeviceConnectedStatus();

	    void onTimer();
	    void stopCecTimeAndUnsubscribeEvent();
            void checkAudioDeviceDetectionTimer();
	    void checkArcDeviceConnected();
	    void checkSADUpdate();
	    void checkAudioDevicePowerStatusTimer();

	    TpTimer m_timer;
            TpTimer m_AudioDeviceDetectTimer;
	    TpTimer m_SADDetectionTimer;
	    TpTimer m_ArcDetectionTimer;
	    TpTimer m_AudioDevicePowerOnStatusTimer;
            bool m_subscribed;
            std::mutex m_callMutex;
            std::mutex m_SadMutex;
	    std::thread m_arcRoutingThread;
	    std::mutex m_AudioDeviceStatesUpdateMutex;
	    bool m_cecArcRoutingThreadRun; 
	    std::condition_variable arcRoutingCV;
	    bool m_hdmiInAudioDeviceConnected;
            bool m_arcEarcAudioEnabled;
            bool m_arcPendingSADRequest;   
	    bool m_hdmiCecAudioDeviceDetected;
	    bool m_systemAudioMode_Power_RequestedAndReceived;
	    dsAudioARCTypes_t m_hdmiInAudioDeviceType;
	    JsonObject m_audioOutputPortConfig;
        PowerManagerInterfaceRef _powerManagerPlugin;
        Core::Sink<PowerManagerNotification> _pwrMgrNotification;
        bool _registeredEventHandlers;
        void InitializePowerManager();
            JsonObject getAudioOutputPortConfig() { return m_audioOutputPortConfig; }
            static PowerState m_powerState;

#if 1
    private:
        Host *_hostListener;
        Core::Sink<DisplayEventNotification> _displayEventNotification;
        Core::Sink<AudioOutputPortEventsNotification> _displayEventNotification;
        Core::Sink<DisplayHDMIHotPlugNotification> _displayEventNotification;
        Core::Sink<HDMIInEventsNotification> _displayEventNotification;
        Core::Sink<VideoDeviceEventsNotification> _displayEventNotification;
        Core::Sink<VideoOutputPortEventsNotification> _displayEventNotification;

        bool _registeredHostEventHandlers;

    public:
        void registerHostEventHandlers();

        /* DisplayEventNotification*/
        void OnDisplayRxSense(DisplayEvent displayEvent);

        /* AudioOutputPortEventsNotification*/
        void OnAudioOutHotPlug(dsAudioPortType_t audioPortType, int uiPortNumber, bool isPortConnected);
        void OnAudioFormatUpdate(dsAudioFormat_t audioFormat);
        void OnDolbyAtmosCapabilitiesChanged(dsATMOSCapability_t atmosCapability, bool status);
        void OnAudioPortStateChanged(dsAudioPortState_t audioPortState);
        void OnAssociatedAudioMixingChanged(bool mixing);
        void OnAudioFaderControlChanged(int mixerBalance);
        void OnAudioPrimaryLanguageChanged(const std::string& primaryLanguage);
        void OnAudioSecondaryLanguageChanged(const std::string& secondaryLanguage);

        /* DisplayHDMIHotPlugNotification */
        void OnDisplayHDMIHotPlug(dsDisplayEvent_t displayEvent);

        /* HDMIInEventsNotification*/
        void OnHDMIInEventHotPlug(dsHdmiInPort_t port, bool isConnected);

        /* VideoDeviceEventsNotification */
        void OnZoomSettingsChanged(dsVideoZoom_t zoomSetting);

        /* VideoOutputPortEventsNotification*/
        void OnResolutionPreChange(const int width, const int height);
        void OnResolutionPostChange(const int width, const int height);
        void OnVideoFormatUpdate(dsHDRStandard_t videoFormatHDR);

#endif

            enum {
                ARC_STATE_REQUEST_ARC_INITIATION,
                ARC_STATE_ARC_INITIATED,
                ARC_STATE_REQUEST_ARC_TERMINATION,
                ARC_STATE_ARC_TERMINATED,
                ARC_STATE_ARC_EXIT
            };

            enum {
                AUDIO_DEVICE_POWER_STATE_UNKNOWN,
                AUDIO_DEVICE_POWER_STATE_REQUEST,
                AUDIO_DEVICE_POWER_STATE_STANDBY,
                AUDIO_DEVICE_POWER_STATE_ON,
            };

	    enum {
		AUDIO_DEVICE_SAD_UNKNOWN,
		AUDIO_DEVICE_SAD_REQUESTED,
		AUDIO_DEVICE_SAD_RECEIVED,
		AUDIO_DEVICE_SAD_UPDATED,
		AUDIO_DEVICE_SAD_CLEARED
	    };

	    enum {
		AVR_POWER_STATE_ON,
		AVR_POWER_STATE_STANDBY,
		AVR_POWER_STATE_STANDBY_TO_ON_TRANSITION
	    };
           typedef enum {
		SEND_AUDIO_DEVICE_POWERON_MSG = 1,
		REQUEST_SHORT_AUDIO_DESCRIPTOR,
		REQUEST_AUDIO_DEVICE_POWER_STATUS,
		SEND_REQUEST_ARC_INITIATION,
		SEND_REQUEST_ARC_TERMINATION,
		} msg_t;

	   typedef struct sendMsgInfo {
                   int msg;
                   void *param;
                } SendMsgInfo;

	    void sendMsgToQueue(msg_t msg, void *param);
            bool m_sendMsgThreadExit;
            bool m_sendMsgThreadRun;

	    static void  sendMsgThread();
            std::thread m_sendMsgThread;
            std::mutex m_sendMsgMutex;
	    std::queue<SendMsgInfo> m_sendMsgQueue;
            std::condition_variable m_sendMsgCV;

            int m_hdmiInAudioDevicePowerState;
            int m_currentArcRoutingState;
            int m_AudioDeviceSADState;
	    bool m_requestSad;
	    bool m_requestSadRetrigger;
            PluginHost::IShell* m_service = nullptr;

        public:
            static DisplaySettings* _instance;

	private: 
	    mutable Core::CriticalSection _adminLock;

        };
	} // namespace Plugin
} // namespace WPEFramework
