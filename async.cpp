//
// Created by explorer on 8/22/19.
//

#include <cstdint>
#include "async.h"
#include "Coroutine/Coroutine.h"
#include "log.h"
#include <cstdio>
#include <unistd.h>
#include <cassert>
#include <signal.h>
#include <pthread.h>
#include <cstring>

// TODO: do not support thread. We should store this in TLS

EventLoop *current_event;

void empty_handler(int signo) {}

EventLoop::EventLoop() {
    thread_id = pthread_self();
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);

    // we block the SIGUSR1, when the executor thread fin, send a SIGUSR1 to this thread
    // and epoll_pwait will unblock this sig and wakeup.
    pthread_sigmask(SIG_BLOCK, &set, nullptr);
    struct sigaction action;
    action.sa_handler = empty_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGUSR1, &action, nullptr);
}

// schedule give up cpu. buf the coro can start immediately
void schedule() {
    current_event->schedule();
}

void EventLoop::schedule() {
    // add current to active for schedule
    this->active_list.push_back(current_run);
    current_run->schedule_back();
}

// the wait give up cpu and wait for something happen
void wait_event() {
    current_event->wait();
}

void EventLoop::wait() {
    current_run->schedule_back();
}

void add_to_poll(Coroutine *coro) {
    current_event->add_to_poll(coro);
}

void EventLoop::add_to_poll(Coroutine *coro) {
    this->active_list.push_back(coro);
//    coro->loop = this;
}

// the main loop for async
// TODO: add a argv to tell loop wait for witch coro
void EventLoop::loop() {
    Coroutine *c;
    current_event = this;
    while (true) {
        int re = this->wakeup_coro();
        if (re != 0) {      // this means all corn is exit.
            break;
        }
        while (true) {
            auto iter = this->active_list.begin();
            if (iter == this->active_list.end()) {
                break;
            }
            c = *iter;
            this->active_list.erase(iter);
            this->current_run = c;
            (*c)();
        }
    }
}

int EventLoop::wakeup_coro() {
    while (true) {
        event_change = false;
        // iter all event list
        auto iter = this->event_list.begin();
        while (iter != this->event_list.end()) {
            if ((*iter).event->should_release()) {
                this->active_list.push_back((*iter).coro);
                iter = this->event_list.erase(iter);
            } else {
                iter++;
            }
        }

        //first add all event witch is timeout
        time_t timestamp = time(nullptr);
        auto end = this->time_event_list.upper_bound(timestamp);
        auto iter2 = this->time_event_list.begin();
        while (iter2 != end) {
            auto coro_iter = (*iter2).second.begin();
            // we are sure the vec is not empty, use do while
            do {
                this->active_list.push_back((*coro_iter).coro);
                coro_iter = (*iter2).second.erase(coro_iter);
            } while (coro_iter != (*iter2).second.end());
            iter2 = this->time_event_list.erase(iter2);
        }

        // iter rest
        iter2 = this->time_event_list.begin();
        while (iter2 != this->time_event_list.end()) {
            auto coro_iter = (*iter2).second.begin();
            do {
                if ((*coro_iter).event->should_release()) {
                    this->active_list.push_back((*coro_iter).coro);
                    coro_iter = (*iter2).second.erase(coro_iter);
                } else {
                    coro_iter++;
                }
            } while (coro_iter != (*iter2).second.end());
            if ((*iter2).second.empty()) {
                iter2 = this->time_event_list.erase(iter2);
            } else {
                iter2++;
            }
        }

        if (this->active_list.empty() && !event_change) {
            auto iter3 = this->time_event_list.begin();
            int timeout;
            if (iter3 == this->time_event_list.end()) {
                if (poll.empty() && thread_count == 0) {
                    return -1; // there is no coro
                }
                timeout = -1;

            } else {
                timeout = (*iter3).first - timestamp;
            }
//            ::sleep((*iter3).first - timestamp);
            auto result_list = poll.wait_poll(timeout);
            auto result_iter = result_list->begin();
            while (result_iter != result_list->end()) {
                IO *io = static_cast<IO *>((*result_iter).data);
                io->get_result(*result_iter);
                result_iter = result_list->erase(result_iter);
            }
            delete result_list;
        } else {
            break;
        }
    }
    return 0;
}

void asleep_to(time_t timestamp) {
    Sleep a;
    a.sleep_to(timestamp);
}

void asleep(int second) {
    Sleep a;
    a.sleep(second);
}

void add_event(Event *e, int timeout) {
    current_event->add_event(e, timeout);
}

// we split add event and wait event. so a coro sometimes have change to wait multi events
void EventLoop::add_event(Event *e, int timeout) {
    event_change = true;
    event_vector a;
    if (timeout != -1) {
        time_t wait_timestamp = time(nullptr) + timeout;
        auto iter = this->time_event_list.find(wait_timestamp);
        this->time_event_list[wait_timestamp].push_back(event_pair(e, current_run));
    } else {
        this->event_list.push_back(event_pair(e, current_run));
    }
}

void run_executor(Executor *executor) {
    current_event->add_executor(executor);
}

void EventLoop::add_executor(Executor *executor) {
    aThread *thread;
    if (!thread_pool.empty()) {
        thread = *thread_pool.begin();
        thread_pool.erase(thread_pool.begin());
    } else {
        thread = new aThread;
    }
    assert(!thread->is_busy());
    executor->thread = thread;
    thread->start(executor);
    thread_count++;
}

void stop_thread(aThread *thread) {
    current_event->add_thread(thread);
}

void EventLoop::add_thread(aThread *thread) {
    if (thread_pool.size() >= THREAD_POLL_SIZE) {
        auto re = thread->stop();
        assert(re);
        delete thread;
    } else {
        thread_pool.push_back(thread);
    }
    thread_count--;
}

void wakeup_notify() {
    current_event->wakeup_notify();
}

void EventLoop::wakeup_notify() {
    auto re = pthread_kill(thread_id, SIGUSR1);
    if (re != 0) {
        logger(ERR, stderr, "kill %s", strerror(errno));
    }
}


void Future::set(void *value) {
    flag = true;
    this->val = value;
}

void Future::clear() {
    flag = false;
}

int Future::is_set() {
    return flag;
}

bool Future::wait(int timeout) {
    if (is_set()) {
        return true;
    }
    add_event(this, timeout);
    wait_event();
    return is_set();
}

bool Future::should_release() {
    return is_set();
}


void Sleep::sleep(int second) {
    assert(second >= 0);
    if (second == 0) {
        return;
    }
    add_event(this, second);
    wait_event();
}

void Sleep::sleep_to(time_t timestamp) {
    this->sleep(timestamp - time(nullptr));
}

bool Sleep::should_release() {
    return false;
}

void Lock::release() {
    is_locked = false;
}

bool Lock::acquire(bool blocking, int timeout) {
    if (!is_locked) {
        is_locked = true;
        return true;
    }

    if (!blocking) {
        return false;
    } else {
        add_event(this, timeout);
        wait_event();
    }

    if (!is_locked) {
        is_locked = true;
        return true;
    } else {
        return false;
    }
}

bool Lock::should_release() {
    return !is_locked;
}


IO::IO(int fd) {
    fileno = fd;
}

void IO::wait_read() {
    current_event->poll.add_read(fileno, this);
    add_event(this, -1);
    wait_event();
}

void IO::wait_write() {
    current_event->poll.add_write(fileno, this);
    add_event(this, -1);
    wait_event();
}

void IO::get_result(poll_result re) {
    event = re.event;
    error_number = re.err_number;
    have_result = true;
}

Executor::Executor(Func *func) {
    this->call_func = func;
    this->new_call_func = false;
}

void Executor::run() {
    run_executor(this);
    add_event(this, -1);
    wait_event();
}

bool Executor::should_release() {
    if (force_stopped) {
        thread->force_stop();
        delete thread;
        error = 1;
        return true;
    }
    if (!thread->is_busy()) {
        stop_thread(thread);
        thread = nullptr;
        return true;
    } else {
        return false;
    }
}

void Executor::force_stop() {
    force_stopped = true;
}
