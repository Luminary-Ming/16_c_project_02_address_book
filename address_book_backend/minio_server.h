#ifndef MINIO_SERVER_H
#define MINIO_SERVER_H

#define ENDPOINT "192.168.2.7"  // ip 地址
#define PORT 9000  // 端口
#define ACCESS_KEY "g0KyMWdYHql3VKrdWFJj"  // 访问密钥
#define SECRET_KEY "7XWT64V22VOnRTQ2Hf0KkeoWaOWFbCFP4Itc8qUY"  // 私有密钥
#define BUCKET "filebucket/address_book"  // 桶名

enum file_error
{
	FILE_OK = 0,
	FILE_PARAM_NULL = -1,
	UNABLE_FILE_INFO = -2,
	CURL_INIT_FAILED = -3,
	FILE_OPEN_FAILED = -4,
	CURL_HEADERS_FAILED = -5,
	FILE_UPLOAD_FAILED = -6,

};

// 上传文件到 minio
int minio_upload(const char *local_filename, const char *object_filename);
// 获取文件预览 URL
char *minio_preview_url(const char *object_filename);

#endif