#ifndef _MEMORY_MQ_H_
#define _MEMORY_MQ_H_

#include <lego/list.h>
#include <lego/mutex.h>
#include <lego/slab.h>

typedef struct list_data{
	struct list_data *next;
	struct list_data *prev;
	void* data;
}list_data;

typedef struct mq_header{
	struct list_data *msg_list;
	struct list_data *current_msg;
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

static inline void add_tail(struct list_data *node, struct mq_header *mq){
	//list_add_tail(node->list, head->list);
	mq->current_msg->next = node;
	node->prev = mq->current_msg;
	node->next = NULL;	
}

static inline void listdata_del(struct list_data *node){
	node->prev->next = node->next->prev;	
	kfree(node->data);
}

unsigned long mq_create(char* mq_name, int msg_size, struct mq_header *mq);
struct mq_header* mc_mq_open(char* mq_name);
unsigned long mc_mq_send(char* name, char* msg_data, unsigned long msg_size);
unsigned long mc_mq_recv(char* name, char* buffer, unsigned long msg_size);
int msg_del(struct mq_header *mq, struct list_data *node);
unsigned long mc_mq_close(char* name);
#endif
