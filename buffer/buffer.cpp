
#include "buffer.h"

#include <error.h>
#include <stddef.h>
#include <iostream>

axy::Buffer::Buffer(uint32_t init_buf_size)
    : _buffer(init_buf_size), _read_pos(0), _write_pos(0) {
}


/*
*   可读范围 [_read_pos, _write_pos)
*   @return 可读字节数
*/
const uint32_t axy::Buffer::get_readable_size() const {
    return _write_pos - _read_pos;  //[_read_pos, _write_pos)
}

/*
    可写范围 [_write_pos, _buffer.size())
    @return 可写字节数
*/
const uint32_t axy::Buffer::get_writeable_size() const {
    return _buffer.size() - _write_pos;
}

//
const uint32_t axy::Buffer::get_read_pos() const {
    return _read_pos;
}

const uint32_t axy::Buffer::get_write_pos() const {
    return _write_pos;
}

void axy::Buffer::resize(const uint32_t len) {
    std::vector<char> v(len);
    _buffer.swap(v);
}

void axy::Buffer::clear() {
    //内存缩小
    if (_buffer.capacity() > INIT_BUF_SIZE) this->resize(INIT_BUF_SIZE);
    _read_pos = 0;
    _write_pos = 0;
}



void axy::Buffer::append(const char *str, uint32_t len) {
    if (!str) {

#ifdef DEBUG
        LOG_WARN("append str in buffer warning, str is nullptr!!!\n");
#endif //!DEBUG
        return ;
    }
    //确保可写
    ensure_writeable(len);

    //写入
    std::copy(str, str + len, get_write_pos_ptr());
    has_write(len);
}
void axy::Buffer::append(const std::string &str) {
    append(str.data(), str.size());
}



//确保可写，容量不够就扩容。
void axy::Buffer::ensure_writeable(const uint32_t len) {
    if (get_writeable_size() >= len) return ;

    //  不可写
    expansion(len);

    if (get_writeable_size() < len) {
#ifdef DEBUG
        LOG_WARN("容量不够，扩容失败\n");
#endif // DEBUG

    }
}

void axy::Buffer::expansion(const uint32_t len) {
    if (get_writeable_size() + _read_pos < len) {
        _buffer.resize(_write_pos + len); // resize不会初始化原来的数据，只是扩容
        return ;
    }
    uint32_t readable_size = get_readable_size();
    std::copy(beginPtr() + _read_pos, beginPtr() + _write_pos, beginPtr());
    _read_pos = 0;
    _write_pos = readable_size;
}



std::string axy::Buffer::retrieve_all_to_str() {
    std::string ret(beginPtr() + _read_pos, get_readable_size());
    return ret;
}





axy::HttpBuf::HttpBuf() : _read_buf(std::make_shared<Buffer>())
    , _write_buf(std::make_shared<Buffer>()) {
}
axy::HttpBuf::~HttpBuf() {
    _read_buf.reset();
    _write_buf.reset();
}



int axy::HttpBuf::readFd(uint32_t fd, int *save_errno) {

    uint32_t writeable_size = _read_buf->get_writeable_size();

    //  可写字节数为0，则扩容
    if (writeable_size <= 0) {
        _read_buf->expansion(INIT_BUF_SIZE);
        writeable_size = _read_buf->get_writeable_size(); // 如果发生扩容，可写字节数就发生变化
    }

    const int len = recv(fd, (void *)_read_buf->get_write_begin(), writeable_size, MSG_WAITALL);

    if (len < 0) { //接收异常
        *save_errno = errno;
        return len;
    }
    _read_buf->has_write(len); //写入字节数

    return len;
}
int axy::HttpBuf::writeFd(uint32_t fd, int *save_errno) {
    //读取需要发送的字节数
    uint32_t readable_size = _write_buf->get_readable_size();
    if (readable_size <= 0) { //已经没有需要发送的数据
        _write_buf->clear();
        return 0; //发送完毕
    }


    int len = send(fd, _write_buf->get_read_begin(), readable_size, MSG_WAITALL);
    if (len < 0) {
        *save_errno = errno;
        return len;
    }

    _write_buf->hasRead(len);
    readable_size = _write_buf->get_readable_size();
    if (readable_size <= 0) _write_buf->clear();
    return len;
}