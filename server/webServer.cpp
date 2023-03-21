#include "webServer.h"
#include "../http/httpconn.h"
#include <iostream>
#include "../log.h"
#include "../pool/sqlConnPool.h"



axy::WebServer::WebServer(const uint16_t port, uint32_t time_ms
    , short sql_port, const char *sql_user, const char *sql_pwd, const char *db_name
        , const char *static_src)
    : _port(port)
    , _is_close(false)
    , _time_out_ms(time_ms)
    , _open_linger(true)
    , _trig_mode('E')
    , _MAX_USER(65536)
    , _timer(std::make_unique<axy::HeapTimer>())
    , _thread_pool(std::make_unique<axy::ThreadPool>(5))
    , _epoll(std::make_unique<axy::Epoller>()) {

    _srcDir = getcwd(nullptr, 256);
    assert(_srcDir);

    static const char *M_INDEX = "/resources/blogSystem";
    if (static_src == nullptr) strncat(_srcDir, M_INDEX, strlen(M_INDEX));
    else strncpy(_srcDir, static_src, strlen(static_src));
    axy::HttpConn::setSrcDir(_srcDir);



    axy::SqlConnPool::instance()->init("localhost", sql_port, sql_user, sql_pwd, db_name, _thread_pool->get_cpu_size());

    // axy::HeapTimer::setTimeOutMs(time_ms);

    _init_event_mode(_trig_mode);
    axy::HttpConn::set_model(_trig_mode == 'E' || _trig_mode == 'e');

    if (!_init_server()) _is_close = true; //服务器启动失败
    // _is_close = true;
}

//  释放资源
axy::WebServer::~WebServer() {
    close(_listen_fd);
    _is_close = true;
    _epoll.reset();
    _timer.reset();
    free(_srcDir);
    _thread_pool.reset();
}


void axy::WebServer::start() {
    // srand(time(0));
    int time_ms = -1; //无事件阻塞等待
    while (!_is_close) {

        if (_time_out_ms > 0) {
            time_ms = _timer->getNextTick();
        }
        // std::cout << "time_ms: " << time_ms << std::endl;
        int event_cnt = _epoll->wait(time_ms);
        // std::cout << "event_cnt : " << event_cnt << std::endl;
        for (int i = 0; i < event_cnt; ++i) {

            int fd = _epoll->get_event_fd(i);
            uint32_t events = _epoll->get_events(i);

            if (fd == _listen_fd) {

                this->_do_listen();

            } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 对端断开 触发 (EPOLLIN | EPOLLRDHUP)
                // EPOLLHUP 表示读写都关闭
                this->_close_client(&_users[fd]);

            } else if (events & EPOLLIN) {
                this->_readClient(&_users[fd]);

            } else if (events & EPOLLOUT) {
                this->_writeClient(&_users[fd]);

            }
        }
    }

}

void axy::WebServer::_extentTime(axy::HttpConn *client_conn) {
    assert(client_conn);
    if (_time_out_ms > 0) {
        if (!_timer->adjust(client_conn->get_fd(), _time_out_ms)) {
            _timer->add(client_conn->get_fd()
                , _time_out_ms
                , &axy::WebServer::_close_client
                , this, client_conn);
        }

    }
}


void axy::WebServer::_y_close_client(axy::HttpConn *client_conn) {
    //把定时任务删除
    _timer->doWork(client_conn->get_fd(), false);
    _close_client(client_conn);
}

void axy::WebServer::_readClient(axy::HttpConn *client_conn) {
    assert(client_conn);
    _extentTime(client_conn);
    _thread_pool->add_task(&axy::WebServer::_do_read, this, client_conn);
}
void axy::WebServer::_writeClient(axy::HttpConn *client_conn) {
    assert(client_conn);
    _extentTime(client_conn);
    _thread_pool->add_task(&axy::WebServer::_do_write, this, client_conn);
}



/******************************多线程环境*********************************/
void axy::WebServer::_do_read(axy::HttpConn *client_conn) {

    assert(client_conn);

    int read_errno = 0;
    int ret = client_conn->read(&read_errno);
    // std::cout << client_conn->get_client_in_msg() << "\n";
    // std::cout << ret << "\n";
    // 对端关闭，//连接异常，需要关闭
    if (ret == 0 || (ret < 0 && read_errno != EAGAIN
        && read_errno != EINTR
        && read_errno != EWOULDBLOCK
        )
    ) {
        _close_client(client_conn);
        return ;
    }

    onProcess(client_conn);

}

void axy::WebServer::onProcess(axy::HttpConn *client_conn) {

    assert(client_conn);

    if (client_conn->process()) {
        _epoll->mod_fd(client_conn->get_fd(), _conn_event | EPOLLOUT);
    } else {
        _epoll->mod_fd(client_conn->get_fd(), _conn_event | EPOLLIN);
    }
}



void axy::WebServer::_do_write(axy::HttpConn *client_conn) {

    assert(client_conn);

    int write_errno;
    int ret = client_conn->write(&write_errno); // ret < 0


    if (client_conn->is_send_ok()) {
        // 发送完毕
        if (client_conn->isKeepAlive()) {
            // printf(GREEN "Client[%d] keep-alive" NONE "\n", client_conn->get_fd());
            onProcess(client_conn); // 判断长连接
            return ;
        }
    }
    else if (ret < 0 && (write_errno == EAGAIN || write_errno == EWOULDBLOCK || write_errno == EINTR)) {
        // (ret < 0 && (write_errno == EAGAIN || write_errno == EWOULDBLOCK || write_errno == EINTR))
        printf(GREEN "client[%d] mod epollout" NONE "\n", client_conn->get_fd());
        _epoll->mod_fd(client_conn->get_fd(), _conn_event | EPOLLOUT);
        return ;
    }


    // printf(RED "Client[%d], %s" NONE "\n", client_conn->get_fd(), ret == 0 ? "主动关闭" : "异常关闭");

    // if (ret == 0 || (ret < 0
    //     && (write_errno != EAGAIN
    //     && write_errno != EWOULDBLOCK
    //     && write_errno != EINTR)))
    // if (!client_conn->isKeepAlive())

    _close_client(client_conn);
    return ;


}

//是件触发模式
void axy::WebServer::_init_event_mode(int trig_mode) {
    _listen_event = EPOLLRDHUP;
    _conn_event = EPOLLONESHOT | EPOLLRDHUP;
    switch (trig_mode) {
        case 'e':
        case 'E': {
            _listen_event |= EPOLLET;
            _conn_event |= EPOLLET;
            break;
        }
    // HttpConn::isET = (connEvent_ & EPOLLET);
    }
}


bool axy::WebServer::_init_server() {


    if(_port > 65535 || _port < 1024) {
#ifdef DEBUG
        LOG_ERROR("Port:%d error!",  _port);
#endif // DEBUG
        return false;
    }



    _listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(_listen_fd < 0) {
#ifdef DEBUG
        LOG_ERROR("Create socket error! port : %d", _port);
#endif // DEBUG
        return false;
    }

    int ret;
    if(_open_linger) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        struct linger optLinger = { 0 };
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
        ret = setsockopt(_listen_fd, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
        if(ret < 0) {

#ifdef DEBUG
            LOG_ERROR("Create socket error! port : %d", _port);
#endif // DEBUG

            close(_listen_fd);
            return false;
        }
    }

    //在所有TCP服务器中，在调用bind之前设置SO_REUSEADDR套接口选项；
    if (_epoll->setSockOptAddrProt(_listen_fd, true) < 0) {
#ifdef DEBUG
    LOG_ERROR("set addr fail fd[%d]!\n", _listen_fd);
#endif // DEBUG
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(_port);
    ret = bind(_listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
#ifdef DEBUG
        LOG_ERROR("Bind Port:%d error!", _port);
#endif // DEBUG
        close(_listen_fd);
        return false;
    }

    ret = listen(_listen_fd, 100);
    if(ret < 0) {
#ifdef DEBUG
        LOG_ERROR("Listen port:%d error!", _port);
#endif // DEBUG
        close(_listen_fd);
        return false;
    }
    ret = _epoll->add_fd(_listen_fd,  _listen_event | EPOLLIN, axy::HttpConn::get_user_count());
    if(ret == 0) {
#ifdef DEBUG
        LOG_ERROR("Add listen error!");
#endif // DEBUG
        close(_listen_fd);
        return false;
    }

    printf(GREEN"Server port: %d, listen fd[%d]" NONE"\n", _port, _listen_fd);

    return true;
}




void axy::WebServer::_do_listen() {
    static char ret[] = "HTTP/1.1 503 ERROR\r\n\n";
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(_listen_fd, (struct sockaddr *)&addr, &len);
        if(fd <= 0) break;
        else if(axy::HttpConn::get_user_count() >= _MAX_USER) {
            _send_error(fd, ret);
#ifdef DEBUG
        LOG_WARN("Clients is full!");
#endif // DEBUG
            break;
        }
        _add_client(fd, addr);
    } while(_listen_event & EPOLLET); //注册et事件的话，得循环
    // _epoll->mod_fd(_listen_fd, _listen_event | EPOLLIN);
}



void axy::WebServer::_add_client(int fd, const sockaddr_in &addr) {

    if ( !_epoll->add_fd(fd, EPOLLIN | _conn_event, axy::HttpConn::get_user_count()) ) {
#ifdef DEBUG
        LOG_WARN("Clients is full!");
#endif // DEBUG
        return ;
    }
    // _users.insert(std::make_pair(fd, std::make_shared<axy::HttpConn>()) );
    if ( !(_users[fd].init(fd, addr)) ) { //add fd user count
        _epoll->del_fd(fd);
        return ; //init fail
    }

    if (_time_out_ms > 0) {
        _timer->add(fd, _time_out_ms, &WebServer::_close_client, this, &_users[fd]);
    }

    if (_epoll->setSockOptAddrProt(fd, true) < 0) {
#ifdef DEBUG
        LOG_WARN("set addr fail fd[%d]!\n", fd);
#endif // DEBUG
    }


}

void axy::WebServer::_close_client(axy::HttpConn *client_conn) {
    if (!client_conn) {
#ifdef DEBUG
        LOG_WARN("client pointer is nullptr\n");
#endif // DEBUG
        return ;
    }
/*
debug日常
2023-03-13中午，改代码改出bug，下午发现，先close _fd，（1）再重置 _fd = -1，
    导致在（1）处时 可能 有新连接进来，此时的新fd就是close 的fd，会发生 accept 出现相同的fd
    改正：先记录要close 的_fd值，再重置_fd = -1，最后close 记录的fd
*/
    int fd = client_conn->get_fd();
    client_conn->clear();
    _epoll->del_fd(fd, true);

}


void axy::WebServer::_send_error(uint32_t fd, const char *info) {
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
#ifdef DEBUG
        LOG_WARN("send error to client[%d] error!", fd);
#endif // DEBUG
    }
    close(fd);
}
