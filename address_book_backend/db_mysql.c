#include "db_mysql.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mysql/mysql.h>


static MYSQL *db_conn = NULL;  // 数据库连接指针

// 初始化连接数据库
int db_init(void)
{
	// 检查数据库是否已经初始化
	if (db_conn != NULL)
	{
		LOG_INFO("数据库已经初始化");
		return DB_OK;
	}

	// 初始化 mysql 连接指针 ( 参数为 NULL 表示由函数自动分配内存 )
	db_conn = mysql_init(NULL);
	if (!db_conn)
	{
		LOG_ERR("mysql_init failed");
		return DB_ERR_INIT;
	}

	// 在建立连接之前设置字符集选项
	mysql_options(db_conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");

	// 设置一个 3 秒超时，连不上直接报错
	unsigned int timeout = 3;
	mysql_options(db_conn, MYSQL_OPT_CONNECT_TIMEOUT, (const void *)&timeout);

	// 建立数据库连接 ( 后三个参数 端口(0默认)、Unix socket(NULL默认)、客户端标志(0默认) )
	if (!mysql_real_connect(db_conn, DB_HOST, DB_USER, DB_PASSWD, DB_NAME, 0, NULL, 0))
	{
		LOG_ERR("mysql_real_connect failed: %s", mysql_error(db_conn));
		mysql_close(db_conn);  // 关闭连接
		db_conn = NULL;  // 置空指针
		return DB_ERR_CONNECT;
	}

	// 再次设置字符集（双重保障）
	if (mysql_set_character_set(db_conn, "utf8mb4"))
	{
		LOG_ERR("mysql_set_character_set failed: %s", mysql_error(db_conn));
		// 不返回错误, 可以继续执行，尝试使用默认字符集
	}

	LOG_INFO("数据库连接 OK, 字符集: %s", mysql_character_set_name(db_conn));

	return DB_OK;
}

// 关闭数据库连接
int db_close()
{
	if (!db_conn)
	{
		LOG_ERR("数据库连接未初始化");
		return DB_ERR_NOT_INIT;  // db_conn 没有初始化
	}

	mysql_close(db_conn);
	db_conn = NULL;
	return DB_OK;
}

// 查询联系人列表
int db_load_contacts(LLIST *handler)
{
	if (!db_conn || !handler)
	{
		LOG_ERR("数据库未初始化或handler为空");
		return DB_ERR_NOT_INIT;
	}

	const char *sql = "select id,name,telephone,email,initial,image,del from contacts where del = 0";

	LOG_INFO("执行SQL: %s", sql);

	// 执行 sql
	if (mysql_query(db_conn, sql) != 0)
	{
		LOG_ERR("查询联系人失败: %s", mysql_error(db_conn));
		return DB_ERR_QUERY;
	}

	// 获取结果集
	MYSQL_RES *result = mysql_store_result(db_conn);
	if (!result)
	{
		LOG_ERR("获取结果集失败: %s", mysql_error(db_conn));
		return DB_ERR_RESULT;
	}

	// 每次获取查询结果前清空链表
	llist_clear(handler);

	// 遍历结果并添加到链表
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))  // 从结果集中循环获取每一行，直到返回NULL
	{
		Contact contact;

		contact.id = atoi(row[0]);

		strncpy(contact.name, row[1] ? row[1] : "", NAMESIZE - 1);
		contact.name[NAMESIZE - 1] = '\0';
		
		strncpy(contact.telephone, row[2] ? row[2] : "", TELEPHONESIZE - 1);
		contact.telephone[TELEPHONESIZE - 1] = '\0';

		strncpy(contact.email, row[3] ? row[3] : "", EMAILSIZE - 1);
		contact.email[EMAILSIZE - 1] = '\0';

		contact.initial = row[4] ? row[4][0] : '#';

		strncpy(contact.image, row[5] ? row[5] : "", IMAGESIZE - 1);
		contact.image[IMAGESIZE - 1] = '\0';

		contact.del = atoi(row[6]);

		if (llist_insert(handler, &contact, TAILINSERT) != 0)  // 查询的联系人存到链表中（保持数据库表和链表数据一致）
			LOG_ERR("插入链表失败，数据不一致！id: %d", contact.id);

	}
	mysql_free_result(result);
	LOG_INFO("从数据库加载了 %d 个联系人", llist_get_count(handler));
	return DB_OK;
}

// 新增联系人
int db_insert_contact(LLIST *handler, Contact *contact)
{
	if (!db_conn || !handler || !contact)
	{
		LOG_ERR("数据库未初始化或新增联系人数据为空");
		return DB_ERR_NOT_INIT;
	}

	contact->del = 0;
	contact->initial = get_initial(contact->name);

	char sql[1024];
	snprintf(sql, sizeof(sql),
			 "insert into contacts (name, telephone, email, initial, image, del) "
			 "values ('%s', '%s', '%s', '%c', '%s', %d)",
			 contact->name, contact->telephone, contact->email, contact->initial, contact->image, contact->del);

	LOG_INFO("执行SQL: %s", sql);

	// 执行 sql
	if (mysql_query(db_conn, sql) != 0)
	{
		LOG_ERR("插入联系人失败: %s", mysql_error(db_conn));
		return DB_ERR_INSERT;
	}

	contact->id = (int)mysql_insert_id(db_conn);  // 获取插入成功的 ID

	if (llist_insert(handler, contact, TAILINSERT) != 0)  // 插入链表（保持数据库表和链表数据一致）
		LOG_ERR("插入链表失败，数据不一致！id: %d", contact->id);
	

	LOG_INFO("添加联系人成功，id: %d\n", contact->id);
	print_contact(contact);

	return DB_OK;
}

// 更新联系人
int db_update_contact(LLIST *handler, Contact *contact)
{
	if (!db_conn || !handler || !contact)
	{
		LOG_ERR("数据库未初始化或新增联系人数据为空");
		return DB_ERR_NOT_INIT;
	}

	contact->initial = get_initial(contact->name);  // 如果名字改了，首字母重新计算

	char sql[1024];
	snprintf(sql, sizeof(sql),
			 "update contacts set name='%s', telephone='%s', email='%s', initial='%c', image='%s' "
			 "where id=%d and del=0",
			 contact->name, contact->telephone, contact->email, contact->initial, contact->image, contact->id);

	LOG_INFO("执行SQL: %s", sql);

	// 执行 sql
	if (mysql_query(db_conn, sql) != 0)
	{
		LOG_ERR("更新联系人失败: %s", mysql_error(db_conn));
		return DB_ERR_UPDATE;
	}

	// mysql_affected_rows() 返回实际被修改的行数
	if (mysql_affected_rows(db_conn) == 0)
	{
		LOG_ERR("未找到联系人，id: %d\n", contact->id);
		return DB_ERR_NOT_FOUND;
	}

	llist_update(handler, contact);  // 更新链表（保持数据库表和链表数据一致）

	LOG_INFO("更新联系人成功，id: %d\n", contact->id);
	print_contact(contact);

	return DB_OK;
}

// 根据 id 删除联系人
int db_delete_contact(LLIST *handler, int id)
{
	if (!db_conn || !handler || id <= 0)
	{
		LOG_ERR("数据库未初始化或 id 不合法");
		return DB_ERR_NOT_INIT;
	}

	char sql[1024];
	snprintf(sql, sizeof(sql),
			 "update contacts set del=1 where id=%d", id);

	LOG_INFO("执行SQL: %s", sql);

	// 执行 sql
	if (mysql_query(db_conn, sql) != 0)
	{
		LOG_ERR("删除联系人失败: %s", mysql_error(db_conn));
		return DB_ERR_DELETE;
	}

	// mysql_affected_rows() 返回实际被修改的行数
	if (mysql_affected_rows(db_conn) == 0)
	{
		LOG_ERR("未找到联系人，id: %d\n", id);
		return DB_ERR_NOT_FOUND;
	}

	if (llist_delete(handler, id) != 0)  // 删除节点（保持数据库表和链表数据一致）
		LOG_ERR("删除节点失败，数据不一致！id: %d", id);

	LOG_INFO("删除联系人成功，id: %d\n", id);

	return DB_OK;
}

// 根据 id 查找联系人
Contact *db_find_contact(LLIST *handler, int id)
{
	if (!db_conn || !handler || id <= 0)
	{
		LOG_ERR("数据库未初始化或 id 不合法");
		return NULL;
	}

	char sql[1024];
	snprintf(sql, sizeof(sql),
			 "select id,name,telephone,email,initial,image,del from contacts where id=%d and del=0", id);

	LOG_INFO("执行SQL: %s", sql);

	// 执行 sql
	if (mysql_query(db_conn, sql) != 0)
	{
		LOG_ERR("查询联系人失败: %s", mysql_error(db_conn));
		return NULL;
	}

	// 获取结果集
	MYSQL_RES *result = mysql_store_result(db_conn);
	if (!result)
	{
		LOG_ERR("获取结果集失败: %s", mysql_error(db_conn));
		return NULL;
	}

	MYSQL_ROW row = mysql_fetch_row(result);
	if (!row)
	{
		mysql_free_result(result);
		return NULL;
	}

	Contact *contact = malloc(sizeof(Contact));
	if (!contact)
	{
		mysql_free_result(result);
		LOG_ERR("malloc failed: %s", strerror(errno));
		return NULL;
	}
	memset(contact, 0, sizeof(Contact));

	contact->id = atoi(row[0]);

	strncpy(contact->name, row[1] ? row[1] : "", NAMESIZE - 1);
	contact->name[NAMESIZE - 1] = '\0';

	strncpy(contact->telephone, row[2] ? row[2] : "", TELEPHONESIZE - 1);
	contact->telephone[TELEPHONESIZE - 1] = '\0';

	strncpy(contact->email, row[3] ? row[3] : "", EMAILSIZE - 1);
	contact->email[EMAILSIZE - 1] = '\0';

	contact->initial = row[4] ? row[4][0] : '#';

	strncpy(contact->image, row[5] ? row[5] : "", IMAGESIZE - 1);
	contact->image[IMAGESIZE - 1] = '\0';

	contact->del = atoi(row[6]);

	mysql_free_result(result);

	LOG_INFO("找到联系人: id=%d, name=%s", contact->id, contact->name);
	print_contact(contact);

	return contact;
}