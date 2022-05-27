#include "http_response.h"
#include "../log/log.h"
const unordered_map<string, string> http_response::_suffix_type = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};
const unordered_map<int, string> http_response::_code_status = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};
const unordered_map<int, string> http_response::_code_path = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};
http_response::http_response() {
    _code = -1;
    _path = _src_dir = "";
    _is_keep_alive = false;
    _mm_file = nullptr;
    _mm_file_state = { 0 };
}
http_response::~http_response() {
    unmap_file();
}
void http_response::init(const string& src_dir,const string& path, bool is_keep_alive, int code) {
    if (_mm_file) {
        unmap_file();
    }
    _code = code;
    _path = path;
    _is_keep_alive = is_keep_alive;
    _src_dir = src_dir;
    _mm_file = nullptr;
    _mm_file_state = { 0 };
}
void http_response::make_response(buffer& buff) {
    
    if (stat((_src_dir + _path).c_str(), &_mm_file_state) < 0 || S_ISDIR(_mm_file_state.st_mode)) {
        _code = 404;//没找到文件
    }
    else if (!(_mm_file_state.st_mode & S_IROTH)) {
        _code = 403;//没有访问权限
    }
    else if (_code == -1) {
        _code = 200;
    }
    if (_code_path.count(_code) == 1) {
        _path = _code_path.find(_code)->second;
        stat((_src_dir + _path).c_str(), &_mm_file_state);
    }
    _add_state_line(buff);
    _add_header(buff);
    _add_content(buff);
}
void http_response::_add_state_line(buffer& buff) {
    string status;
    if (_code_status.count(_code)) {
        status = _code_status.find(_code)->second;
    }
    else {
        _code = 400;
        status = _code_status.find(400)->second;
    }
    buff.add_data("HTTP/1.1 " + std::to_string(_code) + " " + status + "\r\n");
}
void http_response::_add_header(buffer& buff) {
    buff.add_data("Connection: ");
    if (_is_keep_alive) {
        buff.add_data("keep-alive\r\n");
        buff.add_data("keep-alive: max=6, timeout=120\r\n");
    }
    else {
        buff.add_data("close\r\n");
    }
    string filetype = "text/plain";
    string::size_type idx = _path.find_last_of('.');
    if (idx != string::npos) {
        string suffix = _path.substr(idx);
        if (_suffix_type.count(suffix) == 1) {
            filetype = _suffix_type.find(suffix)->second;
        }
    }
    buff.add_data("Content-type: " + filetype + "\r\n");
}
void http_response::_error_content(buffer& buff, string&& message) {
    string body("<html><title>Error</title>");
    body += "<body bgcolor=\"ffffff\">";
    string status;
    if (_code_status.count(_code) == 1) {
        status = _code_status.find(_code)->second;
    }
    else {
        status = "Bad Request";
    }
    body += std::to_string(_code) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";
    buff.add_data("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.add_data(body);
}
void http_response::_add_content(buffer& buff) {
    auto src_fd = open((_src_dir + _path).c_str(), O_RDONLY);
    if (src_fd < 0) {
        _error_content(buff, "File Not Found!");
        return;
    }
    LOG_DEBUG("mmap, file path: %s", (_src_dir + _path).data());
    int* mm_ret = (int*)mmap(0, _mm_file_state.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    if (mm_ret == nullptr) {
        _error_content(buff, "File Not Found!");
        return;
    }
    _mm_file = (char*)mm_ret;
    close(src_fd);
    buff.add_data("Content-length: " + std::to_string(_mm_file_state.st_size) + "\r\n\r\n");
}
void http_response::unmap_file() {
    if (_mm_file) {
        munmap(_mm_file, _mm_file_state.st_size);
        _mm_file = nullptr;
    }
}