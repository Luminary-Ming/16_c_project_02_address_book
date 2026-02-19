#include "contact.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <iconv.h>  // 引入系统字符集转换库（用于 UTF-8 转 GBK）
#include "logger.h"

// 定义结构体, 用于建立 GBK 编码数值与拼音首字母的映射关系
typedef struct
{
	unsigned short code;  // GBK 编码的起始阈值（16位无符号整数）
	char initial;  // 该编码区间对应的拼音首字母
}GBK_Index;

// 常用汉字在 GBK 中的首字母边界表 (GBK 编码)
// 原理：GBK 编码中，一级汉字（常用字）是按拼音字母顺序排列的
// 只要一个汉字的 GBK 编码数值落在 0xB0A1 和 0xB0C4 之间，那么这个字百分之百是以 A 开头的
static GBK_Index gbk_table[] =
{
	{0xB0A1, 'A'}, {0xB0C5, 'B'}, {0xB2C1, 'C'}, {0xB4EE, 'D'}, {0xB6EA, 'E'},
	{0xB7A2, 'F'}, {0xB8C1, 'G'}, {0xB9FE, 'H'}, {0xBBF7, 'J'}, {0xBFA6, 'K'},
	{0xC0AC, 'L'}, {0xC2E8, 'M'}, {0xC4C3, 'N'}, {0xC5B6, 'O'}, {0xC5BE, 'P'},
	{0xC6DA, 'Q'}, {0xC8BB, 'R'}, {0xC8F6, 'S'}, {0xCBFA, 'T'}, {0xCDDA, 'W'},
	{0xCEF4, 'X'}, {0xD1B9, 'Y'}, {0xD4D1, 'Z'}
};

// UTF-8 字符转 GBK 编码值
static unsigned short get_gbk_code(const char *utf8_char)
{
	// 打开一个从 UTF-8 转换到 GBK 的转换描述符
	iconv_t cd = iconv_open("GBK", "UTF-8");
	if (cd == (iconv_t)-1)
		return 0;

	char *inbuf = (char *)utf8_char; // 输入缓冲区指针，指向原始 UTF-8 字符串
	size_t inlen = 3;  // utf-8 汉字占 3 字节

	unsigned short gbk_val = 0;  // 存储转换后的 GBK 编码
	char *outbuf = (char *)&gbk_val;  // 输出缓冲区指针，指向 gbk_val 的地址
	size_t outlen = 2;  // gbk 汉字占 2 字节

	// 调用 iconv 执行实际的编码转换
	if (iconv(cd, &inbuf, &inlen, &outbuf, &outlen) == (size_t)-1)
	{
		iconv_close(cd);  // 转换失败，关闭描述符释放资源
		return 0;
	}
	iconv_close(cd);  // 转换成功，关闭描述符释放资源

	LOG_ERR("gbk_val = %hu", gbk_val);  // gbk_val = 41392 (0xA1B0) (在大端机器上这个值就是 45217 (0xB0A1))
	
	/*
		unsigned short gbk_val = 41392; (十六进制 0xA1B0)
		没有开辟, 没有用 static 修饰, 所以 gbk_val 和 gbk_val 的值(41392)都是存在内存的栈中
		栈竖着看是上面是高地址,下面是低地址的
		栈躺着看是前面是低地址, 后面是高地址

		字节序处理:
		iconv 这个函数很特殊, 它不管你的 CPU 是什么序，它往内存里填 GBK 编码时，永远是按顺序填的。
		比如 "啊" 的编码是 0xB0A1, 
		于是它永远是按顺序填：
		内存地址样子: 低 -> 高
			小端机器: 0xB0  0xA1  -> 组合起来 0xA1B0 (41392)
			大端机器: 0xB0  0xA1  -> 组合起来 0xB0A1 (45217)

		所以当执行：return (p[0] << 8) | p[1];
		p[0] (0xB0) 左移 8 位 -> 变成 0xB000
		或上 p[1] (0xA1) -> 变成 0xB0A1
		最终结果：无论在什么电脑上，这个函数都会准确地返回数值 0xB0A1

		unsigned char *p = (unsigned char *)&gbk_val;
		p 指针每次访问 8 位
		p 指向 0xB0, p+1 指向 0xA1
		在 C 语言中，p[n] 等价于 *(p + n) 
		所以 p[0] = 0xB0, p[1] = 0xA1
	*/
	unsigned char *p = (unsigned char *)&gbk_val;
	return (p[0] << 8) | p[1];
}

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

	if (is_utf8_chinese(name))  // 如果名字开头是 UTF-8 编码汉字
	{
		// 调用转换函数，获取该汉字的 GBK 编码数值
		unsigned short code = get_gbk_code(name);

		// 边界检查：0xB0A1 是"啊"(A) 的开始，0xD7F9 是常用汉字的结尾
		// 如果超出了这个范围，说明是非规范汉字或生僻字，返回 #
		if (code < 0xB0A1 || code > 0xD7F9) return '#';

		// 核心逻辑：倒序遍历映射表
		// 原理：如果当前编码值 >= 某个字母的起始码，它就属于该字母区间
		// 例如：'啊' (0xB0A1) <= 你的字 < '八' (0xB0C5)，则你的字首字母是 'A'
		for (int i = 22; i >= 0; i--)
		{
			if (code >= gbk_table[i].code)
			{
				return gbk_table[i].initial;
			}
		}
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