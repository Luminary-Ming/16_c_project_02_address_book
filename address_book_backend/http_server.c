#include "http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>
#include "contact.h"
#include "contact_llist.h"
#include "db_mysql.h"
#include "minio_server.h"
#include "logger.h"
#include "utils.h"

// 全局 http 服务器
static HttpServer server = { 0 };

// 全局链表
LLIST *contact_llist = NULL;

typedef struct
{
	char *post_data;   // 存储接收到的 json 字符串数据
	size_t data_size;  // 当前数据长度
} RequestContext;

/*
	请求完成后的清理回调 (handle_request 被多次调用，它会把接收到的 POST 数据存入 RequestContext,
	无论请求成功还是意外中断，request_completed_callback 都会被触发，把 RequestContext 释放掉, 防止内存泄漏)

	@param cls 启动服务器时传入的 cls (这里是 NULL)
	@param connection 当前 http 连接对象 ( 用于获取请求信息和发送响应 )
	@param con_cls handle_request 中传入的 *con_cls 的地址
	@param toe 结束原因 (如：请求成功、连接断开等)
*/
static void request_completed_callback(void *cls, struct MHD_Connection *connection,
										void **con_cls, enum MHD_RequestTerminationCode toe)
{
	// 将 void** 类型的 con_cls 解引用并强制转换为我们自定义的结构体指针
	// *con_cls 指向的是我们在 handle_request 中 malloc 申请的那块内存
	RequestContext *req_context = (RequestContext *)*con_cls;
	if (req_context == NULL)
		return;

	// 如果在处理 POST 请求时申请了空间存储 json 字符串
	// 那么释放结构体内部动态分配的成员
	if (req_context->post_data)
	{
		free(req_context->post_data);  // 释放存储 POST 数据的缓冲区
		req_context->post_data = NULL;  // 将指针置空，防止变成野指针
	}

	free(req_context);  // 释放结构体本身占用的内存

	// 将原始指针置为 NULL
	// 告诉 MHD 库，这个连接关联的所有自定义数据已经清理干净了
	*con_cls = NULL;

	// 打印日志，记录请求处理结束的状态码（toe 表示 Termination Code）
	LOG_INFO("请求处理完毕，内存已回收。原因代码: %d", toe);
}

// 发送 json 响应
static int send_json_response(struct MHD_Connection *connection, int status_code, const char *json)
{
	// 创建 http 响应 (参数 : 响应体大小, 响应数据指针, 内存管理模式 - 必须复制)
	struct MHD_Response *resp = MHD_create_response_from_buffer(strlen(json), (void *)json, MHD_RESPMEM_MUST_COPY);
	if (!resp)
	{
		LOG_ERR("创建 http 响应失败");
		return RESP_CREATE_FAILED;
	}

	MHD_add_response_header(resp, "Content-Type", "application/json");  // 告诉客户端返回的数据是 json 格式
	MHD_add_response_header(resp, "Access-Control-Allow-Origin", "*");  // 允许任何域名
	MHD_add_response_header(resp, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");  // 允许的HTTP方法
	MHD_add_response_header(resp, "Access-Control-Allow-Headers", "Content-Type");  // 允许的请求头

	int ret = MHD_queue_response(connection, status_code, resp);  // 将响应加入发送队列
	MHD_destroy_response(resp);  // 释放响应对象内存
	return ret;
}

// 通用的 POST 数据收集逻辑
static int collect_post_data(struct MHD_Connection *connection, const char *upload_data,
							 size_t *upload_data_size, void **con_cls)
{
	if (*con_cls == NULL)  // 初始化"储物柜", 只在请求刚开始时执行一次
	{
		// 为这个请求申请一个专属的上下文结构体，用来跨多次回调保存数据
		RequestContext *req_context = malloc(sizeof(RequestContext));
		if (!req_context)
		{
			LOG_ERR("malloc failed");
			return MHD_NO;  // 内存分配失败，关闭连接
		}

		*con_cls = req_context;  // 将地址存入 con_cls，下次回调时 MHD 会把它还给我们
		return MHD_YES;  // 告诉 MHD 继续接收数据
	}

	// 数据拼接阶段 (MHD 会多次进入这里)
	RequestContext *req_context = (RequestContext *)*con_cls;  // 取出之前的"储物柜"

	// 处理接收到的数据块 (MHD 会多次调用此逻辑直到 upload_data_size 为 0)
	if (*upload_data_size != 0)
	{
		// 根据新到数据的大小，动态调整堆内存缓冲区
		// 动态扩容：原来的数据长度 + 这次新来的长度 + 1 (给 \0 预留空间)
		char *new_post_data = realloc(req_context->post_data, req_context->data_size + *upload_data_size + 1);
		if (!new_post_data)
		{
			LOG_ERR("malloc failed");
			return MHD_NO;  // 内存分配失败，关闭连接
		}

		req_context->post_data = new_post_data;  // req_context->post_data 指向扩容完的内存空间

		// 把新来的数据 upload_data 拷贝到旧数据的末尾
		// req_context->post_data 是这块内存的起点, req_context->data_size 是已经存进去的数据长度
		// req_context->post_data + req_context->data_size 指针偏移 (跳过前面已经写好的部分，从空白的地方开始接着存)
		memcpy(req_context->post_data + req_context->data_size, upload_data, *upload_data_size);
		// 更新总长度
		req_context->data_size += *upload_data_size;
		// 必须手动给字符串加结束符，否则 printf 会越界
		// 在 C 语言中，如果你想用 printf("%s") 或者 LOG_INFO 打印一个字符串，系统会从开头一直往后读，直到遇到 \0 才停止。
		// 不加的后果：如果你不手动加 \0，打印函数就会越过你的数据，继续读取内存中其他乱七八糟的内容（也就是常说的“乱码”），甚至导致程序直接崩溃。
		req_context->post_data[req_context->data_size] = '\0';

		*upload_data_size = 0; // 告诉 MHD 这一块数据我已经处理完了, 可以把这段占用的内存清掉了
		return MHD_YES;  // 继续等待下一块数据
	}
	return MHD_YES;  // 所有数据接收完毕 (upload_data_size == 0)
}


// 处理 OPTIONS 请求（CORS预检, 当浏览器发送跨域请求时，会先发送一个 OPTIONS 请求来检查服务器是否允许）
static int handle_options(struct MHD_Connection *connection)
{
	// 0 : 响应体长度为0字节 (OPTIONS响应不需要body)
	// NULL : 响应体数据为NULL (因为没有数据)
	// MHD_RESPMEM_PERSISTENT : 内存管理标志，表示响应数据是持久的
	struct MHD_Response *resp = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
	MHD_add_response_header(resp, "Access-Control-Allow-Origin", "*");  // 允许任何域名
	MHD_add_response_header(resp, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");  // 允许的HTTP方法
	MHD_add_response_header(resp, "Access-Control-Allow-Headers", "Content-Type");  // 允许的请求头
	// 缓存预检结果 (告诉浏览器把这个预检结果缓存86400秒 (24小时), 避免每次请求都发送OPTIONS预检，提高性能)
	MHD_add_response_header(resp, "Access-Control-Max-Age", "86400");

	int ret = MHD_queue_response(connection, MHD_HTTP_OK, resp);  // 将响应加入发送队列
	MHD_destroy_response(resp);  // 释放响应对象内存
	return ret;
}

// 处理 GET /api/contacts 获取所有联系人
static int handle_get_all_contacts(struct MHD_Connection *connection)
{
	// 每次查询前先同步数据库到链表（保证数据最新）
	if (db_load_contacts(contact_llist) != DB_OK)
	{
		char *resp_json = json_response_no_data(DB_ERR_DELETE, "查询失败");
		int ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, resp_json);
		free(resp_json);
		return ret;
	}

	int count = llist_get_count(contact_llist);
	Contact **contacts_arr = malloc(sizeof(Contact *) * count);
	for (int i = 0; i < count; i++)
		contacts_arr[i] = llist_get_at(contact_llist, i);

	char *data_json = contacts_to_json(contacts_arr, count);
	free(contacts_arr);

	char *resp_json = json_response_data(HTTP_OK, "获取成功", data_json);
	int ret = send_json_response(connection, MHD_HTTP_OK, resp_json);
	
	free(data_json);
	free(resp_json);
	return ret;
}

// 处理 GET /api/contacts/{id} 获取单个联系人
static int handle_get_contact(struct MHD_Connection *connection, const char *url)
{
	// 从 url 中获取 id: /api/contacts/123
	int id = atoi(url + 14);
	if (id <= 0)
	{
		char *resp_json = json_response_no_data(ERROR_INVALID_INPUT, "无效的ID");
		int ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, resp_json);
		free(resp_json);
		return ret;
	}

	Contact * contact = db_find_contact(contact_llist, id);
	if (!contact)
	{
		char *resp_json = json_response_no_data(CONTACT_NOT_FOUND, "未找到联系人或联系人不存在");
		int ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, resp_json);
		free(resp_json);
		return ret;
	}

	char *json_data = contact_to_json(contact);
	if (!json_data)
	{
		char *resp_json = json_response_no_data(CONVERT_FAILED, "联系人转 json 失败");
		int ret = send_json_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, resp_json);
		free(resp_json);
		return ret;
	}

	char *resp_json = json_response_data(HTTP_OK, "查找成功", json_data);
	int ret = send_json_response(connection, MHD_HTTP_OK, resp_json);
	free(resp_json);
	return ret;
}

// 处理 DELETE /api/contacts/{id} 删除联系人
static int handle_delete_contact(struct MHD_Connection *connection, const char *url)
{
	// 从 url 中获取 id: /api/contacts/123
	int id = atoi(url + 14);
	if (id <= 0)
	{
		char *resp_json = json_response_no_data(ERROR_INVALID_INPUT, "无效的ID");
		int ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, resp_json);
		free(resp_json);
		return ret;
	}

	if (db_delete_contact(contact_llist, id) != DB_OK)
	{
		char *resp_json = json_response_no_data(DB_ERR_DELETE, "删除失败或记录不存在");
		int ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, resp_json);
		free(resp_json);
		return ret;
	}

	char *resp_json = json_response_no_data(HTTP_OK, "删除成功");
	int ret = send_json_response(connection, MHD_HTTP_OK, resp_json);
	free(resp_json);
	return ret;	
}

// 处理 POST/PUT /api/contacts 添加联系人/更新联系人
static int handle_save_contact(struct MHD_Connection *connection, RequestContext *req_context, int is_update)
{
	Contact *contact = parse_contact_from_json(req_context->post_data);
	if (!contact)
	{
		char *resp_json = json_response_no_data(ERROR_INVALID_INPUT, "无效的联系人数据");
		int ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, resp_json);
		free(resp_json);
		return ret;
	}

	int db_res = is_update ? db_update_contact(contact_llist, contact) : db_insert_contact(contact_llist, contact);
	if (db_res != DB_OK)
	{
		char *resp_json = json_response_no_data(DB_ERR_INSERT, "保存至数据库失败");
		int ret = send_json_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, resp_json);
		free(resp_json);
		free(contact);
		return ret;
	}
	char *json_data = contact_to_json(contact);
	char *resp_json = json_response_data(HTTP_OK, is_update ? "修改成功" : "添加成功", json_data);
	int ret = send_json_response(connection, MHD_HTTP_OK, resp_json);
	free(resp_json);
	free(contact);
	free(json_data);
	return ret;
}

// 处理 Base64 编码的图片上传
static int handle_upload_base64_image(struct MHD_Connection *connection, RequestContext *req_context)
{
	if (!req_context || !req_context->post_data)
	{
		LOG_ERR("接收到的 POST 数据为空");
		return MHD_NO;
	}

	// 初始化所有需要释放的指针为 NULL
	// 这样在最后 cleanup 时，直接 free(指针) 无论逻辑走到哪一步都是安全的
	// 在 C 语言中，free(NULL) 是完全安全且合法的，它不会执行任何操作，更不会导致程序崩溃。
	char *filename = NULL;
	char *base64_data = NULL;
	unsigned char *decoded_raw = NULL;
	char *object_name = NULL;
	char *image_url = NULL;
	char *resp_json = NULL;  // 返回 json, 初始化为空, 最后统一 free
	int ret = MHD_NO;  // 函数返回值, 初始化为 MHD_NO (0)

	// 确保字符串以空字符结尾，防止 strstr 等函数越界访问内存
	req_context->post_data[req_context->data_size] = '\0';

	// --- A.解析文件名 ---
	char *f_ptr = strstr(req_context->post_data, "\"filename\":\"");
	if (f_ptr)
	{
		f_ptr += 12;  // 偏移获取文件名 "filename":"文件名.jpg"
		char *f_end = strchr(f_ptr, '"');
		if (f_end)
		{
			size_t len = f_end - f_ptr;
			filename = my_strndup(f_ptr, len);  // 申请内存并拷贝文件名
		}
	}

	LOG_ERR("filename = %s", filename);

	// --- B.解析 Base64 data ---
	char *d_ptr = strstr(req_context->post_data, "\"data\":\"");
	if (d_ptr)
	{
		d_ptr += 8;  // 偏移获取 base64 的内容 "data":"base64"
		char *d_end = strchr(d_ptr, '"');
		if (d_end)
		{
			size_t len = d_end - d_ptr;
			base64_data = my_strndup(d_ptr, len);  // 申请内存并拷贝 Base64 字符串
		}
	}

	// --- C.校验解析结果 ---
	if (!filename || !base64_data)
	{
		resp_json = json_response_no_data(ERROR_INVALID_INPUT, "无效的JSON数据格式");
		ret = send_json_response(connection, MHD_HTTP_BAD_REQUEST, resp_json);
		goto cleanup;
	}

	// --- D.Base64 解码 ---
	size_t decoded_size = 0;
	// 调用解码函数，将文本转回二进制图片数据
	decoded_raw = base64_decode(base64_data, &decoded_size);
	if (!decoded_raw)
	{
		resp_json = json_response_no_data(BASE64_DECODED_FAILED, "Base64解码失败");
		ret = send_json_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, resp_json);
		goto cleanup;
	}

	// --- E.存储与上传 ---
	// 防冲突, 给文件名加上时间戳
	object_name = handle_filename(filename);  
	// 保存临时文件
	char temp_path[512];
	snprintf(temp_path, sizeof(temp_path), "/tmp/%s", object_name);
	FILE *fp = fopen(temp_path, "wb");
	if (!fp)
	{
		LOG_ERR("temp_path open failed");
		resp_json = json_response_no_data(ERROR_SYSTEM, "无法创建临时文件");
		ret = send_json_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, resp_json);
		goto cleanup;
	}
	if (fwrite(decoded_raw, 1, decoded_size, fp) < decoded_size)
	{
		LOG_ERR("磁盘满了！写入失败");
		goto cleanup;
	}
	fclose(fp);  // 写入后不再使用流, 关闭流
	fp = NULL;  // 手动置空，防止 cleanup 重复关闭

	// 上传到 minio
	int up_ret = minio_upload(temp_path, object_name);
	if (up_ret != 0)
	{
		LOG_ERR("minio_upload failed");
		resp_json = json_response_no_data(UPLOAD_FAILED, "上传至存储服务器失败");
		ret = send_json_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, resp_json);
		remove(temp_path);  // 删除临时文件
		goto cleanup;
	}
	image_url = minio_preview_url(object_name);

	// 删除临时文件
	remove(temp_path);
	LOG_INFO("已删除临时文件: %s", temp_path);

	// --- F.构建最终响应 ---
	int resp_data_len = snprintf(NULL, 0, "{\"url\": \"%s\", \"filename\": \"%s\"}",
								 image_url ? image_url : "", filename);

	char *resp_data = malloc(resp_data_len + 1);  // 在堆上分配内存（长度 + 1 个结束符）
	if (!resp_data)
	{
		LOG_ERR("malloc failed");
		goto cleanup;
	}

	sprintf(resp_data, "{\"url\": \"%s\", \"filename\": \"%s\"}",
			image_url ? image_url : "", filename);
	
	resp_json = json_response_data(HTTP_OK, "上传成功", resp_data);
	free(resp_data);

	ret = send_json_response(connection, MHD_HTTP_OK, resp_json);

// --- G.清理内存 ---
cleanup:
	if (fp) fclose(fp);
	free(filename);
	free(base64_data);
	free(decoded_raw);
	free(object_name);
	free(image_url);
	free(resp_json);
	return ret;
}

/*
	@brief 处理请求
	@param cls 用户数据
	@param connection 当前 http 连接对象 ( 用于获取请求信息和发送响应 )
	@param url 请求URL
	@param method 请求方法
	@param version http 协议版本
	@param upload_data POST上传的数据
	@param upload_data_size 上传数据大小
	@param con_cls 连接特定数据 ( 用于在多次回调调用之间保持状态，处理分块数据 )
	@return 成功返回 0, 失败返回非 0
*/
static enum MHD_Result handle_request(void *cls, struct MHD_Connection *connection,
						  const char *url, const char *method,
						  const char *version, const char *upload_data,
						  size_t *upload_data_size, void **con_cls)
{
	// 处理 CORS 预检请求
	if (strcmp(method, "OPTIONS") == 0)
		return handle_options(connection);

	// 请求上传文件接口
	if(strcmp(url, "/api/contacts/upload") == 0 && strcmp(method, "POST") == 0)
	{
		// 检查 Content-Type (防御性检查)
		// 确保客户端发送的是你期望的格式（JSON）。如果对方发的是一个巨大的二进制文件或恶意脚本，你可以直接拒绝，节省服务器解析成本。
		//const char *content_type = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Content-Type");
		//// 检查 content_type 是否存在, 再检查里面是否包含 "application/json"
		//if (!content_type || !strstr(content_type, "application/json"))
		//{
		//	char *resp_json = json_response_no_data(CONTENT_TYPE_ERROR, "仅支持 application/json 格式");
		//	int ret = send_json_response(connection, MHD_HTTP_UNSUPPORTED_MEDIA_TYPE, resp_json);
		//	free(resp_json);
		//	return ret;
		//}

		LOG_ERR("upload_data_size = %d", (int)*upload_data_size);
		LOG_ERR("upload_data = %s", upload_data); 

		int ret = collect_post_data(connection, upload_data, upload_data_size, con_cls);
		if (ret == MHD_YES && *upload_data_size == 0 && *con_cls != NULL)
		{
			// 此时 req_context->post_data 里存的是完整的 json 字符串（包含 Base64 图片）
			//LOG_INFO("接收到完整 json: %s", ((RequestContext *)*con_cls)->post_data);

			// 处理 Base64 编码的图片上传, 解析 json、解码 Base64、保存文件
			return handle_upload_base64_image(connection, (RequestContext *)*con_cls);
		}
		return ret;
	}
	// 请求联系人接口
	if (strncmp(url, "/api/contacts", 13) == 0)
	{
		if (strcmp(method, "GET") == 0)
		{
			if (strlen(url) == 13)  // 如果正好是 13 位，说明 URL 是 "/api/contacts"
			{
				return handle_get_all_contacts(connection);
			}

			if (url[13] == '/')  // 如果长度大于 13，且第 13 位是 '/'，说明是 "/api/contacts/{id}"
			{
				return handle_get_contact(connection, url);
			}
		}
			

		if (strcmp(method, "DELETE") == 0)
			return handle_delete_contact(connection, url);
		
		if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0)
		{
			int ret = collect_post_data(connection, upload_data, upload_data_size, con_cls);
			if (ret == MHD_YES && *upload_data_size == 0 && *con_cls != NULL)
			{
				// 此时 req_context->post_data 里存的是完整的 json 字符串
				LOG_INFO("接收到完整 json: %s", ((RequestContext *)*con_cls)->post_data);
				return handle_save_contact(connection, (RequestContext *)*con_cls, strcmp(method, "PUT") == 0);
			}
			return ret;
		}
	}

	// 未找到路由
	char *resp_json = json_response_no_data(ERROR_URL, "未找到路由");
	int ret = send_json_response(connection, MHD_HTTP_NOT_FOUND, resp_json);
	free(resp_json);
	return ret;
}

// 启动 http 服务
int http_server_start(int port)
{
	if (server.running)
	{
		LOG_ERR("http 服务已经在运行");
		return HTTP_OK;
	}

	contact_llist = llist_create(sizeof(Contact));  // 创建链表
	if (!contact_llist)
	{
		LOG_ERR("创建链表失败");
		return LLIST_CREATE_FAILED;
	}


	// 初始化数据库
	if (db_init() != 0)
	{
		LOG_ERR("数据库初始化失败");
		llist_destroy(contact_llist);
		contact_llist = NULL;
		return DB_INIT_FAILED;
	}

	// 初始化 curl 库
	curl_global_init(CURL_GLOBAL_ALL);

	// 启动 http 服务器
	server.daemon = MHD_start_daemon(
		// 启动一个或多个内部线程来处理事件 | 使用高性能 Poll 模式, 使用系统底层的 poll() 函数而不是默认的 select()
		//MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_POLL, 
		MHD_USE_SELECT_INTERNALLY,  // 服务器行为标志, 单线程模式
		port,  // 监听端口
		NULL, NULL, // 第一个 NULL 接受连接策略回调 ( apc ), 第二个 NULL 传递给策略回调函数的 cls 参数 ( apc_cls )
		handle_request, NULL, // handle_request 请求处理回调 ( dh ), NULL 传递给请求处理回调函数的 cls 的参数 ( dh_cls )
		MHD_OPTION_NOTIFY_COMPLETED, request_completed_callback, NULL,  // 当请求结束时，调用我指定的 request_completed_callback 函数, 第一个参数 cls 传 NULL
		//MHD_OPTION_THREAD_POOL_SIZE, 10, // 开启 10 个预分配线程
		/*
			关了, 如果配置上数据表要加锁, 麻烦
			没有线程池时：如果有一个人正在上传大文件（比如你的 Base64 图片上传），整个服务器就会被"卡住"，其他人的请求（比如查询联系人列表）必须排队等他传完。
			设置线程池（Size 为 10）时：服务器会预先开好 10 个房间（线程）。哪怕有 3 个人在上传图片，剩下 7 个房间依然可以快速处理其他人的查询请求。
		
			这 10 个线程会被 MHD 循环利用（复用）。
			当请求进来时，MHD 会从池子里派出一个空闲线程去处理 handle_request 里的业务逻辑。
			处理完后，线程不会销毁，而是回到池子里等待下一个任务。
		*/
		MHD_OPTION_END  // 可变参数，用于额外选项 ( MHD_OPTION_END 可变参数列表的结束 )
	);
	if (!server.daemon)
	{
		LOG_ERR("启动 http 服务器失败");
		curl_global_cleanup();  // 清理 CURL 库
		db_close();  // 关闭数据库连接
		llist_destroy(contact_llist);  // 销毁链表
		contact_llist = NULL;  // 重置空指针
		return HTTP_SERVER_START_FAILED;
	}

	server.port = port;
	server.running = 1;  // 运行

	LOG_INFO("http 服务器运行成功");
	return HTTP_OK;
}

// 停止 http 服务
void http_server_stop(void)
{
	if (server.daemon)
	{
		MHD_stop_daemon(server.daemon);
		server.daemon = NULL;
		server.running = 0;
		LOG_INFO("http 服务已停止");
	}

	db_close();  // 关闭数据库连接
	LOG_INFO("数据库连接已关闭");

	curl_global_cleanup();  // 清理 curl 库
	LOG_INFO("curl 库已清理");

	llist_destroy(contact_llist);  // 销毁链表
	contact_llist = NULL;  // 重置空指针
	LOG_INFO("联系人链表已销毁");
}