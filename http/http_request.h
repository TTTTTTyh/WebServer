#ifndef HTTP_REQUEST_H 
#define HTTP_REQUEST_H 
#include <string>
#include <regex>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include "../buffer/buffer.h"
#include "../pool/sql_RAII.h"
#include"../log/log.h"
using std::string;
using std::regex;
using std::smatch;
using std::unordered_map;
using std::unordered_set;
class http_request
{
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };
    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    http_request() { init(); }
    ~http_request() = default;
    void init();
    bool parse(buffer& buff);
    inline bool is_keep_alive() const {
        if (_header.count("Connection")) {
            return _header.find("Connection")->second == "keep-alive" && _version == "1.1";
        }
        return false;
    }
    inline string& path(){
        return _path;
    }
private:
    PARSE_STATE _state;
    string _method, _path, _version, _body;
    static unordered_set<string> _default_html;
    static unordered_map<string, int> _default_html_tag;
    unordered_map<string, string> _header;
    unordered_map<string, string> _post;
    static int _convert_special_flag(char c);
    void _parse_post();
    bool _parse_request_line(const string& line);
    void _parse_header(const string& line);
    void _parse_body(string& line);
    unsigned int _16sys210sys(char c1);
    bool _user_verify(const string& name, const string& pwd, bool is_login);
};
#endif