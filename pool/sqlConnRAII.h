#ifndef __SQL_CONN_RAII__
#define __SQL_CONN_RAII__

#include "sqlConnPool.h"
#include <assert.h>

namespace axy{

/* 资源在对象构造初始化 资源在对象析构时释放*/
class SqlConnRAII {
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool *connpool) {
        assert(connpool);
        *sql = connpool->getConn();
        sql_ = *sql;
        connpool_ = connpool;
    }
    
    ~SqlConnRAII() {
        if(sql_) { connpool_->freeConn(sql_); }
    }
    
private:
    MYSQL *sql_;
    SqlConnPool* connpool_;
};

}

#endif // !__SQL_CONN_RAII__
