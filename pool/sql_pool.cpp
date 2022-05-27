#include "sql_pool.h"
using std::lock_guard;
using std::mutex;
sql_pool* sql_pool::get_instance() {
    static sql_pool sql_pool;
    return &sql_pool;
}
void sql_pool::init(const char* host, int port,
    const char* user, const char* pwd,
    const char* dbName, int con_size = 10) {
    mysql_library_init(0,NULL,NULL);
    assert(con_size > 0);
    for (int i = 0;i < con_size;++i) {
        MYSQL* con = mysql_init(nullptr);
        if (!con) {
            LOG_ERROR("Mysql init error!");
        }
        con = mysql_real_connect(con, host, user, pwd, dbName, port, NULL, 0);
        if (!con) {
            LOG_ERROR("Mysql connect error!");
        }
        _con_queue.push(con);
    }
    _max_con = con_size;
    sem_init(&_sem_id, 0, _max_con);//0表示用在线程间共享，非0表示用在进程间共享
}
MYSQL* sql_pool::get_con() {
    MYSQL* con = nullptr;
    if (_con_queue.empty()) {
        LOG_WARN("sql_con_pool busy!");
        return nullptr;
    }
    sem_wait(&_sem_id);
    {
        lock_guard<mutex> lock(_mtx);
        con = _con_queue.front();
        _con_queue.pop();
    }
    return con;
}
void sql_pool::free_con(MYSQL* con) {
    if (!con) return;
    lock_guard<mutex> lock(_mtx);
    _con_queue.emplace(con);
    sem_post(&_sem_id);
}
void sql_pool::close_pool() {
    lock_guard<mutex> locker(_mtx);
    while (!_con_queue.empty()) {
        auto con = _con_queue.front();
        _con_queue.pop();
        mysql_close(con);
    }
    mysql_library_end();
}
int sql_pool::get_free_con_conut() {
    lock_guard<mutex> lock(_mtx);
    return _con_queue.size();
}
sql_pool::sql_pool() {
    _free_con = 0;_cur_con = 0;
}
sql_pool::~sql_pool() {
    close_pool();
}