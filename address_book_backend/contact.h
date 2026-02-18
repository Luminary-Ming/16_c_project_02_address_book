#ifndef CONTACT_H
#define CONTACT_H

#define NAMESIZE 100
#define TELEPHONESIZE 20
#define EMAILSIZE 100
#define IMAGESIZE 255

// 联系人结构体
typedef struct contact  
{
	int id;  // ID
	char name[NAMESIZE];  // 姓名
	char telephone[TELEPHONESIZE];  // 手机号
	char email[EMAILSIZE];  // 邮箱
	char initial;  // 名字首字母
	char image[IMAGESIZE];  // 头像 URL
	int del;  // 删除标识 1已删除 0未删除
}Contact;

char get_initial(const char *name);  // 获取名字的首字母
void print_contact(Contact *contact);  // 打印联系人信息

#endif