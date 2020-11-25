//
// Created by explorer on 2020/11/25.
//

#include "ThreadPool.h"
#include "../log.h"
#include <signal.h>
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
            while (!this->thread_stop && this->executor == nullptr) {
                this->busy = false;
                this->cv.wait(lk);
            }
            if (this->thread_stop) {  // stop the thread normally
                return;
            }
        }
        this->executor->exec();
        std::unique_lock<std::mutex> lk(this->m);
        this->done_executor = this->executor;
        this->executor = nullptr;
        this->busy = false;
        this->task_done();
    }
}

// task is done, notify main thread
void ThreadWorker::task_done() {
    auto re = pthread_kill(this->main_thread_id, SIGUSR1);
    if (re != 0) {
        logger(ERR, stderr, "kill %s", strerror(errno));
    }
}

bool ThreadWorker::start(ThreadExecutor *new_executor) {
    std::unique_lock<std::mutex> lk(this->m);
    if (this->busy || this->thread_stop || this->done_executor != nullptr) {
        // This thread worker now do not support new task
        return false;
    }

    this->busy = true;
    this->executor = new_executor;
    this->cv.notify_one();
    return true;
}

// we can stop a thread which is not running task
bool ThreadWorker::stop() {
    std::unique_lock<std::mutex> lk(m);
    if (this->busy) {
        return false; // can not stop a thread witch is running task
    }

    if (this->thread_stop) {
        return true; // thread is already stopped
    }

    this->thread_stop = true;
    this->cv.notify_one();

    lk.unlock();    // unlock lock so thread can lock this and stop

    pthread_join(this->thread_id, nullptr); // this should not take time

    return true;
}

// force to stop a thead is not a good choose
void ThreadWorker::force_stop() {
    pthread_cancel(this->thread_id);
    this->busy = false;
    this->thread_stop = true;
}

ThreadWorker::~ThreadWorker() {
    if (!this->stop()) {
        this->force_stop();
    }
}
