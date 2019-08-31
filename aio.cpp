//
// Created by explorer on 8/25/19.
//

#include "aio.h"
#include "log.h"
#include "async.h"

#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>


aio::aio(int domain, int type, int protocol) {
    fileno = socket(domain, type, protocol);
    set_noblock();
}

void aio::set_noblock() {
    int opts;
    opts = fcntl(fileno, F_GETFL);
    if (opts < 0) {
        logger(ERR, stderr, "fcntl(F_GETFL: %s)\n", strerror(errno));
        exit(1);
    }
    if (fcntl(fileno, F_SETFL, opts | O_NONBLOCK) < 0) {
        logger(ERR, stderr, "fcntl(F_SETFD) %s\n", strerror(errno));
        exit(1);
    }
}

aio::aio(int fd) {
    fileno = fd;
    set_noblock();
}

int aio::read(unsigned char *buf, size_t count, enum READ_MODE mode) {
    unsigned char buffer[BLOCK_SIZE];
    if (mode == read_imm) {
        // array buf will ret the actual data read when count > array_buf's data length
        auto re = arrayBuf.read(buf, count);
        return re;
    } else if (mode == read_fix) {
        // if the cache data is enough
        if (arrayBuf.length() >= count) {
            arrayBuf.read(buf, count);
            return count;
        }
    } else if (mode == read_any) {
        if (arrayBuf.length() != 0) {
            auto re = arrayBuf.read(buf, count);
            return re;
        }
    }

    while (true) {
        IO io(fileno);
        io.wait_read();
        if (io.event == ERROR) {
            // the fd maybe close
            errno = io.error_number;
            return -1;
        }
        auto read_size = ::read(fileno, buffer, BLOCK_SIZE);
        if (read_size == 0 || (read_size == -1 && errno == EAGAIN)) {
            // read end
            if (mode == read_any) {
                if (arrayBuf.length() != 0) {
                    auto re = arrayBuf.read(buf, count);
                    return re;
                }
            } else if (mode == read_fix) {
                if (arrayBuf.length() >= count) {
                    arrayBuf.read(buf, count);
                    return count;
                }
            }
        }
        arrayBuf.write(buffer, read_size);
    }
}

int aio::write(unsigned char *buf, size_t count, enum WRITE_MODE write_mode) {
    int size_write = 0;
    while (true) {
        auto write_size = ::write(fileno, buf, count);
        if (write_size == -1) {
            if (errno == EAGAIN) {
                if (write_mode == write_any) {
                    return size_write;
                } else {
                    IO io(fileno);
                    io.wait_write();
                    if (io.event == ERROR) {
                        errno = io.error_number;
                        return -1;
                    }
                    continue;
                }
            } else {
                return -1;
            }
        }
        size_write += write_size;
        if (count == size_write) {
            return size_write;
        }
    }
}

int aio::bind(const struct sockaddr *addr, socklen_t addrlen) {
    return ::bind(fileno, addr, addrlen);
}

int aio::bind(int port) {
    if (port > 65535) {
        logger(ERR, stderr, "invalid port: %d", port);
        abort();
    }
    struct sockaddr_in addr{};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_aton("0.0.0.0", &addr.sin_addr);
    addr.sin_port = htons(port);
    return ::bind(fileno, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
}

int aio_client::connect(const struct sockaddr *addr, socklen_t addrlen) {
    ::connect(fileno, addr, addrlen);
    IO io(fileno);
    io.wait_write();
    if (io.event == ERROR) {
        errno = io.error_number;
        return -1;
    }
    return 0;
}

int aio_client::connect(char *addr, int port) {
    struct sockaddr_in sock_addr{};
    memset(&sock_addr, 0, sizeof(sock_addr));
    struct in_addr ip{};
    // this should be done in a new thread
    auto re = get_addr(addr, &ip);
    if (re == 0) {
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = htons(port);
        sock_addr.sin_addr = ip;
        return connect((struct sockaddr *) &sock_addr, sizeof(sockaddr_in));
    } else {
        return re;
    }
}

int aio_server::listen(int backlog) {
    return ::listen(fileno, backlog);
}

aio *aio_server::accept() {
    int fd;
    while (true) {
        fd = ::accept(fileno, NULL, NULL);
        if (fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                IO io(fileno);
                io.wait_read();
                if (io.event == ERROR) {
                    errno = io.error_number;
                    return NULL;
                }
            } else {
                return NULL;
            }
        }
        return new aio(fileno);
    }
}