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

#include "DeviceSettingsHdmiInImplementation.h"

#include "UtilsLogging.h"
#include <syscall.h>

using namespace std;

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(DeviceSettingsHdmiInImp, 1, 0);

    DeviceSettingsHdmiInImp* DeviceSettingsHdmiInImp::_instance = nullptr;

    DeviceSettingsHdmiInImp::DeviceSettingsHdmiInImp()
        : _hdmiIn(HdmiIn::Create(*this))
    {
        ENTRY_LOG;
        DeviceSettingsHdmiInImp::_instance = this;
        LOGINFO("DeviceSettingsHdmiInImp Constructor - Instance Address: %p", this);
        EXIT_LOG;
    }

    DeviceSettingsHdmiInImp::~DeviceSettingsHdmiInImp() {
        ENTRY_LOG;
        LOGINFO("DeviceSettingsHdmiInImp Destructor - Instance Address: %p", this);
        EXIT_LOG;
    }

    template<typename Func, typename... Args>
    void DeviceSettingsHdmiInImp::dispatchHDMIInEvent(Func notifyFunc, Args&&... args) {
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


    template <typename T>
    Core::hresult DeviceSettingsHdmiInImp::Register(std::list<T*>& list, T* notification)
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
    Core::hresult DeviceSettingsHdmiInImp::Unregister(std::list<T*>& list, const T* notification)
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


    Core::hresult DeviceSettingsHdmiInImp::Register(DeviceSettingsHDMIIn::INotification* notification)
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

    Core::hresult DeviceSettingsHdmiInImp::Unregister(DeviceSettingsHDMIIn::INotification* notification)
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

    void DeviceSettingsHdmiInImp::OnHDMIInEventHotPlugNotification(const HDMIInPort port, const bool isConnected)
    {
        ENTRY_LOG;
        LOGINFO("OnHDMIInEventHotPlug event Received");
        dispatchHDMIInEvent(&DeviceSettingsHDMIIn::INotification::OnHDMIInEventHotPlug, port, isConnected);
        EXIT_LOG;
    }

    void DeviceSettingsHdmiInImp::OnHDMIInEventSignalStatusNotification(const HDMIInPort port, const HDMIInSignalStatus signalStatus)
    {
        ENTRY_LOG;
        LOGINFO("OnHDMIInEventSignalStatus event Received");
        dispatchHDMIInEvent(&DeviceSettingsHDMIIn::INotification::OnHDMIInEventSignalStatus, port, signalStatus);
        EXIT_LOG;
    }

    void DeviceSettingsHdmiInImp::OnHDMIInAVLatencyNotification(const int32_t audioDelay, const int32_t videoDelay)
    {
        ENTRY_LOG;
        LOGINFO("OnHDMIInAVLatency event Received");
        dispatchHDMIInEvent(&DeviceSettingsHDMIIn::INotification::OnHDMIInAVLatency, audioDelay, videoDelay);
        EXIT_LOG;
    }

    void DeviceSettingsHdmiInImp::OnHDMIInEventStatusNotification(const HDMIInPort activePort, const bool isPresented)
    {
        ENTRY_LOG;
        LOGINFO("OnHDMIInEventStatus event Received");
        dispatchHDMIInEvent(&DeviceSettingsHDMIIn::INotification::OnHDMIInEventStatus, activePort, isPresented);
        EXIT_LOG;
    }

    void DeviceSettingsHdmiInImp::OnHDMIInVideoModeUpdateNotification(const HDMIInPort port, const HDMIVideoPortResolution videoPortResolution)
    {
        ENTRY_LOG;
        LOGINFO("OnHDMIInVideoModeUpdate event Received");
        dispatchHDMIInEvent(&DeviceSettingsHDMIIn::INotification::OnHDMIInVideoModeUpdate, port, videoPortResolution);
        EXIT_LOG;
    }

    void DeviceSettingsHdmiInImp::OnHDMIInAllmStatusNotification(const HDMIInPort port, const bool allmStatus)
    {
        ENTRY_LOG;
        LOGINFO("OnHDMIInAllmStatus event Received");
        dispatchHDMIInEvent(&DeviceSettingsHDMIIn::INotification::OnHDMIInAllmStatus, port, allmStatus);
        EXIT_LOG;
    }

    void DeviceSettingsHdmiInImp::OnHDMIInAVIContentTypeNotification(const HDMIInPort port, const HDMIInAviContentType aviContentType)
    {
        ENTRY_LOG;
        LOGINFO("OnHDMIInAVIContentType event Received");
        dispatchHDMIInEvent(&DeviceSettingsHDMIIn::INotification::OnHDMIInAVIContentType, port, aviContentType);
        EXIT_LOG;
    }

    void DeviceSettingsHdmiInImp::OnHDMIInVRRStatusNotification(const HDMIInPort port, const HDMIInVRRType vrrType)
    {
        ENTRY_LOG;
        LOGINFO("OnHDMIInVRRStatus event Received");
        dispatchHDMIInEvent(&DeviceSettingsHDMIIn::INotification::OnHDMIInVRRStatus, port, vrrType);
        EXIT_LOG;
    }

    Core::hresult DeviceSettingsHdmiInImp::GetHDMIInNumbefOfInputs(int32_t &count) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInNumberOfInputs");
        _apiLock.Lock();
        _hdmiIn.GetHDMIInNumberOfInputs(count);
        _apiLock.Unlock();
        LOGINFO("GetHDMIInNumberOfInputs: count=%d", count);
        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInStatus");

        _apiLock.Lock();
        _hdmiIn.GetHDMIInStatus(hdmiStatus, portConnectionStatus);
        _apiLock.Unlock();
        LOGINFO("GetHDMIInStatus: activePort=%d, isPresented=%s", hdmiStatus.activePort, hdmiStatus.isPresented ? "true" : "false");

        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) {
        ENTRY_LOG;

        LOGINFO("SelectHDMIInPort: port=%d, requestAudioMix=%s, topMostPlane=%s, videoPlaneType=%d",
            port, requestAudioMix ? "true" : "false", topMostPlane ? "true" : "false", videoPlaneType);
        _apiLock.Lock();
        _hdmiIn.SelectHDMIInPort(port, requestAudioMix, topMostPlane, videoPlaneType);
        _apiLock.Unlock();

        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) {
        ENTRY_LOG;

        LOGINFO("ScaleHDMIInVideo: x=%d, y=%d, w=%d, h=%d", videoPosition.x, videoPosition.y, videoPosition.width, videoPosition.height);
        _apiLock.Lock();
        _hdmiIn.ScaleHDMIInVideo(videoPosition);
        _apiLock.Unlock();

        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) {
        ENTRY_LOG;

        LOGINFO("SelectHDMIZoomMode: zoomMode=%d", zoomMode);
        _apiLock.Lock();
        _hdmiIn.SelectHDMIZoomMode(zoomMode);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator *& gameFeatureList) {
        ENTRY_LOG;

        LOGINFO("GetSupportedGameFeaturesList");
        _apiLock.Lock();
        _hdmiIn.GetSupportedGameFeaturesList(gameFeatureList);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInAVLatency");
        _apiLock.Lock();
        _hdmiIn.GetHDMIInAVLatency(videoLatency, audioLatency);
        _apiLock.Unlock();
        LOGINFO("GetHDMIInAVLatency: videoLatency=%u, audioLatency=%u", videoLatency, audioLatency);

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInAllmStatus: port=%d", port);
        _apiLock.Lock();
        _hdmiIn.GetHDMIInAllmStatus(port, allmStatus);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) {
        ENTRY_LOG;

        LOGINFO("GetHDMIInEdid2AllmSupport: port=%d", port);
        _apiLock.Lock();
        _hdmiIn.GetHDMIInEdid2AllmSupport(port, allmSupport);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) {
        ENTRY_LOG;

        LOGINFO("SetHDMIInEdid2AllmSupport: port=%d, allmSupport=%s", port, allmSupport ? "true" : "false");
        _apiLock.Lock();
        _hdmiIn.SetHDMIInEdid2AllmSupport(port, allmSupport);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]) {
        ENTRY_LOG;

        LOGINFO("GetEdidBytes: port=%d, edidBytesLength=%u", port, edidBytesLength);
        _apiLock.Lock();
        _hdmiIn.GetEdidBytes(port, edidBytesLength, edidBytes);
        _apiLock.Unlock();
        LOGINFO("GetEdidBytes: port=%d, edidBytes[0]=0x%X", port, edidBytes[0]);

        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) {
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

    Core::hresult DeviceSettingsHdmiInImp::GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion) {
        ENTRY_LOG;

        LOGINFO("GetHDMIEdidVersion: port=%d", port);
        _apiLock.Lock();
        _hdmiIn.GetHDMIEdidVersion(port, edidVersion);
        _apiLock.Unlock();
        LOGINFO("GetHDMIEdidVersion: port=%d, edidVersion=%d", port, edidVersion);

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) {
        ENTRY_LOG;

        LOGINFO("SetHDMIEdidVersion: port=%d, edidVersion=%d", port, edidVersion);
        _apiLock.Lock();
        _hdmiIn.SetHDMIEdidVersion(port, edidVersion);
        _apiLock.Unlock();

        EXIT_LOG;

        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution) {
        ENTRY_LOG;

        LOGINFO("GetHDMIVideoMode");

        _apiLock.Lock();
        _hdmiIn.GetHDMIVideoMode(videoPortResolution);
        _apiLock.Unlock();
        LOGINFO("GetHDMIVideoMode: resolution=%s", videoPortResolution.name.c_str());

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion) {
        ENTRY_LOG;

        LOGINFO("GetHDMIVersion: port=%d", port);
        _apiLock.Lock();
        _hdmiIn.GetHDMIVersion(port, capabilityVersion);
        _apiLock.Unlock();

        LOGINFO("GetHDMIVersion: port=%d, capabilityVersion=%d", port, capabilityVersion);

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::GetVRRSupport(const HDMIInPort port, bool &vrrSupport) {
        ENTRY_LOG;

        LOGINFO("GetVRRSupport: port=%d", port);
        _apiLock.Lock();
        _hdmiIn.GetVRRSupport(port, vrrSupport);
        _apiLock.Unlock();
        LOGINFO("GetVRRSupport: port=%d, vrrSupport=%s", port, vrrSupport ? "true" : "false");

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::SetVRRSupport(const HDMIInPort port, const bool vrrSupport) {
        ENTRY_LOG;

        LOGINFO("SetVRRSupport: port=%d, vrrSupport=%s", port, vrrSupport ? "true" : "false");
        _apiLock.Lock();
        _hdmiIn.SetVRRSupport(port, vrrSupport);
        _apiLock.Unlock();

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsHdmiInImp::GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus) {
        ENTRY_LOG;

        LOGINFO("GetVRRStatus: port=%d", port);
        _apiLock.Lock();
        _hdmiIn.GetVRRStatus(port, vrrStatus);
        _apiLock.Unlock();
        LOGINFO("GetVRRStatus: port=%d, vrrStatus.vrrType=%d", port, vrrStatus.vrrType);

        EXIT_LOG;
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
