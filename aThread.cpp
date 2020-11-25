//
// Created by explorer on 19-8-31.
//

#include <assert.h>
#include "aThread.h"
#include "async.h"
#include "log.h"
#include <signal.h>
#include <pthread.h>

aThread::aThread() {
    auto caller = [](void *thread) -> void * {
        auto t = (aThread *) thread;
        t->run();
        return nullptr;
    };
    auto re = pthread_create(&this->thread_id, nullptr, caller, this);
    if (re) {
        logger(ERR, stderr, "thread create failed");
        exit(-1);
    }
}

void aThread::run() {
    while (true) {
        {
            std::unique_lock<std::mutex> lk(this->m);
            while (!this->thread_stop && this->executor == nullptr) {
                this->running = false;
                this->cv.wait(lk);
            }
            if (this->thread_stop) {  // stop the thread normally
                return;
            }
        }
        (*this->executor->call_func)();
        std::unique_lock<std::mutex> lk(this->m);
        this->executor = nullptr;
        this->running = false;
        wakeup_notify();
    }
}

bool aThread::stop() {
    std::unique_lock<std::mutex> lk(this->m);
    if (this->running) {           // we can not stop a run thread
        return false;
    }
    if (this->thread_stop) {
        return true;        // the thread already stop
    }
    this->thread_stop = true;
    this->cv.notify_one();
    lk.unlock();
    // the thread will return soon
    pthread_join(this->thread_id, nullptr);
    return true;
}

bool aThread::is_busy() {
    std::unique_lock<std::mutex> lk(m);
    // a stop thread will be mark as busy
    return this->running || this->thread_stop;
}

bool aThread::start(Executor *executor) {
    std::unique_lock<std::mutex> lk(this->m);
    if (this->running || this->thread_stop) {
        // this thread is run or is stop
        return false;
    }

    this->running = true;
    this->executor = executor;
    this->cv.notify_one();

    // task start to start
    return true;
}

// force to stop a thead is not a good choose
void aThread::force_stop() {
    pthread_cancel(this->thread_id);
    this->running = false;
    this->thread_stop = true;
}

