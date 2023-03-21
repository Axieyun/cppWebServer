#include "sqlConnPool.h"

axy::SqlConnPool axy::SqlConnPool::_sql_conn_pool;

void axy::SqlConnPool::init(const char *host, short port
    , const char *user, const char *pwd
    , const char *db_name, const int cpu_size) {

    int conn_size = cpu_size + 1;
    for (int i = 0; i < conn_size; ++i) {
        MYSQL *mq = nullptr;
        mq = mysql_init(mq);
        if (!mq) {
#ifdef DEBUG
            LOG_ERROR("mysql init return nullptr.\n");
#endif // DEBUG
        }

        mq = mysql_real_connect(mq, host, user, pwd, db_name, port, nullptr, 0);
        if (!mq) {
#ifdef DEBUG
            LOG_ERROR("mysql create return nullptr.\n");
#endif // DEBUG
        }

        _conns.emplace(mq);
    }
    _max_conn = conn_size;
    sem_init(&_sem, 0, conn_size);
}











axy::SqlConnPool::~SqlConnPool() {
    _clear();
}


//连接池销毁
void axy::SqlConnPool::_clear() {
    std::lock_guard<std::mutex> guard(_mtx);
    
    while (!_conns.empty()) {
        MYSQL *item = _conns.front();
        _conns.pop();
        mysql_close(item);
    }

    mysql_library_end();

    return ;
}