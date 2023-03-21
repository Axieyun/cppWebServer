#ifndef __EPOLLER_H__
#define __EPOLLER_H__

#include <sys/epoll.h> //epoll_ctl()
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <vector>
#include <errno.h>
#include <stdint.h>
#include <memory>
#include "../head.h"
#include "../log.h"




#define MAX_FD 65536

namespace axy {





class Epoller {
public:
    typedef std::unique_ptr<Epoller> ptr;
    //explicit只能写在在声明中，不能写在定义中。
    explicit Epoller(uint32_t  maxEvent = 1024);

    ~Epoller();
    int setSockOptAddrProt(int fd, bool is_listen = false);
    //  默认非阻塞
    bool add_fd(int  fd, uint32_t events, uint32_t user_size, const bool is_nonblocking = true);

    bool mod_fd(int  fd, uint32_t events);

    bool del_fd(int  fd, bool is_close = true); //默认关闭连接

    int wait(int timeoutMs = -1); //默认阻塞等待

    int get_event_fd(uint32_t  i) const;

    uint32_t get_events(uint32_t  i) const;


private:
    uint32_t  _epoll_fd;

    std::vector<struct epoll_event> _events;
};



















}
































#endif // !__EPOLLER_H__
