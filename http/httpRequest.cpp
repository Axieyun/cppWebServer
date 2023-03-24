#include "httpRequest.h"



const std::unordered_set<std::string> axy::HttpRequest::_DEFAULT_HTML{
            "/index", "/register", "/login",
            "/welcome", "/video", "/picture"
};
const std::unordered_map<std::string, int> axy::HttpRequest::_DEFAULT_HTML_TAG {
            {"/register", 0}, {"/login", 1}
};





void axy::HttpRequest::init() {
    _method = _path = _version = _body = "";
    _state = REQUEST_LINE;
    _header.clear();
    _post.clear();
    _range_end = _range_start = -1;
}


bool axy::HttpRequest::parse(axy::Buffer::ptr &buff) {

    int readable_size = buff->get_readable_size();
    if (readable_size <= 0) { return 0; }

    static const char CRLF[] = "\r\n";
    bool is_parse_body = 0; //记录解析body返回的状态


    while (readable_size > 0 && _state != FINISH) {
        //Content-Length
        const char *line_end = KMP::kmp(buff->get_read_begin(), readable_size, CRLF, 2);
        std::string line(buff->get_read_begin(), line_end);

        readable_size -= line.size();
        int has_read_l = line.size() + 2;

        switch(_state) {
            case REQUEST_LINE : {
                if (!_parseRequestLine(line)) {
                    return 0;
                }

                break;
            }
            case HEADERS : {
                _parseHeader(line);
                if (readable_size <= 2) {
                    _state = FINISH;
                    has_read_l -= 2;
                }
                break;
            }
            case BODY : {
                is_parse_body = 1;
                _parseBody(std::move(line));
                break;
            }
            default : break;
        }
        //只处理post的body
        if (_state == BODY && _method != "POST") {
            buff->clear();
            break;
        }

        // buff->hasRead(_state == BODY ? has_read_l : has_read_l);
        buff->hasRead(has_read_l);
    }

    // for (const auto &h : _header) {
    //     std::cout << h.first << ": " << h.second << "\n";
    // }


    if (!is_parse_body) _parsePath(); //没有解析body
#ifdef DEBUG
    LOG1("http request parse ok, [%s], [%s], [%s]\n", _method.c_str(), _path.c_str(), _version.c_str());
#endif // DEBUG
    return true; //解析成功
}

void axy::HttpRequest::_parsePath() {
    std::string::size_type idx = _path.find_last_of('?');
    if (idx != std::string::npos) {
        _url_parse = _path.substr(idx + 1);
        _path = _path.substr(0, idx);
        // printf(GREEN "path: %s, url parse: %s" NONE "\n", _path.c_str(), _url_parse.c_str());
    }

    if (_path == "/") {
        _path = "/login.html";
    }
    // else if (_path == "/blog") {
    //     _path = "/blog_list.html";
    // }
    else {
        for(auto &item: _DEFAULT_HTML) {
            if(item == _path) {
                _path += ".html";
                break;
            }
        }
    }
}

bool axy::HttpRequest::_parseRequestLine(const std::string& line) {
    static std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, patten)) {
        _method = subMatch[1];
        _path = subMatch[2];
        _version = subMatch[3];
        // _state = (_method == "GET") ? FINISH : HEADERS;
        _state = HEADERS;
        return true;
    }
#ifdef DEBUG
    LOG_ERROR("RequestLine Error");
#endif // DEBUG
    return false;
}


void axy::HttpRequest::_parseHeader(const std::string& line) {
    static std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, patten)) {
        _header[subMatch[1]] = subMatch[2];
    }
    else {
/*
请求文件的第一个字节：Range: bytes=0-0
请求文件的前 100 个字节：Range: bytes=0-99
请求文件的第二个字节到第五个字节：Range: bytes=1-4
请求文件的最后 500 个字节：Range: bytes=-500
请求文件的第 500 个字节到最后一个字节：Range: bytes=500-
*/
        auto it = _header.find("Range");
        if (it != _header.end()) {
            std::string::size_type idx1 = it->second.find_first_of('=');
            std::string::size_type idx2 = it->second.find_last_of('-');
            if (idx1 + 1 < idx2) _range_start = atoi(it->second.substr(idx1 + 1, idx2 - idx1 - 1).c_str());
            if (idx2 + 1 < it->second.size()) _range_end = atoi(it->second.substr(idx2 + 1).c_str());
            // printf(RED "Range: bytes=%d-%d" NONE "\n", _range_start, _range_end);
        }
        _state = BODY;
    }
}


void axy::HttpRequest::_parseBody(std::string&& line) {

    _body = std::move(std::forward<std::string>(line));
    _state = FINISH;
#ifdef DEBUG
    LOG1("Body:%s, len:%ld\n", _body.c_str(), _body.size());
#endif // DEBUG

    _parsePost();; //解析 body
}


void axy::HttpRequest::_parsePost() {
    //只处理post的body
    if (/*_method == "POST" && */_header["Content-Type"] == "application/x-www-form-urlencoded") {
        _parseFromUrlencoded();
        auto it = _DEFAULT_HTML_TAG.find(_path);
        if (it != _DEFAULT_HTML_TAG.end()) {
            int tag = it->second; //1:login, 0:注册
            _path += (UserVerify(_post["username"], _post["password"], (tag == 1))) ? "_ok.json" : "_fail.json";
        }
    }

}

void axy::HttpRequest::_parseFromUrlencoded() {
    if(_body.size() == 0) { return; }

    std::string key, value;
    int num = 0;
    int n = _body.size();
    int i = 0, j = 0;
    //username=admin&password=123456
    for(; i < n; i++) {
        char ch = _body[i];

        switch (ch) {
        case '=':
            key = _body.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            _body[i] = ' ';
            break;
        case '%':
            num = ConverHex(_body[i + 1]) * 16 + ConverHex(_body[i + 2]);
            _body[i + 2] = num % 10 + '0';
            _body[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = _body.substr(j, i - j);
            j = i + 1;
            _post[key] = value;
#ifdef DEBUG
            LOG1("%s = %s\n", key.c_str(), value.c_str());
#endif // DEBUG
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if(_post.count(key) == 0 && j < i) {
        value = _body.substr(j, i - j);
        _post[key] = value;
    }
}

bool axy::HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool isLogin) {
    return true;

}



std::string& axy::HttpRequest::path() { return _path; }



bool axy::HttpRequest::_is_header_key_exist_val(const char *key, const char *val) {
    auto it = _header.find(key);
    if (it != _header.end()) {
        return (it->second.find(val) != std::string::npos);
    }
    return false;
}

bool axy::HttpRequest::is_gzip() {
    // auto it = _header.find("Accept-Encoding");
    // if (it != _header.end()) {
    //     std::string val = it->second;
    //     return val.find("gzip") != std::string::npos;
    // }
    return _is_header_key_exist_val("Accept-Encoding", "gzip");
}
