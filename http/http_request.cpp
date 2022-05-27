#include "http_request.h"
unordered_set<string> http_request::_default_html{
    "/index", "/register", "/login",
    "/welcome", "/video", "/picture",
};
unordered_map<string, int> http_request::_default_html_tag{
            {"/register.html", 0}, {"/login.html", 1}, };
void http_request::init() {
    _method = _path = _version = _body = "";
    _state = REQUEST_LINE;
    _header.clear();
    _post.clear();
}
bool http_request::parse(buffer& buff) {
    const char crlf[] = "\r\n";
    const char* line_end;
    while (_state != FINISH && buff.readable_bytes()) {
        line_end = std::search(buff.read_start_pos(), buff.write_start_pos(), crlf, crlf + 2);
        string now_line(buff.read_start_pos(), line_end);
        switch (_state) {
        case REQUEST_LINE:
            if (!_parse_request_line(now_line)) {
                return false;
            }
            if (_path == "/") {
                _path = "/index.html";
            }
            else {
                for (auto& item : _default_html) {
                    if (item == _path) {
                        _path += ".html";
                        break;
                    }
                }
            }
            break;
        case HEADERS:
            _parse_header(now_line);
            if (buff.readable_bytes() <= 2) {
                _state = FINISH;
            }
            break;
        case BODY:
            _parse_body(now_line);
            break;
        default:
            break;
        }
        if (line_end == buff.write_start_pos()) break;
        buff.read_to(line_end+2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", _method.c_str(), _path.c_str(), _version.c_str());
    return true;
}
bool http_request::_parse_request_line(const string& line) {
    const static regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch sm;
    if (std::regex_match(line, sm, patten)) {
        _method = sm.str(1);
        _path = sm.str(2);
        _version = sm.str(3);
        _state = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}
void http_request::_parse_header(const string& line) {
    const static regex patten("^([^:]*): ?(.*)$");
    smatch sm;
    if (std::regex_match(line, sm, patten)) {
        _header.emplace(sm.str(1), sm.str(2));
    }
    else {
        _state = BODY;
    }
}
unsigned int http_request::_16sys210sys(char c) {
    if (c >= 'a' && c <= 'f') return c - 'a';
    else if (c >= 'A' && c <= 'F') return c - 'A';
    else return c - '0';
}
void http_request::_parse_body(string& line) {
    if (_method != "POST" /*|| _header.find("Content-Type")->second == "application/x-www-form-urlencoded"*/)
        return;
    int last = 0;string key;string value;std::vector<int> record;
    int  n = line.size();
    for (int i = 0;i < n;++i) {
        char c = line[i];
        switch (c) {
        case '=':
            key = line.substr(last, i - last);
            last = i + 1;
            break;
        case '&':
            if (record.size() > 0) {
                value = "";
                int llast = last;
                for (auto ii : record) {
                    value += line.substr(llast, ii - llast) + static_cast<char>(_16sys210sys(line[ii + 1]) * 16 + _16sys210sys(line[ii + 2]));
                    llast = ii + 3;
                }
                record.clear();
            }
            else {
                value = line.substr(last, i - last);
            }
            last = i + 1;
            _post.emplace(std::move(key), std::move(value));
            break;
        case '+':
            line[i] = ' ';
            break;
        case '%':
            record.emplace_back(i);
            i += 2;
            break;
        default:
            break;
        }
    }
    if (last <= n) {
        if (record.size() > 0) {
            value = "";
            int llast = last;
            for (auto ii : record) {
                value += line.substr(llast, ii - llast) + static_cast<char>(_16sys210sys(line[ii + 1]) * 16 + _16sys210sys(line[ii + 2]));
                llast = ii + 3;
            }
        }
        else {
            value = line.substr(last, n - last);
        }
        _post.emplace(std::move(key), std::move(value));
    }
    if (_default_html_tag.count(_path)) {
        int tag = _default_html_tag.find(_path)->second;
        LOG_DEBUG("Tag:%d", tag);
        if (tag == 0 || tag == 1) {
            bool isLogin = (tag == 1);
            if (_user_verify(_post["username"], _post["password"], isLogin)) {
                _path = "/welcome.html";
            }
            else {
                _path = "/error.html";
            }
        }
    }
    _state = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}
void http_request::_parse_post() {
    if (_method == "POST" /*&& _header["Content-Type"] == "application/x-www-form-urlencoded"*/) {
        _parse_body(_body);
        if (_default_html_tag.count(_path)) {
            int tag = _default_html_tag.find(_path)->second;
            if (tag == 1 || tag == 0) {
                bool is_login = (tag == 1);
                if (_user_verify(_post["username"], _post["password"], is_login)) {
                    _path = "/welcome.html";
                }
                else {
                    _path = "/error.html";
                }
            }

        }
    }
}
bool http_request::_user_verify(const string& name, const string& pwd, bool is_login) {
    if (name == "" || pwd == "") return false;
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    sql_RAII(&sql, sql_pool::get_instance());
    MYSQL_RES* res = nullptr;
    assert(sql);
    bool return_res;
    char sql_statement[256];
    snprintf(sql_statement, 256, "SELECT username, password FROM users where username='%s'", name.c_str());
    LOG_DEBUG("%s", sql_statement);
    if (mysql_query(sql, sql_statement)) {
        return_res = false;
        goto tag;
    }
    res = mysql_store_result(sql);
    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW:%s %s", row[0], row[1]);
        if (strcmp(row[0], name.c_str()) == 0 && !is_login) {
            LOG_DEBUG("%s already registered");
            return_res = false;
            goto tag;
        }
        string true_psw(row[1]);
        if (true_psw == pwd) {
            return_res = true;
            LOG_DEBUG("success!");
        }
        else {
            return_res = false;
            LOG_DEBUG("error pwd!");
        }
    }
    if (!is_login) {
        sql_statement[256];
        snprintf(sql_statement, 256, "INSERT INTO users(username,password) VALUES ('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", sql_statement);
        if (mysql_query(sql, sql_statement)) {
            return_res = false;
            LOG_DEBUG("insert error!");
        }
        else {
            return_res = true;
            LOG_DEBUG("success!");
        }
    }
tag:
    sql_pool::get_instance()->free_con(sql);
    mysql_free_result(res);
    return return_res;
}