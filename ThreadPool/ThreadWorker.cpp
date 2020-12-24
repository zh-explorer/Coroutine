//
// Created by explorer on 2020/11/30.
//

#include "ThreadExecutor.h"
#include "ThreadWorker.h"
#include "log.h"
#include <csignal>
#include <cstring>

ThreadWorker::ThreadWorker() {
    this->main_thread_id = pthread_self();
    auto caller = [](void *thread) -> void * {
        auto t = (ThreadWorker *) thread;
        t->run();
        return nullptr;
    };
    auto re = pthread_create(&this->thread_id, nullptr, caller, this);
    if (re) {
        logger(ERR, stderr, "thread create failed");
        exit(-1);
    }
}

void ThreadWorker::run() {
    while (true) {
        {
            std::unique_lock<std::mutex> lk(this->m);
            while (this->status != stopped && this->executor == nullptr) {
                this->cv.wait(lk);
            }
            if (this->status == stopped) {  // stop the thread normally
                return;
            }
        }
        this->executor->exec();
        std::unique_lock<std::mutex> lk(this->m);
        this->executor->set_status(finished);
        this->executor = nullptr;
        this->status = finish;
        this->task_done();
    }
}

// task is done, notify main thread
void ThreadWorker::task_done() const {
    auto re = pthread_kill(this->main_thread_id, SIGUSR1);
    if (re != 0) {
        logger(ERR, stderr, "kill %s", strerror(errno));
    }
}

bool ThreadWorker::start(ThreadExecutor *new_executor) {
    std::unique_lock<std::mutex> lk(this->m);
    if (this->status != waiting) {
        // This thread worker now do not support new task
        return false;
    }

    this->status = working;
    this->executor = new_executor;
    this->executor->set_status(running);
    this->cv.notify_one();
    return true;
}

// we can stop a thread which is not running task
bool ThreadWorker::stop() {
    std::unique_lock<std::mutex> lk(m);
    if (this->status == working) {
        return false; // can not stop a thread witch is running task
    }

    if (this->status == stopped) {
        return true; // thread is already stopped
    }

    this->status = stopped;
    this->cv.notify_one();

    lk.unlock();    // unlock lock so thread can lock this and stop

    pthread_join(this->thread_id, nullptr); // this should not take time

    return true;
}

// force to stop a thead is not a good choose
void ThreadWorker::force_stop() {
    std::unique_lock<std::mutex> lk(m);
    if (this->status == stopped) {
        //already stopped
        return;
    }
    if (this->status != working) {
        // not need force
        lk.unlock();        // avoid deadlock
        this->stop();
        return;
    }
    pthread_cancel(this->thread_id);
    this->status = stopped;
    if (this->executor) {
        this->executor->set_status(canceled);
        this->executor = nullptr;
    }

    this->task_done();      // we should notify ThreadPool that this thread is quit. and worker will delete in ThreadPool later
}

ThreadWorker::~ThreadWorker() {
    if (!this->stop()) {
        this->force_stop();
    }
}
