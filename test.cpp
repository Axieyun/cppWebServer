#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <string>
#include <string.h>
using namespace std;
#include "./pool/threadPool.h"
#include "./http/httpconn.h"
#include "./server/webServer.h"


void f1(int a) {
    std::cout << a << std::endl;
}
void f2(int a, int b) {
    std::cout << a << " - " << b << std::endl;
}

void f3(int &i) {
    std::cout << i++ << std::endl;
}

class KMP {
private:
    static void getNext(std::vector<int> &next, const char *str, int n) {

        int len = n;               // 字符串长度
        for(int i = 1; i < len; i++) {
            int k = next[i - 1];             // k 表示需要比较的位置，初始值为 next[i - 1]
            while(k > 0 && str[i] != str[k]) // 比较，若不相等则继续分割，直到相等或为0(即不含相同部分)
                k = next[k - 1];
            if(str[i] == str[k])             // 若相等，则 next[i] = k + 1，否则为0，其中 k 为索引
                k++;
            next[i] = k;                     // 更新 next[i]
        }
    }
public:
    static const char *kmp(const char *P, int n1, const char *T, int n2) {
        //T为文本串，P模式串
        int np = n1;
        int nt = n2;
        //最长相同前后缀表
        // std::cout << n1 << " " << n2 << std::endl;
        std::vector<int> next(nt, 0);

        getNext(next, T, nt);
        int j = 0;
        for (int i = 0; i < np; ++i) {
            while (j > 0 && P[i] != T[j]) {
                j = next[j - 1];
            }
            if (T[j] == P[i]) {
                ++j;
            }
            //当j等于自身长度说明匹配完成
            if (j == nt) {
                //此时i是匹配成功的最后一个位置，因此起始位置为 i -j + 1的位置
                return P + (i - j + 1);
            }

        }
        return nullptr;
    }
};


void get(std::string &ret, std::string&& s) {
    ret = std::move(std::forward<std::string>(s));
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        printf("%s\n", argv[1]);
    }

    // axy::ThreadPool::ptr thread_pool = std::make_shared<axy::ThreadPool>(10);
    // // axy::ThreadPool thread_pool;

    // string s("3453453453");
    // string ret;
    // cout << s << "&&&&" << ret << endl;
    // get(ret, std::move(s));
    // cout << s << "&&&&" << ret << endl;

    // std::cout << "****************\n";
    // int cnt = 1;
    // while (cnt < 4) {
    //     thread_pool->add_task(f3, std::ref(cnt));
    // }
    // int n = 100;
    // for (int i = 0; i < n; ++i) {
    //     int a = i;
    //     int b = i + 1;

    //     // if (i & 1) thread_pool.add_task(&f1, a);
    //     // else thread_pool.add_task(&f2, a, b);
    //     if (i & 1) thread_pool->add_task(&f1, std::ref(a));
    //     else thread_pool->add_task(&f2, std::ref(a), std::ref(b));


    //     std::cout << "add " << i << " \n";
    // }
    // for (int i = 1; i <= 10; ++i) {
    //     std::cout << "sleep " << i << " s\n";
    //     sleep(1);
    // }
    // char c[] = "23h43dh4a545dfghdfghdfg";
    // std::string s(c + 1, 6);
    // std::cout << s << std::endl;

    // std::cout << &(*s.begin()) << std::endl;
    // std::cout << s.data() << std::endl;
    // std::vector<char> v(1024, 1);
    // std::cout << v.size() << " - " << v.capacity() << std::endl;

    // v.resize(1024 << 1);
    // std::cout << v.size() << " - " << v.capacity() << std::endl;

    // v.resize(1024);
    // std::cout << v.size() << " - " << v.capacity() << std::endl;

    // v.reserve(1024);
    // std::cout << v.size() << " - " << v.capacity() << std::endl;

    // std::vector<char> c(1024);
    // v.swap(c);
    // std::cout << v.size() << " - " << v.capacity() << std::endl;

    // std::cout << axy::HttpConn::get_user_count() << std::endl;

    // axy::HttpConn::set_user_count(3);
    // std::cout << axy::HttpConn::get_user_count() << std::endl;
	if(-1 == daemon(1, 0)) {
	  	printf("daemon error\n");
 	    exit(1);
    }

    uint32_t time_out = 120000;
    // std::time_t c1 = Clock::to_time_t(Clock::now() + MS(time_out));
    // std::cout << time_out << "  " << std::put_time(std::localtime(&c1), "%F %T\n");
    axy::WebServer server{8000, time_out, 3306, "XXXX", "XXXXX", "test"};
    // sleep(5);
    server.start();
    // const char *P = "asdgasfgdhgsd sjdfghskjfgh sdfghsgHSFyhg 454 dfgfghsdfg dfghjadgfajgfka GHDFasdaASDFAs";
    // char *T = "HSF";
    // std::cout << KMP::kmp(P, strlen(P), T, strlen(T)) << std::endl;
    // std::cout << std::string(P, KMP::kmp(P, strlen(P), T, strlen(T))) << std::endl;
    // T = " dfgf";
    // std::cout << KMP::kmp(P, strlen(P), T, strlen(T)) << std::endl;

    // std::cout << std::string(P, KMP::kmp(P, strlen(P), T, strlen(T))) << std::endl;

    // T = " dfgf^^^^";
    // std::cout << KMP::kmp(P, strlen(P), T, strlen(T)) << std::endl;
    return 0;
}

/*

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <zlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>

using namespace std;

// 定义块大小为1MB
#define BLOCK_SIZE 1048576

// 压缩文件并分块传输
void compressAndSendFile(int connfd, string filename) {
    // 打开文件并获取文件大小
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        cerr << "Failed to open file " << filename << endl;
        return;
    }
    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) {
        cerr << "Failed to get file stats" << endl;
        close(fd);
        return;
    }
    off_t file_size = file_stat.st_size;

    // 映射文件到内存
    char* file_data = (char*)mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        cerr << "Failed to mmap file" << endl;
        close(fd);
        return;
    }

    // 初始化zlib
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        cerr << "Failed to initialize zlib" << endl;
        munmap(file_data, file_size);
        close(fd);
        return;
    }

    // 循环处理每个块并发送
    int block_count = (file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int sent_block_count = 0;
    int total_sent_size = 0;
    for (int i = 0; i < block_count; ++i) {
        // 计算块的大小
        int block_size = (i == block_count - 1) ? (file_size % BLOCK_SIZE) : BLOCK_SIZE;
        if (block_size == 0) {
            block_size = BLOCK_SIZE;
        }

        // 压缩块数据
        zs.next_in = (Bytef*)(file_data + i * BLOCK_SIZE);
        zs.avail_in = block_size;
        char compressed_block_data[BLOCK_SIZE];
        zs.next_out = (Bytef*)compressed_block_data;
        zs.avail_out = sizeof(compressed_block_data);
        if (deflate(&zs, Z_FINISH) != Z_STREAM_END) {
            cerr << "Failed to compress block " << i << endl;
            deflateEnd(&zs);
            munmap(file_data, file_size);
            close(fd);
            return;
        }
        int compressed_block_size = sizeof(compressed_block_data) - zs.avail_out;

        // 发送块大小
        int block_size_network_order = htonl(block_size);
        int sent_size = send(connfd, &block_size_network_order, sizeof(block_size_network_order), 0);
        if (sent_size < 0) {
            cerr << "Failed to send block size" << endl;
            deflateEnd(&zs);
            munmap(file_data, file_size);
            close(fd);
            return;
        }
        total_sent_size += sent_size;

        // 发送

*/