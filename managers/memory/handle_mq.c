#include <lego/list.h>
#include <memory/mq.h>

int mq_create(char* mq_name, int msg_size, struct mq_header *mq){
	mq->mq_name = mq_name;
	mq->msg_size = msg_size;
	LIST_HEAD_INIT(mq->msg_list->list);
	mq->msg_list->data = kmalloc(msg_size, GFP_KERNEL);
	if (!mq->msg_list->data) {
		pr_err("Fail to alloc space for data\n");
		return -1;
	}
	mq->current_msg = NULL;
	mq->next_msg = mq->msg_list;
	return 0;
}

struct mq_header* mq_open(char* mq_name){
	//TODO: retrieve header info through name
}

//TODO: add concurrency control
int mq_send(struct mq_header *mq, void* msg_data, int msg_size){
	memcpy(mq->current_msg->data, msg_data, msg_size);

	//allocate a new data node in the list, so that the ptrs in mq can point to this new nodes
	struct list_data *new_aval_msg = kmalloc(sizeof(struct list_data), GFP_KERNEL);
	new_aval_msg->data = kmalloc(msg_size, GFP_KERNEL);
	listdata_add_tail(new_aval_msg, mq->msg_list);
	
	//change ptr in mq to point to new nodes
	mq->current_msg = mq->next_msg;
	mq->next_msg = new_aval_msg;
	return 0;
}

//TODO: add concurrency control
int mq_recv(struct mq_header *mq, void* buffer, int msg_size){
	if(mq->current_msg!=NULL)
		memcpy(buffer, mq->current_msg->data, msg_size);
	else
		return -1;
}

int mq_del(struct mq_header *mq, struct list_data *node){
	if(mq->current_msg == node){
		//converting the ptr of list_head to list_data
		mq->current_msg = (struct list_data *)(node->list->prev);
	}
	if(mq->next_msg == node){
		//converting the ptr of list_head to list_data
                mq->next_msg = (struct list_data *)(node->list->prev);
		mq->current_msg = (struct list_data *)(mq->next_msg->list->prev);
	}
	listdata_del(node);
}	
