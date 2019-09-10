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
        t->running();
        return NULL;
    };
    auto re = pthread_create(&thread_id, NULL, caller, this);
    if (re) {
        logger(ERR, stderr, "thread create failed");
        exit(-1);
    }
}

void aThread::running() {
    while (true) {
        {
            std::unique_lock<std::mutex> lk(m);
            while (!mark_stop && executor == NULL) {
                is_running = false;
                cv.wait(lk);
            }
            if (mark_stop) {
                return;
            }
        }
        executor->ret_val = executor->call_func(executor->argv);
        std::unique_lock<std::mutex> lk(m);
        executor = NULL;
        // TODO_fin: need notify main thead that some work fin
        wakeup_notify();
    }
}

bool aThread::stop() {
    std::unique_lock<std::mutex> lk(m);
    if (is_running) {           // we can not stop a running thread
        return false;
    }
    if (mark_stop) {
        return true;        // the thread already stop
    }
    this->mark_stop = true;
    cv.notify_one();
    lk.unlock();
    pthread_join(thread_id, NULL);
    return true;
}

bool aThread::is_busy() {
    std::unique_lock<std::mutex> lk(m);
    // a stop thread will be mark as busy
    return is_running || mark_stop;
}

bool aThread::run(Executor *new_executor) {
    std::unique_lock<std::mutex> lk(m);
    if (is_running || mark_stop) {
        // this thread is running or is stop
        return false;
    }

    is_running = true;
    this->executor = new_executor;
    cv.notify_one();

    // task start to run
    return true;
}

void aThread::force_stop() {
    pthread_cancel(thread_id);
    is_running = false;
    mark_stop = true;
}

