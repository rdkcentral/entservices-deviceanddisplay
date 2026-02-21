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
    // HDMIIn Implementation Methods
    // ================================

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

    Core::hresult DeviceSettingsManagerImp::GetHDMIInNumbefOfInputs(int32_t &count) {
        ENTRY_LOG;
        LOGINFO("GetHDMIInNumbefOfInputs");
        _apiLock.Lock();
        count = _hdmiIn.GetHDMIInNumbefOfInputs(count);
        _apiLock.Unlock();
        LOGINFO("GetHDMIInNumbefOfInputs: count=%d", count);
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) {
        ENTRY_LOG;
        LOGINFO("GetHDMIInStatus");
        
        _apiLock.Lock();
        _hdmiIn.GetHDMIInStatus(hdmiStatus, portConnectionStatus);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) {
        ENTRY_LOG;
        LOGINFO("SelectHDMIInPort: port=%d, audioMix=%s, topMost=%s, planeType=%d", 
                port, requestAudioMix ? "true" : "false", topMostPlane ? "true" : "false", videoPlaneType);
        
        _apiLock.Lock();
        _hdmiIn.SelectHDMIInPort(port, requestAudioMix, topMostPlane, videoPlaneType);
        _apiLock.Unlock();
        
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) {
        ENTRY_LOG;
        LOGINFO("ScaleHDMIInVideo: x=%d, y=%d, width=%d, height=%d", videoPosition.x, videoPosition.y, videoPosition.width, videoPosition.height);
        
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
        
        LOGINFO("GetHDMIInAllmStatus: port=%d, allmStatus=%s", port, allmStatus ? "true" : "false");
        EXIT_LOG;
        return Core::ERROR_NONE;
    }

    Core::hresult DeviceSettingsManagerImp::GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) {
        ENTRY_LOG;
        LOGINFO("GetHDMIInEdid2AllmSupport: port=%d", port);
        
        _apiLock.Lock();
        _hdmiIn.GetHDMIInEdid2AllmSupport(port, allmSupport);
        _apiLock.Unlock();
        
        LOGINFO("GetHDMIInEdid2AllmSupport: port=%d, allmSupport=%s", port, allmSupport ? "true" : "false");
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

    // ================================
    // HDMIIn Event Handlers
    // ================================

    void DeviceSettingsManagerImp::OnHDMIInEventHotPlugNotification(const HDMIInPort port, const bool isConnected)
    {
        LOGINFO("OnHDMIInEventHotPlugNotification: port=%d, connected=%s", port, isConnected ? "true" : "false");
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInEventHotPlug, port, isConnected);
    }

    void DeviceSettingsManagerImp::OnHDMIInEventSignalStatusNotification(const HDMIInPort port, const HDMIInSignalStatus signalStatus)
    {
        LOGINFO("OnHDMIInEventSignalStatusNotification: port=%d, signalStatus=%d", port, signalStatus);
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInEventSignalStatus, port, signalStatus);
    }

    void DeviceSettingsManagerImp::OnHDMIInAVLatencyNotification(const int32_t audioDelay, const int32_t videoDelay)
    {
        LOGINFO("OnHDMIInAVLatencyNotification: audioDelay=%d, videoDelay=%d", audioDelay, videoDelay);
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInAVLatency, audioDelay, videoDelay);
    }

    void DeviceSettingsManagerImp::OnHDMIInEventStatusNotification(const HDMIInPort activePort, const bool isPresented)
    {
        LOGINFO("OnHDMIInEventStatusNotification: activePort=%d, isPresented=%s", activePort, isPresented ? "true" : "false");
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInEventStatus, activePort, isPresented);
    }

    void DeviceSettingsManagerImp::OnHDMIInVideoModeUpdateNotification(const HDMIInPort port, const HDMIVideoPortResolution videoPortResolution)
    {
        LOGINFO("OnHDMIInVideoModeUpdateNotification: port=%d", port);
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInVideoModeUpdate, port, videoPortResolution);
    }

    void DeviceSettingsManagerImp::OnHDMIInAllmStatusNotification(const HDMIInPort port, const bool allmStatus)
    {
        LOGINFO("OnHDMIInAllmStatusNotification: port=%d, allmStatus=%s", port, allmStatus ? "true" : "false");
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInAllmStatus, port, allmStatus);
    }

    void DeviceSettingsManagerImp::OnHDMIInAVIContentTypeNotification(const HDMIInPort port, const HDMIInAviContentType aviContentType)
    {
        LOGINFO("OnHDMIInAVIContentTypeNotification: port=%d, aviContentType=%d", port, aviContentType);
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInAVIContentType, port, aviContentType);
    }

    void DeviceSettingsManagerImp::OnHDMIInVRRStatusNotification(const HDMIInPort port, const HDMIInVRRType vrrType)
    {
        LOGINFO("OnHDMIInVRRStatusNotification: port=%d, vrrType=%d", port, vrrType);
        dispatchHDMIInEvent(&DeviceSettingsManagerHDMIIn::INotification::OnHDMIInVRRStatus, port, vrrType);
    }

} // namespace Plugin
} // namespace WPEFramework