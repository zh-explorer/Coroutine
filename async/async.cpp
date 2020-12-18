//
// Created by explorer on 2020/12/9.
//

#include "./async.h"
#include "./Event.h"
#include "../log.h"

EventLoop::EventLoop() {
    this->epoll = new Epoll();
    this->pool = new ThreadPool(10);
}

void EventLoop::loop() {
    // main loop
    Coroutine *c;
    current_loop = this;
    while (!this->all_done()) {
        auto coro_list = this->find_active_coro();
        if (!coro_list.empty()) {
            for (auto run_coro : coro_list) {
                this->current_coro = run_coro;
                (*run_coro)();
            }
        } else {    // not any coro is ready
            this->wait_active_coro();
        }
    }
}

void EventLoop::add_to_loop(Coroutine *coro) {
    this->new_coro_list.push_back(coro);
}

void EventLoop::wait_event(Event *e, int timeout) {
    if (timeout == -1) {
        this->event_list.emplace_back(e, this->current_coro);
    } else {
        time_t wait_time = time(nullptr) + timeout;
        this->time_event_list[wait_time].emplace_back(e, this->current_coro);
    }
    this->do_schedule();
}

bool EventLoop::all_done() {
    return this->new_coro_list.empty() && this->time_event_list.empty() && this->event_list.empty();
}

std::vector<Coroutine *> EventLoop::find_active_coro() {
    this->pool->schedule();     // let thread poll do schedule
    auto act_vec = std::vector<Coroutine *>();
    // add all timeout coro to begin
    time_t timestamp = time(nullptr);
    auto timeout_iter = this->time_event_list.upper_bound(timestamp);
    auto time_event_iter = this->time_event_list.begin();
    while (time_event_iter != timeout_iter) {
        auto timeout_vec = time_event_iter->second;
        for (auto pair : timeout_vec) {
            act_vec.push_back(pair.coro);
        }
    }
    auto rest = this->time_event_list.erase(this->time_event_list.begin(), timeout_iter);

    // check the rest event in time_event_list
    while (rest != this->time_event_list.end()) {
        auto event_vec = rest->second;
        auto event_iter = event_vec.begin();
        while (event_iter != event_vec.end()) {
            auto pair = *event_iter;
            if (pair.event->event_set()) {
                act_vec.push_back(pair.coro);
                event_iter = event_vec.erase(event_iter);
            } else {
                event_iter++;
            }
        }
        if (event_vec.empty()) {
            rest = this->time_event_list.erase(rest);
        } else {
            rest++;
        }
    }

    // check event in event_list
    auto event_iter = this->event_list.begin();
    while (event_iter != this->event_list.end()) {
        auto pair = *event_iter;
        if (pair.event->event_set()) {
            act_vec.push_back(pair.coro);
            event_iter = this->event_list.erase(event_iter);
        } else {
            event_iter++;
        }
    }

    // mov all coro in new_coro_list
    for (auto new_coro : this->new_coro_list) {
        act_vec.push_back(new_coro);
    }
    this->new_coro_list.clear();
    return act_vec;
}

void EventLoop::wait_active_coro() {
    // not coro ready, so we can wait for a IO or a timeout
    auto iter = this->time_event_list.begin();
    int timeout;
    if (iter == this->time_event_list.end()) {
        timeout = -1;
        // do some dead lock check here
        if (this->epoll->empty() && this->pool->empty()) {
            logger(ERR, stderr, "dead lock");
            exit(0);
        }

    } else {
        timeout = iter->first - time(nullptr);
    }
    this->epoll->wait_poll(timeout);
}

void EventLoop::do_schedule() {
    this->current_coro->schedule_back();
}

EventLoop::~EventLoop() {
    delete this->pool;
    delete this->epoll;
    // delete all coro
    for (auto i: this->new_coro_list) {
        i->destroy(true);
    }
    this->new_coro_list.clear();

    for (auto i: this->event_list) {
        i.coro->destroy(true);
    }
    this->event_list.clear();

    for (auto i: this->time_event_list) {
        for (auto j : i.second) {
            j.coro->destroy();
        }
    }
    this->time_event_list.clear();
}





