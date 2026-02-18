#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>  // uint32_t
#include <sys/time.h>  // struct timeval, gettimeofday
#include "logger.h"

// 联系人转 json 字符串
char *contact_to_json(Contact *contact)
{
	if (!contact)
	{
		LOG_ERR("contact is NULL");
		return NULL;
	}

	// snprintf 返回格式化后字符串的长度 (不包括 '\0')
	// 当第一个参数是 NULL，第二个参数是 0 时
	// snprintf 不实际写入任何内容
	// 但它仍然计算格式化后的字符串长度
	// 返回的是理论上需要的缓冲区大小
	int needed = snprintf(NULL, 0,
					   "{\"id\": %d, \"name\": \"%s\", \"telephone\": \"%s\", "
					   "\"email\": \"%s\", \"initial\": \"%c\", \"image\": \"%s\", \"del\": %d}",
					   contact->id, contact->name, contact->telephone, contact->email, contact->initial, contact->image, contact->del);
	if (needed < 0)
	{
		LOG_ERR("snprintf failed");
		return NULL;
	}

	char *json = malloc(needed + 1);  // +1 给 '\0'
	if (!json)
	{
		LOG_ERR("malloc failed");
		return NULL;
	}

	// 使用 sprintf (因为已经确保缓冲区足够大了)
	sprintf(json,
			 "{\"id\": %d, \"name\": \"%s\", \"telephone\": \"%s\", "
			 "\"email\": \"%s\", \"initial\": \"%c\", \"image\": \"%s\", \"del\": %d}",
			 contact->id, contact->name, contact->telephone, contact->email, contact->initial, contact->image, contact->del);

	return json;
}

// 联系人列表转 json 字符串
char *contacts_to_json(Contact **contacts, int count)
{
	if (!contacts || count <= 0)
	{
		char *empty = malloc(3);  // "[]" + '\0' = 3字节
		if (!empty)
		{
			LOG_ERR("malloc failed");
			return NULL;
		}
		strcpy(empty, "[]");  // 复制包括尾零
		return empty;
	}

	// 分配指针数组，用于存储每个联系人的 json 字符串指针
	char **contacts_json = malloc(count * sizeof(char *));
	if (!contacts_json)
	{
		LOG_ERR("malloc failed");
		return NULL;
	}

	size_t total_size = 3;  // 计算所需空间大小, 初始为3 ("[]" + '\0')
	for (int i = 0; i < count; i++)  // 遍历所有联系人，计算生成 json 字符串的总大小
	{
		contacts_json[i] = contact_to_json(contacts[i]);  // 将单个联系人转换为 json 字符串
		if (!contacts_json[i])
		{
			LOG_ERR("第 %d 个联系人转换为 json 字符串失败", i);
			for (int j = 0; j < i; j++)
				free(contacts_json[j]);  // 释放已经转换成功的 json
			free(contacts_json);  // 释放指针数组
			return NULL;
		}

		total_size += strlen(contacts_json[i]);  // 累加当前联系人的 json 字符串长度
		if (i != count - 1)  // 如果不是最后一个，需要加逗号分隔符
			total_size += 1;
	}

	// 分配最终的 json 字符串内存
	char *json = malloc(total_size + 1);  // +1 是'\0'
	if (!json)
	{
		LOG_ERR("malloc failed");
		for (int i = 0; i < count; i++)
			free(contacts_json[i]);
		free(contacts_json);
		return NULL;
	}

	// 构建 json 数组
	strcpy(json, "[");
	for (int i = 0; i < count; i++)
	{
		strcat(json, contacts_json[i]);  // 追加当前联系人的 json

		if (i != count - 1)  // 如果不是最后一个，需要加逗号
			strcat(json, ",");

		free(contacts_json[i]);  // 释放单个联系人的 json
	}
	strcat(json, "]");  // 追加完成后自动添加新的尾零

	free(contacts_json);  // 释放指针数组
	return json;
}

// 返回响应的 json 字符串 (没有data)
char *json_response_no_data(const int code, const char *message)
{
	if (!message) message = "";

	// 计算所需空间
	int needed = snprintf(NULL, 0,
						  "{\"code\":%d,\"message\":\"%s\"}",
						  code, message);
	if (needed < 0)
	{
		LOG_ERR("snprintf failed");
		return NULL;
	}

	// 分配最终的 json 字符串内存
	char *json = malloc(needed + 1);  // +1 是'\0'
	if (!json)
	{
		LOG_ERR("malloc failed");
		return NULL;
	}

	sprintf(json,
			"{\"code\":%d,\"message\":\"%s\"}",
			code, message);

	return json;
}

// 返回响应的 json 字符串 (有data)
char *json_response_data(const int code, const char *message, const char *data)
{
	if (!message) message = "";
	if (!data) data = "";

	// 计算所需空间
	int needed = snprintf(NULL, 0,
						  "{\"code\":%d,\"message\":\"%s\",\"data\":%s}",
						  code, message, data);
	if (needed < 0)
	{
		LOG_ERR("snprintf failed");
		return NULL;
	}

	// 分配最终的 json 字符串内存
	char *json = malloc(needed + 1);  // +1 是'\0'
	if (!json)
	{
		LOG_ERR("malloc failed");
		return NULL;
	}

	sprintf(json,
			"{\"code\":%d,\"message\":\"%s\",\"data\":%s}",
			code, message, data);

	return json;
}

/*
	@brief 解码 Base64 函数
	@param input 输入的 Base64 字符串（json 中提取出来的 data 部分）
	@param output_length 解码后的二进制数据大小（字节）
	@return unsigned char* 返回堆内存指针，使用完毕后必须 free，失败返回 NULL
 */
unsigned char *base64_decode(const char *input, size_t *output_length)
{
	if (!input || !output_length)
		return NULL;

	/*
		完整的静态查找表：覆盖 256 个 ASCII 字符
		Base64 的标准字符集是：A - Z(0 - 25), a - z(26 - 51), 0 - 9 (52 - 61), +(62), / (63)
		
		这个表的作用是将 ASCII 码 转换回 Base64 索引值：
		数字 0-9：
			在 ASCII 表中， 字符 '0' 的值是 48
			在 Base64 表中，数字 '0' 代表的值是 52
			所以 table[48]（第 4 组第 1 个）的值正好是 52
		大写字母 A-Z：
			字符 'A' 的 ASCII 是 65
			Base64 索引是 0
			所以 table[65] 是 0
		符号 + 和 /：
			'+' 的 ASCII 是 43，所以 table[43] 是 62
			'/' 的 ASCII 是 47，所以 table[47] 是 63

		0xFF 二进制是 11111111（十进制 255）我们在代码里把它规定为非法 Base64 字符（如换行、空格等）
		因为 Base64 真正有意义的索引只有 0 到 63
		如果你输入的字符串里包含空格、回车、或者像 $、# 这种不在 Base64 字符集里的东西，
		程序查表查到 0xFF，立刻就知道："噢！这不是有效的 Base64 数据"，然后就可以报错返回，防止解析出乱码。
	*/
	static const unsigned char table[256] = {
		// 16 * 16
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 62,   0xFF, 0xFF, 0xFF, 63,
		52,   53,   54,   55,   56,   57,   58,   59,   60,   61,   0xFF, 0xFF, 0xFF, 0,    0xFF, 0xFF,
		0xFF, 0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    10,   11,   12,   13,   14,
		15,   16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
		41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};

	size_t input_len = strlen(input);
	// Base64 标准要求长度必须是 4 的倍数（包含填充符 '='）
	if (input_len == 0 || input_len % 4 != 0)
		return NULL;
	/*
		Base64 的核心逻辑是：把 3 个字节（24 位）转换成 4 个 Base64 字符（每 6 位二进制转成一个 Base64 字符）
		但现实中，你上传的图片字节数不一定总能被 3 整除。这时候就会出现余数：
		情况 A：余数为 0 字节 : 3 字节 -> 4 字符。刚好凑齐，没有等号。
		情况 B：余数为 2 字节 : 还差 1 字节凑够一组。Base64 会在末尾补 0，然后用 1 个等号表示补位。
		情况 C：余数为 1 字节 : 还差 2 字节凑够一组。Base64 会补更多的 0，然后用 2 个等号表示补位。
	*/
	// 预估输出长度：每 4 个 Base64 字符转为 3 字节二进制
	size_t out_len = input_len / 4 * 3;
	// 根据末尾的等号个数修正实际长度 (最多有两个等号)
	if (input[input_len - 1] == '=') out_len--;
	if (input[input_len - 2] == '=') out_len--;

	*output_length = out_len;  //  存入解码后的二进制数据大小

	// 分配合适的内存
	unsigned char *output = malloc(out_len + 1);
	if (!output) return NULL;

	// 解码主循环, 每次处理 4 个 6 bit 的字符 (Base64), 重新拼成 3 个 8 bit 的字节
	for (size_t i = 0, j = 0; i < input_len; i += 4)
	{
		/*
			第一步：查字典
			(unsigned char) 强制转换是为了防止字符值为负导致数组越界

			使用 uint32_t 处理固定位宽的数据, 在 Base64 解码中, 我们要处理的是确定的 24 位
			用一个明确知道是 32 位的容器来装这 24 位，代码的可读性非常强, 别人一看就知道你在玩位运算

			例: 字符 Q 的 Base64 编码是 UQ==
			Q 的 ASCII 是 81, 转为二进制是 0101 0001
			重组分组, 6位一组, 后面不足 6 位, 补 0 
				第一组: 010100 (十进制 20)
				第二组: 010000 (补 4 个 0, 十进制 16)
			查 Base64 索引表 (A-Z 0-25): 
				索引 20 对应字符 U
				索引 16 对应字符 Q
			处理等号:
				标准是 3字节转 4个 base64 字符, 所以再补 12 个 0
				010100 010000 000000 000000
				U      Q      =      =
			所以字符 Q 的 Base64 编码是 UQ==
		*/
		uint32_t a = table[(unsigned char)input[i]];
		uint32_t b = table[(unsigned char)input[i + 1]];
		uint32_t c = table[(unsigned char)input[i + 2]];
		uint32_t d = table[(unsigned char)input[i + 3]];

		/*
			第二步：安全性检查
			Base64 编码后至少有两个字符 (再补两个等号就凑成 4 字符了)
			所以如果检查前两个字符都是 0xFF (非法字符), 说明这串数据坏了，直接释放内存并退出
		*/
		if (a == 0xFF || b == 0xFF)
		{
			free(output);
			return NULL;
		}

		/*
			第三步：拼装
			把 4 个 6 bit 的二进制，塞进一个 32 位的"大抽屉" triple 里
			a 左移 18 位, b 左移 12 位, c 左移 6 位, d 不动
			010100 010000 000000 000000
		*/
		uint32_t triple = (a << 18) | (b << 12) | ((c == 0xFF ? 0 : c) << 6) | (d == 0xFF ? 0 : d);

		/*
			第四步：拆分
			现在 triple 里存的是：[8位][8位][8位][8位] 的连续二进制
			00000000 01010001 00000000 00000000
			需要把它们一个个"抠"出来存进输出数组 output 里
		*/

		// 取出最高 8 位（16-23位），存入第 1 个字节
		if (j < out_len)
			output[j++] = (triple >> 16) & 0xFF;
		// 取出中间 8 位（8-15位），存入第 2 个字节
		if (j < out_len)
			output[j++] = (triple >> 8) & 0xFF;
		// 取出最低 8 位（0-7位），存入第 3 个字节
		if (j < out_len)
			output[j++] = triple & 0xFF;	
	}

	return output;
}

// 在堆上申请一块内存, 从字符串 str 中拷贝最多 n 个字符, 自动在末尾添加 \0 终止符, 返回这块内存的地址
char *my_strndup(const char *str, size_t n)
{
	// 确定实际需要拷贝的长度
	size_t len = 0;
	while (len < n && str[len] != '\0')
		len++;

	// 申请内存（数据长度 + 1个尾零）
	char *new_str = malloc(len + 1);
	if (!new_str)
		return NULL;

	memcpy(new_str, str, len);
	new_str[len] = '\0';
	return new_str;
}

// 处理文件名
char *handle_filename(const char *object_name)
{
	if (!object_name || strlen(object_name) == 0)
		return  NULL;

	// 获取当前时间戳(秒数)
	// time_t timestamp = time(NULL);
	struct timeval tv;  // 时间结构体(秒, 微秒)
	gettimeofday(&tv, NULL);  // 获取当前时间, 赋值给 tv 结构体
	long timestamp = (long)tv.tv_sec;

	// 文件名拼上时间戳
	const char *dot = strrchr(object_name, '.');  // 文件名 '.' 的位置

	char prefix[128] = { 0 };  // 文件名前缀
	int len = dot - object_name;  // 前缀长度
	strncpy(prefix, object_name, len);
	prefix[len] = '\0';

	char *new_name = malloc(256);  // 存储新名字
	if (!new_name) return NULL;
	snprintf(new_name, 256, "%s_%ld.%s", prefix, timestamp, dot + 1);

	return new_name;
}

// 解析 POST/PUT 请求中的 json 数据
Contact *parse_contact_from_json(const char *json_str)
{
	Contact *contact = malloc(sizeof(Contact));
	if (!contact)
		return NULL;
	memset(contact, 0, sizeof(Contact));

	LOG_ERR("json_str = %s", json_str);
	char *id_str = strstr(json_str, "\"id\":");
	LOG_ERR("id_str = %s", id_str);
	if (id_str)
		contact->id = atoi(id_str + 5);

	char *name_str = strstr(json_str, "\"name\":\"");
	LOG_ERR("name_str = %s", name_str);
	if (id_str)
	{
		name_str += 8;
		char *name_end = strchr(name_str, '"');
		if (name_end)
			strncpy(contact->name, name_str, name_end - name_str);
	}

	char *telephone_str = strstr(json_str, "\"telephone\":\"");
	if (id_str)
	{
		telephone_str += 8;
		char *telephone_end = strchr(telephone_str, '"');
		if (telephone_end)
			strncpy(contact->name, telephone_str, telephone_end - telephone_str);
	}

	char *email_str = strstr(json_str, "\"email\":\"");
	if (id_str)
	{
		email_str += 8;
		char *email_end = strchr(email_str, '"');
		if (email_end)
			strncpy(contact->name, email_str, email_end - email_str);
	}

	char *image_str = strstr(json_str, "\"image\":\"");
	if (image_str)
	{
		image_str += 9;
		char *image_end = strchr(image_str, '"');
		if (image_end) strncpy(contact->image, image_str, image_end - image_str);
	}

	return contact;
}