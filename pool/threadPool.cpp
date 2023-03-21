#include "threadPool.h"




#include "../log.h"


uint32_t axy::get_thread_id() { return (uint32_t)syscall(SYS_gettid); }





//运行函数
void axy::Task::runTask() {
    _func();
}



axy::ThreadPool::ThreadPool(uint16_t thread_size) : _pool(std::make_shared<Pool>()) {
    _m_cpu_size = std::thread::hardware_concurrency();
    if (thread_size < (_m_cpu_size << 1 | 1)) thread_size = (_m_cpu_size << 1) + 2;
    if (!thread_size) thread_size = 1;

    for (uint16_t i = 0; i < thread_size; ++i) {
        std::thread(&ThreadPool::_doWork, this).detach();
    }

}


axy::ThreadPool::~ThreadPool() {
    std::lock_guard<std::mutex> guard(_pool->_mtx);
    _pool->_is_closed = true;
    _pool->_cond.notify_all();
}


void axy::ThreadPool::_doWork() {

#ifdef DEBUG1
    LOG1("run thread id : %d\n", get_thread_id());
#endif //!DEBUG1

    while (!_pool->_is_closed) {
        std::unique_lock<std::mutex> lock(_pool->_mtx);

#ifdef DEBUG1
        LOG1("get task thread id : %d\n", get_thread_id());
#endif //!DEBUG1
            
        if (_pool->_tasks.empty()) _pool->_cond.wait(lock); //释放锁，挂起等待条件变量
        else {
            Task::ptr task = _pool->_tasks.front();
            _pool->_tasks.pop();
            lock.unlock();

            task->runTask(); //执行任务
        }
    }

#ifdef DEBUG1
    LOG1("quit thread id : %d\n", get_thread_id());
#endif //!DEBUG1
}

