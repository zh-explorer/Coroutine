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
#include <netdb.h>

#define check_close if (is_close) { return -1; }

std::pair<sockaddr, unsigned int> *do_dns_req(char *url, unsigned short port);

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
//    check_close
    if (!count) {
        return 0;
    }
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
        if (is_close) {
            return -1;
        }
        IO io(fileno);
        io.wait_read();
        if (io.event == ERROR) {
            // the fd maybe close
            errno = io.error_number;
            is_close = true;
//            return -1;
        }
        while (true) {
            auto read_size = ::read(fileno, buffer, BLOCK_SIZE);
            if (read_size > 0) {
                arrayBuf.write(buffer, read_size);
            }
            if ((read_size == -1 && errno != EAGAIN) || is_close) {
                if (mode == read_any && arrayBuf.length() != 0) {
                    auto re = arrayBuf.read(buf, count);
                    return re;
                } else if (mode == read_fix && arrayBuf.length() >= count) {
                    auto re = arrayBuf.read(buf, count);
                    return re;
                } else {
                    return -1;
                }
            }
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
                break;
            }
        }
    }
}

int aio::write(unsigned char *buf, size_t count, enum WRITE_MODE write_mode) {
    check_close
    if (!count) {
        return 0;
    }
    int size_write = 0;
    while (true) {
        auto write_size = ::write(fileno, buf + size_write, count - size_write);
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
    check_close
    return ::bind(fileno, addr, addrlen);
}

int aio::bind(int port) {
    check_close
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


int aio::close() {
    if (is_close) {
        return 0;
    }
    IO *re = static_cast<IO *>(current_event->poll.delete_write(fileno));
    if (re) {
        poll_result a(fileno, ERROR, re, EBADFD);
        re->get_result(a);
    }
    IO *re2 = static_cast<IO *>(current_event->poll.delete_read(fileno));
    if (re) {
        poll_result a(fileno, ERROR, re2, EBADFD);
        re->get_result(a);
    }
    is_close = true;
    return ::close(fileno);
}

aio::~aio() {
    if (!is_close) {
        close();
    }
}


int aio_client::connect(const struct sockaddr *addr, socklen_t addrlen) {
    check_close
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
    check_close
    struct sockaddr_in sock_addr{};
    memset(&sock_addr, 0, sizeof(sock_addr));
    // this should be done in a new thread
    auto re = get_addr(addr, port);
    if (re != nullptr) {
        auto conn_re = connect(&re->first, re->second);
        delete re;
        return conn_re;
    } else {
        return -1;
    }
}

int aio_server::listen(int backlog) {
    check_close
    return ::listen(fileno, backlog);
}

aio *aio_server::accept() {
    int fd;
    if (is_close) {
        return nullptr;
    }
    while (true) {
        fd = ::accept(fileno, nullptr, nullptr);
        if (fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                IO io(fileno);
                io.wait_read();
                if (io.event == ERROR) {
                    errno = io.error_number;
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        } else {
            return new aio(fd);
        }
    }
}


std::pair<sockaddr, unsigned int> *aio_client::get_addr(char *url, unsigned short port) {
    auto f = make_func(do_dns_req, url, port);
    executor = new Executor(&f);
    executor->run();
    if (executor->error) {
        delete executor;
        return nullptr;
    }
    auto ret_val = f.r;
    delete executor;
    executor = nullptr;
    return ret_val;
}

int aio_client::close() {
    if (executor) {
        executor->force_stop();
    }
    return aio::close();
}

std::pair<sockaddr, unsigned int> *do_dns_req(char *url, unsigned short port) {
    struct hostent *host;
    auto t = clock();

    char port_buf[6];
    snprintf(port_buf, 6, "%u", port);

    host = gethostbyname(url);
    if (host == NULL || host->h_addrtype == AF_INET6) {
        return nullptr;
    }

    sockaddr_in addr;
    memcpy(&addr.sin_addr.s_addr, host->h_addr, 4);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    sockaddr saddr;
    memcpy(&saddr, &addr, sizeof(addr));

    if (clock() - t > CLOCKS_PER_SEC * 0.1) {
        logger(ERR, stderr, "dns slow %ds", (clock() - t) / CLOCKS_PER_SEC);
    }
    return new std::pair<sockaddr, unsigned int>(saddr, sizeof(addr));
}

//std::pair<sockaddr, unsigned int> *do_dns_req(char *url, unsigned short port) {
//    struct hostent host, *phost;
//    char buffer[8192];
//    int error_code;
//    auto t = clock();
//    struct addrinfo hints, *result;
//    hints.ai_family = AF_INET;
//    hints.ai_socktype = SOCK_STREAM;
//    hints.ai_flags = 0;
//    hints.ai_protocol = 0;
//
//    char port_buf[6];
//    snprintf(port_buf, 6, "%u", port);
//
//    auto re = getaddrinfo(url, port_buf, &hints, &result);
//    if (re != 0) {
//        return nullptr;
//    }
//
//    if (clock() - t > CLOCKS_PER_SEC * 0.1) {
//        logger(ERR, stderr, "dns slow %ds", (clock() - t) / CLOCKS_PER_SEC);
//    }
//    return new std::pair<sockaddr, unsigned int>(*(result->ai_addr), result->ai_addrlen);
//}
