#ifndef HTTP_RESPONSE_H 
#define HTTP_RESPONSE_H 
#include<string>
#include "../buffer/buffer.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <unordered_map>
using std::string;
using std::unordered_map;
class http_response {
public:
    http_response();
    ~http_response();
    void init(const string& src_dir,const string& path, bool is_keep_alive = false, int code = -1);
    void make_response(buffer& buffer);
    void unmap_file();
    void error_content(buffer& buffer, string message);
    inline size_t file_len() {
        return _mm_file_state.st_size;
    }
    inline char* file(){
        return _mm_file;
    }
private:
    void _add_state_line(buffer& buffer);
    void _add_header(buffer& buffer);
    void _add_content(buffer& buffer);
    void _error_content(buffer& buff, string&& message);
    int _code;
    bool _is_keep_alive;
    string _path;
    string _src_dir;
    char* _mm_file;
    struct stat _mm_file_state;
    static const unordered_map<string, string> _suffix_type;
    static const unordered_map<int, string> _code_status;
    static const unordered_map<int, string> _code_path;
};
#endif