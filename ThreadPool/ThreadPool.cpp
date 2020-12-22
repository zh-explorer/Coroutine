//
// Created by explorer on 2020/11/25.
//

#include "ThreadPool.h"
#include <assert.h>

ThreadPool::ThreadPool(int max_workers) {
    this->max_workers = max_workers;

    // init of signal event
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);

    // we block the SIGUSR1, when the worker thread fin, send a SIGUSR1 to this thread
    // and epoll_pwait will unblock this sig and wakeup.
    pthread_sigmask(SIG_BLOCK, &set, nullptr);
    struct sigaction action{};
    auto f = [](int signal) -> void {}; // a empty handler
    action.sa_handler = f;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGUSR1, &action, nullptr);   // we should deal with old handle of SIGUSER1?
}

void ThreadPool::submit(ThreadExecutor *executor) {
    this->padding_executor.push_back(executor);
    executor->set_pool(this);
    this->schedule();
}

inline int min(int a, int b) {
    return a < b ? a : b;
}

void ThreadPool::schedule() {
    // check worker status
    auto i = this->busy_workers.begin();
    while (i != this->busy_workers.end()) {
        auto worker = *i;
        if (worker->is_finish()) {
            i = this->busy_workers.erase(i);
            this->padding_works.push_back(worker);
        } else if (worker->is_stop()) {
            // the thread is cancel, delete it
            i = this->busy_workers.erase(i);
            delete worker;
        } else {
            // this thread is working, continue
            i++;
        }
    }

    int task_count;
    // count which executor we can start
    if (this->max_workers) {
        task_count = min(this->max_workers - this->busy_workers.size(), this->padding_executor.size());
    } else {
        task_count = this->padding_executor.size();
    }

    for (int i = 0; i < task_count; i++) {

        // get a worker
        ThreadWorker *worker;
        if (this->padding_works.empty()) {
            worker = this->padding_works.back();
            this->padding_works.pop_back();
        } else {
            worker = new ThreadWorker();
        }
        // get a executor
        auto executor = this->padding_executor.back();
        this->padding_executor.pop_back();

        this->busy_workers.push_back(worker);
        auto re = worker->start(executor);
        assert(re);
    }
}

void ThreadPool::cancel(ThreadExecutor *executor) {
    if (executor->is_padding()) {
        // ThreadPool itself can not run in mutilthread, so iter padding executor is safe
        for (auto i = this->padding_executor.begin(); i != this->padding_executor.end(); i++) {
            if (*i == executor) {
                // we find it
                this->padding_executor.erase(i);
                executor->set_status(canceled); // we set it in executor->cancel, but it not harmful
                return;
            }
        }
    } else if (executor->is_running() || executor->is_finish()) {
        // we should find it in busy_works
        for (auto worker: this->busy_workers) {     // executor maybe change from running to finish, but no effect
            if (worker->executor_find(executor)) {
                // we find
                worker->force_stop();   // just stop the worker, and the worker will be delete and do_schedule later in get_executor_result
                return;
            }
        }
    } else if (executor->is_cancel()) {
        // duplicate cancel
        return;
    }
}

ThreadPool::~ThreadPool() {
    for (auto i : this->padding_executor) {
        i->set_status(canceled);
    }
    this->padding_executor.clear();

    for (auto i : this->padding_works) {
        delete i;
    }
    this->padding_works.clear();

    for (auto i: this->busy_workers) {
        delete i;
    }
    this->busy_workers.clear();
}
