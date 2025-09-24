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

#include <functional> // for function
#include <unistd.h>   // for access, F_OK

#include <core/IAction.h>    // for IDispatch
#include <core/Time.h>       // for Time
#include <core/WorkerPool.h> // for IWorkerPool, WorkerPool

#include "UtilsLogging.h"   // for LOGINFO, LOGERR
#include "secure_wrapper.h" // for v_secure_system
#include "HdmiIn.h"

using IPlatform = hal::dHdmiIn::IPlatform;
using DefaultImpl = dHdmiInImpl;

using HDMIInPort               = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort;

HdmiIn::HdmiIn(INotification& parent, std::shared_ptr<IPlatform> platform) 
    : _parent(parent)
    , _platform(std::move(platform))
{
    ENTRY_LOG;
    LOGINFO("HdmiIn Constructor");
    init();
    EXIT_LOG;
}

void HdmiIn::init()
{
    ENTRY_LOG;
    LOGINFO("HdmiIn Init");

    CallbackBundle bundle;
    bundle.OnHDMIInEventHotPlug = [this](HDMIInPort port, bool isConnected) {
        this->OnHDMIInEventHotPlug(port, isConnected);
    };
    if (_platform) {
        _platform->setAllCallbacks(bundle);
    }
    EXIT_LOG;
}

void HdmiIn::OnHDMIInEventHotPlug(const HDMIInPort port, const bool isConnected)
{
    LOGINFO("OnHDMIInEventHotPlug event Received");
}

uint32_t HdmiIn::GetHDMIInNumbefOfInputs(int32_t &count) {
    ENTRY_LOG;

    LOGINFO("GetHDMIInNumbefOfInputs");
    count = 2; // Example value

    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

/*uint32_t HdmiIn::GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) {
    ENTRY_LOG;

    LOGINFO("GetHDMIInStatus");
    hdmiStatus.activePort = HDMIInPort::DS_HDMI_IN_PORT_0; // Example value
    hdmiStatus.isPresented = true; // Example value
    portConnectionStatus = nullptr; // Example value
    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}*/

uint32_t HdmiIn::SelectHDMIInPort(const HDMIInPort port, const bool requestAudioMix, const bool topMostPlane, const HDMIVideoPlaneType videoPlaneType) {
    ENTRY_LOG;

    LOGINFO("SelectHDMIInPort: port=%d, requestAudioMix=%s, topMostPlane=%s, videoPlaneType=%d",
        port, requestAudioMix ? "true" : "false", topMostPlane ? "true" : "false", videoPlaneType);
    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) {
    ENTRY_LOG;

    LOGINFO("ScaleHDMIInVideo: x=%d, y=%d, w=%d, h=%d", videoPosition.x, videoPosition.y, videoPosition.width, videoPosition.height);
    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) {
    ENTRY_LOG;

    LOGINFO("SelectHDMIZoomMode: zoomMode=%d", zoomMode);

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

/*uint32_t HdmiIn::GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator *& gameFeatureList) {
    ENTRY_LOG;

    LOGINFO("GetSupportedGameFeaturesList");
    gameFeatureList = nullptr; // Example value

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}*/

uint32_t HdmiIn::GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency) {
    ENTRY_LOG;

    LOGINFO("GetHDMIInAVLatency");
    videoLatency = 10; // Example value
    audioLatency = 5; // Example value

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus) {
    ENTRY_LOG;

    LOGINFO("GetHDMIInAllmStatus: port=%d", port);
    allmStatus = true; // Example value

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) {
    ENTRY_LOG;

    LOGINFO("GetHDMIInEdid2AllmSupport: port=%d", port);
    allmSupport = true; // Example value

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) {
    ENTRY_LOG;

    LOGINFO("SetHDMIInEdid2AllmSupport: port=%d, allmSupport=%s", port, allmSupport ? "true" : "false");

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]) {
    ENTRY_LOG;

    LOGINFO("GetEdidBytes: port=%d, edidBytesLength=%u", port, edidBytesLength);
    if (edidBytes && edidBytesLength > 0) {
        edidBytes[0] = 0x00; // Example value
    }
    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) {
    ENTRY_LOG;

    LOGINFO("GetHDMISPDInformation: port=%d, spdBytesLength=%u", port, spdBytesLength);
    if (spdBytes && spdBytesLength > 0) {
        spdBytes[0] = 0x00; // Example value
    }
    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion) {
    ENTRY_LOG;

    LOGINFO("GetHDMIEdidVersion: port=%d", port);
    edidVersion = HDMIInEdidVersion::HDMI_EDID_VER_14; // Example value
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) {
    ENTRY_LOG;

    LOGINFO("SetHDMIEdidVersion: port=%d, edidVersion=%d", port, edidVersion);
    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution) {
    ENTRY_LOG;

    LOGINFO("GetHDMIVideoMode");
    videoPortResolution.name = "1920x1080"; // Example value

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion) {
    ENTRY_LOG;

    LOGINFO("GetHDMIVersion: port=%d", port);
    capabilityVersion = HDMIInCapabilityVersion::HDMI_COMPATIBILITY_VERSION_20; // Example value

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetVRRSupport(const HDMIInPort port, bool &vrrSupport) {
    ENTRY_LOG;

    LOGINFO("GetVRRSupport: port=%d", port);
    vrrSupport = true; // Example value

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus) {
    ENTRY_LOG;

    LOGINFO("GetVRRStatus: port=%d", port);
    vrrStatus.vrrType = HDMIInVRRType::DS_HDMIIN_HDMI_VRR; // Example value

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}