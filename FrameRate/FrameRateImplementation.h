/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
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
 **/

#pragma once

#include <mutex>

#include "Module.h"

#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>
#include <interfaces/Ids.h>
#include <interfaces/IFrameRate.h>
#include "tracing/Logging.h"

#include "tptimer.h"
#include "libIARM.h"

#include <vector>
#include <boost/variant.hpp>

namespace WPEFramework {
    namespace Plugin {
        class FrameRateImplementation : public Exchange::IFrameRate {
            public:
                // We do not allow this plugin to be copied !!
                FrameRateImplementation();
                ~FrameRateImplementation() override;

                static FrameRateImplementation* instance(FrameRateImplementation *FrameRateImpl = nullptr);

                // We do not allow this plugin to be copied !!
                FrameRateImplementation(const FrameRateImplementation&) = delete;
                FrameRateImplementation& operator=(const FrameRateImplementation&) = delete;

                BEGIN_INTERFACE_MAP(FrameRateImplementation)
                    INTERFACE_ENTRY(Exchange::IFrameRate)
                END_INTERFACE_MAP

            public:
                enum Event
                {
                    //ON_FPS_EVENT,
                    DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE,
                    DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE
                };
                class EXTERNAL Job : public Core::IDispatch {
                    protected:
                        Job(FrameRateImplementation* FrameRateImplementation, Event event, JsonValue &params)
                            : _framerateImplementation(FrameRateImplementation)
                              , _event(event)
                              , _params(params) {
                                  if (_framerateImplementation != nullptr) {
                                      _framerateImplementation->AddRef();
                                  }
                              }

                    public:
                        Job() = delete;
                        Job(const Job&) = delete;
                        Job& operator=(const Job&) = delete;
                        ~Job() {
                            if (_framerateImplementation != nullptr) {
                                _framerateImplementation->Release();
                            }
                        }

                    public:
                        static Core::ProxyType<Core::IDispatch> Create(FrameRateImplementation* framerateImplementation, Event event, JsonValue params) {
#ifndef USE_THUNDER_R4
                           return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<Job>::Create(framerateImplementation, event, params)));
#else
                           return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(framerateImplementation, event, params)));
#endif
                        }

                        virtual void Dispatch() {
                            _framerateImplementation->DispatchDSMGRDisplayFramerateChangeEvent(_event, _params);
                        }

                    private:
                        FrameRateImplementation *_framerateImplementation;
                        const Event _event;
                        JsonValue _params;
                };
            public:
                virtual Core::hresult Register(Exchange::IFrameRate::INotification *notification) override;
                virtual Core::hresult Unregister(Exchange::IFrameRate::INotification *notification) override;

                //Begin methods
                Core::hresult GetDisplayFrameRate(string& framerate, bool& success) override;
                Core::hresult GetFrmMode(int &frmmode, bool& success) override;
                Core::hresult SetCollectionFrequency(int frequency, bool& success) override;
                Core::hresult SetDisplayFrameRate(const string& framerate , bool& success) override;
                Core::hresult SetFrmMode(int frmmode, bool& success) override;
                Core::hresult StartFpsCollection(bool& success) override;
                Core::hresult StopFpsCollection(bool& success) override;
                Core::hresult UpdateFps(int newFpsValue, bool& success) override;
                //End methods

                int getCollectionFrequency();
                void fpsCollectionUpdate( int averageFps, int minFps, int maxFps);
                virtual void enableFpsCollection() {}
                virtual void disableFpsCollection() {}
                void onReportFpsTimer();
                void onReportFpsTimerTest();
                void InitializeIARM();
                void DeinitializeIARM();
                void frameRatePreChange(char *displayFrameRate);
                static void FrameRatePreChange(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
                void frameRatePostChange(char *displayFrameRate);
                static void FrameRatePostChange(const char *owner, IARM_EventId_t eventId, void *data, size_t len);

                static FrameRateImplementation* _instance;

            private:
                std::shared_ptr<WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>> m_systemServiceConnection;
                mutable Core::CriticalSection _adminLock;
                Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> _engine;
                Core::ProxyType<RPC::CommunicatorClient> _communicatorClient;
                PluginHost::IShell* _service;
                std::list<Exchange::IFrameRate::INotification*> _framerateNotification;

                void dispatchOnFpsEvent(int average, int min, int max);
                void dispatchOnDisplayFrameRateChangingEvent(const string& displayFrameRate);
                void dispatchOnDisplayFrameRateChangedEvent(const string& displayFrameRate);
                void DispatchDSMGRDisplayFramerateChangeEvent(Event event, const JsonValue params);

            private:
                int m_fpsCollectionFrequencyInMs;
                int m_minFpsValue;
                int m_maxFpsValue;
                int m_totalFpsValues;
                int m_numberOfFpsUpdates;
                bool m_fpsCollectionInProgress;
                TpTimer m_reportFpsTimer;
                int m_lastFpsValue;
                std::mutex m_callMutex;
                friend class Job;
        };
    } // namespace Plugin
} // namespace WPEFramework
