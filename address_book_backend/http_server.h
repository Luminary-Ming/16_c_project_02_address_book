#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <microhttpd.h>  // sudo apt-get install libmicrohttpd-dev -y

enum http_error
{
	HTTP_OK = 0,
	LLIST_CREATE_FAILED = -1,
	DB_INIT_FAILED = -2,
	HTTP_SERVER_START_FAILED = -3,
	ERROR_URL = -4,
	RESP_CREATE_FAILED = -5,
	CONTENT_TYPE_ERROR = -6,
	UP_DATA_IS_NULL = -7,
	ERROR_INVALID_INPUT = -8,
	BASE64_DECODED_FAILED = -9,
	UPLOAD_FAILED = -10,
	ERROR_SYSTEM = -11,
	CONTACT_NOT_FOUND = -12,
	CONVERT_FAILED = -13,
};

typedef struct http  // HTTP服务器结构
{
	struct MHD_Daemon *daemon;  // HTTP 服务器实例, 创建后服务器自动开始监听端口
	int port;  // 端口
	int running; // 运行状态(1运行中, 0已停止)
}HttpServer;

int http_server_start(int port);  // 启动 http 服务
void http_server_stop(void);  // 停止 http 服务

#endif