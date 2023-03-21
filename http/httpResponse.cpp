#include "httpResponse.h"

const std::unordered_set<std::string> axy::HttpResponse::_COMPRESSION_YTPE = {
    ".html", ".xml", ".js", ".css", ".txt",
};

const std::unordered_map<std::string, std::string> axy::HttpResponse::_SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".js",    "text/javascript "},
    { ".css",   "text/css "},
    { ".txt",   "text/plain" },
    { ".php",   "text/html" },

    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },

    { ".au",    "audio/basic" },

    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".mp4",   "video/mp4" },
    { ".avi",   "video/x-msvideo" },

    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".xhtml", "application/xhtml+xml" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".txt",   "application/octet-stream" },
    { ".word",  "application/nsword" },

};

const std::unordered_map<int, std::string> axy::HttpResponse::_CODE_STATUS = {
    { 200, "OK" },
    { 206, "Partial Content" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string> axy::HttpResponse::_CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

axy::HttpResponse::HttpResponse() {
    _code = -1;
    _path = _srcDir = "";
    _file_suffix = "";
    _isKeepAlive = false;
    _is_up_https = false;
    _mmFile = nullptr;
    _is_gzip = true;
    _src_fd = -1;
    _mmFileStat = { 0 };
    _is_piecemeal = false;
};

axy::HttpResponse::~HttpResponse() {
    unmapFile();
}

void axy::HttpResponse::init(const char *srcDir, axy::HttpRequest &request, int code) {

    if (_mmFile) {
        unmapFile();
    }

    _code = code;
    _is_gzip = request.is_gzip(); //获取客户是否支持gzip压缩
    _range_start = request.getRangeStart();
    _range_end = request.getRangeEnd();
    if (is_range()) _code = 206;
    // printf(RED "Range: bytes=%d-%d" NONE "\n", _range_start, _range_end);
    _isKeepAlive = request.isKeepAlive();
    _is_piecemeal = false;
    // printf(GREEN "client %s" NONE "\n", _is_gzip ? "支持压缩" : "不支持压缩");
    _is_up_https = request.isUpGradeInsecureRequests();
    _path = request.path();
    _srcDir = srcDir;
    _mmFile = nullptr;
    _mmFileStat = { 0 };


}

/*
 @piece_size：块大小，如果文件大于，这分块传输
 @is_mmap：是否对该文件进行mmap
*/
void axy::HttpResponse::makeResponse(Buffer& buff, int piece_size, bool is_mmap) {
    /* 判断请求的资源文件 */
    // 获取文件信息
    int rt = stat((_srcDir + _path).data(), &_mmFileStat);
    if(rt < 0 || S_ISDIR(_mmFileStat.st_mode)) {
        _code = 404;
    }
    else if(!(_mmFileStat.st_mode & S_IROTH)) {
        _code = 403;
    }
    else if(_code == -1) {
        _code = 200;
    }
    _errorHtml();
    // printf(RED "%s" NONE "\n", (_srcDir + _path).data());
    //常规头添加
    _addHeader(buff);

    if (_is_gzip) { //必须放在 _addHeader后面，因为_is_gzip 在 _getFileType里面赋值
        _is_piecemeal = true; //压缩发送必定分块
    }
    /*
    1、直接发送，不压缩、不分块
    2、不压缩、分块发送
    3、压缩、分块发送
    */
    _is_piecemeal = (_mmFileStat.st_size > piece_size) || _is_piecemeal;

    _addContent(buff, is_mmap);

}


void axy::HttpResponse::_addDate(Buffer& buff) {
    static time_t timep;
    static std::string s;
    s = "Date: ";
    time (&timep);
    static char tmp[80];
    memset(tmp, 0, sizeof(tmp));
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&timep) );
    s.append(tmp);
    s.append("\r\n");
    buff.append(s);
}

void axy::HttpResponse::_addAccessControlAllowOrigin(Buffer& buff) {
    buff.append("access-control-allow-origin: *\r\n");
}

void axy::HttpResponse::_addServer(Buffer& buff) {
    buff.append("server: Axieyun\r\n");
    return ;
}


void axy::HttpResponse::_errorHtml() {
    if (_CODE_PATH.count(_code) == 1) {
        _path = _CODE_PATH.find(_code)->second;
        stat((_srcDir + _path).data(), &_mmFileStat);
    }
}

void axy::HttpResponse::_addStateLine(Buffer& buff) {
    std::string status;
    if (_CODE_STATUS.count(_code) == 1) {

        status = _CODE_STATUS.find(_code)->second;
    }
    else {
        _code = 400;
        status = _CODE_STATUS.find(400)->second;
    }
    buff.append("HTTP/1.1 " + std::to_string(_code) + " " + status + "\r\n");
}

void axy::HttpResponse::_addHeader(Buffer& buff) {
    _addStateLine(buff);
    _addServer(buff);
    _addAcceptRanges(buff); //支持范围请求
    _addDate(buff);
    _addAccessControlAllowOrigin(buff); //允许跨域请求
    buff.append("Connection: ");
    if(_isKeepAlive) {
        buff.append("keep-alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
        //# timeout：估计了服务器希望将连接保持在活跃状态的（空闲）时间。不是一个承诺值
        //# max：估计了服务器还希望为多少个事务保持此连接的活跃状态。不是一个承诺值
    } else{
        buff.append("close\r\n");
    }

    if (_is_up_https) buff.append("Content-Security-Policy: upgrade-insecure-requests\r\n");

    buff.append("Content-type: " + _getFileType() + "\r\n");
}

/**
 * @brief 无
 *
 * @param buff：响应buff，负责响应头的
 * @param is_mmap ：是否启用共享文件传输数据
 * @param is_piecemeal ：是否分块
 */
void axy::HttpResponse::_addContent(Buffer& buff, bool is_mmap) {

    _src_fd = open((_srcDir + _path).data(), O_RDONLY);
    if(_src_fd < 0) {
        errorContent(buff, "File NotFound!");
        return;
    }

    if (!is_mmap) return ;
    /* 将文件映射到内存提高文件的访问速度
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/

    int *mmRet = (int*)mmap(0, _mmFileStat.st_size, PROT_READ, MAP_PRIVATE, _src_fd, 0);
    if (*mmRet == -1) {
        errorContent(buff, "File NotFound!");
        return;
    }

    _mmFile = (char*)mmRet;

    close(_src_fd);

}
/**
 * @brief 关闭共享内存的句柄
 *
 */
void axy::HttpResponse::unmapFile() {
    if (_mmFile) {
        munmap(_mmFile, _mmFileStat.st_size);
        _mmFile = nullptr;
    }
}

std::string axy::HttpResponse::_getFileType() {
    /* 判断文件类型 */
    std::string::size_type idx = _path.find_last_of('.');
    if(idx == std::string::npos) {
        return "text/plain";
    }

    std::string suffix = _path.substr(idx);
    _is_gzip = _is_gzip && _COMPRESSION_YTPE.count(suffix); //如果客户支持压缩 且 该文件推荐压缩，那就支持压缩
    // printf(GREEN "%d, %s" NONE "\n", _is_gzip, suffix.c_str());

    auto it = _SUFFIX_TYPE.find(suffix);
    if(it != _SUFFIX_TYPE.end()) {
        return it->second;
    }

    return "text/plain";
}

void axy::HttpResponse::errorContent(Buffer& buff, std::string message) {
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(_CODE_STATUS.count(_code) == 1) {
        status = _CODE_STATUS.find(_code)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(_code) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>AxieyunWebServer</em></body></html>";

    buff.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}
