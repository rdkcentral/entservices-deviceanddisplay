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
#include "DeviceSettingsManagerTypes.h"

using IPlatform = hal::dHdmiIn::IPlatform;
using DefaultImpl = dHdmiInImpl;

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
    // HDMI In service initialized
    Platform_init();
}

void HdmiIn::Platform_init()
{
    // Initialize HDMI In platform

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
        this->platform().setAllCallbacks(bundle);
        this->platform().getPersistenceValue();
    }
}

void HdmiIn::OnHDMIInHotPlugEvent(const HDMIInPort port, const bool isConnected)
{
    _parent.OnHDMIInEventHotPlugNotification(port, isConnected);
}

void HdmiIn::OnHDMIInSignalStatusEvent(const HDMIInPort port, const HDMIInSignalStatus signalStatus)
{
    _parent.OnHDMIInEventSignalStatusNotification(port, signalStatus);
}

void HdmiIn::OnHDMIInStatusEvent(const HDMIInPort activePort, const bool isPresented)
{
    _parent.OnHDMIInEventStatusNotification(activePort, isPresented);
}

void HdmiIn::OnHDMIInVideoModeUpdateEvent(const HDMIInPort port, const HDMIVideoPortResolution videoPortResolution)
{
    _parent.OnHDMIInVideoModeUpdateNotification(port, videoPortResolution);
}

void HdmiIn::OnHDMIInAllmStatusEvent(const HDMIInPort port, const bool allmStatus)
{
    _parent.OnHDMIInAllmStatusNotification(port, allmStatus);
}

void HdmiIn::OnHDMIInAVIContentTypeEvent(const HDMIInPort port, const HDMIInAviContentType aviContentType)
{
    _parent.OnHDMIInAVIContentTypeNotification(port, aviContentType);
}

void HdmiIn::OnHDMIInAVLatencyEvent(const int32_t audioDelay, const int32_t videoDelay)
{
    _parent.OnHDMIInAVLatencyNotification(audioDelay, videoDelay);
}

void HdmiIn::OnHDMIInVRRStatusEvent(const HDMIInPort port, const HDMIInVRRType vrrType)
{
    _parent.OnHDMIInVRRStatusNotification(port, vrrType);
}

uint32_t HdmiIn::GetHDMIInNumberOfInputs(int32_t &count) {
    ENTRY_LOG;

    LOGINFO("GetHDMIInNumbefOfInputs");
    this->platform().GetHDMIInNumberOfInputs(count);

    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMIInStatus(HDMIInStatus &hdmiStatus, IHDMIInPortConnectionStatusIterator*& portConnectionStatus) {
    ENTRY_LOG;

    LOGINFO("GetHDMIInStatus");
    this->platform().GetHDMIInStatus(hdmiStatus, portConnectionStatus);
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
    this->platform().SelectHDMIInPort(port, requestAudioMix, topMostPlane, videoPlaneType);
    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::ScaleHDMIInVideo(const HDMIInVideoRectangle videoPosition) {
    ENTRY_LOG;

    LOGINFO("ScaleHDMIInVideo: x=%d, y=%d, w=%d, h=%d", videoPosition.x, videoPosition.y, videoPosition.width, videoPosition.height);
    this->platform().ScaleHDMIInVideo(videoPosition);
    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::SelectHDMIZoomMode(const HDMIInVideoZoom zoomMode) {
    ENTRY_LOG;

    LOGINFO("SelectHDMIZoomMode: zoomMode=%d", zoomMode);
    this->platform().SelectHDMIZoomMode(zoomMode);

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetSupportedGameFeaturesList(IHDMIInGameFeatureListIterator *& gameFeatureList) {
    ENTRY_LOG;

    LOGINFO("GetSupportedGameFeaturesList");
    this->platform().GetSupportedGameFeaturesList(gameFeatureList);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMIInAVLatency(uint32_t &videoLatency, uint32_t &audioLatency) {
    ENTRY_LOG;

    LOGINFO("GetHDMIInAVLatency");
    this->platform().GetHDMIInAVLatency(videoLatency, audioLatency);
    videoLatency = 10; // Example value
    audioLatency = 5; // Example value

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMIInAllmStatus(const HDMIInPort port, bool &allmStatus) {
    ENTRY_LOG;

    LOGINFO("GetHDMIInAllmStatus: port=%d", port);
    this->platform().GetHDMIInAllmStatus(port, allmStatus);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMIInEdid2AllmSupport(const HDMIInPort port, bool &allmSupport) {
    ENTRY_LOG;

    LOGINFO("GetHDMIInEdid2AllmSupport: port=%d", port);
    this->platform().GetHDMIInEdid2AllmSupport(port, allmSupport);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::SetHDMIInEdid2AllmSupport(const HDMIInPort port, bool allmSupport) {
    ENTRY_LOG;

    LOGINFO("SetHDMIInEdid2AllmSupport: port=%d, allmSupport=%s", port, allmSupport ? "true" : "false");
    this->platform().SetHDMIInEdid2AllmSupport(port, allmSupport);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetEdidBytes(const HDMIInPort port, const uint16_t edidBytesLength, uint8_t edidBytes[]) {
    ENTRY_LOG;

    LOGINFO("GetEdidBytes: port=%d, edidBytesLength=%u", port, edidBytesLength);
    this->platform().GetEdidBytes(port, edidBytesLength, edidBytes);
    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMISPDInformation(const HDMIInPort port, const uint16_t spdBytesLength, uint8_t spdBytes[]) {
    ENTRY_LOG;

    LOGINFO("GetHDMISPDInformation: port=%d, spdBytesLength=%u", port, spdBytesLength);
    this->platform().GetHDMISPDInformation(port, spdBytesLength, spdBytes);
    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMIEdidVersion(const HDMIInPort port, HDMIInEdidVersion &edidVersion) {
    ENTRY_LOG;

    LOGINFO("GetHDMIEdidVersion: port=%d", port);
    this->platform().GetHDMIEdidVersion(port, edidVersion);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::SetHDMIEdidVersion(const HDMIInPort port, const HDMIInEdidVersion edidVersion) {
    ENTRY_LOG;

    this->platform().SetHDMIEdidVersion(port, edidVersion);
    LOGINFO("SetHDMIEdidVersion: port=%d, edidVersion=%d", port, edidVersion);
    EXIT_LOG;

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMIVideoMode(HDMIVideoPortResolution &videoPortResolution) {
    ENTRY_LOG;

    LOGINFO("GetHDMIVideoMode");
    this->platform().GetHDMIVideoMode(videoPortResolution);

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetHDMIVersion(const HDMIInPort port, HDMIInCapabilityVersion &capabilityVersion) {
    ENTRY_LOG;

    LOGINFO("GetHDMIVersion: port=%d", port);
    this->platform().GetHDMIVersion(port, capabilityVersion);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetVRRSupport(const HDMIInPort port, bool &vrrSupport) {
    ENTRY_LOG;

    LOGINFO("GetVRRSupport: port=%d", port);
    this->platform().GetVRRSupport(port, vrrSupport);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::SetVRRSupport(const HDMIInPort port, const bool vrrSupport) {
    ENTRY_LOG;

    LOGINFO("SetVRRSupport: port=%d, vrrSupport=%s", port, vrrSupport ? "true" : "false");
    this->platform().SetVRRSupport(port, vrrSupport);
    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t HdmiIn::GetVRRStatus(const HDMIInPort port, HDMIInVRRStatus &vrrStatus) {
    ENTRY_LOG;

    LOGINFO("GetVRRStatus: port=%d", port);
    this->platform().GetVRRStatus(port, vrrStatus);
    LOGINFO("GetVRRStatus: port=%d, vrrType=%d", port, vrrStatus.vrrType);

    EXIT_LOG;
    return WPEFramework::Core::ERROR_NONE;
}