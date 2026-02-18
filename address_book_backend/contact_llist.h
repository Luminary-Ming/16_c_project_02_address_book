#ifndef CONTACT_LLIST_H
#define CONTACT_LLIST_H

#include "contact.h"

#define HEADINSERT 0
#define TAILINSERT 1

typedef void (*llist_contact_print)(const void *);

enum llist_error  // 错误码
{
	LLIST_OK = 0,
	LLIST_ERR_MALLOC = -1,
	LLIST_ERR_PARAM = -2,
	LLIST_ERR_MODE = -3,
	LLIST_ERR_NOT_FOUNT = -4,
};

struct llist_node  // 数据节点结构体
{
	struct llist_node *prev;  // 前驱指针
	struct llist_node *next;  // 后继指针
	char contact[0];  // 标记联系人数据域首地址
};

typedef struct llist_head  // 头节点结构体
{
	int size;  // 联系人数据大小
	struct llist_node head;  // 头节点
	int count;  // 联系人数量
}LLIST;

LLIST *llist_create(int size);  // 创建链表
int llist_insert(LLIST *handler, const Contact *contact_data, int mode);  // 插入数据
void llist_display(LLIST *handler, llist_contact_print print);  // 遍历链表
void llist_destroy(LLIST *handler);  // 销毁链表
void llist_clear(LLIST *handler);  // 清空链表 (只保留头节点, 释放所有数据节点)
Contact *llist_find(LLIST *handler, int contact_id);  // 根据 id 查找联系人(编辑联系人数据)
int llist_update(LLIST *handler, const Contact *contact_data);  // 更新节点(更新联系人信息)
int llist_delete(LLIST *handler, int contact_id);  // 根据 id 删除节点( del=1 时删除节点 )
int llist_get_count(LLIST *handler);  // 获取节点数量 (联系人数量)
Contact *llist_get_at(LLIST *handler, int index);  // 根据下标获取节点中的联系人数据

#endif