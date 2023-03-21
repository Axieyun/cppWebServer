#ifndef __BUFFER_H__
#define __BUFFER_H__


#include <vector>
#include <atomic>
#include <stdint.h>
#include <memory>
#include <cstring>   //perror
#include <unistd.h>  // write

#include <sys/types.h>
#include <sys/socket.h>

#include "../log.h"


#define INIT_BUF_SIZE (1 << 11)


namespace axy {






class Buffer {


public:
    typedef std::shared_ptr<Buffer> ptr;
    // 1024
    Buffer(uint32_t init_buf_size = INIT_BUF_SIZE);


public:

    const uint32_t get_readable_size() const;
    const uint32_t get_writeable_size() const;
    const uint32_t get_read_pos() const;
    const uint32_t get_write_pos() const;

    // 返回实际使用字节数
    const uint32_t size() const { return _buffer.size(); }
    // 返回实际的内存大小
    uint32_t capacity() const { return _buffer.capacity(); }



    // 数据清除，并没有销毁
    void clear();
    void has_write(const int len) { _write_pos += len; }
    void hasRead(const int len) { _read_pos += len; }

    const char *get_write_begin() const { return beginPtr() + _write_pos; }
    const char *get_read_begin() const { return beginPtr() + _read_pos; }
    void append(const char *str, uint32_t len);
    void append(const std::string &str);

    //确保可写，容量不够就扩容。
    void ensure_writeable(const uint32_t len);
    void expansion(const uint32_t len);





    std::string retrieve_all_to_str();



private:

    //data
    std::vector<char> _buffer;
    std::atomic<uint32_t> _read_pos;        // 已读的范围 [0, _read_pos)
    std::atomic<uint32_t> _write_pos;       // 已写的范围 [0, _write_pos)
    // struct iovec iov[2];
    char *beginPtr() { return &(_buffer[0]); }
    const char *beginPtr() const { return &(_buffer[0]); }
    char *get_write_pos_ptr() { return (&_buffer[0]) + _write_pos; }
    void resize(const uint32_t len = INIT_BUF_SIZE);//  内存缩小
};







class HttpBuf {

public:
    typedef std::unique_ptr<HttpBuf> ptr;
    HttpBuf();
    ~HttpBuf();

public:

    int readFd(uint32_t fd, int *save_errno);
    int writeFd(uint32_t fd, int *save_errno);

public:
    std::string get_write_msg() { return _read_buf->retrieve_all_to_str(); }

    Buffer::ptr& get_read_ptr() { return _read_buf; }
    Buffer::ptr& get_write_ptr() { return _write_buf; }

private:
    axy::Buffer::ptr _read_buf;         // 读缓冲区
    axy::Buffer::ptr _write_buf;        // 写缓冲区

};








} //!namespace axy


#endif // !__BUFFER_H__
