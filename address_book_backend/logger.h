/*
	日志打印
	TODO 加时间
*/
#ifndef LOGGER_H
#define LOGGER_H

#define _GNU_SOURCE  // 打开 GNU 扩展, 使用 syscall 
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>  // 提供 SYS_gettid 宏定义
#include <unistd.h>  // 提供 syscall 函数声明
#include <time.h>  // 提供 time 函数声明

#define LOG_DEBUG

// 颜色定义
#define COLOR_RED   "\033[31m"
#define COLOR_RESET "\033[0m"

/*
	获取当前时间字符串，格式：YYYY-mm-dd HH:MM:SS
	@param buf 输出缓冲区（至少20字节）
	@param size 缓冲区大小
*/
static inline void get_timestamp(char *buf, size_t size)
{
	time_t now;
	time(&now);  // 获取当前时间戳 ( 从1970-01-01 00:00:00到现在的秒数 )

	struct tm tm_info;
	localtime_r(&now, &tm_info);  // 转换为本地时间

	// strftime() 将 struct tm 结构体格式化为指定格式的字符串
	strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm_info);
}

// 获取线程 ID
static inline long get_tid()
{
	// syscall() 是直接调用 Linux 内核系统调用的函数
	// 调用编号为 SYS_gettid(宏值186) 的系统调用
	return syscall(SYS_gettid);  // 获取线程 ID (tid) ( getpid() 是获取进程 ID (pid) )
}

#ifdef LOG_DEBUG
// [时间] [级别] [线程ID] [请求ID] [文件:行号] [函数名] - 日志内容 
// error 信息 (红色) 
#define LOG_ERR(fmt, ...) \
	do{ \
		char timestamp[32]; \
		get_timestamp(timestamp, sizeof(timestamp)); \
		fprintf(stderr, COLOR_RED "%s [ERROR] [tid=%ld] [%s:%d] [%s()] - " fmt COLOR_RESET "\n", timestamp, get_tid(), __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
	} while (0)
// info 信息
#define LOG_INFO(fmt, ...) \
	do{ \
		char timestamp[20]; \
		get_timestamp(timestamp, sizeof(timestamp)); \
		fprintf(stdout, "%s [INFO] [tid=%ld] [%s:%d] [%s()] - " fmt "\n", timestamp, get_tid(), __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
	} while (0)
#else
#define LOG_ERR(fmt, ...) ((void)0)
#define LOG_INFO(fmt, ...) ((void)0)
#endif

#endif

/*
	inline 是 C/C++ 中建议编译器将函数内联展开的关键字。
	inline 的作用 :
		1. 减少函数调用开销
		2. 适用于短小频繁调用的函数
	=====================================================
	1. 减少函数调用开销

	// 普通函数
	int add(int a, int b) {
		return a + b;
	}

	// 调用时：
	int result = add(1, 2);
	// 过程：压栈 → 跳转 → 执行 → 返回 → 出栈（有开销）

	// inline函数
	static inline int add(int a, int b) {
		return a + b;
	}

	// 调用时：
	int result = add(1, 2);
	// 编译后可能变成：int result = 1 + 2;（无调用开销）
	=====================================================
	2. 适用于短小频繁调用的函数

	// 这个函数会被频繁调用
	LOG_INFO("用户登录");      // 每次都要调用 get_timestamp()
	LOG_ERR("连接失败");       // 每次都要调用 get_timestamp()
	LOG_DEBUG("SQL: %s", sql); // 每次都要调用 get_timestamp()

	如果不 inline，每次都有函数调用开销
	用 inline 后，代码直接嵌入，减少开销
	=====================================================
*/