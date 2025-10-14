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
#include <syscall.h>

#include <type_traits>

using namespace std;

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DeviceSettingsManagerImp, 1, 0);
    //PLUGIN_REGISTRATION(DeviceSettingsManagerImp)
    DeviceSettingsManagerImp* DeviceSettingsManagerImp::_instance = nullptr;

    DeviceSettingsManagerImp::DeviceSettingsManagerImp()
        : _fpd()
        , _hdmiIn(HdmiIn::Create(*this))
    {
        ENTRY_LOG;
        std::cout << " - IDeviceSettingsManager ID: " << WPEFramework::Exchange::ID_DEVICESETTINGS_MANAGER_FPD << std::endl;
        DeviceSettingsManagerImp::_instance = this;
        LOGINFO("DeviceSettingsManagerImp Is abstract class: %d", std::is_abstract<DeviceSettingsManagerImp>::value);
        DeviceManager_Init();
        InitializeIARM();
        LOGINFO("DeviceSettingsManagerImp Constructor");
        EXIT_LOG;
    }

    DeviceSettingsManagerImp::~DeviceSettingsManagerImp() {
        ENTRY_LOG;
        LOGINFO("DeviceSettingsManagerImp Destructor");
        EXIT_LOG;
    }

    void DeviceSettingsManagerImp::DeviceManager_Init()
    {
        ENTRY_LOG;
        dsMgr_init();
        //_dsFPInit(nullptr);
        EXIT_LOG;
    }

    void DeviceSettingsManagerImp::InitializeIARM()
    {
        ENTRY_LOG;

        EXIT_LOG;
    }

    void DeviceSettingsManagerImp::dispatchHDMIInHotPlugEvent(const HDMIInPort port, const bool isConnected)
    {
        LOGINFO(">>");
        _callbackLock.Lock();
        for (auto& notification : _HDMIInNotifications) {
            auto start = std::chrono::steady_clock::now();
            notification->OnHDMIInEventHotPlug(port, isConnected);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("client %p took %" PRId64 "ms to process IModeChanged event", notification, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }
        _callbackLock.Unlock();
        LOGINFO("<<");
    }

    template <typename T>
    Core::hresult DeviceSettingsManagerImp::Register(std::list<T*>& list, T* notification)
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
        }

        _callbackLock.Unlock();
        EXIT_LOG;
        return status;
    }

    template <typename T>
    Core::hresult DeviceSettingsManagerImp::Unregister(std::list<T*>& list, const T* notification)
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

    Core::hresult DeviceSettingsManagerImp::Register(Exchange::IDeviceSettingsManagerFPD::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Register(_FPDNotifications, notification);
        LOGINFO("IDeviceSettingsManagerAudio %p, errorCode: %u", notification, errorCode);
        EXIT_LOG;
        return errorCode;
    }
    Core::hresult DeviceSettingsManagerImp::Unregister(Exchange::IDeviceSettingsManagerFPD::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Unregister(_FPDNotifications, notification);
        LOGINFO("IModeChanged %p, errorcode: %u", notification, errorCode);
        EXIT_LOG;
        return errorCode;
    }

    Core::hresult DeviceSettingsManagerImp::Register(Exchange::IDeviceSettingsManagerHDMIIn::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Register(_HDMIInNotifications, notification);
        LOGINFO("IDeviceSettingsManagerAudio %p, errorCode: %u", notification, errorCode);
        EXIT_LOG;
        return errorCode;
    }
    Core::hresult DeviceSettingsManagerImp::Unregister(Exchange::IDeviceSettingsManagerHDMIIn::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Unregister(_HDMIInNotifications, notification);
        LOGINFO("IModeChanged %p, errorcode: %u", notification, errorCode);
        EXIT_LOG;
        return errorCode;
    }

    void DeviceSettingsManagerImp::OnHDMIInEventHotPlugNotification(const HDMIInPort port, const bool isConnected)
    {
        LOGINFO("OnHDMIInEventHotPlug event Received");
        dispatchHDMIInHotPlugEvent(port, isConnected);
    }

    void DeviceSettingsManagerImp::OnHDMIInEventSignalStatusNotification(const HDMIInPort port, const HDMIInSignalStatus signalStatus)
    {
        LOGINFO("OnHDMIInEventSignalStatus event Received");
        //dispatchHDMIInSignalStatusEvent(port, signalStatus);
    }

    void DeviceSettingsManagerImp::OnHDMIInAVLatencyNotification(const int32_t audioDelay, const int32_t videoDelay)
    {
        LOGINFO("OnHDMIInAVLatency event Received");
        //dispatchHDMIInAVLatencyEvent(audioDelay, videoDelay);
    }

    void DeviceSettingsManagerImp::OnHDMIInEventStatusNotification(const HDMIInPort activePort, const bool isPresented)
    {
        LOGINFO("OnHDMIInEventStatus event Received");
        //dispatchHDMIInStatusEvent(activePort, isPresented);
    }

    void DeviceSettingsManagerImp::OnHDMIInVideoModeUpdateNotification(const HDMIInPort port, const HDMIVideoPortResolution videoPortResolution)
    {
        LOGINFO("OnHDMIInVideoModeUpdate event Received");
        //dispatchHDMIInVideoModeUpdateEvent(port, videoPortResolution);
    }

    void DeviceSettingsManagerImp::OnHDMIInAllmStatusNotification(const HDMIInPort port, const bool allmStatus)
    {
        LOGINFO("OnHDMIInAllmStatus event Received");
        //dispatchHDMIInAllmStatusEvent(port, allmStatus);
    }

    void DeviceSettingsManagerImp::OnHDMIInAVIContentTypeNotification(const HDMIInPort port, const HDMIInAviContentType aviContentType)
    {
        LOGINFO("OnHDMIInAVIContentType event Received");
        //dispatchHDMIInAVIContentTypeEvent(port, aviContentType);
    }

    void DeviceSettingsManagerImp::OnHDMIInVRRStatusNotification(const HDMIInPort port, const HDMIInVRRType vrrType)
    {
        LOGINFO("OnHDMIInVRRStatus event Received");
        //dispatchHDMIInVRRStatusEvent(port, vrrType);
    }

    //Depricated
    Core::hresult DeviceSettingsManagerImp::SetFPDTime(const FPDTimeFormat timeFormat, const uint32_t minutes, const uint32_t seconds) {
        ENTRY_LOG;
        LOGINFO("SetFPDTime: timeFormat=%d, minutes=%u, seconds=%u", timeFormat, minutes, seconds);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDScroll(const uint32_t scrollHoldDuration, const uint32_t nHorizontalScrollIterations, const uint32_t nVerticalScrollIterations) {
        ENTRY_LOG;
        LOGINFO("SetFPDScroll: scrollHoldDuration=%u, horizontal=%u, vertical=%u", scrollHoldDuration, nHorizontalScrollIterations, nVerticalScrollIterations);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDTextBrightness(const FPDTextDisplay textDisplay, const uint32_t brightNess) {
        ENTRY_LOG;
        LOGINFO("SetFPDTextBrightness: textDisplay=%d, brightNess=%u", textDisplay, brightNess);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDTextBrightness(const FPDTextDisplay textDisplay, uint32_t &brightNess) {
        ENTRY_LOG;
        brightNess = 50; // Example value
        LOGINFO("GetFPDTextBrightness: textDisplay=%d", textDisplay);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::EnableFPDClockDisplay(const bool enable) {
        ENTRY_LOG;
        LOGINFO("EnableFPDClockDisplay: enable=%s", enable ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDTimeFormat(FPDTimeFormat &fpdTimeFormat) {
        ENTRY_LOG;
        fpdTimeFormat = FPDTimeFormat::DS_FPD_TIMEFORMAT_24_HOUR; // Example value
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDTimeFormat(const FPDTimeFormat fpdTimeFormat) {
        ENTRY_LOG;
        LOGINFO("SetFPDTimeFormat: fpdTimeFormat=%d", fpdTimeFormat);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }
    //Depricated

    Core::hresult DeviceSettingsManagerImp::SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) {
        ENTRY_LOG;
        LOGINFO("SetFPDBlink: indicator=%d, blinkDuration=%u, blinkIterations=%u", indicator, blinkDuration, blinkIterations);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDBrightness(const FPDIndicator indicator, const uint32_t brightNess, const bool persist) {
        ENTRY_LOG;
        LOGINFO("SetFPDBrightness: indicator=%d, brightNess=%u, persist=%s", indicator, brightNess, persist ? "true" : "false");

        _apiLock.Lock();
        _fpd.SetFPDBrightness(indicator, brightNess, persist);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDBrightness(const FPDIndicator indicator, uint32_t &brightNess) {
        ENTRY_LOG;

        _apiLock.Lock();
        _fpd.GetFPDBrightness(indicator, brightNess);
        _apiLock.Unlock();

        LOGINFO("GetFPDBrightness: indicator=%d, brightNess=%d", indicator, brightNess);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDState(const FPDIndicator indicator, const FPDState state) {
        ENTRY_LOG;
        LOGINFO("SetFPDState: indicator=%d, state=%d", indicator, state);

        _apiLock.Lock();
        _fpd.SetFPDState(indicator, state);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDState(const FPDIndicator indicator, FPDState &state) {
        ENTRY_LOG;

        _apiLock.Lock();
        _fpd.GetFPDState(indicator, state);
        _apiLock.Unlock();

        LOGINFO("GetFPDState: indicator=%d, state=%d", indicator, state);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetFPDColor(const FPDIndicator indicator, uint32_t &color) {
        ENTRY_LOG;

        _apiLock.Lock();
        _fpd.GetFPDColor(indicator, color);
        _apiLock.Unlock();

        LOGINFO("GetFPDColor: indicator=%d, color=0x%X", indicator, color);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDColor(const FPDIndicator indicator, const uint32_t color) {
        ENTRY_LOG;
        LOGINFO("SetFPDColor: indicator=%d, color=0x%X", indicator, color);

        _apiLock.Lock();
        _fpd.SetFPDColor(indicator, color);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDMode(const FPDMode fpdMode) {
        ENTRY_LOG;
        LOGINFO("SetFPDMode: fpdMode=%d", fpdMode);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIInNumbefOfInputs(int32_t &count) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInNumbefOfInputs");
        count = 2; // Example value

        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInStatus");
        hdmiStatus.activePort = HDMIInPort::DS_HDMI_IN_PORT_0; // Example value
        hdmiStatus.isPresented = true; // Example value
        portConnectionStatus = nullptr; // Example value
        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) {
        ENTRY_LOG;

        LOGINFO("SelectHDMIInPort: port=%d, requestAudioMix=%s, topMostPlane=%s, videoPlaneType=%d",
            port, requestAudioMix ? "true" : "false", topMostPlane ? "true" : "false", videoPlaneType);
        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) {
        ENTRY_LOG;

        LOGINFO("ScaleHDMIInVideo: x=%d, y=%d, w=%d, h=%d", videoPosition.x, videoPosition.y, videoPosition.width, videoPosition.height);
        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) {
        ENTRY_LOG;

        LOGINFO("SelectHDMIZoomMode: zoomMode=%d", zoomMode);

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator *& gameFeatureList) {
        ENTRY_LOG;

        LOGINFO("GetSupportedGameFeaturesList");
        gameFeatureList = nullptr; // Example value

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInAVLatency");
        videoLatency = 10; // Example value
        audioLatency = 5; // Example value

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInAllmStatus: port=%d", port);
        allmStatus = true; // Example value

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInEdid2AllmSupport: port=%d", port);
        allmSupport = true; // Example value

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) {
        ENTRY_LOG;

        LOGINFO("SetHDMIInEdid2AllmSupport: port=%d, allmSupport=%s", port, allmSupport ? "true" : "false");

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]) {
        ENTRY_LOG;

        LOGINFO("GetEdidBytes: port=%d, edidBytesLength=%u", port, edidBytesLength);
        if (edidBytes && edidBytesLength > 0) {
            edidBytes[0] = 0x00; // Example value
        }
        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) {
        ENTRY_LOG;

        LOGINFO("GetHDMISPDInformation: port=%d, spdBytesLength=%u", port, spdBytesLength);
        if (spdBytes && spdBytesLength > 0) {
            spdBytes[0] = 0x00; // Example value
        }
        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion) {
        ENTRY_LOG;

        LOGINFO("GetHDMIEdidVersion: port=%d", port);
        edidVersion = HDMIInEdidVersion::HDMI_EDID_VER_14; // Example value
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) {
        ENTRY_LOG;

        LOGINFO("SetHDMIEdidVersion: port=%d, edidVersion=%d", port, edidVersion);
        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution) {
        ENTRY_LOG;

        LOGINFO("GetHDMIVideoMode");
        videoPortResolution.name = "1920x1080"; // Example value

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion) {
        ENTRY_LOG;

        LOGINFO("GetHDMIVersion: port=%d", port);
        capabilityVersion = HDMIInCapabilityVersion::HDMI_COMPATIBILITY_VERSION_20; // Example value

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetVRRSupport(const HDMIInPort port, bool &vrrSupport) {
        ENTRY_LOG;

        LOGINFO("GetVRRSupport: port=%d", port);
        vrrSupport = true; // Example value

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetVRRSupport(const HDMIInPort port, const bool vrrSupport) {
        ENTRY_LOG;

        LOGINFO("SetVRRSupport: port=%d, vrrSupport=%s", port, vrrSupport ? "true" : "false");

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus) {
        ENTRY_LOG;

        LOGINFO("GetVRRStatus: port=%d", port);
        vrrStatus.vrrType = DS_HDMIIN_HDMI_VRR; // Example value

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
