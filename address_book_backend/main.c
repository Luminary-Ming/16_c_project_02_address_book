// 启用 GNU C 库 (glibc) 中的所有扩展功能, 这里是为了使用 sigemptyset sigaddset sigprocmask sigsuspend等函数
#define _GNU_SOURCE
#include <signal.h>  // signal 函数
#include <unistd.h>  // sleep pause 函数头文件
#include <pthread.h>  // 为了操作线程级别的信号屏蔽
#include "http_server.h"
#include "logger.h"

int main(int argc, char *argv[])
{
	// 配置信号集：准备捕获 Ctrl+C (SIGINT) 和 kill (SIGTERM)
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);  // Ctrl+C
	sigaddset(&mask, SIGTERM); // kill, systemctl stop, poweroff

	/*
		在所有子线程启动前，屏蔽这些信号。
		这样以后创建的 libmicrohttpd 线程都会继承这个屏蔽状态，
		确保信号只能由主线程通过 sigwait 来接手。
	*/
	pthread_sigmask(SIG_BLOCK, &mask, NULL);


	LOG_INFO("通讯录后端启动");

	int port = 8080;  // 默认端口8080
	if (argc > 1)
	{
		port = atoi(argv[1]);  // 自定义端口
		if (port <= 0 || port > 65535)
		{
			LOG_ERR("无效端口: %d，将使用默认端口 8080", port);
			port = 8080;  // 无效端口, 使用默认端口
		}
	}

	// 启动 http 服务
	if (http_server_start(port) != 0)
	{
		LOG_ERR("无法在 %d 端口上启动服务器", port);
		return -1;
	}

	LOG_INFO("服务器正在运行, 按 Ctrl+C 可停止\n");

	/*
		同步等待信号
		sigwait 会挂起主线程，直到捕获到 mask 里的信号。
		它不需要 signal_handler 回调，收到信号后会直接往下执行。
	*/
	int sig;
	sigwait(&mask, &sig);

	// 停止 http 服务
	http_server_stop();  
	LOG_INFO("程序已安全退出");

	return 0;
}