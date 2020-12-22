//
// Created by explorer on 2020/12/1.
//

#include "ThreadExecutor.h"
#include "ThreadPool.h"

void ThreadExecutor::cancel() {
    if (this->pool) {
        this->pool->cancel(this);
    }
    {
        std::unique_lock<std::mutex> lk(m);
        this->status = canceled;
    }    // not submit yet, so cancel with no effect
}

bool ThreadExecutor::is_padding() {
    std::unique_lock<std::mutex> lk(m);
    return this->status == padding;
}

bool ThreadExecutor::is_cancel() {
    std::unique_lock<std::mutex> lk(m);
    return this->status == canceled;
}

bool ThreadExecutor::is_finish() {
    std::unique_lock<std::mutex> lk(m);
    return this->status == finished || this->status == canceled;
}



//enum ThreadStatus ThreadExecutor::get_status() {
//    std::unique_lock<std::mutex> lk(m);
//    return this->status;
//}

void ThreadExecutor::set_status(enum ThreadStatus s) {
    std::unique_lock<std::mutex> lk(m);
    this->status = s;
}

void ThreadExecutor::set_pool(ThreadPool *p) {
    this->pool = p;
}

bool ThreadExecutor::is_running() {
    return this->status == running;
}

bool ThreadExecutor::is_success() {
    return this->status == finished;
}
