//
// Created by explorer on 2020/12/17.
//

#include "AIO.h"
#include "log.h"
#include "async.h"
#include "poll.h"
#include <cstring>
#include <unistd.h>
#include <netdb.h>

// return the accurate size read.
// if read size not equal size arg
// 1. timeout
// 2. timeout is 0 and no enough data
// 3. error or close while read but get some data
// if ret -1, this mean the socket is close and can not read data anymore
int AIO::read(unsigned char *buffer, size_t size, int timeout) {
    int read_size = 0;
    int result;
    bool in_pool = false;
    int start_time = time(nullptr);

    while (read_size != size) {
        result = ::read(this->fd, buffer + read_size, size - read_size);

        if (result == -1 && errno != EAGAIN) {
            // get a error while read data
            logger(INFO, stderr, "error while read socket fd: %d, err: %s", this->fd, strerror(errno));
//            if (read_size == 0) {
//                // this mean that socket is close or error before read. so return -1
//                return -1;
//            } else {
//                // we read some data and get error, return the size we read;
//                return read_size;
//            }
            return read_size == 0 ? -1 : read_size;
        } else if (result == 0 || (result == -1 && errno == EAGAIN)) {
            // no data to read
            if (timeout == 0) {
                return read_size;
            } else {
                // check whether this socket is finish.
                // we move this here to avoid futile actions
                if (this->aSock->is_fin()) {
                    return read_size == 0 ? -1 : read_size;
                }

                if (!in_pool) {
                    auto poll = current_loop->get_epoll();
                    poll->add_read(this->aSock);
                    in_pool = true;
                }

                // calc timeout
                int t;
                if (timeout == -1) {
                    t = -1;
                } else {
                    t = timeout - (time(nullptr) - start_time);
                    if (t <= 0) {
                        return read_size; // timeout
                    }
                }

                // when wait_read return, there is 4 situation
                // 1. could_read && is_fin: socket is close but still some data to read. read data and return normal
                // 2. could_read && !is_fin: most normal situation, read data and continue
                // 3. !could_read && is_fin: socket close and no data to read. return -1
                // 4. !could_read && !is_fin: timeout, return data read.
                // situation 2/4 is the most normally, and this code need optimize and simplify
                this->aSock->wait_read(t);
                // this will return while timeout, the fd can not be read in this time
                // but we don't handle it in this place.
                // the read next time will failed and timeout will be check
                if (!this->aSock->could_read() && this->aSock->is_fin()) {
                    // the sock is fin, is read_size == 0, ret -1 to notify
                    return read_size == 0 ? -1 : read_size;
                }
                // we not read data in last read, so set result to 0 and continue
                result = 0;
            }
        }
        read_size += result;
    }
    if (in_pool) {
        auto poll = current_loop->get_epoll();
        poll->delete_read(this->aSock);
    }
    return read_size;
}

int AIO::write(unsigned char *buffer, size_t size, int timeout) {
    int write_size = 0;
    int result;
    bool in_pool = false;
    int start_time = time(nullptr);

    while (write_size != size) {
        result = ::write(this->fd, buffer + write_size, size - write_size);

        if (result == -1 && errno != EAGAIN) {
            logger(INFO, stderr, "error while write socket fd: %d, err: %s", this->fd, strerror(errno));
            return write_size == 0 ? -1 : write_size;
        } else if (result == 0 || (result == -1 && errno == EAGAIN)) {
            if (timeout == 0) {
                return write_size;
            } else {
                if (!in_pool) {
                    auto poll = current_loop->get_epoll();
                    poll->add_write(this->aSock);
                    in_pool = true;
                }

                int t;
                if (timeout == -1) {
                    t = -1;
                } else {
                    t = timeout - (time(nullptr) - start_time);
                    if (t <= 0) {
                        return write_size;
                    }
                }

                this->aSock->wait_write(t);
                if (this->aSock->is_fin()) {      // not like read, if a socket is close, we can never write any more
                    return write_size == 0 ? -1 : write_size;
                }
                result = 0;
            }
        }

        write_size += result;
    }
    if (in_pool) {
        auto poll = current_loop->get_epoll();
        poll->delete_write(this->aSock);
    }
    return write_size;
}

// block until there is data to read;
int AIO::read_any(unsigned char *buffer, size_t size, int timeout) {

    int result = ::read(this->fd, buffer, size);
    if (result > 0) {
        return result;
    }

    if (result == -1 && errno != EAGAIN) {
        logger(INFO, stderr, "error while write socket fd: %d, err: %s", this->fd, strerror(errno));
        return -1;
    } else if (result == 0 || (result == -1 && errno == EAGAIN)) {
        if (this->aSock->is_fin()) {
            return -1;
        }
        // there is not data to read, so wait for any data
        auto epoll = current_loop->get_epoll();
        epoll->add_read(this->aSock);   // add to poll and wait for epoll_in event;
        this->aSock->wait_read(timeout);
        if (!this->aSock->could_read() && this->aSock->is_fin()) {
            // the socket is close, and not any data to read
            return -1;
        }
        result = ::read(this->fd, buffer, size);
        epoll->delete_read(this->aSock);
        return result;
    }
    // is if always true: result >= -1
}

int AIO::bind(int port) const {
    if (port > 65535) {
        logger(ERR, stderr, "invalid port: %d", port);
        return -1;
    }
    struct sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_aton("0.0.0.0", &addr.sin_addr);
    addr.sin_port = htons(port);
    return ::bind(this->fd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
}

int AIO::bind(const struct sockaddr *addr, socklen_t addrlen) const {
    return ::bind(this->fd, addr, addrlen);
}


AIOClient::AIOClient() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    this->class_init(fd);
}

bool AIOClient::connect(char *domain, int port, int timeout) {
    int now_time = time(nullptr);
    auto addr = this->get_addr(domain, port, timeout);
    if (addr == nullptr) {
        return false;
    }
    int t = timeout - (time(nullptr) - now_time);
    return this->connect(addr->addr, addr->addrlen, t);
}

bool AIOClient::connect(const struct sockaddr *addr, socklen_t addrlen, int timeout) {
    ::connect(this->fd, addr, addrlen);
    auto epoll = current_loop->get_epoll();
    epoll->add_write(this->aSock);
    this->aSock->wait_write(timeout);
    epoll->delete_write(this->aSock);
    if (this->aSock->could_write()) {
        return true;
    } else {
        return false;
    }
}

AIO::address *AIO::get_addr(char *domain, int port, int timeout) {
    std::function<address *(char *, int)> f = [](char *domain, int port) -> address * {
        struct hostent *host;

        char port_buf[6];
        snprintf(port_buf, 6, "%u", port);

        host = gethostbyname(domain);
        if (host == NULL || host->h_addrtype == AF_INET6) {
            return nullptr;
        }

        sockaddr_in addr{};
        memcpy(&addr.sin_addr.s_addr, host->h_addr, 4);
        addr.sin_port = htons(port);
        addr.sin_family = AF_INET;
        // TODO: it's not good to malloc a new class here
        return new address(reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
    };

    this->asyncThread = new AsyncThread<address *>(f, domain, port);
    auto r = this->asyncThread->start(timeout);
    delete this->asyncThread;
    this->asyncThread = nullptr;
    return r;
}


AIOServer::AIOServer() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    this->class_init(fd);
}

int AIOServer::listen(int backlog) {
    return ::listen(this->fd, backlog);
}

AIO *AIOServer::accept() {
    while (true) {
        int fd = ::accept(this->fd, nullptr, nullptr);
        if (fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                auto epoll = current_loop->get_epoll();
                epoll->add_read(this->aSock);
                this->aSock->wait_read();
                epoll->delete_read(this->aSock);
                if (!this->aSock->could_read()) {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        } else {
            return new AIO(fd);
        }
    }
}



