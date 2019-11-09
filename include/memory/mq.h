#ifndef _MEMORY_MQ_H_
#define _MEMORY_MQ_H_

#include <lego/list.h>
#include <lego/mutex.h>
#include <lego/slab.h>

typedef struct list_data{
	struct list_head *list;
	void* data;
}list_data;

typedef struct mq_header{
	struct list_data *msg_list;
	struct list_data *current_msg;
	struct list_data *next_msg;
	struct mutex mutex;
	char* mq_name;
	int msg_size;
}mq_header;

typedef struct addrmap{
	char* mq_name;
	void* mq_addr;
}addrmap;

struct addrmap *addr_map;
int current_mq;
int mq_size;

static inline void listdata_add_tail(struct list_data *node, struct list_data *head){
	list_add_tail(node->list, head->list);
}

static inline void listdata_del(struct list_data *node){
	list_del(node->list);
	kfree(node->data);
}

#endif
