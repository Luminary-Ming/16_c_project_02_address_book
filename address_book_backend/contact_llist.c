#include "contact_llist.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// 创建链表
LLIST *llist_create(int size)
{
	LLIST *handler = malloc(sizeof(LLIST));
	if (!handler)
	{
		LOG_ERR("malloc failed: %s", strerror(errno));
		return NULL;
	}

	handler->size = size;
	handler->head.prev = &handler->head;
	handler->head.next = &handler->head;
	handler->count = 0;  // 初始联系人数量为 0

	return handler;
}

// 插入数据
int llist_insert(LLIST *handler, const Contact *contact_data, int mode)
{
	if (!handler || !contact_data)
		return LLIST_ERR_PARAM;

	struct llist_node *newnode = malloc(sizeof(struct llist_node) + handler->size);
	if (!newnode)
	{
		LOG_ERR("malloc failed: %s", strerror(errno));
		return LLIST_ERR_MALLOC;
	}

	memcpy(newnode->contact, contact_data, handler->size);

	struct llist_node *p = &handler->head;
	switch (mode)
	{
		case HEADINSERT: break;
		case TAILINSERT: p = p->prev; break;
		default:
			free(newnode);
			LOG_ERR("mode failed: %s", strerror(errno));
			return LLIST_ERR_MODE;
	}
	newnode->prev = p;
	newnode->next = p->next;
	newnode->prev->next = newnode;
	newnode->next->prev = newnode;
	handler->count++;  // 联系人数量 +1

	return LLIST_OK;
}

// 遍历链表 (用不到, 不会打印所有联系人数据, 也就更新数据的时候打印当前联系人的信息)
void llist_display(LLIST *handler, llist_contact_print print)
{
	if (!handler)
		return;

	LOG_INFO("联系人数据: ");
	printf("联系人数量: %d 个\n", handler->count);
	printf("====================\n");

	struct llist_node *cur = handler->head.next;
	while (cur != &handler->head)
	{
		Contact *contact = (Contact *)cur->contact;
		if (contact->del == 0)  // 只打印未删除的联系人数据
			print(contact);
		cur = cur->next;
	}
}

// 销毁链表
void llist_destroy(LLIST *handler)
{
	if (!handler)
		return;

	struct llist_node *cur = handler->head.next;
	while (cur != &handler->head)
	{
		cur->prev->next = cur->next;
		cur->next->prev = cur->prev;
		free(cur);
		cur = handler->head.next;
	}
	free(handler);  // 释放头节点
}

// 清空链表 (只保留头节点, 释放所有数据节点)
void llist_clear(LLIST *handler)
{
	if (!handler)
		return;

	struct llist_node *cur = handler->head.next;
	while (cur != &handler->head)
	{
		cur->prev->next = cur->next;
		cur->next->prev = cur->prev;
		free(cur);
		cur = handler->head.next;
	}

	handler->head.prev = &handler->head;
	handler->head.next = &handler->head;
	handler->count = 0;  // 联系人数量清零
}

// 查找数据节点
static struct llist_node *_find(LLIST *handler, int contact_id)
{
	if (!handler || contact_id <= 0)
		return NULL;

	struct llist_node *cur = handler->head.next;
	while (cur != &handler->head)
	{
		Contact *contact = (Contact *)cur->contact;
		if (contact->id == contact_id && contact->del == 0)  // id正确并且未被删除
			return cur;
		cur = cur->next;
	}

	return NULL;
}

// 根据 id 查找联系人(编辑联系人数据)
Contact *llist_find(LLIST *handler, int contact_id)
{
	struct llist_node *cur = _find(handler, contact_id);
	if (!cur)
		return NULL;

	 return (Contact *)cur->contact;
}

// 更新节点(更新联系人信息)
int llist_update(LLIST *handler, const Contact *contact_data)
{
	if (!handler || !contact_data)
		return LLIST_ERR_PARAM;
	
	struct llist_node *cur = _find(handler, contact_data->id);
	if (!cur)
		return LLIST_ERR_NOT_FOUNT;

	memcpy(cur->contact, contact_data, handler->size);

	return LLIST_OK;
}

// 删除节点(del = 1 时删除节点)
int llist_delete(LLIST *handler, int contact_id)
{
	if (!handler || contact_id <= 0)
		return LLIST_ERR_PARAM;

	struct llist_node *cur = _find(handler, contact_id);
	if (!cur)
		return LLIST_ERR_NOT_FOUNT;

	cur->prev->next = cur->next;
	cur->next->prev = cur->prev;
	free(cur);

	handler->count--;

	return LLIST_OK;
}

// 获取联系人数量
int llist_get_count(LLIST *handler)
{
	if (!handler)
		return LLIST_ERR_PARAM;

	return handler->count;
}

// 根据下标获取节点中的联系人数据
Contact *llist_get_at(LLIST *handler, int index)
{
	if (!handler || index < 0 || index >= handler->count)
		return NULL;

	struct llist_node *cur = handler->head.next;

	for (int i = 0; i < index; i++)  // i = index 时退出循环
		cur = cur->next;

	Contact *contact = (Contact *)cur->contact;
	if (contact->del != 0)
		return NULL;

	return contact;
}