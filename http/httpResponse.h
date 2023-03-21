#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__



#include <unordered_map>
#include <unordered_set>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap
#include <assert.h>

#include <iostream>

#include <time.h>

/*
Zlib是一个跨平台的压缩函数库，提供了一套 in-memory 压缩和解压函数，
并能检测解压出来的数据的完整性(integrity)。
关于zlib的更多信息请访问 http://www.zlib.net
*/


#include "../buffer/buffer.h"
#include "./httpRequest.h"
namespace axy {


#define K1 (1 << 10)
#define K4 (1 << 12)


// const static int RANGE_SIZE = 16 * (1 << 10);
// const static int RANGE_SIZE = 18644;
const static int RANGE_SIZE = 8 * K4;


class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void init(const char *srcDir, axy::HttpRequest &request, int code = -1);
    void makeResponse(Buffer& buff, int piece_size, bool is_mmap = true);
    void unmapFile();

    char* file() { return _mmFile; }
    const int getSrcFd() const { return _src_fd; }
    size_t fileLen() const { return _mmFileStat.st_size; }

    void errorContent(Buffer& buff, std::string message);
    int code() const { return _code; }
    const bool is_gzip() const { return _is_gzip; }
    const bool is_piecemeal() { return _is_piecemeal; }


public:
/*
请求文件的第一个字节：Range: bytes=0-0
请求文件的前 100 个字节：Range: bytes=0-99
请求文件的第二个字节到第五个字节：Range: bytes=1-4
请求文件的最后 500 个字节：Range: bytes=-500
请求文件的第 500 个字节到最后一个字节：Range: bytes=500-
*/
    void addRange(Buffer &buff) {
        char s[64] = "Content-Range: bytes ";
        if (_range_start != -1 && _range_end != -1) {

            _range_send_begin = _mmFile + _range_start;
            _range_send_size = _range_end - _range_start + 1;

            _range_send_size = RANGE_SIZE > _range_send_size ? _range_send_size : RANGE_SIZE;
            sprintf(s + 21, "%d-%d/%ld\r\n", _range_start, _range_end, _mmFileStat.st_size);
        } else if (_range_start != -1) {

            _range_send_begin = _mmFile + _range_start;
            _range_send_size = _mmFileStat.st_size - _range_start;

            _range_send_size = RANGE_SIZE > _range_send_size ? _range_send_size : RANGE_SIZE;
            // sprintf(s + 21, "%d-%ld/%ld\r\n", _range_start, _mmFileStat.st_size - 1, _mmFileStat.st_size);
            sprintf(s + 21, "%d-%d/%ld\r\n", _range_start, _range_send_size + _range_start - 1, _mmFileStat.st_size);
        } else if (_range_end != -1) {

            _range_send_begin = _mmFile + (_mmFileStat.st_size - _range_end);
            _range_send_size = _range_end;

            _range_send_size = RANGE_SIZE > _range_send_size ? _range_send_size : RANGE_SIZE;
            // sprintf(s + 21, "%ld-%ld/%ld\r\n", _mmFileStat.st_size - _range_end, _mmFileStat.st_size - 1, _mmFileStat.st_size);
            sprintf(s + 21, "%ld-%ld/%ld\r\n", _mmFileStat.st_size - _range_send_size, _mmFileStat.st_size - 1, _mmFileStat.st_size);
        }
        buff.append(s, strlen(s));
    }
    void addCRLF(Buffer &buff) { buff.append("\r\n"); }

    void addContentLength(Buffer &buff, int size) {
        char s[32] = "Content-length: ";
        sprintf(s + 16, "%d\r\n", size);
        buff.append(s, strlen(s));
    }

public:
    void _addStateLine(Buffer &buff);
    void _addHeader(Buffer &buff);
    void _addAcceptRanges(Buffer &buff) { buff.append("Accept-Ranges: bytes\r\n"); }
    void _addDate(Buffer &buff);

    void _addContent(Buffer &buff, bool is_mmap = true);
    void _addServer(Buffer& buff);
    void _addAccessControlAllowOrigin(Buffer& buff);

    void addGzip(Buffer& buff) { buff.append("Content-Encoding: gzip\r\n"); }
    void addTransferEncoding(Buffer& buff) { buff.append("Transfer-Encoding: chunked\r\n"); }
    void addVary(Buffer& buff) { buff.append("Vary: Accept-Encoding\r\n"); }

    void _errorHtml();
    std::string _getFileType();


    int _code;
    bool _isKeepAlive;
    bool _is_up_https;

    std::string _path;
    std::string _srcDir;
    std::string _file_suffix; //文件后缀

private:
    int _range_start = -1;
    int _range_end = -1;
    char *_range_send_begin;
    int _range_send_size;
public:
    const char *getRangeSendBegin() { return _range_send_begin; }
    const int getRangeSendSize() { return _range_send_size; }
    const bool is_range() { return _range_end != -1 || _range_start != -1; }
    const int getRangeStart() { return _range_start; }
    const int getRangeEnd() { return _range_end; }

private:
    bool _is_gzip; //是否压缩
    bool _is_piecemeal; //是否分块


    char *_mmFile;
    int _src_fd;
    struct stat _mmFileStat; //


    static const std::unordered_set<std::string> _COMPRESSION_YTPE;          // 推荐压缩的类型
    static const std::unordered_map<std::string, std::string> _SUFFIX_TYPE; // 后缀类型
    static const std::unordered_map<int, std::string> _CODE_STATUS;

    static const std::unordered_map<int, std::string> _CODE_PATH;
};


















} // ! namespace axy




#endif // !__HTTP_RESPONSE_H__
