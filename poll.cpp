//
// Created by explorer on 8/25/19.
//

#include "poll.h"
#include "log.h"

#include <sys/epoll.h>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <signal.h>

poll_ev::poll_ev(int fd) {
    events = EPOLLET | EPOLLRDHUP;
    this->fd = fd;
    read_data = NULL;
    write_data = NULL;
}

void poll_ev::add_read(void *data) {
    events |= EPOLLIN;
    read_data = data;
}

void poll_ev::add_write(void *data) {
    events |= EPOLLOUT;
    write_data = data;
}

void *poll_ev::delete_read() {
    events &= ~EPOLLIN;
    auto ret = read_data;
    read_data = NULL;
    return ret;
}

void *poll_ev::delete_write() {
    events &= ~EPOLLOUT;
    auto ret = write_data;
    write_data = NULL;
    return ret;
}

EPoll::EPoll() {
    epoll_fd = epoll_create(10);
    if (epoll_fd == -1) {
        logger(ERR, stderr, "epoll create error: %s", strerror(errno));
        abort();
    }
}

bool EPoll::add_read(int fd, void *data) {
    auto iter = fds.find(fd);
    int result;
    if (iter == fds.end()) {
        auto p = fds.emplace(fd, fd).first;
        p->second.add_read(data);
        result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, p->second.ret_ev());
    } else {
        auto p = *iter;
        p.second.add_read(data);
        result = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, p.second.ret_ev());
    }
    if (result == -1) {
        logger(ERR, stderr, "epoll_ctl: %s", strerror(errno));
        return false;
    }
    return true;
}

bool EPoll::add_write(int fd, void *data) {
    auto iter = fds.find(fd);
    int result;
    if (iter == fds.end()) {
        auto p = fds.emplace(fd, fd).first;
        p->second.add_write(data);
        result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, p->second.ret_ev());
    } else {
        auto p = *iter;
        p.second.add_write(data);
        result = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, p.second.ret_ev());
    }
    if (result == -1) {
        logger(ERR, stderr, "epoll_ctl: %s", strerror(errno));
        return false;
    }
    return true;
}

void *EPoll::delete_read(int fd) {
    auto iter = fds.find(fd);
    int result;
    if (iter == fds.end()) {
        // this should not happen
        logger(ERR, stderr, "try to delete a not exist fd %d", fd);
        return NULL;
    }
    auto re = (*iter).second.delete_read();
    fds.erase(iter);
    if ((*iter).second.in_poll()) {
        result = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, (*iter).second.ret_ev());
    } else {
        result = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    }
    if (result == -1) {
        logger(ERR, stderr, "epoll ctl : %s", strerror(errno));
    }
    return re;
}

void *EPoll::delete_write(int fd) {
    auto iter = fds.find(fd);
    int result;
    if (iter == fds.end()) {
        // this should not happen
        logger(ERR, stderr, "try to delete a not exist fd %d", fd);
        return NULL;
    }
    auto re = (*iter).second.delete_write();
    fds.erase(iter);
    if ((*iter).second.in_poll()) {
        result = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, (*iter).second.ret_ev());
    } else {
        result = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    }
    if (result == -1) {
        logger(ERR, stderr, "epoll ctl : %s", strerror(errno));
    }
    return re;
}

// timeout in second
std::vector<poll_result> *EPoll::wait_poll(int timeout) {
    struct epoll_event evs[20];
    int result;
    auto fd_result = new std::vector<poll_result>;
    sigset_t sigmask;
    pthread_sigmask(SIG_BLOCK, NULL, &sigmask);
    sigdelset(&sigmask, SIGUSR1);
    result = epoll_pwait(epoll_fd, evs, 20, timeout * 1000, &sigmask);
    if (result == -1) {
        if (errno == EINTR) {
            return fd_result;
        }
        // no result to make epoll wait error
        logger(ERR, stderr, "epoll_wait return error %s", strerror(errno));
        abort();
    }
    for (int i = 0; i < result; i++) {
        auto event = evs[i].events;
        auto *ev = static_cast<poll_ev *>(evs[i].data.ptr);
        if (event & EPOLLERR || event & EPOLLHUP || event & EPOLLRDHUP) {
            // we should send error to both read and write side
            if (ev->events & EPOLLIN) {
                fd_result->emplace(fd_result->end(), ev->fd, ERROR, ev->read_data, errno);
                delete_read(ev->fd);
            }
            if (ev->events & EPOLLOUT) {
                fd_result->emplace(fd_result->end(), ev->fd, ERROR, ev->write_data, errno);
                delete_write(ev->fd);
            }
        } else {
            if (event & EPOLLIN) {
                fd_result->emplace(fd_result->end(), ev->fd, READ, ev->read_data);
                delete_read(ev->fd);
            }
            if (event & EPOLLOUT) {
                fd_result->emplace(fd_result->end(), ev->fd, WRITE, ev->write_data);
                delete_write(ev->fd);
            }
        }
    }
    return fd_result;
}
