#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__


#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <assert.h>
#include <errno.h>
#include <mysql/mysql.h>  //mysql
#include <iostream>

#include "../buffer/buffer.h"
#include "../math/math.h"

namespace axy{






class HttpRequest {
private:

    //http 请求解析状态
    enum PARSE_STATE {
        REQUEST_LINE,   //请求行
        HEADERS,        //请求头
        BODY,           //请求体
        FINISH,         //解析成功
    };


    enum HTTP_CODE {
        NO_REQUEST = 0,         // 无请求
        GET_REQUEST,            // get请求
        BAD_REQUEST,            // 错误的请求
        NO_RESOURSE,            // 没有资源
        FORBIDDENT_REQUEST,     // 请求超时
        FILE_REQUEST,           // 文件请求
        INTERNAL_ERROR,         // 内部出错
        CLOSED_CONNECTION,      // 连接关闭
    };
    void _parsePath();
    bool _parseRequestLine(const std::string& line);
    void _parseHeader(const std::string& line);
    void _parseBody(std::string&& line);
    void _parsePost();
private:
    void _parseFromUrlencoded();
    bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);
    bool _is_header_key_exist_val(const char *key, const char *val);

public:

    void init();
    bool parse(axy::Buffer::ptr &buff);
    std::string& path();


    bool isKeepAlive() const {
        auto it = _header.find("Connection");
        if (it != _header.end()) {
            return (it->second == "keep-alive") && (_version == "1.1");
        }
        // it = _header.find("Proxy-Connection");
        // if (it != _header.end()) {
        //     return it->second == "keep-alive";
        // }
        return false;
    }
    bool isUpGradeInsecureRequests() const {
        return false;
        auto it = _header.find("Upgrade-Insecure-Requests");
        if (it != _header.end()) {
            return (it->second == "1") && (_version == "1.1");
        }
        return false;
    }
    bool is_gzip();





private:
    PARSE_STATE _state;         // 解析状态
    std::string _method, _path, _version, _body;            // 请求类型，请求路径，http版本，请求体
    std::string _url_parse;
    std::unordered_map<std::string, std::string> _header;   // 请求头
    std::unordered_map<std::string, std::string> _post;     // post 表单



private:
    int _range_start = -1;
    int _range_end = -1;
public:
    const int getRangeStart() { return _range_start; }
    const int getRangeEnd() { return _range_end; }



private:
    static const std::unordered_set<std::string> _DEFAULT_HTML;
    static const std::unordered_map<std::string, int> _DEFAULT_HTML_TAG;

    int ConverHex(char ch) {
        if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
        if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
        return ch;
    }
};


}

#endif // !__HTTP_REQUEST_H__
