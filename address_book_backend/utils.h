#ifndef UTILS_H
#define UTILS_H

#include "contact.h"
#include <stdlib.h>

// 联系人转 json 字符串
char *contact_to_json(Contact *contact);

// 联系人列表转 json 字符串
char *contacts_to_json(Contact **contact, int count);

// 返回响应的 json 字符串 (没有data)
char *json_response_no_data(const int code, const char *message);

// 返回响应的 json 字符串 (有data)
char *json_response_data(const int code, const char *message, const char *data);

// 解码 Base64 函数
unsigned char *base64_decode(const char *input, size_t *output_length);

// 在堆上申请一块内存, 从字符串 str 中拷贝最多 n 个字符, 自动在末尾添加 \0 终止符, 返回这块内存的地址
char *my_strndup(const char *str, size_t n);

// 处理文件名
char *handle_filename(const char *object_name);

// 解析 POST/PUT 请求中的 json 数据
Contact *parse_contact_from_json(const char *json_str);

#endif