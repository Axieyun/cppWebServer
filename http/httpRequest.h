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


namespace axy{

class KMP {
private:
    KMP();
    static void getNext(std::vector<int> &next, const char *str, int n) {

        int len = n;               // 字符串长度
        for(int i = 1; i < len; i++) {
            int k = next[i - 1];             // k 表示需要比较的位置，初始值为 next[i - 1]
            while(k > 0 && str[i] != str[k]) // 比较，若不相等则继续分割，直到相等或为0(即不含相同部分)
                k = next[k - 1];
            if(str[i] == str[k])             // 若相等，则 next[i] = k + 1，否则为0，其中 k 为索引
                k++;
            next[i] = k;                     // 更新 next[i]
        }
    }
public:
    static const char *kmp(const char *P, int n1, const char *T, int n2) {
        //T为文本串，P模式串
        int np = n1;
        int nt = n2;
        //最长相同前后缀表
        std::vector<int> next(nt, 0);

        getNext(next, T, nt);
        int j = 0;
        for (int i = 0; i < np; ++i) {
            while (j > 0 && P[i] != T[j]) {
                j = next[j - 1];
            }
            if (T[j] == P[i]) {
                ++j;
            }
            //当j等于自身长度说明匹配完成
            if (j == nt) {
                //此时i是匹配成功的最后一个位置，因此起始位置为 i -j + 1的位置
                return P + (i - j + 1);
            }

        }
        return P + n1;
    }
};




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
        if (_header.find("Connection") != _header.end()) {
            return (_header.find("Connection")->second == "keep-alive") && (_version == "1.1");
        }
        return false;
    }
    bool isUpGradeInsecureRequests() const {
        if (_header.find("Upgrade-Insecure-Requests") != _header.end()) {
            return false;
            // return (_header.find("Connection")->second == '1') && (_version == "1.1");
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
