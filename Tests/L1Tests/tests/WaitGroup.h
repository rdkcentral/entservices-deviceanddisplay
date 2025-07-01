#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

class WaitGroup {
public:
    WaitGroup()
        : _count(0)
    {
    }

    void Add(int count = 1)
    {
        _count += count;

        if (_count <= 0)
            _cv.notify_all();
    }

    void Done() { Add(-1); }

    void Wait()
    {
        if (_count <= 0)
            return;

        std::unique_lock<std::mutex> _lock { _m };
        _cv.wait(_lock, [&]() {
            return _count == 0;
        });
    }

    // avoid copies of this class
    WaitGroup(const WaitGroup&) = delete;
    WaitGroup operator=(const WaitGroup&) = delete;

private:
    std::atomic<int> _count;
    std::mutex _m;
    std::condition_variable _cv;
};
