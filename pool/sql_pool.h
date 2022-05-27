#ifndef SQL_POOL_H
#define SQL_POOL_H
#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <thread>
#include <semaphore.h>
#include <assert.h>
#include "../log/log.h"
class sql_pool
{
private:
    sql_pool();
    ~sql_pool();
    int _cur_con;
    int _free_con;
    std::queue<MYSQL*> _con_queue;
    std::mutex _mtx;
    int _max_con;
    sem_t _sem_id;

public:
    static sql_pool* get_instance();
    MYSQL* get_con();
    void free_con(MYSQL* con);
    int get_free_con_conut();
    void close_pool();
    void init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
};

#endif