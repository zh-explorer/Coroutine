//
// Created by explorer on 2020/12/1.
//

#include "poll.h"
#include "log.h"
#include <cstring>
#include <csignal>
#include "sock.h"
#include <exception>
#include <unistd.h>

#define MAXEVENTS 20

class NotFindException : public std::exception {
    const char *what() const noexcept {
        return "not find";
    }
};

Epoll::Epoll() {
    this->epoll_fd = epoll_create(10);
    if (epoll_fd == -1) {
        logger(ERR, stderr, "Epoll create error: %s", strerror(errno));
        abort();
    }
}

bool Epoll::add_read(Sock *s) {
    int fd = s->get_fd();
    int op;
    struct epoll_event *ev;
    try {
        auto p = this->get_ev(fd);
        if (p.read_sock != nullptr) {
            logger(ERR, stderr, "duplicate register read for same fd %d", fd);
            return false;
        }
        p.set_read(s);
        ev = p.pack_ev();
        op = EPOLL_CTL_MOD;
    } catch (NotFindException &e) {
        this->evs.insert(std::pair<int, poll_ev>(fd, poll_ev(fd, s)));
        auto p = this->get_ev(fd);  // no result to exception
        ev = p.pack_ev();
        op = EPOLL_CTL_ADD;
    }
    int result = epoll_ctl(this->epoll_fd, op, fd, ev);
    if (result == -1) {
        logger(ERR, stderr, "epoll_ctl: %s", strerror(errno));
        return false;
    }
    return true;
}

bool Epoll::add_write(Sock *s) {
    int fd = s->get_fd();
    int op;
    struct epoll_event *ev;
    try {
        auto p = this->get_ev(fd);
        if (p.write_sock != nullptr) {
            logger(ERR, stderr, "duplicate register write for same fd %d", fd);
            return false;
        }
        p.set_write(s);
        ev = p.pack_ev();
        op = EPOLL_CTL_MOD;
    } catch (NotFindException &e) {
        this->evs.insert(std::pair<int, poll_ev>(fd, poll_ev(fd, nullptr, s)));
        auto p = this->get_ev(fd);  // no result to exception
        ev = p.pack_ev();
        op = EPOLL_CTL_ADD;
    }

    int result = epoll_ctl(this->epoll_fd, op, fd, ev);
    if (result == -1) {
        logger(ERR, stderr, "epoll_ctl: %s", strerror(errno));
        return false;
    }
    return true;
}

void Epoll::delete_read(Sock *s) {
    int fd = s->get_fd();
    auto iter = this->evs.find(fd);
    if (iter == this->evs.end()) {
        logger(WARN, stderr, "can not find fd in poll %d", fd);
        return;
    }
    auto ev = (*iter).second;
    ev.del_read();
    int result;
    if (ev.cleaned()) {
        result = epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
        this->evs.erase(iter);
    } else {
        result = epoll_ctl(this->epoll_fd, EPOLL_CTL_MOD, fd, ev.pack_ev());
    }

    if (result == -1) {
        logger(ERR, stderr, "epoll_ctl: %s", strerror(errno));
    }
}

void Epoll::delete_write(Sock *s) {
    int fd = s->get_fd();
    auto iter = this->evs.find(fd);
    if (iter == this->evs.end()) {
        logger(WARN, stderr, "can not find fd in poll %d", fd);
    }
    auto ev = (*iter).second;
    ev.del_write();
    int result;
    if (ev.cleaned()) {
        result = epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
        this->evs.erase(iter);
    } else {
        result = epoll_ctl(this->epoll_fd, EPOLL_CTL_MOD, fd, ev.pack_ev());
    }

    if (result == -1) {
        logger(ERR, stderr, "epoll_ctl: %s", strerror(errno));
    }
}

void Epoll::wait_poll(int timeout) {
    // It's not good to take many time in a task.
    // so handle 20 event at a time.so other task can take time to run
    struct epoll_event events[MAXEVENTS];

    //get old sigmask
    sigset_t sigmask;
    pthread_sigmask(SIG_BLOCK, nullptr, &sigmask);
    sigdelset(&sigmask, SIGUSR1);  // unblock SIGUSER1 so that other thead can send this to notify a thead task is done
    int t = timeout != -1 ? timeout * 1000 : -1;
    int event_count = epoll_pwait(epoll_fd, events, MAXEVENTS, t, &sigmask);
    for (int i = 0; i < event_count; i++) {
        auto poll_event = events[i].events;
        auto fd = events[i].data.fd;
        try {

            auto ev = this->get_ev(fd);
            // when socket hup, there is still data can read.
            // whether delete this fd from poll will handle by aSock itself
            ev = this->get_ev(fd);
            if (poll_event & EPOLLIN) {
                if (ev.read_sock) {
                    ev.read_sock->ready_to_read();
                }
            }

            ev = this->get_ev(fd);
            // TODO: del write when handle read will case a condition raise, we should get new ev from this->evs
            if (poll_event & EPOLLOUT) {
                if (ev.write_sock) {
                    ev.write_sock->ready_to_write();
                }
            }

            if (poll_event & EPOLLERR || poll_event & EPOLLHUP || poll_event & EPOLLRDHUP) {
                // socket get some error or closed, notify both read and write
                // this fd should delete from poll
                if (ev.read_sock) {
                    ev.read_sock->sock_fin(poll_event);
                    this->delete_read(ev.read_sock);
                }
                ev = this->get_ev(fd);
                if (ev.write_sock) {
                    ev.write_sock->sock_fin(poll_event);
                    this->delete_write(ev.write_sock);
                }
            }
        }
        catch (NotFindException &e) {
            logger(WARN, stderr, "find a fd not in evs: %d", fd);
        }
    }
}

poll_ev &Epoll::get_ev(int fd) {
    auto iter = this->evs.find(fd);
    if (iter == this->evs.end()) {
        throw NotFindException();
    }
    return (*iter).second;
}

Epoll::~Epoll() {
    close(this->epoll_fd);
}
