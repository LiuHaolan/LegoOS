#include "memory/thread_pool.h"

int mem_create_mq(struct thpool_buffer *tb, int msg_size, char* name, uint64_t senderid){
	//build a linked list for storing msgs, with each component size msg_size
	struct mq_header *header = kmalloc (sizeof(struct mq_header), GFP_KERNEL);
	header->msg_size = msg_size;
	header->mq_name  = name;
	header->sender   = senderid;
	
	struct msg *first_msg = kmalloc(sizeof(struct msg), GFP_KERNEL);
	first_msg->prev_msg = NULL;
	first_msg->next_msg = kmalloc(sizeof(struct msg), GFP_KERNEL);
	first_msg->data     = kmalloc(msg_size, GFP_KERNEL);
	header->first_msg = first_msg;
	header->latest_ptr = (void*) first_msg;
}
		
