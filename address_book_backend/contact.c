#include "contact.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// 判断字符串是不是空串或空白串
static int is_blank_str(const char* str)
{
	if (!str)
		return 1;
	for (int i = 0; str[i] != '\0'; i++)  // 遍历字符串
		// isspace() 检测当前字符是否是空白符 ( 空格, Tab, 换行, 回车 )
		if (!isspace(str[i]))  // 如果出现非空白符
			return 0;
	return 1;
}

// 判断字符串开头是否是 UTF-8 编码的汉字
static int is_utf8_chinese(const char *str)
{
	unsigned char first = (unsigned char)str[0];  // 取第一个字节

	// UTF-8 汉字是 3 字节编码，首字节范围：0xE4-0xE9
	// 示例 : const char *str = "张新明";  占 10 字节
	// 位置 : [0]   [1]   [2]   [3]   [4]   [5]   [6]   [7]   [8]   [9]
	// 汉字 : |-----张------|   |-----新------|   |-----明------|   '\0'
	// 字节 : 0xE5  0xBC  0xA0  0xE6  0x96  0xB0  0xE6  0x98  0x8E  0x00
	if (first >= 0xE4 && first <= 0xE9)
	{
		if (str[1] != '\0' && str[2] != '\0')
		{
			unsigned char second = (unsigned char)str[1];  // 取第二个字节
			unsigned char third = (unsigned char)str[2];   // 取第三个字节

			// 首字节范围：0xE4-0xE9, 后续字节以 10xx xxxx 开头, 即 ( 0x80-0xBF )
			// 0xC0 的二进制是 1100 0000，它作为一个掩码（mask），用于提取字节的前两位。
			if ((second & 0xC0) == 0x80 && (third & 0xC0) == 0x80)
				return 1;  // 是汉字
		}
	}
	return 0;  // 不是汉字
}

// 获取名字的首字母
char get_initial(const char *name)
{
	if (is_blank_str(name))  // 如果是空串, 空白串
		return '#';  // 空白返回 '#'

	while (isspace(*name))  // 跳过空白字符
		name++;

	if (isalpha(name[0]))  // 如果名字开头是字母
		return toupper(name[0]);  // 都返回大写字母

	if (is_utf8_chinese(name))  // 如果名字开头是汉字
	{
		// TODO : 随后实现汉字转拼音
		return 'A';  // 先返回 'A'
	}
	return '#';  // 其他字符都返回 '#'
}

// 打印联系人信息
void print_contact(Contact *contact)
{
	if (!contact)
	{
		printf("Contact is NULL\n");
		return;
	}

	printf("==================================================================================\n");
	printf("id = %d\n", contact->id);
	printf("name = %s\n", contact->name);
	printf("telephone = %s\n", contact->telephone);
	printf("email = %s\n", contact->email);
	printf("initial = %c\n", contact->initial);
	printf("image = %s\n", contact->image);
	printf("del = %d\n", contact->del);
	printf("==================================================================================\n");
}