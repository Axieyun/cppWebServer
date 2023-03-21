#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <mutex>    
#include <condition_variable> //条件变量
#include <functional>
#include <thread>
#include <queue>
#include <vector>
#include <memory>   //只能指针
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>






// #include "../log.h"




namespace axy {



uint32_t get_thread_id();


//任务类，可以传递任意函数、参数
struct Task {

public:
    typedef std::shared_ptr<Task> ptr;


    template<typename FUNC, typename ...ARGS>
    Task(FUNC&& func, ARGS&& ...args) {//函数闭包
        _func = std::bind(func, std::forward<ARGS>(args)...);
    }

    //运行函数
    void runTask();

private:
    std::function<void()> _func;
};




class ThreadPool {


public:
    typedef std::unique_ptr<ThreadPool> ptr;
    // std::thread::hardware_concurrency() 获取并行线程数
    // io密集型
    ThreadPool(uint16_t thread_size = (std::thread::hardware_concurrency() << 1));

    ~ThreadPool();

public:

    void _doWork();
    int get_cpu_size() { return _m_cpu_size; }

    template<class FUNC, class ...ARGS>
    void add_task(FUNC&& func, ARGS&& ...args) {
        std::lock_guard<std::mutex> lock(_pool->_mtx);
        _pool->_tasks.emplace(std::make_shared<Task>(func, std::forward<ARGS>(args)...));
        _pool->_cond.notify_one();
    }


private:
    struct Pool {
        std::mutex _mtx;
        std::condition_variable _cond;
        bool _is_closed = false;
        std::queue<Task::ptr> _tasks;
    };

    std::shared_ptr<Pool> _pool; //任务临界区
    // uint16_t thread_size;

    // std::vector<std::thread *> _threads;
    int _m_cpu_size;



};





} // ! namespace axy



#endif // !__THREAD_POOL_H__