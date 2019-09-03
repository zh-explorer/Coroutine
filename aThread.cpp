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
    auto re = pthread_create(&thread_id, NULL, reinterpret_cast<void *(*)(void *)>(thread_caller), this);
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
    this->mark_stop = true;
    cv.notify_all();
    lk.unlock();
    thread.join();
    return true;
}

bool aThread::is_busy() {
    std::unique_lock<std::mutex> lk(m);
    // a stop thread will be mark as busy
    return is_running;
}

bool aThread::run(Executor *new_executor) {
    std::unique_lock<std::mutex> lk(m);
    if (is_running) {
        // this thread is running or is stop
        return false;
    }

    is_running = true;
    this->executor = new_executor;
    cv.notify_one();

    // task start to run
    return true;
}

aThread::~aThread() {
    if (!stop()) {
        // the thread is running, we send a SIGKILL to force thread stop.
        pthread_kill(thread_id, SIGKILL);
    }
}


void thread_caller(aThread *thread) {
    thread->running();
}