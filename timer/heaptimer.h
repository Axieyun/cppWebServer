/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */ 
#ifndef __HEAP_TIMER_H__
#define __HEAP_TIMER_H__

#include <queue>
#include <iomanip>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../log.h"

#include <iostream>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

// std::chrono::high_resolution_clock::now() 获取时间点，按秒为单位的
/*
   typedef duration <Rep, ratio<3600,1>> hours;
   typedef duration <Rep, ratio<60,1>> minutes;
   typedef duration <Rep, ratio<1,1>> seconds;
   typedef duration <Rep, ratio<1,1000>> milliseconds;
   typedef duration <Rep, ratio<1,1000000>> microseconds;
   typedef duration <Rep, ratio<1,1000000000>> nanoseconds;
*/


namespace axy{



struct TimerNode {
    TimerNode(int id, TimeStamp ep, TimeoutCallBack cb) : id(id), expires(ep), cb(cb) {}
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    TimerNode(const TimerNode& t) {
        id = t.id;
        expires = t.expires;
        cb = t.cb;
    }
    TimerNode(const TimerNode&& t) {
        id = t.id;
        expires = t.expires;
        cb = t.cb;
    }
    TimerNode& operator=(const TimerNode& t) { //返回值为TimerNode& 可以达到连续赋值的目的 a = b = c;
        id = t.id;
        expires = t.expires;
        cb = t.cb;
        return *this;
    }
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};
class HeapTimer {
public:
    HeapTimer() { heap_.reserve(64); }

    ~HeapTimer() { clear(); }
    
    bool adjust(int id, int newExpires);

    
    template<typename FUNC, typename ...ARGS>
    void add(int id, int time_out, FUNC&& func, ARGS&& ...args) {
        assert(id >= 0);
        size_t i;
        TimeoutCallBack cb = std::bind(func, std::forward<ARGS>(args)...);

        // std::time_t c1 = Clock::to_time_t(Clock::now() + MS(time_out));
        // std::cout << time_out << "  " << std::put_time(std::localtime(&c1), "%F %T\n");
        // printf("add fd[%d] in heap, heap size %ld, ref_count(): %ld\n", id, heap_.size() + 1, ref_.count(id));
        if(ref_.count(id) == 0) {
            /* 新节点：堆尾插入 */
            i = heap_.size();
            ref_[id] = i;
            heap_.push_back(axy::TimerNode{id, Clock::now() + MS(time_out), cb}); //不能用 std::move(cb)
            // siftup_(i); //新来的都是晚来，不需要调整
        }
        else {
            /* 已有结点：调整堆 */
            i = ref_[id];
            heap_[i].expires = Clock::now() + MS(time_out);
            heap_[i].cb = cb;
            if(!siftdown_(i, heap_.size())) {
                siftup_(i);
            }
        }
    }
    
    void doWork(int id, bool is_call = false);

    void clear();

    void tick();

    

    int getNextTick();

// public:
//     static int getTimeOutMs() { return _time_out_ms; }
//     static void setTimeOutMs(int time_out_ms) { _time_out_ms = time_out_ms; } 
// private:
//     static int _time_out_ms;


private:
    void del_(size_t i);
    void _pop();
    void siftup_(int i);

    // bool siftdown_(size_t index, size_t n);
    bool siftdown_(int index, int n);

    // void _swapNode(size_t i, size_t j);
    void _swapNode(int i, int j);

    std::vector<TimerNode> heap_;

    std::unordered_map<int, size_t> ref_;
    
};



}



#endif //__HEAP_TIMER_H__