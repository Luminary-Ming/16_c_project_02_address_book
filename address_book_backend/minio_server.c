#include "minio_server.h"
#include "logger.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
//#include <x86_64-linux-gnu/curl/curl.h>  // curl 头文件
#include <curl/curl.h>

/**
	@brief 从文件中读取数据给 curl 发送
	@param ptr 指向要填充数据的内存缓冲区
	@param size 每个数据块的大小（通常是1字节）
	@param nmemb 数据块的数量
	@param stream 文件指针（通过 CURLOPT_READDATA 设置）
	@return 实际读取的字节数
 */
static size_t read_from_file(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return fread(ptr, size, nmemb, stream);
}

// 上传文件到 minio
int minio_upload(const char *local_filename, const char *object_filename)
{
	if (!local_filename || !object_filename)
	{
		LOG_ERR("文件参数为空");
		return FILE_PARAM_NULL;
	}

	// 获取文件信息
	struct stat file_info = {0};  // 全部初始化为 0
	if (stat(local_filename, &file_info) != 0)
	{
		LOG_ERR("无法获取文件信息");
		return UNABLE_FILE_INFO;
	}

	// 初始化curl
	CURL *curl = curl_easy_init();
	if (!curl)
	{
		LOG_ERR("curl 初始化失败");
		return CURL_INIT_FAILED;
	}

	// 构建上传 url
	char url[1024];
	snprintf(url, sizeof(url), "http://%s:%d/%s/%s",
			 ENDPOINT, PORT, BUCKET, object_filename);

	// 设置 curl 选项
	// 设置要请求的 URL
	curl_easy_setopt(curl, CURLOPT_URL, url);

	// 设置为上传模式（使用 HTTP PUT 方法）
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

	// 以二进制只读方式打开本地文件
	FILE *fp = fopen(local_filename, "rb");
	if (!fp)
	{
		LOG_ERR("打开文件失败: %s", strerror(errno));
		return FILE_OPEN_FAILED;
	}
	// 设置文件指针, curl 将从这个文件读取数据
	curl_easy_setopt(curl, CURLOPT_READDATA, fp);

	// 设置读取数据的回调函数
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_from_file);

	// 设置要上传的文件大小
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);

	// 创建 HTTP 请求头链表
	struct curl_slist *headers = NULL;

	// 添加 Content-Type 头，指定上传的是二进制数据
	headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
	if (!headers)
	{
		LOG_ERR("curl 设置请求头失败");
		fclose(fp);
		curl_easy_cleanup(curl);
		return CURL_HEADERS_FAILED;
	}
	// 设置 HTTP 请求头
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	LOG_INFO("上传到 %s 到 %s\n", local_filename, url);

	// 执行上传
	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		LOG_ERR("上传失败: %s", curl_easy_strerror(res));
		fclose(fp);  // 关闭文件
		curl_slist_free_all(headers);  // 释放请求头链表
		curl_easy_cleanup(curl);  // 清理 curl 会话
		return FILE_UPLOAD_FAILED;
	}

	LOG_INFO("上传成功");

	// 清理资源
	fclose(fp);  // 关闭文件
	curl_slist_free_all(headers);  // 释放请求头链表
	curl_easy_cleanup(curl);  // 清理 curl 会话

	return FILE_OK;
}

// 获取文件预览 URL
char *minio_preview_url(const char *object_filename)
{
	if (!object_filename)
	{
		LOG_ERR("文件参数为空");
		return NULL;
	}

	char *preview_url = malloc(1024);
	if (!preview_url)
	{
		LOG_ERR("malloc failed: %s", strerror(errno));
		return NULL;
	}

	snprintf(preview_url, 1024, "http://%s:%d/%s/%s", ENDPOINT, PORT, BUCKET, object_filename);

	return preview_url;
}