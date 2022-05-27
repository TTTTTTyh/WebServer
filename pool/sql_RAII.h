#ifndef SQL_RAII_H
#define SQL_RAII_H
#include "sql_pool.h"
class sql_RAII {
public:
    sql_RAII(MYSQL** con_ptr, sql_pool* pool) {
        *con_ptr = pool->get_con();
        _con = con_ptr;
        _pool = pool;
    }
    ~sql_RAII() {
        if(_con)
            _pool->free_con(*_con);
    }
private:
    MYSQL** _con;
    sql_pool* _pool;
};
#endif