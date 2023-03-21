#ifndef __LOG_H__
#define __LOG_H__

#include "./color.h"

#include <stdio.h>
#include <ctime>



extern FILE *pLogFile;

static time_t now;


#ifdef DEBUG

//打印时间、文件名、函数名、行数
/*
格式：
Www Mmm dd hh:mm:ss yyyy(Mon Apr 05 15:23:17 2021)
其中，Www表示星期，Mmm表示月份，dd表示天数，hh:mm:ss表示时间，yyyy表示年份。
*/


// 打印正常调试信息，绿色
#define LOG() do { now = time(nullptr); printf(GREEN"RUN" NONE": FILE[%s], LINE[%d], TIME: %s" , __FILE__, __LINE__, ctime(&now)); } while(0)
//                                                                                          不能写 GREEN"RUN"NONE""，这样会报警告
#define LOG1(fmt, args...) do { now = time(nullptr); printf(fmt, ##args); printf(GREEN"RUN" NONE": FILE[%s], LINE[%d], TIME: %s", __FILE__, __LINE__, ctime(&now)); } while(0)


// 打印警告 warning ，黄色
#define LOG_WARN(fmt, args...) do { now = time(nullptr); printf(fmt, ##args); printf(YELLOW"WARNING" NONE": FILE[%s], LINE[%d], TIME: %s", __FILE__, __LINE__, ctime(&now)); } while(0)



// 打印错误信息，用红色
#define LOG_ERROR(fmt, args...) do { now = time(nullptr); printf(fmt, ##args); printf(RED"ERRROR " NONE": FILE[%s], LINE[%d], TIME: %s", __FILE__, __LINE__, ctime(&now)); } while(0)



#elif RUN




//打印时间、文件名、函数名、行数
/*
格式：
Www Mmm dd hh:mm:ss yyyy(Mon Apr 05 15:23:17 2021)
其中，Www表示星期，Mmm表示月份，dd表示天数，hh:mm:ss表示时间，yyyy表示年份。
*/



// 打印正常调试信息，绿色
#define LOG(stream) do { now = time(nullptr); fprintf(stream, GREEN"RUN" NONE": FILE[%s], LINE[%d], TIME: %s" , __FILE__, __LINE__, ctime(&now)); } while(0)
#define LOG1(fmt, args...) do { now = time(nullptr); fprintf(fmt, ##args); fprintf(stream, GREEN" RUN" NONE": FILE[%s], LINE[%d], TIME: %s ", __FILE__, __LINE__, ctime(&now)); } while(0)


// 打印警告 warning ，黄色
#define LOG_WARN(stream, fmt, args...) do { now = time(nullptr); fprintf(stream, fmt, ##args); fprintf(stream, YELLOW" WARNING" NONE": FILE[%s], LINE[%d], TIME: %s", __FILE__, __LINE__, ctime(&now)); } while(0)



// 打印错误信息，用红色
#define LOG_ERROR(stream, fmt, args...) do { now = time(nullptr); fprintf(stream, fmt, ##args); fprintf(stream, RED" ERRROR " NONE": FILE[%s], LINE[%d], TIME: %s", __FILE__, __LINE__, ctime(&now)); } while(0)

#endif 








#endif //!__LOG_H__