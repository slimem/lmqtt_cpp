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
        //worker.join();
        _time = std::chrono::milliseconds(time);
        //worker = [this]() { wait_for_call(); } };
    }

    void run() {
        while (!_exit) {
            std::unique_lock<std::mutex> lock(mtx);
            //cv.wait(lock, [this] {return has_work || exiting; });
            std::cout << "Thread " << worker.get_id() << " WAITING FOR " << std::chrono::duration_cast<std::chrono::milliseconds>(_time).count() << std::endl;
            if (!_work) {
                cv.wait(lock, [this] {return _work || _exit; });
            } else {
                cv.wait_for(lock, _time, [this] { return _exit; });
            }
            if (_exit) {
                std::cout << "exiting from loop\n";
                break;
            }
            _f();
            lock.unlock();
            cv.notify_all();
        }
        //std::cout << "joining thread\n";
        //worker.join();
    }

    size_t get_time() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(_time).count();
    }

    void exit() {
        _exit = true;
    }

    void resume() {
        _work = true;
    }

    void stop() {
        _work = false;
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

    bool _exit = false;
    bool _work = true;

    std::mutex mtx;
    std::condition_variable cv;
    std::chrono::milliseconds _time;
    std::function<void(void)> _f;

    //std::thread worker{ [this]() { wait_for_call(); } };
    std::thread worker{ [this]() { run(); } };
public:
    int count = 5;

};

} // namespace lmqtt
