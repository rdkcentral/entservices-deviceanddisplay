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

using namespace std;

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DeviceSettingsManagerImp, 1, 0);

    DeviceSettingsManagerImp* DeviceSettingsManagerImp::_instance = nullptr;

    DeviceSettingsManagerImp::DeviceSettingsManagerImp()
        : _fpd(FPD::Create(*this))
        , _hdmiIn(HdmiIn::Create(*this))
        , _audio(Audio::Create(*this))
    {
        ENTRY_LOG;
        DeviceSettingsManagerImp::_instance = this;
        LOGINFO("DeviceSettingsManagerImp Constructor - Instance Address: %p", this);
        EXIT_LOG;
    }

    DeviceSettingsManagerImp::~DeviceSettingsManagerImp() {
        ENTRY_LOG;
        LOGINFO("DeviceSettingsManagerImp Destructor - Instance Address: %p", this);
        EXIT_LOG;
    }

    template<typename Func, typename... Args>
    void DeviceSettingsManagerImp::dispatchHDMIInEvent(Func notifyFunc, Args&&... args) {
        LOGINFO(">>");
        _callbackLock.Lock();
        for (auto& notification : _HDMIInNotifications) {
            auto start = std::chrono::steady_clock::now();
            (notification->*notifyFunc)(std::forward<Args>(args)...);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("client %p took %" PRId64 "ms to process IHDMIIn event", notification, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }
        _callbackLock.Unlock();
        LOGINFO("<<");
    }

    template<typename Func, typename... Args>
    void DeviceSettingsManagerImp::dispatchFPDEvent(Func notifyFunc, Args&&... args) {
        LOGINFO(">>");
        _callbackLock.Lock();
        for (auto& notification : _FPDNotifications) {
            auto start = std::chrono::steady_clock::now();
            (notification->*notifyFunc)(std::forward<Args>(args)...);
            auto elapsed = std::chrono::steady_clock::now() - start;
            LOGINFO("client %p took %" PRId64 "ms to process IFPD event", notification, std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
        }
        _callbackLock.Unlock();
        LOGINFO("<<");
    }

    template<typename Func, typename... Args>
    void DeviceSettingsManagerImp::dispatchAudioEvent(Func notifyFunc, Args&&... args) {
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
        } else {
            LOGWARN("Notification %p already registered - skipping", notification);
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

    Core::hresult DeviceSettingsManagerImp::Register(DeviceSettingsManagerFPD::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Register(_FPDNotifications, notification);
        if (errorCode != Core::ERROR_NONE) {
            LOGERR("IFPD %p, errorCode: %u", notification, errorCode);
        } else {
            LOGINFO("IFPD %p registered successfully", notification);
        }
        EXIT_LOG;
        return errorCode;
    }

    Core::hresult DeviceSettingsManagerImp::Unregister(DeviceSettingsManagerFPD::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Unregister(_FPDNotifications, notification);
        if (errorCode != Core::ERROR_NONE) {
            LOGERR("IFPD %p, errorcode: %u", notification, errorCode);
        } else {
            LOGINFO("IFPD %p unregistered successfully", notification);
        }
        EXIT_LOG;
        return errorCode;
    }

    Core::hresult DeviceSettingsManagerImp::Register(DeviceSettingsManagerHDMIIn::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Register(_HDMIInNotifications, notification);
        if (errorCode != Core::ERROR_NONE) {
            LOGERR("IHDMIIn %p, errorCode: %u", notification, errorCode);
        } else {
            LOGINFO("IHDMIIn %p registered successfully", notification);
        }
        EXIT_LOG;
        return errorCode;
    }

    Core::hresult DeviceSettingsManagerImp::Unregister(DeviceSettingsManagerHDMIIn::INotification* notification)
    {
        ENTRY_LOG;
        Core::hresult errorCode = Unregister(_HDMIInNotifications, notification);
        if (errorCode != Core::ERROR_NONE) {
            LOGERR("IHDMIIn %p, errorcode: %u", notification, errorCode);
        } else {
            LOGINFO("IHDMIIn %p unregistered successfully", notification);
        }
        EXIT_LOG;
        return errorCode;
    }

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

    void DeviceSettingsManagerImp::OnHDMIInEventHotPlugNotification(const HDMIInPort port, const bool isConnected)
    {
        LOGINFO("OnHDMIInEventHotPlug event Received");
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInEventHotPlug, port, isConnected);
    }

    void DeviceSettingsManagerImp::OnHDMIInEventSignalStatusNotification(const HDMIInPort port, const HDMIInSignalStatus signalStatus)
    {
        LOGINFO("OnHDMIInEventSignalStatus event Received");
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInEventSignalStatus, port, signalStatus);
    }

    void DeviceSettingsManagerImp::OnHDMIInAVLatencyNotification(const int32_t audioDelay, const int32_t videoDelay)
    {
        LOGINFO("OnHDMIInAVLatency event Received");
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInAVLatency, audioDelay, videoDelay);
    }

    void DeviceSettingsManagerImp::OnHDMIInEventStatusNotification(const HDMIInPort activePort, const bool isPresented)
    {
        LOGINFO("OnHDMIInEventStatus event Received");
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInEventStatus, activePort, isPresented);
    }

    void DeviceSettingsManagerImp::OnHDMIInVideoModeUpdateNotification(const HDMIInPort port, const HDMIVideoPortResolution videoPortResolution)
    {
        LOGINFO("OnHDMIInVideoModeUpdate event Received");
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInVideoModeUpdate, port, videoPortResolution);
    }

    void DeviceSettingsManagerImp::OnHDMIInAllmStatusNotification(const HDMIInPort port, const bool allmStatus)
    {
        LOGINFO("OnHDMIInAllmStatus event Received");
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInAllmStatus, port, allmStatus);
    }

    void DeviceSettingsManagerImp::OnHDMIInAVIContentTypeNotification(const HDMIInPort port, const HDMIInAviContentType aviContentType)
    {
        LOGINFO("OnHDMIInAVIContentType event Received");
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInAVIContentType, port, aviContentType);
    }

    void DeviceSettingsManagerImp::OnHDMIInVRRStatusNotification(const HDMIInPort port, const HDMIInVRRType vrrType)
    {
        LOGINFO("OnHDMIInVRRStatus event Received");
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInVRRStatus, port, vrrType);
    }

    // FPD notification implementation
    void DeviceSettingsManagerImp::OnFPDTimeFormatChanged(const FPDTimeFormat timeFormat)
    {
        LOGINFO("OnFPDTimeFormatChanged event Received: timeFormat=%d", timeFormat);
        dispatchFPDEvent(&DeviceSettingsManagerFPD::INotification::OnFPDTimeFormatChanged, timeFormat);
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

    Core::hresult DeviceSettingsManagerImp::SetFPDBlink(const FPDIndicator indicator, const uint32_t blinkDuration, const uint32_t blinkIterations) {
        ENTRY_LOG;
        LOGINFO("SetFPDBlink: indicator=%d, blinkDuration=%u, blinkIterations=%u", indicator, blinkDuration, blinkIterations);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetFPDMode(const FPDMode fpdMode) {
        ENTRY_LOG;
        LOGINFO("SetFPDMode: fpdMode=%d", fpdMode);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }
    //Depricated

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

    Core::hresult DeviceSettingsManagerImp::GetHDMIInNumbefOfInputs(int32_t &count) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInNumberOfInputs");
        _apiLock.Lock();
        _hdmiIn.GetHDMIInNumberOfInputs(count);
        _apiLock.Unlock();
        LOGINFO("GetHDMIInNumberOfInputs: count=%d", count);
        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInStatus");

        _apiLock.Lock();
        _hdmiIn.GetHDMIInStatus(hdmiStatus, portConnectionStatus);
        _apiLock.Unlock();
        LOGINFO("GetHDMIInStatus: activePort=%d, isPresented=%s", hdmiStatus.activePort, hdmiStatus.isPresented ? "true" : "false");

        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) {
        ENTRY_LOG;

        LOGINFO("SelectHDMIInPort: port=%d, requestAudioMix=%s, topMostPlane=%s, videoPlaneType=%d",
            port, requestAudioMix ? "true" : "false", topMostPlane ? "true" : "false", videoPlaneType);
        _apiLock.Lock();
        _hdmiIn.SelectHDMIInPort(port, requestAudioMix, topMostPlane, videoPlaneType);
        _apiLock.Unlock();

        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) {
        ENTRY_LOG;

        LOGINFO("ScaleHDMIInVideo: x=%d, y=%d, w=%d, h=%d", videoPosition.x, videoPosition.y, videoPosition.width, videoPosition.height);
        _apiLock.Lock();
        _hdmiIn.ScaleHDMIInVideo(videoPosition);
        _apiLock.Unlock();

        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) {
        ENTRY_LOG;

        LOGINFO("SelectHDMIZoomMode: zoomMode=%d", zoomMode);
        _apiLock.Lock();
        _hdmiIn.SelectHDMIZoomMode(zoomMode);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetSupportedGameFeaturesList(DeviceSettingsManagerHDMIIn::IHDMIInGameFeatureListIterator *& gameFeatureList) {
        ENTRY_LOG;

        LOGINFO("GetSupportedGameFeaturesList");
        _apiLock.Lock();
        _hdmiIn.GetSupportedGameFeaturesList(gameFeatureList);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInAVLatency");
        _apiLock.Lock();
        _hdmiIn.GetHDMIInAVLatency(videoLatency, audioLatency);
        _apiLock.Unlock();
        LOGINFO("GetHDMIInAVLatency: videoLatency=%u, audioLatency=%u", videoLatency, audioLatency);

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInAllmStatus: port=%d", port);
        _apiLock.Lock();
        _hdmiIn.GetHDMIInAllmStatus(port, allmStatus);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInEdid2AllmSupport: port=%d", port);
        _apiLock.Lock();
        _hdmiIn.GetHDMIInEdid2AllmSupport(port, allmSupport);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) {
        ENTRY_LOG;

        LOGINFO("SetHDMIInEdid2AllmSupport: port=%d, allmSupport=%s", port, allmSupport ? "true" : "false");
        _apiLock.Lock();
        _hdmiIn.SetHDMIInEdid2AllmSupport(port, allmSupport);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]) {
        ENTRY_LOG;

        LOGINFO("GetEdidBytes: port=%d, edidBytesLength=%u", port, edidBytesLength);
        _apiLock.Lock();
        _hdmiIn.GetEdidBytes(port, edidBytesLength, edidBytes);
        _apiLock.Unlock();
        LOGINFO("GetEdidBytes: port=%d, edidBytes[0]=0x%X", port, edidBytes[0]);

        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) {
        ENTRY_LOG;

        LOGINFO("GetHDMISPDInformation: port=%d, spdBytesLength=%u", port, spdBytesLength);
        if (spdBytes && spdBytesLength > 0) {
            spdBytes[0] = 0x00; // Example value
        }
        _apiLock.Lock();
        _hdmiIn.GetHDMISPDInformation(port, spdBytesLength, spdBytes);
        _apiLock.Unlock();
        LOGINFO("GetHDMISPDInformation: port=%d, spdBytes[0]=0x%X", port, spdBytes[0]);

        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion) {
        ENTRY_LOG;

        LOGINFO("GetHDMIEdidVersion: port=%d", port);
        _apiLock.Lock();
        _hdmiIn.GetHDMIEdidVersion(port, edidVersion);
        _apiLock.Unlock();
        LOGINFO("GetHDMIEdidVersion: port=%d, edidVersion=%d", port, edidVersion);

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) {
        ENTRY_LOG;

        LOGINFO("SetHDMIEdidVersion: port=%d, edidVersion=%d", port, edidVersion);
        _apiLock.Lock();
        _hdmiIn.SetHDMIEdidVersion(port, edidVersion);
        _apiLock.Unlock();

        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution) {
        ENTRY_LOG;

        LOGINFO("GetHDMIVideoMode");

        _apiLock.Lock();
        _hdmiIn.GetHDMIVideoMode(videoPortResolution);
        _apiLock.Unlock();
        LOGINFO("GetHDMIVideoMode: resolution=%s", videoPortResolution.name.c_str());

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion) {
        ENTRY_LOG;

        LOGINFO("GetHDMIVersion: port=%d", port);
        _apiLock.Lock();
        _hdmiIn.GetHDMIVersion(port, capabilityVersion);
        _apiLock.Unlock();

        LOGINFO("GetHDMIVersion: port=%d, capabilityVersion=%d", port, capabilityVersion);

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetVRRSupport(const HDMIInPort port, bool &vrrSupport) {
        ENTRY_LOG;

        LOGINFO("GetVRRSupport: port=%d", port);
        _apiLock.Lock();
        _hdmiIn.GetVRRSupport(port, vrrSupport);
        _apiLock.Unlock();
        LOGINFO("GetVRRSupport: port=%d, vrrSupport=%s", port, vrrSupport ? "true" : "false");

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SetVRRSupport(const HDMIInPort port, const bool vrrSupport) {
        ENTRY_LOG;

        LOGINFO("SetVRRSupport: port=%d, vrrSupport=%s", port, vrrSupport ? "true" : "false");
        _apiLock.Lock();
        _hdmiIn.SetVRRSupport(port, vrrSupport);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus) {
        ENTRY_LOG;

        LOGINFO("GetVRRStatus: port=%d", port);
        _apiLock.Lock();
        _hdmiIn.GetVRRStatus(port, vrrStatus);
        _apiLock.Unlock();
        LOGINFO("GetVRRStatus: port=%d, vrrStatus.vrrType=%d", port, vrrStatus.vrrType);

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    // Audio Interface implementations
    Core::hresult DeviceSettingsManagerImp::GetAudioPort(const AudioPortType type, const int32_t index, int32_t &handle) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioPort: type=%d, index=%d", type, index);
        _apiLock.Lock();
        uint32_t result = _audio.GetAudioPort(type, index, handle);
        _apiLock.Unlock();
        
        if (result == Core::ERROR_NONE) {
            LOGINFO("GetAudioPort success: handle=%d", handle);
        } else {
            LOGERR("GetAudioPort failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::IsAudioPortEnabled(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        
        LOGINFO("IsAudioPortEnabled: handle=%d", handle);
        _apiLock.Lock();
        uint32_t result = _audio.IsAudioPortEnabled(handle, enabled);
        _apiLock.Unlock();
        
        if (result == Core::ERROR_NONE) {
            LOGINFO("IsAudioPortEnabled success: enabled=%d", enabled);
        } else {
            LOGERR("IsAudioPortEnabled failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::EnableAudioPort(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        
        LOGINFO("EnableAudioPort: handle=%d, enable=%d", handle, enable);
        _apiLock.Lock();
        uint32_t result = _audio.EnableAudioPort(handle, enable);
        _apiLock.Unlock();
        
        if (result != Core::ERROR_NONE) {
            LOGERR("EnableAudioPort failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetStereoMode(const int32_t handle, StereoModes &mode) {
        ENTRY_LOG;
        
        LOGINFO("GetStereoMode: handle=%d", handle);
        _apiLock.Lock();
        uint32_t result = _audio.GetStereoMode(handle, mode);
        _apiLock.Unlock();
        
        if (result == Core::ERROR_NONE) {
            LOGINFO("GetStereoMode success: mode=%d", mode);
        } else {
            LOGERR("GetStereoMode failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetStereoMode(const int32_t handle, const StereoModes mode, const bool persist) {
        ENTRY_LOG;
        
        LOGINFO("SetStereoMode: handle=%d, mode=%d, persist=%d", handle, mode, persist);
        _apiLock.Lock();
        uint32_t result = _audio.SetStereoMode(handle, mode, persist);
        _apiLock.Unlock();
        
        if (result != Core::ERROR_NONE) {
            LOGERR("SetStereoMode failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioMute(const int32_t handle, const bool mute) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioMute: handle=%d, mute=%d", handle, mute);
        _apiLock.Lock();
        uint32_t result = _audio.SetAudioMute(handle, mute);
        _apiLock.Unlock();
        
        if (result != Core::ERROR_NONE) {
            LOGERR("SetAudioMute failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::IsAudioMuted(const int32_t handle, bool &muted) {
        ENTRY_LOG;
        
        LOGINFO("IsAudioMuted: handle=%d", handle);
        _apiLock.Lock();
        uint32_t result = _audio.IsAudioMuted(handle, muted);
        _apiLock.Unlock();
        
        if (result == Core::ERROR_NONE) {
            LOGINFO("IsAudioMuted success: muted=%d", muted);
        } else {
            LOGERR("IsAudioMuted failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetSupportedARCTypes(const int32_t handle, int32_t &types) {
        ENTRY_LOG;
        
        LOGINFO("GetSupportedARCTypes: handle=%d", handle);
        _apiLock.Lock();
        uint32_t result = _audio.GetSupportedARCTypes(handle, types);
        _apiLock.Unlock();
        
        if (result == Core::ERROR_NONE) {
            LOGINFO("GetSupportedARCTypes success: types=%d", types);
        } else {
            LOGERR("GetSupportedARCTypes failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetSAD(const int32_t handle, const uint8_t sadList[], const uint8_t count) {
        ENTRY_LOG;
        
        LOGINFO("SetSAD: handle=%d, count=%d", handle, count);
        _apiLock.Lock();
        uint32_t result = _audio.SetSAD(handle, sadList, count);
        _apiLock.Unlock();
        
        if (result != Core::ERROR_NONE) {
            LOGERR("SetSAD failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::EnableARC(const int32_t handle, const AudioARCStatus arcStatus) {
        ENTRY_LOG;
        
        LOGINFO("EnableARC: handle=%d", handle);
        _apiLock.Lock();
        uint32_t result = _audio.EnableARC(handle, arcStatus);
        _apiLock.Unlock();
        
        if (result != Core::ERROR_NONE) {
            LOGERR("EnableARC failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetStereoAuto(const int32_t handle, int32_t &mode) {
        ENTRY_LOG;
        
        LOGINFO("GetStereoAuto: handle=%d", handle);
        _apiLock.Lock();
        uint32_t result = _audio.GetStereoAuto(handle, mode);
        _apiLock.Unlock();
        
        if (result == Core::ERROR_NONE) {
            LOGINFO("GetStereoAuto success: mode=%d", mode);
        } else {
            LOGERR("GetStereoAuto failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetStereoAuto(const int32_t handle, const int32_t mode, const bool persist) {
        ENTRY_LOG;
        
        LOGINFO("SetStereoAuto: handle=%d, mode=%d, persist=%d", handle, mode, persist);
        _apiLock.Lock();
        uint32_t result = _audio.SetStereoAuto(handle, mode, persist);
        _apiLock.Unlock();
        
        if (result != Core::ERROR_NONE) {
            LOGERR("SetStereoAuto failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioDucking(const int32_t handle, const AudioDuckingType duckingType, const AudioDuckingAction duckingAction, const uint8_t level) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioDucking: handle=%d, type=%d, action=%d, level=%d", handle, duckingType, duckingAction, level);
        _apiLock.Lock();
        uint32_t result = _audio.SetAudioDucking(handle, duckingType, duckingAction, level);
        _apiLock.Unlock();
        
        if (result != Core::ERROR_NONE) {
            LOGERR("SetAudioDucking failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioLevel(const int32_t handle, const float audioLevel) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioLevel: handle=%d, level=%f", handle, audioLevel);
        _apiLock.Lock();
        uint32_t result = _audio.SetAudioLevel(handle, audioLevel);
        _apiLock.Unlock();
        
        if (result != Core::ERROR_NONE) {
            LOGERR("SetAudioLevel failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioLevel(const int32_t handle, float &audioLevel) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioLevel: handle=%d", handle);
        _apiLock.Lock();
        uint32_t result = _audio.GetAudioLevel(handle, audioLevel);
        _apiLock.Unlock();
        
        if (result == Core::ERROR_NONE) {
            LOGINFO("GetAudioLevel success: level=%f", audioLevel);
        } else {
            LOGERR("GetAudioLevel failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioGain(const int32_t handle, const float gainLevel) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioGain: handle=%d, gain=%f", handle, gainLevel);
        _apiLock.Lock();
        uint32_t result = _audio.SetAudioGain(handle, gainLevel);
        _apiLock.Unlock();
        
        if (result != Core::ERROR_NONE) {
            LOGERR("SetAudioGain failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioGain(const int32_t handle, float &gainLevel) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioGain: handle=%d", handle);
        _apiLock.Lock();
        uint32_t result = _audio.GetAudioGain(handle, gainLevel);
        _apiLock.Unlock();
        
        if (result == Core::ERROR_NONE) {
            LOGINFO("GetAudioGain success: gain=%f", gainLevel);
        } else {
            LOGERR("GetAudioGain failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioFormat(const int32_t handle, AudioFormat &audioFormat) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioFormat: handle=%d", handle);
        _apiLock.Lock();
        uint32_t result = _audio.GetAudioFormat(handle, audioFormat);
        _apiLock.Unlock();
        
        if (result == Core::ERROR_NONE) {
            LOGINFO("GetAudioFormat success: format=%d", audioFormat);
        } else {
            LOGERR("GetAudioFormat failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioEncoding(const int32_t handle, AudioEncoding &encoding) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioEncoding: handle=%d", handle);
        _apiLock.Lock();
        uint32_t result = _audio.GetAudioEncoding(handle, encoding);
        _apiLock.Unlock();
        
        if (result == Core::ERROR_NONE) {
            LOGINFO("GetAudioEncoding success: encoding=%d", encoding);
        } else {
            LOGERR("GetAudioEncoding failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioEnablePersist(const int32_t handle, bool &enabled, string &portName) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioEnablePersist: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioEnablePersist not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioEnablePersist(const int32_t handle, const bool enable, const string portName) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioEnablePersist: handle=%d, enable=%d", handle, enable);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioEnablePersist not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::IsAudioMSDecoded(const int32_t handle, bool &hasms11Decode) {
        ENTRY_LOG;
        
        LOGINFO("IsAudioMSDecoded: handle=%d", handle);
        _apiLock.Lock();
        uint32_t result = _audio.IsAudioMSDecoding(handle, hasms11Decode);
        _apiLock.Unlock();
        
        if (result == Core::ERROR_NONE) {
            LOGINFO("IsAudioMSDecoded success: hasms11Decode=%d", hasms11Decode);
        } else {
            LOGERR("IsAudioMSDecoded failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::IsAudioMS12Decoded(const int32_t handle, bool &hasms12Decode) {
        ENTRY_LOG;
        
        LOGINFO("IsAudioMS12Decoded: handle=%d", handle);
        _apiLock.Lock();
        uint32_t result = _audio.IsAudioMS12Decoding(handle, hasms12Decode);
        _apiLock.Unlock();
        
        if (result == Core::ERROR_NONE) {
            LOGINFO("IsAudioMS12Decoded success: hasms12Decode=%d", hasms12Decode);
        } else {
            LOGERR("IsAudioMS12Decoded failed with result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioLEConfig(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioLEConfig: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioLEConfig not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::EnableAudioLEConfig(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        
        LOGINFO("EnableAudioLEConfig: handle=%d, enable=%d", handle, enable);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("EnableAudioLEConfig not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioDelay(const int32_t handle, const uint32_t audioDelay) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioDelay: handle=%d, delay=%u", handle, audioDelay);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioDelay not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioDelay(const int32_t handle, uint32_t &audioDelay) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioDelay: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioDelay not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioDelayOffset(const int32_t handle, const uint32_t delayOffset) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioDelayOffset: handle=%d, offset=%u", handle, delayOffset);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioDelayOffset not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioDelayOffset(const int32_t handle, uint32_t &delayOffset) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioDelayOffset: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioDelayOffset not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioSinkDeviceAtmosCapability(const int32_t handle, DolbyAtmosCapability &atmosCapability) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioSinkDeviceAtmosCapability: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioSinkDeviceAtmosCapability not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioAtmosOutputMode(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioAtmosOutputMode: handle=%d, enable=%d", handle, enable);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioAtmosOutputMode not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioCompression(const int32_t handle, const int32_t compressionLevel) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioCompression: handle=%d, level=%d", handle, compressionLevel);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioCompression not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioCompression(const int32_t handle, int32_t &compressionLevel) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioCompression: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioCompression not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioDialogEnhancement(const int32_t handle, const int32_t level) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioDialogEnhancement: handle=%d, level=%d", handle, level);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioDialogEnhancement not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioDialogEnhancement(const int32_t handle, int32_t &level) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioDialogEnhancement: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioDialogEnhancement not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioDolbyVolumeMode(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioDolbyVolumeMode: handle=%d, enable=%d", handle, enable);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioDolbyVolumeMode not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioDolbyVolumeMode(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioDolbyVolumeMode: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioDolbyVolumeMode not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioIntelligentEqualizerMode(const int32_t handle, const int32_t mode) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioIntelligentEqualizerMode: handle=%d, mode=%d", handle, mode);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioIntelligentEqualizerMode not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioIntelligentEqualizerMode(const int32_t handle, int32_t &mode) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioIntelligentEqualizerMode: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioIntelligentEqualizerMode not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioVolumeLeveller(const int32_t handle, const VolumeLeveller volumeLeveller) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioVolumeLeveller: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioVolumeLeveller not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioVolumeLeveller(const int32_t handle, VolumeLeveller &volumeLeveller) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioVolumeLeveller: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioVolumeLeveller not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioBassEnhancer(const int32_t handle, const int32_t boost) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioBassEnhancer: handle=%d, boost=%d", handle, boost);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioBassEnhancer not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioBassEnhancer(const int32_t handle, int32_t &boost) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioBassEnhancer: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioBassEnhancer not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::EnableAudioSurroudDecoder(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        
        LOGINFO("EnableAudioSurroudDecoder: handle=%d, enable=%d", handle, enable);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("EnableAudioSurroudDecoder not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::IsAudioSurroudDecoderEnabled(const int32_t handle, bool &enabled) {
        ENTRY_LOG;
        
        LOGINFO("IsAudioSurroudDecoderEnabled: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("IsAudioSurroudDecoderEnabled not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioDRCMode(const int32_t handle, const int32_t drcMode) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioDRCMode: handle=%d, mode=%d", handle, drcMode);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioDRCMode not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioDRCMode(const int32_t handle, int32_t &drcMode) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioDRCMode: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioDRCMode not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioSurroudVirtualizer(const int32_t handle, const SurroundVirtualizer surroundVirtualizer) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioSurroudVirtualizer: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioSurroudVirtualizer not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioSurroudVirtualizer(const int32_t handle, SurroundVirtualizer &surroundVirtualizer) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioSurroudVirtualizer: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioSurroudVirtualizer not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioMISteering(const int32_t handle, const bool enable) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioMISteering: handle=%d, enable=%d", handle, enable);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioMISteering not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioMISteering(const int32_t handle, bool &enable) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioMISteering: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioMISteering not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioGraphicEqualizerMode(const int32_t handle, const int32_t mode) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioGraphicEqualizerMode: handle=%d, mode=%d", handle, mode);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioGraphicEqualizerMode not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioGraphicEqualizerMode(const int32_t handle, int32_t &mode) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioGraphicEqualizerMode: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioGraphicEqualizerMode not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioMS12ProfileList(const int32_t handle, WPEFramework::Exchange::IDeviceSettingsManagerAudio::IDeviceSettingsAudioMS12AudioProfileIterator*& ms12ProfileList) const {
        ENTRY_LOG;
        
        LOGINFO("GetAudioMS12ProfileList: handle=%d", handle);
        // Note: This method is not implemented in the HAL layer yet
        // Cannot use _apiLock in const method
        uint32_t result = Core::ERROR_UNAVAILABLE;
        ms12ProfileList = nullptr;
        
        LOGERR("GetAudioMS12ProfileList not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioMS12Profile(const int32_t handle, string &profile) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioMS12Profile: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioMS12Profile not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioMS12Profile(const int32_t handle, const string profile) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioMS12Profile: handle=%d, profile=%s", handle, profile.c_str());
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioMS12Profile not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioMixerLevels(const int32_t handle, const AudioInput audioInput, const int32_t volume) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioMixerLevels: handle=%d, input=%d, volume=%d", handle, static_cast<int>(audioInput), volume);
        _apiLock.Lock();
        uint32_t result = Core::ERROR_NONE;
        
        // Call platform layer with appropriate parameters
        // For now, pass the volume to both primary and input since we only have one volume parameter
        result = _audio.SetAudioMixerLevels(handle, volume, volume);
        
        _apiLock.Unlock();
        
        if (result != Core::ERROR_NONE) {
            LOGERR("SetAudioMixerLevels failed, result=%u", result);
        }
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAssociatedAudioMixing(const int32_t handle, const bool mixing) {
        ENTRY_LOG;
        
        LOGINFO("SetAssociatedAudioMixing: handle=%d, mixing=%d", handle, mixing);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAssociatedAudioMixing not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAssociatedAudioMixing(const int32_t handle, bool &mixing) {
        ENTRY_LOG;
        
        LOGINFO("GetAssociatedAudioMixing: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAssociatedAudioMixing not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioFaderControl(const int32_t handle, const int32_t mixerBalance) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioFaderControl: handle=%d, balance=%d", handle, mixerBalance);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioFaderControl not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioFaderControl(const int32_t handle, int32_t &mixerBalance) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioFaderControl: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioFaderControl not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioPrimaryLanguage(const int32_t handle, const string primaryAudioLanguage) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioPrimaryLanguage: handle=%d, language=%s", handle, primaryAudioLanguage.c_str());
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioPrimaryLanguage not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioPrimaryLanguage(const int32_t handle, string &primaryAudioLanguage) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioPrimaryLanguage: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioPrimaryLanguage not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioSecondaryLanguage(const int32_t handle, const string secondaryAudioLanguage) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioSecondaryLanguage: handle=%d, language=%s", handle, secondaryAudioLanguage.c_str());
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioSecondaryLanguage not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioSecondaryLanguage(const int32_t handle, string &secondaryAudioLanguage) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioSecondaryLanguage: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioSecondaryLanguage not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioCapabilities(const int32_t handle, int32_t &capabilities) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioCapabilities: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioCapabilities not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioMS12Capabilities(const int32_t handle, int32_t &capabilities) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioMS12Capabilities: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioMS12Capabilities not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::SetAudioMS12SettingsOverride(const int32_t handle, const string profileName, const string profileSettingsName, const string profileSettingValue, const string profileState) {
        ENTRY_LOG;
        
        LOGINFO("SetAudioMS12SettingsOverride: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("SetAudioMS12SettingsOverride not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::IsAudioOutputConnected(const int32_t handle, bool &isConnected) {
        ENTRY_LOG;
        
        LOGINFO("IsAudioOutputConnected: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("IsAudioOutputConnected not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::ResetAudioDialogEnhancement(const int32_t handle) {
        ENTRY_LOG;
        
        LOGINFO("ResetAudioDialogEnhancement: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("ResetAudioDialogEnhancement not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::ResetAudioBassEnhancer(const int32_t handle) {
        ENTRY_LOG;
        
        LOGINFO("ResetAudioBassEnhancer: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("ResetAudioBassEnhancer not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::ResetAudioSurroundVirtualizer(const int32_t handle) {
        ENTRY_LOG;
        
        LOGINFO("ResetAudioSurroundVirtualizer: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("ResetAudioSurroundVirtualizer not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::ResetAudioVolumeLeveller(const int32_t handle) {
        ENTRY_LOG;
        
        LOGINFO("ResetAudioVolumeLeveller: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("ResetAudioVolumeLeveller not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    Core::hresult DeviceSettingsManagerImp::GetAudioHDMIARCPortId(const int32_t handle, int32_t &portId) {
        ENTRY_LOG;
        
        LOGINFO("GetAudioHDMIARCPortId: handle=%d", handle);
        _apiLock.Lock();
        // Note: This method is not implemented in the HAL layer yet
        uint32_t result = Core::ERROR_UNAVAILABLE;
        _apiLock.Unlock();
        
        LOGERR("GetAudioHDMIARCPortId not implemented, result=%u", result);
        
        EXIT_LOG;
        return result;
    }

    // Audio notification implementations
    void DeviceSettingsManagerImp::OnAssociatedAudioMixingChangedNotification(bool mixing) {
        LOGINFO("OnAssociatedAudioMixingChanged event Received: mixing=%d", mixing);
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAssociatedAudioMixingChanged, mixing);
    }

    void DeviceSettingsManagerImp::OnAudioOutHotPlugNotification(AudioPortType portType, uint32_t uiPortNumber, bool isPortConnected) {
        LOGINFO("OnAudioOutHotPlug event Received: portType=%d, port=%d, connected=%d", portType, uiPortNumber, isPortConnected);
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioOutHotPlug, portType, uiPortNumber, isPortConnected);
    }

    void DeviceSettingsManagerImp::OnAudioFormatUpdateNotification(AudioFormat audioFormat) {
        LOGINFO("OnAudioFormatUpdate event Received: format=%d", audioFormat);
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioFormatUpdate, audioFormat);
    }

    void DeviceSettingsManagerImp::OnAudioLevelChangedEventNotification(int32_t audioLevel) {
        LOGINFO("OnAudioLevelChangedEvent event Received: level=%d", audioLevel);
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioLevelChangedEvent, audioLevel);
    }

    void DeviceSettingsManagerImp::OnAudioFaderControlChangedNotification(int32_t mixerBalance) {
        LOGINFO("OnAudioFaderControlChanged event Received: balance=%d", mixerBalance);
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioFaderControlChanged, mixerBalance);
    }

    void DeviceSettingsManagerImp::OnAudioPrimaryLanguageChangedNotification(const string& primaryLanguage) {
        LOGINFO("OnAudioPrimaryLanguageChanged event Received: language=%s", primaryLanguage.c_str());
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioPrimaryLanguageChanged, primaryLanguage);
    }

    void DeviceSettingsManagerImp::OnAudioSecondaryLanguageChangedNotification(const string& secondaryLanguage) {
        LOGINFO("OnAudioSecondaryLanguageChanged event Received: language=%s", secondaryLanguage.c_str());
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioSecondaryLanguageChanged, secondaryLanguage);
    }

    void DeviceSettingsManagerImp::OnDolbyAtmosCapabilitiesChangedNotification(DolbyAtmosCapability atmosCapability, bool status) {
        LOGINFO("OnDolbyAtmosCapabilitiesChanged event Received: capability=%d, status=%d", atmosCapability, status);
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnDolbyAtmosCapabilitiesChanged, atmosCapability, status);
    }

    void DeviceSettingsManagerImp::OnAudioPortStateChangedNotification(AudioPortState audioPortState) {
        LOGINFO("OnAudioPortStateChanged event Received: state=%d", audioPortState);
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioPortStateChanged, audioPortState);
    }

    void DeviceSettingsManagerImp::OnAudioModeEventNotification(AudioPortType audioPortType, StereoModes audioMode) {
        LOGINFO("OnAudioModeEvent notification received: portType=%d, mode=%d", static_cast<int>(audioPortType), static_cast<int>(audioMode));
        dispatchAudioEvent(&DeviceSettingsManagerAudio::INotification::OnAudioModeEvent, audioPortType, audioMode);
    }

} // namespace Plugin
} // namespace WPEFramework
