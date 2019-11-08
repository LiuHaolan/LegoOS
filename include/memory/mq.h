#ifndef _MEMORY_MQ_H_
#define _MEMORY_MQ_H_

#include <lego/list.h>

typedef struct list_data{
	struct list_head *list;
	void* data;
}list_data;

typedef struct mq_header{
	struct list_data *msg_list;
	struct list_data *current_msg;
	struct list_data *next_msg;
	char* mq_name;
	int msg_size;
}mq_header;

static inline void listdata_add_tail(struct list_data *node, struct list_data *head){
	list_add_tail(node->list, head->list);
}

static inline void listdata_del(struct list_data *node){
	list_del(node->list);
}
