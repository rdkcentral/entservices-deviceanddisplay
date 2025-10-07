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

//#include <functional> // for function
//#include <unistd.h>   // for access, F_OK

//#include <core/IAction.h>    // for IDispatch
//#include <core/Time.h>       // for Time
//#include <core/WorkerPool.h> // for IWorkerPool, WorkerPool

#include "UtilsLogging.h"   // for LOGINFO, LOGERR
//#include "secure_wrapper.h" // for v_secure_system
#include "HdmiIn.h"

using IPlatform = hal::dHdmiIn::IPlatform;
using DefaultImpl = dHdmiInImpl;

using HDMIInPort               = WPEFramework::Exchange::IDeviceSettingsManager::IHDMIIn::HDMIInPort;

// Out-of-line destructor for hal::dHdmiIn::IPlatform to ensure typeinfo symbol is generated
#include "hal/dHdmiIn.h"
namespace hal {
namespace dHdmiIn {
    IPlatform::~IPlatform() {}
}
}

HdmiIn::HdmiIn(INotification& parent, std::shared_ptr<IPlatform> platform) 
    : _platform(std::move(platform))
    , _parent(parent)
{
    ENTRY_LOG;
    LOGINFO("HdmiIn Constructor");
    LOGINFO("HDMI version: %s\n", HdmiConnectionToStrMapping[0].name);
    LOGINFO("HDMI version: %s\n", HdmiStatusToStrMapping[0].name);
    LOGINFO("HDMI version: %s\n", HdmiVerToStrMapping[0].name);
    Platform_init();
    EXIT_LOG;
}

void HdmiIn::Platform_init()
{
    ENTRY_LOG;
    LOGINFO("HdmiIn::Platform_init");

    CallbackBundle bundle;
    bundle.OnHDMIInHotPlugEvent = [this](HDMIInPort port, bool isConnected) {
        this->OnHDMIInHotPlugEvent(port, isConnected);
    };
    bundle.OnHDMIInSignalStatusEvent = [this](HDMIInPort port, HDMIInSignalStatus signalStatus) {
        this->OnHDMIInSignalStatusEvent(port, signalStatus);
    };
    bundle.OnHDMIInStatusEvent = [this](HDMIInPort port, bool isConnected) {
        this->OnHDMIInStatusEvent(port, isConnected);
    };
    bundle.OnHDMIInVideoModeUpdateEvent = [this](HDMIInPort port, HDMIVideoPortResolution videoPortResolution) {
        this->OnHDMIInVideoModeUpdateEvent(port, videoPortResolution);
    };
    bundle.OnHDMIInAllmStatusEvent = [this](HDMIInPort port, bool allmStatus) {
        this->OnHDMIInAllmStatusEvent(port, allmStatus);
    };
    bundle.OnHDMIInAVIContentTypeEvent = [this](HDMIInPort port, HDMIInAviContentType aviContentType) {
        this->OnHDMIInAVIContentTypeEvent(port, aviContentType);
    };
    bundle.OnHDMIInAVLatencyEvent = [this](int32_t audioDelay, int32_t videoDelay) {
        this->OnHDMIInAVLatencyEvent(audioDelay, videoDelay);
    };
    bundle.OnHDMIInVRRStatusEvent = [this](HDMIInPort port, HDMIInVRRType vrrType) {
        this->OnHDMIInVRRStatusEvent(port, vrrType);
    };
    if (_platform) {
        _platform->setAllCallbacks(bundle);
        _platform->getPersistenceValue();
    }
    EXIT_LOG;
}

void HdmiIn::OnHDMIInHotPlugEvent(const HDMIInPort port, const bool isConnected)
{
    _parent.OnHDMIInEventHotPlugNotification(port, isConnected);
    LOGINFO("OnHDMIInHotPlugEvent event Received");
}

void HdmiIn::OnHDMIInSignalStatusEvent(const HDMIInPort port, const HDMIInSignalStatus signalStatus)
{
    _parent.OnHDMIInEventSignalStatusNotification(port, signalStatus);
    LOGINFO("OnHDMIInSignalStatusEvent event Received");
}

void HdmiIn::OnHDMIInStatusEvent(const HDMIInPort port, const bool isPresented)
{
    _parent.OnHDMIInEventStatusNotification(port, isPresented);
    LOGINFO("OnHDMIInStatusEvent event Received");
}

void HdmiIn::OnHDMIInVideoModeUpdateEvent(const HDMIInPort port, const HDMIVideoPortResolution videoPortResolution)
{
    _parent.OnHDMIInVideoModeUpdateNotification(port, videoPortResolution);
    LOGINFO("OnHDMIInVideoModeUpdateEvent event Received");
}

void HdmiIn::OnHDMIInAllmStatusEvent(const HDMIInPort port, const bool allmStatus)
{
    _parent.OnHDMIInAllmStatusNotification(port, allmStatus);
    LOGINFO("OnHDMIInAllmStatusEvent event Received");
}

void HdmiIn::OnHDMIInAVIContentTypeEvent(const HDMIInPort port, const HDMIInAviContentType aviContentType)
{
    _parent.OnHDMIInAVIContentTypeNotification(port, aviContentType);
    LOGINFO("OnHDMIInAVIContentTypeEvent event Received");
}

void HdmiIn::OnHDMIInAVLatencyEvent(int32_t audioDelay, int32_t videoDelay)
{
    _parent.OnHDMIInAVLatencyNotification(audioDelay, videoDelay);
    LOGINFO("OnHDMIInAVLatencyEvent event Received");
}

void HdmiIn::OnHDMIInVRRStatusEvent(const HDMIInPort port, const HDMIInVRRType vrrType)
{
    _parent.OnHDMIInVRRStatusNotification(port, vrrType);
    LOGINFO("OnHDMIInVRRStatusEvent event Received");
}

uint32_t HdmiIn::GetHDMIInNumbefOfInputs(int32_t &count) {
    ENTRY_LOG;

    LOGINFO("GetHDMIInNumbefOfInputs");
    count = 2; // Example value

    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) {
    ENTRY_LOG;

    LOGINFO("GetHDMIInStatus");
    hdmiStatus.activePort = HDMIInPort::DS_HDMI_IN_PORT_0; // Example value
    hdmiStatus.isPresented = true; // Example value
    portConnectionStatus = nullptr; // Example value
    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

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

uint32_t HdmiIn::GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator *& gameFeatureList) {
    ENTRY_LOG;

    LOGINFO("GetSupportedGameFeaturesList");
    gameFeatureList = nullptr; // Example value

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

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

uint32_t HdmiIn::SetVRRSupport(const HDMIInPort port, const bool vrrSupport) {
    ENTRY_LOG;

    LOGINFO("SetVRRSupport: port=%d, vrrSupport=%s", port, vrrSupport ? "true" : "false");

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