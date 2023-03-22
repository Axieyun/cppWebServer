/*
 * @Author       : mark
 * @Date         : 2020-06-17
 * @copyleft Apache 2.0
 */
#include "heaptimer.h"

void axy::HeapTimer::siftup_(int child) { //节点上升
#ifdef DEBUG
    assert(child >= 0 && child < (int)heap_.size());
#else
    if ( child < 0 || child >= (int)heap_.size()) return ;
#endif // DEBUG

    // size_t j = (child - 1) / 2;   //child == 0时，j = 9223372036854775807
    int father = (child - 1) >> 1;
    TimerNode temp = heap_[child];
    while(father >= 0) {
        if(heap_[father] < heap_[child]) { break; }
        // _swapNode(child, father);
        heap_[child] = heap_[father];
        child = father;
        father = (child - 1) >> 1;
    }
    heap_[child] = temp;
}

void axy::HeapTimer::_swapNode(int i, int j) {
#ifdef DEBUG
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
#else
    if (i < 0 || i >= heap_.size()) return ;
    if (j < 0 || j >= heap_.size()) return ;
#endif // DEBUG


    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

bool axy::HeapTimer::siftdown_(int index, int heap_size) {//节点下降
    if (!(index >= 0 && index < heap_.size())) return false;
    if (!(heap_size >= 0 && heap_size <= heap_.size())) return false;
    size_t father = index;
    size_t j = father << 1 | 1;
    TimerNode temp = heap_[father]; //挖坑
    while (j < heap_size) {
        if (j + 1 < heap_size && heap_[j + 1] < heap_[j]) j++;
        if (heap_[father] < heap_[j]) break;

        heap_[father] = heap_[j];
        // _swapNode(i, j);
        father = j;
        j = father << 1 | 1;
    }
    heap_[father] = temp;
    return father > index;
}


void axy::HeapTimer::doWork(int id, bool is_call) {
    /* 删除指定id结点，并触发回调函数 */
    if (id < 0) return ;

    if (heap_.empty() || ref_.count(id) == 0) {
        return ;
    }
    // std::cout << "doWork fd is " << id << (is_call ? ", callback" : ", no callback") << ", heap size if " << heap_.size() - 1 << "\n";
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    if (is_call) node.cb();
    del_(i);
}

void axy::HeapTimer::del_(size_t index) {
    /* 删除指定位置的结点 */
#ifdef DEBUG
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
#else
    if (heap_.empty() || index < 0 || index >= heap_.size()) return ;
#endif // DEBUG

    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t i = index;
    size_t n = heap_.size() - 1;

#ifdef DEBUG
    assert(i <= n);
#else
    if (i > n) return ;
#endif // DEBUG

    if (i < n) {
        _swapNode(i, n);
        if(!siftdown_(i, n)) {
            siftup_(i);
        }
    }
    /* 队尾元素删除 */
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

//调整定时任务时间
bool axy::HeapTimer::adjust(int id, int timeout) {
    /* 调整指定id的结点 */
    // printf("fd[%d], heap_size[%ld], ref_.count[%d]%ld\n", id, heap_.size(), id, ref_.count(id));

    if (!(!heap_.empty() && ref_.count(id) > 0)) return false;
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);
    siftdown_(ref_[id], heap_.size());
    return true;
}

void axy::HeapTimer::tick() {
    /* 清除超时结点 */
    if(heap_.empty()) {
        return;
    }

    while(!heap_.empty()) {
        TimerNode node = (heap_.front());

        // std::time_t c1 = Clock::to_time_t(node.expires);
        // std::cout << std::put_time(std::localtime(&c1), "%F %T\n");
        // std::time_t c2 = Clock::to_time_t(Clock::now());
        // std::cout << std::put_time(std::localtime(&c2), "%F %T\n");

        if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) {
            break;
        }
        _pop();
#ifdef DEBUG
        LOG1("time out, tick\n");
#endif // DEBUG
        node.cb();
    }
}

void axy::HeapTimer::_pop() {
    del_(0);
}

void axy::HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

int axy::HeapTimer::getNextTick() {
    tick();
    int res = -1;
    if(!heap_.empty()) {
        res = std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
        if (res < 0) res = 0;
    }

    return res;
}