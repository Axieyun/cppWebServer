/*************************************************************************
	> File Name: math/math.h
	> Author: 
	> Mail: 
	> Created Time: Fri 24 Mar 2023 02:51:46 PM CST
 ************************************************************************/

#ifndef _MATH_MATH_H_
#define _MATH_MATH_H_




namespace axy {



class KMP {
private:
    KMP();
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
        return P + n1;
    }

};












} // !namespace axy




#endif
