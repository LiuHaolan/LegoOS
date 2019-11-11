#include <lego/list.h>
#include <memory/mq.h>

unsigned long init_mqs(int mq_num){
	addr_map = kmalloc(mq_num * sizeof(struct addrmap), GFP_KERNEL);
	if(addr_map == NULL)
		return -1;
	current_mq = 0;
	mq_size = mq_num;
	return 0;
}

unsigned long mq_create(char* mq_name, int msg_size, struct mq_header *mq){
	mq->mq_name = mq_name;
	mq->msg_size = msg_size;
	mq->msg_list->prev = NULL;
	mq->msg_list->next = NULL;
	mq->msg_list->data = kmalloc(msg_size, GFP_KERNEL);
	if (!mq->msg_list->data) {
		pr_err("Fail to alloc space for data\n");
		return -1;
	}
	mq->current_msg = msg_list;

	mutex_init(mq->mutex);

	addr_map[current_mq].mq_name = mq_name;
	addr_map[current_mq].mq_addr = mq;
	current_mq++;

	return 0;
}

struct mq_header* mc_mq_open(char* mq_name){
	for(int i=0; i<mq_size; i++){
		if(strcmp(mq_name, addr_map[i].mq_name)==0)
			return (struct mq_header*)(addr_map[i].mq_addr);
	}
	return NULL;
}

//TODO: add concurrency control
unsigned long mc_mq_send(char* name, char* msg_data, unsigned long msg_size){
	struct mq_header *mq = mc_mq_open(name);

	mutex_lock(&(mq->mutex));
	memcpy(mq->current_msg->data, msg_data, msg_size);

	//allocate a new data node in the list, so that the ptrs in mq can point to this new nodes
	struct list_data *new_aval_msg = kmalloc(sizeof(struct list_data), GFP_KERNEL);
	new_aval_msg->data = kmalloc(msg_size, GFP_KERNEL);
	add_tail(new_aval_msg, mq);
	
	//change ptr in mq to point to new nodes
	mq->current_msg = new_aval_msg;

	mutex_unlock(&(mq->mutex));
	return 0;
}

//TODO: add concurrency control
unsigned long mc_mq_recv(char* name, char* buffer, unsigned long msg_size){
	struct mq_header *mq = mc_mq_open(name);

	mutex_lock(&(mq->mutex));
	if(mq->current_msg->prev!=NULL)
		memcpy(buffer, mq->current_msg->prev->data, msg_size);
	else{
		mutex_unlock(&(mq->mutex));
		return -1;
	}

	mq_del(mq, mq->current_msg->prev);
	return 0;
	mutex_unlock(&(mq->mutex));
}

int msg_del(struct mq_header *mq, struct list_data *node){
	
	if(mq->current_msg == node){
		//converting the ptr of list_head to list_data
		mq->current_msg = (struct list_data *)(node->list->prev);
	}
	listdata_del(node);
}

unsigned long mc_mq_close(char* name){
	struct mq_header *mq = mc_mq_open(name);
	if(mq == NULL)
		return -1;

	kfree(mq_name);
	mutex_destroy(&(mq->mutex));

	struct list_data *node = current_msg;
	while(node->next!=NULL){
		kfree(node->data);
		node = node->next;
	}
}	
