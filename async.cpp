//
// Created by explorer on 8/22/19.
//

#include <cstdint>
#include "async.h"
#include "Coroutine.h"
#include <cstdio>
#include <unistd.h>
#include <cassert>

// TODO: do not support thread. We should store this in TLS
EventLoop *current_event;

// schedule give up cpu. buf the coro can run immediately
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

void EventLoop::add_to_poll(Coroutine *coro) {
    this->active_list.push_back(coro);
    coro->loop = this;
}

// the main loop for async
// TODO: add a argv to tell loop wait for witch coro
void EventLoop::loop() {
    Coroutine *c;
    while (true) {
        int re = this->wakeup_coro();
        if (re != 0) {      // this means all corn is exit.
            break;
        }
        auto iter = this->active_list.begin();
        if (iter == this->active_list.end()) {
            break;
        }
        c = *iter;
        this->active_list.erase(iter);
        if (c->coro_status == INIT) {
            this->current_run = c;
            current_event = this;
            c->func_run();
        } else if (c->coro_status == RUNNING) {
            this->current_run = c;
            current_event = this;
            c->continue_run();
        } else if (c->coro_status == END) {
            c->func_stop();
        } else {
            // unknown status, should not happen
            abort();
        }
    }
}

int EventLoop::wakeup_coro() {
    while (true) {
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
        time_t timestamp = time(NULL);
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

        if (this->active_list.empty()) {
            auto iter3 = this->time_event_list.begin();
            if (iter3 == this->time_event_list.end()) {
                return -1; // there is no coro
            }
//            ::sleep((*iter3).first - timestamp);
            auto result_list = poll.wait_poll((*iter3).first - timestamp);
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
    event_vector a;
    if (timeout != -1) {
        time_t wait_timestamp = time(NULL) + timeout;
        auto iter = this->time_event_list.find(wait_timestamp);
        // TODO: not sure
//        if (iter == this->time_event_list.end()) {
//            this->time_event_list[wait_timestamp] = a;
//        }
        this->time_event_list[wait_timestamp].push_back(event_pair(e, current_run));
    } else {
        this->event_list.push_back(event_pair(e, current_run));
    }
}

void Future::set() {
    flag = true;
}

void Future::clear() {
    flag = false;
}

int Future::is_set() {
    return flag;
}

bool Future::wait(int timeout) {
    add_event(this, timeout);
    wait_event();
    return is_set();
}

bool Future::should_release() {
    return is_set();
}


void Sleep::sleep(int second) {
    assert(second >= 0);
    add_event(this, second);
    wait_event();
}

void Sleep::sleep_to(time_t timestamp) {
    this->sleep(timestamp - time(NULL));
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

