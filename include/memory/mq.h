
typedef struct mq_header{
	uint64_t sender;//identify sender of the msg
	uint64_t msg_size;//all msgs of this queue should be the same size
	struct msg *first_msg;//pointer to the first msg
	void* latest_ptr;//ptr to the next available node
	char* mq_name;//name of the queue
}msg_header

typedef struct msg{
	struct *prev_msg;
	struct *next_msg;
	void* data;
}

void mem_mq_create(struct thpool_buffer *tb, int msg_size);
