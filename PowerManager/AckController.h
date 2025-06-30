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
#include <cstdint>
#include <unordered_set>

#include <core/Portability.h>
#include <core/Timer.h>

#include "UtilsLogging.h"
#include "interfaces/IPowerManager.h"

/**
 * @class AckTimer
 * @brief Manages timeout functionality for acknowledgement operations.
 *        This class internally manages a timer thread to handle timeouts.
 *
 * @tparam T The parent type that implements the Timed function.
 */
template <typename T>
class AckTimer {
public:
    /**
     * @brief Constructs an AckTimer instance.
     * @param parent The parent object that implements the Timed function.
     */
    AckTimer(T& parent)
        : _parent(parent)
        , _timeout(WPEFramework::Core::Time {})
    {
    }

    /**
     * @brief Schedules the timer with a specified timeout.
     * @param timeout The timeout value to start the timer with.
     */
    inline void Schedule(const WPEFramework::Core::Time& timeout)
    {
        _timeout = timeout;
        timerThread.Schedule(_timeout, *this);
    }

    /**
     * @brief Reschedules / updates timer with specified timeout.
     *        In case if timout was already set, it will be replaced with new timeout.
     * @param timeout The timeout value to start timer with.
     */
    inline void Reschedule(const WPEFramework::Core::Time& timeout)
    {
        timerThread.Trigger(timeout.Ticks(), *this);
    }

    /**
     * @brief Revokes / stops the scheduled timer. The timer will not be triggered after this.
     * @return True if the timer was successfully revoked, otherwise false.
     */
    inline bool Revoke()
    {
        return timerThread.Revoke(*this);
    }

    /**
     * @brief Callback function for timer expiration.
     * @param timeout The timeout duration.
     * @return The result of the Timed function from the parent.
     */
    uint64_t Timed(uint64_t timeout)
    {
        return _parent.Timed(timeout);
    }

    /**
     * @brief Checks if the timer is currently running.
     * @return True if the timer is running, otherwise false.
     */
    bool IsRunning() const
    {
        return _timeout > WPEFramework::Core::Time {} && timerThread.HasEntry(*this);
    }

    /**
     * @brief Equality operator for comparing two AckTimer instances.
     * @param rhs The other AckTimer instance to compare.
     * @return True if both instances are equal, otherwise false.
     */
    bool operator==(const AckTimer& rhs) const
    {
        // pointer comparison
        return &_parent == &(rhs._parent) && (_timeout == rhs._timeout);
    }

    /**
     * @brief Inequality operator for comparing two AckTimer instances.
     * @param rhs The other AckTimer instance to compare.
     * @return True if both instances are not equal, otherwise false.
     */
    bool operator!=(const AckTimer& rhs) const
    {
        return !(*this == rhs);
    }

    /**
     * @brief Greater-than operator for comparing two AckTimer instances.
     * @param rhs The other AckTimer instance to compare.
     * @return True if this instance has a greater timeout than the other, otherwise false.
     */
    bool operator>(const AckTimer& rhs) const
    {
        return _timeout > rhs._timeout;
    }

    /**
     * @brief Gets the current timeout value.
     * @return The current timeout value.
     */
    const WPEFramework::Core::Time& Timeout() const
    {
        return _timeout;
    }

private:
    T& _parent;
    WPEFramework::Core::Time _timeout;
    static WPEFramework::Core::TimerType<AckTimer<T>> timerThread;
};

/**
 * @class AckController
 * @brief Controls Awaits acknowledgement operations, timer timeouts & completion handler.
 *        The completion handler is triggered in one of these scenarios:
 *        - Acknowledgement received from all clients.
 *        - Scheduled timer times out.
 *        - Scheduled without any clients awaiting.
 *
 * IMPORTANT: This class is not thread-safe. It expects thread safety
 *            from the instantiating class.
 */
class AckController {
    using AckTimer_t = AckTimer<AckController>;
    using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;

public:
    /**
     * @brief Constructs an AckController instance for given `powerState` transition
     *        The TransactionId is unique for each instance.
     */
    AckController(PowerState powerState)
        : _powerState(powerState)
        , _transactionId(++_nextTransactionId)
        , _timer(*this)
        , _handler(nullptr)
        , _running(false)
    {
    }

    /**
     * @brief Destroys an AckController instance.
     *        If running, the controller will be revoked.
     *        The completion handler will not be called.
     */
    ~AckController()
    {
        revoke();
    }

    AckController(const AckController& o)            = delete;
    AckController& operator=(const AckController& o) = delete;
    AckController(AckController&& o)                 = delete;
    AckController& operator=(AckController&& o)      = delete;

    /**
     * @brief target power state for current state transition session
     */
    inline PowerState powerState() const
    {
        return _powerState;
    }

    /**
     * @brief Adds an expectation to await an acknowledgement from the given client.
     * @param clientId The ID of the client to await acknowledgement from.
     */
    void AckAwait(const uint32_t clientId)
    {
        _pending.insert(clientId);
        LOGINFO("Append clientId: %u, transactionId: %d, pending %d", clientId, _transactionId, int(_pending.size()));
    }

    /**
     * @brief Removes the expectation to await an acknowledgement from the given client associated with the given transaction ID.
     *        On acknowledgement from the last client, the completion handler will be triggered.
     * @param clientId The ID of the client.
     * @param transactionId The transaction ID associated with the client.
     * @return status - ERROR_INVALID_PARAMETER if the transaction ID is invalid.
     *                  ERROR_NONE on success.
     */
    uint32_t Ack(const uint32_t clientId, const int transactionId)
    {
        uint32_t status = WPEFramework::Core::ERROR_NONE;

        do {

            if (transactionId != _transactionId) {
                LOGERR("Invalid transactionId: %d", transactionId);
                status = WPEFramework::Core::ERROR_INVALID_PARAMETER;
                break;
            }

            _pending.erase(clientId);

            if (_pending.empty() && _running) {
                _running = false;
                runHandler(false);
            }
        } while (false);

        LOGINFO("AckController::Ack: clientId: %u, transactionId: %d, status: %d, pending %d",
            clientId, transactionId, status, int(_pending.size()));
        return WPEFramework::Core::ERROR_NONE;
    }

    /**
     * @brief Removes the expectation to await an acknowledgement from the given client.
     * @param clientId The ID of the client.
     * @return status - ERROR_INVALID_PARAMETER if the client ID is invalid.
     *                  ERROR_NONE on success.
     */
    uint32_t Ack(const uint32_t clientId)
    {
        return Ack(clientId, _transactionId);
    }

    /**
     * @brief Gets the current transaction ID.
     * @return The current transaction ID.
     */
    int TransactionId() const
    {
        return _transactionId;
    }

    /**
     * @brief Checks if the controller is running.
     * @return True if the controller is running, otherwise false.
     */
    bool IsRunning() const
    {
        return _running && _timer.IsRunning();
    }

    /**
     * @brief Gets the set of pending client IDs.
     * @return A set of pending client IDs.
     */
    const std::unordered_set<uint32_t>& Pending() const
    {
        return _pending;
    }

    /**
     * @brief Schedules a completion handler trigger with a timeout.
     *
     * @param offsetInMilliseconds The timeout offset in milliseconds.
     * @param handler The completion handler to be invoke.
     *        The completion handler is triggered in one of these scenarios:
     *        - Acknowledgement received from all clients (Triggered from the last Ack caller thread).
     *        - Scheduled timer times out (Triggered in the timerThread thread).
     *        - Scheduled without any clients awaiting (Triggered in the caller thread).
     */
    template <typename COMPLETION_HANDLER>
    void Schedule(const uint64_t offsetInMilliseconds, COMPLETION_HANDLER handler)
    {
        ASSERT(false == _running);
        ASSERT(nullptr == _handler);

        _handler = handler;

        LOGINFO("time offset: %lldms, pending: %d", offsetInMilliseconds, int(_pending.size()));

        if (_pending.empty()) {
            // no clients acks to wait for, trigger completion handler immediately
            runHandler(false);
        } else {
            _running = true;
            _timer.Schedule(WPEFramework::Core::Time::Now().Add(offsetInMilliseconds));
        }
    }

    /**
     * @brief Advances the timeout of the completion handler.
     * @param clientId The ID of the client.
     * @param transactionId The transaction ID associated with the client.
     * @param offsetInMilliseconds The new timeout offset in milliseconds from the current time.
     * @return status - ERROR_INVALID_PARAMETER if the client ID or transaction ID is invalid.
     *                  ERROR_ILLEGAL_STATE if the controller is not running.
     *                  ERROR_NONE on success.
     */
    uint32_t Reschedule(const uint32_t clientId, const int transactionId, const int offsetInMilliseconds)
    {
        uint32_t status = WPEFramework::Core::ERROR_NONE;
        ASSERT(IsRunning());
        ASSERT(nullptr != _handler);

        do {
            if (!_running) {
                status = WPEFramework::Core::ERROR_ILLEGAL_STATE;
                break;
            }

            if (transactionId != _transactionId) {
                LOGERR("Invalid transactionId: %d", transactionId);
                status = WPEFramework::Core::ERROR_INVALID_PARAMETER;
                break;
            }

            const auto it = _pending.find(clientId);
            if (it == _pending.cend()) {
                LOGERR("Invalid clientId: %u", clientId);
                status = WPEFramework::Core::ERROR_INVALID_PARAMETER;
                break;
            }

            auto newTimeout = WPEFramework::Core::Time::Now().Add(offsetInMilliseconds);

            // set new timeout only if it's greater than previous timeout, else fail silently
            if (newTimeout > _timer.Timeout()) {
                _timer.Reschedule(newTimeout);
            } else {
                LOGWARN("Skipping new timeout %lld is less than previous timeout %lld",
                    newTimeout.Ticks(), _timer.Timeout().Ticks());
            }
        } while (false);

        LOGINFO("clientId: %u, transactionId: %d, offset: %d, status: %d",
            clientId, transactionId, offsetInMilliseconds, status);

        return status;
    }

    /**
     * @brief Handles the AckController timeout event.
     *        Ideally, this method should have been made private, but due to
     *        Thunder Timer implementation limitations, it's made public.
     *
     * IMPORTANT: This API is not meant to be invoked by clients.
     *
     * @param timeout The timeout duration.
     * @return Always returns 0 (to make the implementation generic).
     */
    uint64_t Timed(uint64_t timeout)
    {
        // ack timedout, invoke completion handler now
        if (_running) {
            _running = false;
            runHandler(true);
        }
        return 0;
    }

private:
    /**
     * @brief Executes the completion handler.
     * @param isTimedout Indicates whether the handler is triggered due to timeout.
     */
    void runHandler(bool isTimedout)
    {
        LOGINFO("isTimedout: %d, pending: %d", isTimedout, int(_pending.size()));
        if (!isTimedout) {
            _timer.Revoke();
        }
        _handler(isTimedout);
    }

    /**
     * @brief Stops or revokes the AckController if it is already running.
     *        After revoke the completion handler will not be called.
     *        This method is deliberately void; use `IsRunning` to check the status.
     */
    void revoke()
    {
        if (_running) {
            _running = false;
            _timer.Revoke();
        }
    }

private:
    PowerState _powerState;
    std::unordered_set<uint32_t> _pending; // Set of pending acknowledgements.
    int _transactionId;                    // Unique transaction ID for each AckController instance.
    AckTimer_t _timer;                     // Timer for managing completion timeouts.
    std::function<void(bool)> _handler;    // Completion handler to be called on timeout or all acknowledgements.
    std::atomic<bool> _running;            // Flag to synchronize timer timeout callback and Ack* APIs.

    static int _nextTransactionId; // static counter for unique transaction ID generation.
};
