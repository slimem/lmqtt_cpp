#pragma once

#include "lmqtt_common.h"

namespace lmqtt {

class lmqtt_timer {
public:
    lmqtt_timer(size_t time, const std::function<void(void)>& f)
        : _time(std::chrono::milliseconds(time)), _f(f) {}

    ~lmqtt_timer() {
        worker.join();
    }

    void reset(size_t time) {
        worker.join();
        _time = std::chrono::milliseconds(time);
        //worker = [this]() { wait_for_call(); } };
    }

    void run() {
        while (!exiting) {
            std::unique_lock<std::mutex> lock(mtx);
            //cv.wait(lock, [this] {return has_work || exiting; });
            cv.wait_for(lock, _time);
            if (exiting) {
                break;
            }
            _f();
            has_work = false;
            lock.unlock();
            cv.notify_all();
        }
    }

private:
    void wait_for_call() {
        std::unique_lock<std::mutex> lck{ mtx };
        for (int i{ 10 }; i > 0; --i) {
            std::cout << "Thread " << worker.get_id() << " countdown at: " << '\t' << i << '\n';
            cv.wait_for(lck, _time / 10);
        }
        _f();
    }

    bool exiting = false;
    bool has_work = true;

    std::mutex mtx;
    std::condition_variable cv;
    std::chrono::milliseconds _time;
    std::function<void(void)> _f;

    //std::thread worker{ [this]() { wait_for_call(); } };
    std::thread worker{ [this]() { run(); } };

};

} // namespace lmqtt
