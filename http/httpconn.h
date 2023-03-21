#ifndef __HTTPCONN_H__
#define __HTTPCONN_H__

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>
#include <atomic>
#include <stdint.h>
#include <sys/sendfile.h>

#include <string>
#include <sstream>




#include "../buffer/buffer.h"
#include "../log.h"
#include "../head.h"
#include "./httpRequest.h"
#include "../http/httpResponse.h"


#include <iostream>
#include <stdio.h>

#include <zlib.h>





// 数据大小大于 PIECE_ZIP_COND_SIZE就进行f分块压缩传输
const static int PIECE_ZIP_COND_SIZE = ((1 << 12) * 60); //块大小


/*
16384字节是一个经验值，通常同于网络传输中TCP协议的最大数据段大小，
使用MSS大小的数据块可以避免网络拥塞和性能问题，同时也可以更好的利用网络宽带，提高数据传输速度
对于大文件传输，将文件分成MSS大小的数据块进行压缩和传输可以减少传输时间和网络带宽的消耗。
当然，具体的分块大小也需要根据具体情况进行调整，例如网络延迟、带宽限制等。
*/
#define MAX_MSS (PIECE_ZIP_COND_SIZE * 4)


/*
设置压缩比率，最小为1，处理速度快，传输速度慢；9为最大压缩比，处理速度慢，传输速度快;
这里表示压缩级别，可以是0到9中的任一个，级别越高，压缩就越小，
节省了带宽资源，但同时也消耗CPU资源，所以一般折中为6
*/
static const int GZIP_COMP_LEVEL = 6;



namespace axy {


struct HttpPiece {
public:

    // const int getSize() const { return _size;}
    void setSize(int len) { _size = len; }
    char *getBegin() { return _val; }
    void setChar(int ind, char c) { _val[ind] = c; }
    const int getMaxSize() const { return MAX_MSS - 2; }

private:
    int _size = 0;
    char _val[MAX_MSS];
};

class HttpConn {

public:
    HttpConn();
    ~HttpConn();
    void hClose();

public:
    static const int get_user_count() { return _user_count; }
private:
    static std::atomic<int> _user_count;
    static void _add_user_count() { ++_user_count; }
    static void _jj_user_count() { if (_user_count > 0) --_user_count; }



public:
    bool init(int fd, const sockaddr_in &addr);
    void clear();
    bool isKeepAlive() const { return _request->isKeepAlive(); }
    const char *get_ip() const { return _ip; }
    const uint16_t get_prot() const { return _port; }
    const int get_fd() const { return _fd; }
    std::string get_client_in_msg() { return _http_buf.get_read_ptr()->retrieve_all_to_str(); }
    std::string get_client_out_msg() { return _http_buf.get_write_ptr()->retrieve_all_to_str(); }
    int get_out_readable_size() { return _http_buf.get_write_ptr()->get_readable_size(); }
    int send_header(int *save_errno);

private:
    // 不进行文件压缩、分块的传输
    int _write_base(int *save_errno);
    int _write_piecemeal(int *save_errno); //不压缩、分块传输
    int _write_gzip(int *save_errno);
    int _send_base(const char *buff, int buff_size, int *save_errno);
    int _send_range(int *save_errno);

public:
    int read(int *save_errno);
    int write(int *save_errno);
    bool process();
    bool is_send_ok() { return (_http_buf.get_write_ptr()->get_readable_size() <= 0); }

public:
    static void set_model(bool is) { _is_ET = is; }
    static bool is_ET() { return _is_ET; }
private:
    static bool _is_ET;


public:
    static void setSrcDir(char* srcDir) { _srcDir = srcDir; }
    static const char* const getSrcDir() { return _srcDir; }
private:
    static char *_srcDir;


private:
    int _fd = -1;
    axy::HttpBuf _http_buf;
    char *_ip;
    uint16_t _port;

    int _iovCnt;
    struct iovec _iov[2];

    bool _is_close;

    std::unique_ptr<axy::HttpRequest> _request;  // http 请求
    std::unique_ptr<axy::HttpResponse> _response;


    // struct sockaddr_in _client_addr; //客户地址信息
};




}  // !namespace axy

#endif // !__HTTPCONN_H__
