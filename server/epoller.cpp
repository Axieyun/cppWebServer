#include "epoller.h"



/*
对于注册EPOLLONSHOT事件的文件描述符，操作系统最多触发其上注册的一个可读、
可写、或者异常事件，且仅触发一次，除非使用epoll_ctl重置该文件描述符上注册
的EPOLLONSHOT事件。这样，当一个线程在处理某个socket时，其他线程就不可能
有机会操作该socket的。所以，注册了EPOLLONSHOT事件的socket一旦被某个线程
处理完，该线程就应该立即重置这个socket的EPOLLONSHOT事件，以确保这个socket
下一次可读时，其EPOLLIN事件能被触发，进而让其他线程有机会继续处理这个socket.
// 重置fd上的EPOLLONSHOT事件
*/
void reset_oneshot(const int epollFd, const int fd, uint32_t events) {
    //printf("fd : %d 重置 EPOLLONSHOT事件\n", fd);
    struct epoll_event event;
    event.data.fd = fd;
    //event.events = EPOLLET | EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLRDHUP;
    event.events = events | EPOLLONESHOT;
    epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &event);
    return ;
}

//关闭Nagle算法
static void _set_tcp_nodelay(int fd) {
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));
}


//设置非阻塞
void make_nonblocking(uint32_t fd) {
/*
    int old_option = fcntl( fd, F_GETFL );
    old_option |= O_NONBLOCK;
    fcntl( fd, F_SETFL, old_option );
*/
    fcntl( fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK );
}

// 设置阻塞
void make_blocking(uint32_t fd) {
/*
    int old_option = fcntl( fd, F_GETFL );
    old_option &= ~O_NONBLOCK;
    fcntl( fd, F_SETFL, old_option );
*/
    fcntl( fd, F_SETFL, fcntl(fd, F_GETFL) & (~O_NONBLOCK) );
}



axy::Epoller::Epoller(uint32_t  maxEvent) : _epoll_fd(epoll_create(512))
    , _events(maxEvent) {
}

axy::Epoller::~Epoller() {
    close(_epoll_fd);
}

int axy::Epoller::setSockOptAddrProt(int fd, bool is_listen) {
    int optval = 1;
    /* 端口复用 */
    // ret = setsockopt(_)
    /* 只有最后一个套接字会正常接收数据。 */
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
#ifdef DEBUG
        LOG_ERROR("set socket setsockopt error !");
#endif // DEBUG
        close(fd);
        return ret;
    }
    if (is_listen) {
        ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const void*)&optval, sizeof(int));
        if(ret == -1) {
#ifdef DEBUG
            LOG_ERROR("set socket setsockopt error !");
#endif // DEBUG
            close(fd);
            return ret;
        }
    }
    return 0;
}

bool axy::Epoller::add_fd(int fd, uint32_t events, uint32_t user_size, const bool is_nonblocking) {
    if (user_size >= MAX_FD) return false;
    if(fd < 0) return false;

    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    if (is_nonblocking) make_nonblocking(fd);
    _set_tcp_nodelay(fd);


    return (0 == epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev));
}

bool axy::Epoller::mod_fd(int fd, uint32_t events) {
    if(fd < 0) return false;

    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return (0 == epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev));
}

bool axy::Epoller::del_fd(int fd, bool is_close) {
    if(fd < 0) return false;

    bool ret = (0 == epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, nullptr));

    if (is_close) close(fd); //关闭连接

    return ret;
}

int axy::Epoller::wait(int timeoutMs) {
    return epoll_wait(_epoll_fd, &_events[0], static_cast<int>(_events.size()), timeoutMs);
}

int axy::Epoller::get_event_fd(uint32_t  i) const {
    // assert(i < _events.size() && i >= 0);
    return _events[i].data.fd;
}

uint32_t axy::Epoller::get_events(uint32_t  i) const {
    // assert(i < _events.size() && i >= 0);
    return _events[i].events;
}