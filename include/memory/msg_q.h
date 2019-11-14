#include <lego/list.h>
#define MAX_FILENAME_LENGTH 100

#ifndef MSG_H
#define MSG_H

#define MAX_FILENAME_LENGTH 100

/* list api
 *  * this list api
 *   */

struct mc_msg_queue{
	char* msg_data;
	unsigned int msg_size;
	struct list_head list;
};

struct name_mq_map{
	char mq_name[MAX_FILENAME_LENGTH];
	unsigned int max_size;
	struct list_head* mq;
	struct list_head list;
};

unsigned int append(char* msg_data, unsigned int msg_size, struct list_head* name_nid_dict){
	struct mc_msg_queue* tmp;
	
	tmp = kmalloc(sizeof(struct mc_msg_queue), GFP_KERNEL);
	if(!tmp){
		printk("allocated wrong\n");
		return 0;
	}

	tmp->msg_data = kmalloc(sizeof(char)*(msg_size+1), GFP_KERNEL);
	strcpy(tmp->msg_data, msg_data);
	tmp->msg_size = msg_size;
	list_add_tail(&tmp->list, name_nid_dict);	
	
	return 1;
	/* return error
 * 	*/
}



/* the msg data passed in should copy a new memory here, msg_data should point to a continous memory
 * */
unsigned int pop(char* msg_data, int* msg_size, struct list_head* name_nid_dict){
	struct list_head* next = name_nid_dict->next;
	
	struct mc_msg_queue* item = list_entry(next, struct mc_msg_queue, list);
		
	strcpy(msg_data, item->msg_data);
	*msg_size = item->msg_size;
	list_del(&item->list);

	kfree(item->msg_data);	
	kfree(item);
	name_nid_dict = next;

	return 1;
}

/*
 *  * print name_nid_dict
 *   */


void print(struct list_head* name_nid_dict){
	struct mc_msg_queue *pos = NULL;
	list_for_each_entry(pos, name_nid_dict, list){
		printk(pos->msg_data);
		printk(" ");
		printk("message data: %d\n", pos->msg_size);
	}
}


void free_all(struct list_head* name_nid_dict){
	struct list_head* cur=name_nid_dict->next;
	while(cur != name_nid_dict){		
		struct mc_msg_queue* item = list_entry(cur, struct mc_msg_queue, list);
		printk("message freed:%s \n", item->msg_data);
		struct list_head* tmp = cur->next;
		list_del(cur);

		cur = tmp;
		kfree(item->msg_data);
		kfree(item);
	}
	
}

LIST_HEAD(addr_map);

unsigned int mc_mq_open(char* mq_name, unsigned int max_size){
	struct name_mq_map* tmp;
	
	tmp = kmalloc(sizeof(struct name_mq_map), GFP_KERNEL);
	if(!tmp){
		printk("allocated wrong\n");
		return 0;
	}

	strcpy(tmp->mq_name, mq_name);
	tmp->max_size = max_size;
	tmp->mq = kmalloc(sizeof(struct list_head), GFP_KERNEL);	
	INIT_LIST_HEAD(tmp->mq);	

	list_add_tail(&tmp->list, &addr_map);	
	
	return 1;
}

unsigned int mc_mq_send(char *mq_name, char* msg_data, unsigned int msg_size){
	/* find out where is our mq head pointer
 * 	 *
 * 	          */	
	struct name_mq_map *pos, *target = NULL;
	list_for_each_entry(pos, &addr_map, list){
		if(strcmp(mq_name, pos->mq_name)==0){
			target = pos;		
		}
	}
	if(target != NULL){
		append(msg_data, msg_size, target->mq);
		return 1;	
	}
	return 0;
}

unsigned int mc_mq_receive(char *mq_name, char* msg_data, unsigned int* msg_size){
	/* find out where is our mq head pointer
 * 	 */	
	struct name_mq_map *pos, *target = NULL;
	list_for_each_entry(pos, &addr_map, list){
		if(strcmp(mq_name, pos->mq_name)==0){
			target = pos;		
		}
	}
	if(target ==NULL)
		return 0;
	pop(msg_data, msg_size, target->mq);
	return 1;
}

unsigned int mc_mq_close(char* mq_name){
	struct name_mq_map *pos, *target = NULL;
	list_for_each_entry(pos, &addr_map, list){
		
		if(strcmp(mq_name, pos->mq_name)==0){
			target = pos;		
		}
	}
	if(target ==NULL)
		return 0;	

	printk("delete: ");		
	printk(target->mq_name);
	printk(" ");
	printk("max size data: %d\n", target->max_size);
	list_del(&target->list);

/* possible leak
 *  * what if message queue still gots some thing, then we free then
 *   */
	free_all(target->mq);
	kfree(target->mq);
	kfree(target);

	return 1;
}

unsigned int mc_mq_free(void){
	struct list_head* cur=addr_map.next;
	while(cur != &addr_map){		
		struct name_mq_map* item = list_entry(cur, struct name_mq_map, list);
		printk("%s: %d\n", item->mq_name, item->max_size);
		struct list_head* tmp = cur->next;
		list_del(cur);

		cur = tmp;
		
		/* test
 * 		 * print all the mq message 
 * 		 		 */
		print(item->mq);

		free_all(item->mq);
		kfree(item->mq);
		kfree(item);
	}
	return 1;
}

#endif