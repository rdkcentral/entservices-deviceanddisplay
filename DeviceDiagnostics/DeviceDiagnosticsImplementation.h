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

#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#ifdef ENABLE_ERM
#include <essos-resmgr.h>
#define AVDECODERSTATUS_RETRY_INTERVAL 30 // sec
#endif
#ifdef RDK_LOG_MILESTONE
#include "rdk_logger_milestone.h"
#endif

#include "Module.h"
#include <interfaces/Ids.h>
#include <interfaces/IDeviceDiagnostics.h>

#include <com/com.h>
#include <core/core.h>

namespace WPEFramework
{
    namespace Plugin
    {
        class DeviceDiagnosticsImplementation : public Exchange::IDeviceDiagnostics
        {
            public:
                // We do not allow this plugin to be copied !!
                DeviceDiagnosticsImplementation();
                ~DeviceDiagnosticsImplementation() override;

                // We do not allow this plugin to be copied !!
                DeviceDiagnosticsImplementation(const DeviceDiagnosticsImplementation&) = delete;
                DeviceDiagnosticsImplementation& operator=(const DeviceDiagnosticsImplementation&) = delete;

                BEGIN_INTERFACE_MAP(DeviceDiagnosticsImplementation)
                INTERFACE_ENTRY(Exchange::IDeviceDiagnostics)
                END_INTERFACE_MAP

            public:
                enum Event
                {
                    ON_AVDECODER_STATUSCHANGED
                };
 
            class EXTERNAL Job : public Core::IDispatch {
            protected:
                Job(DeviceDiagnosticsImplementation* deviceDiagnosticsImplementation, Event event, JsonValue &params)
                    : _deviceDiagnosticsImplementation(deviceDiagnosticsImplementation)
                    , _event(event)
                    , _params(params) {
                    if (_deviceDiagnosticsImplementation != nullptr) {
                        _deviceDiagnosticsImplementation->AddRef();
                    }
                }

            public:
                Job() = delete;
                Job(const Job&) = delete;
                Job& operator=(const Job&) = delete;
                ~Job() {
                    if (_deviceDiagnosticsImplementation != nullptr) {
                        _deviceDiagnosticsImplementation->Release();
                    }
                }

            public:
                static Core::ProxyType<Core::IDispatch> Create(DeviceDiagnosticsImplementation* deviceDiagnosticsImplementation, Event event, JsonValue  params ) {
#ifndef USE_THUNDER_R4
                    return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<Job>::Create(deviceDiagnosticsImplementation, event, params)));
#else
                    return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(deviceDiagnosticsImplementation, event, params)));
#endif
                }

                virtual void Dispatch() {
                    _deviceDiagnosticsImplementation->Dispatch(_event, _params);
                }

            private:
                DeviceDiagnosticsImplementation *_deviceDiagnosticsImplementation;
                const Event _event;
                JsonValue _params;
        };
        public:
            virtual Core::hresult Register(Exchange::IDeviceDiagnostics::INotification *notification ) override ;
            virtual Core::hresult Unregister(Exchange::IDeviceDiagnostics::INotification *notification ) override;

            Core::hresult GetConfiguration(IStringIterator* const& names, Exchange::IDeviceDiagnostics::IDeviceDiagnosticsParamListIterator*& paramList, bool& success) override;
            Core::hresult GetMilestones(IStringIterator*& milestones, bool& success) override;
            Core::hresult LogMilestone(const string& marker, bool& success) override;
            Core::hresult GetAVDecoderStatus(AvDecoderStatusResult& AVDecoderStatus) override;

        private:
            mutable Core::CriticalSection _adminLock;
            PluginHost::IShell* _service;
            std::list<Exchange::IDeviceDiagnostics::INotification*> _deviceDiagnosticsNotification;

#ifdef ENABLE_ERM
            std::thread m_AVPollThread;
            std::mutex m_AVDecoderStatusLock;
            EssRMgr* m_EssRMgr;
            int m_pollThreadRun;
            std::condition_variable m_avDecoderStatusCv;
#endif

            int getMostActiveDecoderStatus();
            void onDecoderStatusChange(int status);
            int getConfig(const std::string& postData, std::list<ParamList>& paramListInfo);

#ifdef ENABLE_ERM
            static void *AVPollThread(void *arg);
#endif
            void dispatchEvent(Event, const JsonValue &params);
            void Dispatch(Event event, const JsonValue params);
        public:
            static DeviceDiagnosticsImplementation* _instance;

            friend class Job;
        };
    } // namespace Plugin
} // namespace WPEFramework
