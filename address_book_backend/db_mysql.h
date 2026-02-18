#ifndef DB_MYSQL_H
#define DB_MYSQL_H

#include "contact.h"
#include "contact_llist.h"

#define DB_HOST "192.168.2.227"   // 主机地址
#define DB_USER "jiahao"        // 用户名
#define DB_PASSWD "jiahao123"   // 密码
#define DB_NAME "address_book"  // 数据库名

enum db_error  // 错误码
{
	DB_OK = 0,
	DB_ERR_INIT = -1,
	DB_ERR_CONNECT = -2,
	DB_ERR_NOT_INIT = -3,
	DB_ERR_QUERY = -4,
	DB_ERR_RESULT = -5,
	DB_ERR_INSERT = -6,
	DB_ERR_UPDATE = -7,
	DB_ERR_DELETE = -8,
	DB_ERR_NOT_FOUND = -9,
};

int db_init(void);   // 初始化连接数据库
int db_close(void);  // 关闭数据库连接

int db_load_contacts(LLIST *handler);  // 查询联系人列表
int db_insert_contact(LLIST *handler, Contact *contact);  // 新增联系人
int db_update_contact(LLIST *handler, Contact *contact);  // 更新联系人
int db_delete_contact(LLIST *handler, int id);  // 根据 id 删除联系人
Contact *db_find_contact(LLIST *handler, int id);  // 根据 id 查找联系人

#endif