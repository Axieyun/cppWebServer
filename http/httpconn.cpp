#include "./httpconn.h"





std::atomic<int> axy::HttpConn::_user_count{0};
char *axy::HttpConn::_srcDir;
bool axy::HttpConn::_is_ET;



axy::HttpConn::HttpConn()
    : _request(std::make_unique<axy::HttpRequest>())
    , _response(std::make_unique<axy::HttpResponse>())
    , _is_close(true) {
}

axy::HttpConn::~HttpConn() {
    hClose();
    _response.reset();
    _request.reset();
}

void axy::HttpConn::hClose() {
    _response->unmapFile();
    if(_is_close == false) {
        _is_close = true;
        --_user_count;
        if (~_fd) close(_fd);
#ifdef DEBUG
        LOG1("Client[%d](%s:%d) quit, UserCount:%d", _fd, _ip, _port, get_user_count());
#endif // DEBUG
    }
}

bool axy::HttpConn::init(int fd, const sockaddr_in &addr) {
    if (_fd > -1) {
#ifdef DEBUG
        LOG_ERROR("fd first[%d], fd second[%d]重复了\n", fd, _fd);
#endif // DEBUG
        return false;
    }
    _add_user_count();
    _fd = fd;
    _is_close = true;
    _iovCnt = 0; _iov[1].iov_len = _iov[0].iov_len = 0; _iov[1].iov_base = _iov[0].iov_base = nullptr;
    _http_buf.get_read_ptr()->clear();
    _http_buf.get_write_ptr()->clear();
    _ip = inet_ntoa(addr.sin_addr);
    _port = addr.sin_port;

#ifdef DEBUG
    LOG1("Client[%d](%s:%d) in, userCount:%d\n", _fd, _ip, _port, get_user_count());
#endif // DEBUG
    return true;
}


void axy::HttpConn::clear() {

    if (_fd == -1) return ;

    _jj_user_count();

#ifdef DEBUG
        LOG1("Client[%d](%s:%d) quit, userCount: %d\n", _fd, _ip, _port, get_user_count());
#endif // DEBUG

    _fd = -1;
}


int axy::HttpConn::read(int *save_errno) {

    int len = -1;

    do {
        len = _http_buf.readFd(_fd, save_errno);

        if (len <= 0) break; //对端关闭或者出现异常

    } while (is_ET());

    return len;
}

//集中写
int axy::HttpConn::_write_base(int *save_errno) {
    _iov[1].iov_len = _response->fileLen();
    _iov[1].iov_base = _response->file();
    _response->addContentLength(*_http_buf.get_write_ptr(), _iov[1].iov_len);
    _response->addCRLF(*_http_buf.get_write_ptr());

    _iov[0].iov_base = (char *)_http_buf.get_write_ptr()->get_read_begin();
    _iov[0].iov_len = _http_buf.get_write_ptr()->get_readable_size();
    _iovCnt = 2;

    int len = -1;
    do {

        len = writev(_fd, _iov, _iovCnt);
        if (len < 0) { //出错
            *save_errno = errno;
            break;
        }
        if (_iov[0].iov_len + _iov[1].iov_len == 0) break;
        else if (len > _iov[0].iov_len) {
            _iov[1].iov_base = (uint8_t*)_iov[1].iov_base + (len - _iov[0].iov_len);
            _iov[1].iov_len -= len - _iov[0].iov_len;
            if (_iov[0].iov_len > 0) {
                _iov[0].iov_base = nullptr;
                _http_buf.get_write_ptr()->hasRead(_iov[0].iov_len);
                _iov[0].iov_len = 0;
            }

        }
        else {
            if (_iov[0].iov_base) {
                _iov[0].iov_base = (uint8_t*)_iov[0].iov_base + len;
                _iov[0].iov_len -= len;
                _http_buf.get_write_ptr()->hasRead(len);
            }
            else {
                _iov[1].iov_base = (uint8_t *)_iov[1].iov_base + len;
                _iov[1].iov_len -= len;
            }
        }
    } while (is_ET());

    return len;
}

int axy::HttpConn::_send_base(const char *buff, int buff_size, int *save_errno) {
    int len = -1;
    int total_send_size = 0;
    do {
        len = send(_fd, buff + total_send_size, buff_size - total_send_size, MSG_WAITALL);
        total_send_size += len;
        if (len < 0) { //出错
            *save_errno = errno;
            break;
        }
    } while (is_ET() && (buff_size != total_send_size));

    return len;
}


int axy::HttpConn::send_header(int *save_errno) {
    int len = -1;
    do {
        len = _http_buf.writeFd(_fd, save_errno);
        if (len < 0) { //异常或者发送完毕 需要处理
            return len;
        }
    } while (is_ET() && _http_buf.get_write_ptr()->get_readable_size() > 0);
    _iov[0].iov_base = nullptr;
    _iov[0].iov_len = 0;
    return len;
}


int axy::HttpConn::_write_piecemeal(int *save_errno) {

    _iov[1].iov_len = _response->fileLen();
    _iov[1].iov_base = _response->file();

    _response->addTransferEncoding(*_http_buf.get_write_ptr());
    _response->addVary(*_http_buf.get_write_ptr());
    _response->addCRLF(*_http_buf.get_write_ptr());
    int len = send_header(save_errno);
    if (len < 0) return len;

    /*************************发送body***********************/
    // 计算块的数量
    int file_size = _response->fileLen();
    int block_count = (file_size + PIECE_ZIP_COND_SIZE - 1) / PIECE_ZIP_COND_SIZE;

    int total_send_size = 0;
    len = -1;

    for (int i = 0; i < block_count; ++i) {

        int block_size = (i == block_count - 1) ? (file_size - i * PIECE_ZIP_COND_SIZE) : PIECE_ZIP_COND_SIZE;

        //发送
        std::stringstream ss;
        ss << std::hex << block_size << "\r\n";
        std::string line = ss.str();
        // len = send(_fd, line.data(), line.size(), MSG_WAITALL);
        len = _send_base(line.data(), line.size(), save_errno);
        if (len < 0) goto _close_end;

        // len = send(_fd, (char *)_iov[1].iov_base + total_send_size, block_size, MSG_WAITALL);
        len = _send_base((char *)_iov[1].iov_base + total_send_size, block_size, save_errno);
        if (len < 0) goto _close_end;

        // len = send(_fd, "\r\n", 2, MSG_WAITALL);
        len = _send_base("\r\n", 2, save_errno);
        if (len < 0) goto _close_end;

        total_send_size += block_size;
#ifdef DEBUG
        printf(GREEN "fd[%d] [%d -> %d], [%d -> %d]" NONE "\n", _fd, total_send_size, file_size, i, block_count);
#endif // DEBUG

    }

    // len = send(_fd, "0\r\n\r\n", 5, MSG_WAITALL);
    len = _send_base("0\r\n\r\n", 5, save_errno);
    if (len < 0) goto _close_end;

    _iov[1].iov_base = nullptr;
    _iov[1].iov_len = 0;

_close_end:
    // *save_errno = errno;
    return len;
}


int axy::HttpConn::_send_range(int *save_errno) {

    _response->addRange(*_http_buf.get_write_ptr());

    int send_size = _response->getRangeSendSize();
    const char *send_begin = _response->getRangeSendBegin();

    _response->addContentLength(*_http_buf.get_write_ptr(), send_size);
    _response->addCRLF(*_http_buf.get_write_ptr());

    int len = send_header(save_errno);
    if (len < 0) return len;

    len = _send_base(send_begin, send_size, save_errno);
    return len;
}


int axy::HttpConn::write(int *save_errno) {



    if (_response->is_range()) {
#ifdef DEBUG
        printf(GREEN "范围传输" NONE "\n");
#endif // DEBUG

        return _send_range(save_errno);
    }

    bool is_gzip = _response->is_gzip();
    bool is_piecemeal = _response->is_piecemeal();
#ifdef DEBUG
    if (is_gzip) {
        printf(GREEN "分块压缩发送" NONE "\n");
    } else if (is_piecemeal) {
        printf(GREEN "分块发送" NONE "\n");
    } else {
        printf(GREEN "直接发送" NONE "\n");
    }
#endif // DEBUG

    // 不压缩、不分块传输
    if (!is_gzip && !is_piecemeal) return _write_base(save_errno); //不用分块压缩传输
    // 不压缩、分块传输
    else if (!is_gzip && is_piecemeal) return _write_piecemeal(save_errno);

    /*******************分块压缩传输*********************/
    _iov[1].iov_len = _response->fileLen();
    _iov[1].iov_base = _response->file();
    _response->addTransferEncoding(*_http_buf.get_write_ptr());
    _response->addVary(*_http_buf.get_write_ptr());
    _response->addGzip(*_http_buf.get_write_ptr());

    _response->addCRLF(*_http_buf.get_write_ptr());
    int len = send_header(save_errno);
    if (len < 0) return len;

    /*************************发送body***********************/

    // 计算块的数量
    int file_size = _response->fileLen();
    int block_count = (file_size + PIECE_ZIP_COND_SIZE - 1) / PIECE_ZIP_COND_SIZE;


    z_stream zs;
    // 初始化zlib
    //zalloc, zfree和opaque字段必须在 调用之前初始化
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    if (deflateInit2(&zs, GZIP_COMP_LEVEL, Z_DEFLATED, MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return 0; //发送失败
    }

    std::unique_ptr<HttpPiece> _piece = std::make_unique<HttpPiece>();
    int total_send_size = 0;
    int write_size = _piece->getMaxSize();

    for (int i = 0; i < block_count; ++i) {

        int block_size = (i == block_count - 1) ? (file_size - i * PIECE_ZIP_COND_SIZE) : PIECE_ZIP_COND_SIZE;

        //压缩
        zs.next_in = (Bytef *)((Bytef *)_iov[1].iov_base + i * PIECE_ZIP_COND_SIZE/**/);
        zs.avail_in = block_size;
        int flush = (i != block_count - 1) ? Z_NO_FLUSH : Z_FINISH;

        do {
            zs.next_out = (Bytef *)_piece->getBegin();
            zs.avail_out = write_size;

            int rc = deflate(&zs, flush);

            if ((i != block_count - 1) && rc != Z_OK) {
                deflateEnd(&zs);
#ifdef DEBUG
                LOG_WARN("hh\n");
#endif // DEBUG
                // return 0; //发送失败
                // break;
                continue;
            }

            if ((i == block_count - 1) && rc != Z_STREAM_END) {
                deflateEnd(&zs);
#ifdef DEBUG
                LOG_WARN("gg\n");
#endif // DEBUG
                // return 0; //发送失败
                // break;
                continue;
            }
            // _iov[1].iov_base = (long *)_iov[1].iov_base + block_size;
            int compressed_size = write_size - zs.avail_out; //压缩后的字节数

            //发送
            std::stringstream ss;
            ss << std::hex << compressed_size << "\r\n";
            std::string line = ss.str();
            // len = send(_fd, line.data(), line.size(), MSG_WAITALL);
            len = _send_base(line.data(), line.size(), save_errno);
            if (len < 0) goto _close_end;

            // len = send(_fd, _piece->getBegin(), compressed_size, MSG_WAITALL);
            len = _send_base(_piece->getBegin(), compressed_size, save_errno);
            if (len < 0) goto _close_end;

            // len = send(_fd, "\r\n", 2, MSG_WAITALL);
            len = _send_base("\r\n", 2, save_errno);
            if (len < 0) goto _close_end;

        } while (zs.avail_out == 0);

        total_send_size += block_size;
#ifdef DEBUG
        printf(GREEN "fd[%d] [%d -> %d], [%d -> %d]" NONE "\n", _fd, total_send_size, file_size, i, block_count);
#endif // DEBUG

    }

    // len = send(_fd, "0\r\n\r\n", 5, MSG_WAITALL);
    len = _send_base("0\r\n\r\n", 5, save_errno);
    if (len < 0) goto _close_end;


    _iov[1].iov_base = nullptr;
    _iov[1].iov_len = 0;

_close_end:

    // *save_errno = errno;
    deflateEnd(&zs);
    // printf("len: %d\n", len);
    return len;
}


bool axy::HttpConn::process() {

    if (_http_buf.get_read_ptr()->get_readable_size() <= 0) return false;

    _request->init();
    if ( _request->parse(_http_buf.get_read_ptr()) ) {
        _response->init(getSrcDir(), *(_request.get()), 200); // code = 200, ok
    } else {
        _response->init(getSrcDir(), *(_request.get()), 400); // code = 400, error
    }

    _response->makeResponse(*(_http_buf.get_write_ptr()), PIECE_ZIP_COND_SIZE, true);

    return true;
}
