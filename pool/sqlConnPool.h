#ifndef __SQL_CONN_POOL__
#define __SQL_CONN_POOL__



#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <assert.h>
#include "../log.h"



namespace axy {


// 线程安全的饿汉模式，空间换时间，类加载的时候就创建对象
class SqlConnPool {

public:
    static SqlConnPool *instance() {
        return &_sql_conn_pool;
    }
private:
    static SqlConnPool _sql_conn_pool;


public:
    void init(const char *host, short port
        , const char *user, const char *pwd
        , const char *db_name, const int cpu_size = 1);
    
    MYSQL *getConn() {
        if (_conns.empty()) {
            return nullptr;
        }
        sem_wait(&_sem); //阻塞当前线程直到信号量sem的值大于0，解除阻塞后将sem的值减一
    
        std::lock_guard<std::mutex> guard(_mtx);
        MYSQL *mq = _conns.front();
        _conns.pop();
        return mq;
    }

    void freeConn(MYSQL *mq) {
        assert(mq);
        std::lock_guard<std::mutex> guard(_mtx);
        _conns.emplace(mq);
        
        sem_post(&_sem); //信号量++
    }


private:

    SqlConnPool() {}
    ~SqlConnPool();
    void _clear();

    int _max_conn;
    int _user_count;

    std::queue<MYSQL *> _conns;
    std::mutex _mtx;
    sem_t _sem;
};




}


#endif // !__SQL_CONN_POOL__
