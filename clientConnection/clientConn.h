#ifndef __CLIENT_CONN_H__
#define __CLIENT_CONN_H__


#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      
#include <atomic>
#include <stdint.h>


#include "../buffer/buffer.h"
#include "../log.h"


namespace axy {



class ClientConn {

    ClientConn();

    void close();
    void _read_client();
    void _write_client();

public:
    static void add_user_count() { ++_user_count; }
    static const uint32_t get_user_count() { return _user_count; }
    static void set_ET(bool is_et) { _is_ET = is_et; }
private:
    static std::atomic<uint32_t> _user_count;
    static std::atomic<bool> _is_ET;

    

private:
    uint32_t _fd;

    axy::HttpBuf::ptr http_buf;
};




}  // !namespace axy

#endif // !__CLIENT_CONN_H__
