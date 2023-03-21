#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__



#include "../head.h"
#include "epoller.h"
#include "../pool/threadPool.h"
#include "../buffer/buffer.h"
// #include "../clientConnection/clientConn.h"
#include "../http/httpconn.h"
#include "../timer/heaptimer.h"

#include "../pool/sqlConnPool.h"

namespace axy {


class WebServer {


public:

    WebServer(const uint16_t port, uint32_t time_ms
    , short sql_port, const char *sql_user, const char *sql_pwd
    , const char *db_name, const char *static_src = "/home/axieyun_aly_kkb/resources/blogSystem");
    ~WebServer();
    void start();




private:
    void _add_client(int fd, const sockaddr_in &addr);
    void _do_read(axy::HttpConn *client_conn);
    void _readClient(axy::HttpConn *client_conn);
    void onProcess(axy::HttpConn *client_conn);
    void _writeClient(axy::HttpConn *client_conn);
    void _do_write(axy::HttpConn *client_conn);
    void _close_client(axy::HttpConn *client_conn);
    void _y_close_client(axy::HttpConn *client_conn); //因为
    void _extentTime(axy::HttpConn *client_conn);


    bool _init_server();
    void _init_event_mode(int trig_mode);
    void _do_listen();


    void _send_error(uint32_t fd, const char *info);

// data
private:
    uint16_t _port;            //端口号
    int _listen_fd;    //监听的fd
    bool _is_close;         //是否关闭服务器
    bool _open_linger;
    uint32_t _listen_event;
    uint32_t _conn_event;
    char _trig_mode;        // ET or LT model
    uint32_t _time_out_ms;  //小根堆处理
    uint32_t _MAX_USER;


    char *_srcDir; //当前工作目录的绝对路径



    axy::Epoller::ptr _epoll;
    axy::ThreadPool::ptr _thread_pool;
    std::unique_ptr<HeapTimer> _timer;
    std::unordered_map<int, axy::HttpConn> _users; // fd -> HttpConnection

};


}   // !namespace axy



#endif // !__WEB_SERVER_H__