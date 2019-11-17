#define pr_fmt(fmt) "Processor: " fmt

#include <lego/slab.h>
#include <lego/math64.h>
#include <lego/timer.h>
#include <lego/kernel.h>
#include <lego/kthread.h>
#include <lego/syscalls.h>
#include <lego/profile.h>
#include <processor/zerofill.h>
#include <processor/processor.h>
#include <processor/distvm.h>
#include <processor/vnode.h>
#include <processor/pcache.h>

#include <monitor/gpm_handler.h>

#include "processor.h"
#include <lego/fit_ibapi.h>

SYSCALL_DEFINE4(mq_send, char*, name, unsigned long, name_size, unsigned long, msg_size, const char*, msg_data)
{
	ssize_t retval, retlen;
	u32 len_msg;
	void *msg;
	struct common_header* hdr;
	struct p2m_mqsend_payload* payload;
	len_msg = sizeof(*hdr)+sizeof(*payload);
	msg = kmalloc(len_msg, GFP_KERNEL);
	if(!msg)
		return -ENOMEM;
	
	hdr = (struct common_header *)msg;
	hdr->opcode = P2M_MQSEND;
	hdr->src_nid = LEGO_LOCAL_NID;
	payload=to_payload(msg);

/*
 *  * should we use strlen(name)+1, plus 1 really need?
 *   */ 
	copy_from_user(payload->mq_name, name, name_size+1);
	copy_from_user(payload->msg, msg_data, msg_size+1);
	payload->msg_size=msg_size;

	retlen = ibapi_send_reply_imm(current_pgcache_home_node(), msg, len_msg, &retval, sizeof(retval),false);	
	
	/* check return value
 *  	*/
	if(retlen == -ETIMEDOUT){
		return -1;
	}		

	/* free allocated memory */
	kfree(msg);

	return retval;
	
}

SYSCALL_DEFINE4(mq_receive, char*, name, unsigned long, name_size, unsigned long*, msg_size, char*, msg_data)
{	
	struct p2m_mqrecv_reply_struct retval; 
	ssize_t retlen;
	u32 len_msg;
	void *msg;
	struct common_header* hdr;
	struct p2m_mqrecv_payload* payload;
	len_msg = sizeof(*hdr)+sizeof(*payload);
	msg = kmalloc(len_msg, GFP_KERNEL);
	if(!msg)
		return -ENOMEM;
	
	hdr = (struct common_header *)msg;
	hdr->opcode = P2M_MQRECV;
	hdr->src_nid = LEGO_LOCAL_NID;
	payload=to_payload(msg);

/*
 *  * should we use strlen(name)+1, plus 1 really need?
 *   */ 
	copy_from_user(payload->mq_name, name, name_size+1);
	
	retlen = ibapi_send_reply_imm(current_pgcache_home_node(), msg, len_msg, &retval, sizeof(retval),false);	
	
	/* check return value
 *  	*/
	if(retlen == -ETIMEDOUT){
		return -1;
	}		
	
	/*
 *  	 *
 *  	  	 * reply, reply 0 means good
 *  	  	  	 */	
	if(retval.ret == 0){
		printk("msg received in kernel: %s\n", retval.mq_data);
		printk("strlen: %lu", strlen(retval.mq_data));
		if(copy_to_user(msg_data, (void*)retval.mq_data, (unsigned long)strlen(retval.mq_data)+1)){
			printk("copy to user failed\n");
			kfree(msg);
			return -EFAULT;
		}
		
	/*	unsigned long* data_size = kmalloc(sizeof(unsigned long), GFP_KERNEL);
 *		 *let's move this to user space
 *		 		*data_size = (unsigned long)strlen(retval.mq_data);
 *		 				if(copy_to_user(msg_size, data_size, sizeof(unsigned long))){
 *		 							kfree(msg);
 *		 										return -EFAULT;
 *		 												}
 *		 													
 *		 															kfree(data_size); */
	}
	else{
		printk("mq receive failed: return other than 0\n");
	}

	/* free allocated memory */
	kfree(msg);

	return retval.ret;
	
}
SYSCALL_DEFINE2(mq_close, char*, name, unsigned long, name_size)
{	
	ssize_t retval, retlen;
	u32 len_msg;
	void *msg;
	struct common_header* hdr;
	struct p2m_mqclose_payload* payload;
	len_msg = sizeof(*hdr)+sizeof(*payload);
	msg = kmalloc(len_msg, GFP_KERNEL);
	if(!msg)
		return -ENOMEM;
	
	hdr = (struct common_header *)msg;
	hdr->opcode = P2M_MQCLOSE;
	hdr->src_nid = LEGO_LOCAL_NID;
	payload=to_payload(msg);

/*
 *  * should we use strlen(name)+1, plus 1 really need?
 *   */ 
	copy_from_user(payload->mq_name, name, name_size+1);

	retlen = ibapi_send_reply_imm(current_pgcache_home_node(), msg, len_msg, &retval, sizeof(retval),false);	
	
	/* check return value
 *  	*/
	if(retlen == -ETIMEDOUT){
		return -1;
	}		

	/* free allocated memory */
	kfree(msg);

	return retval;
	
}

SYSCALL_DEFINE3(mq_open, char* , name, unsigned long, name_size, unsigned long, msg_size)
{
	ssize_t retval, retlen;
	u32 len_msg;
	void *msg;
	struct common_header* hdr;
	struct p2m_mqopen_payload* payload;
	len_msg = sizeof(*hdr)+sizeof(*payload);
	msg = kmalloc(len_msg, GFP_KERNEL);
	if(!msg)
		return -ENOMEM;
	
	hdr = (struct common_header *)msg;
	hdr->opcode = P2M_MQOPEN;
	hdr->src_nid = LEGO_LOCAL_NID;
	payload=to_payload(msg);

	printk("msg max size send number: %lu\n",msg_size);
	int lansize = (int) msg_size;
	printk("after conversion: %d", lansize);

	copy_from_user(payload->mq_name, name, name_size+1);
	payload->msg_size=msg_size;

	retlen = ibapi_send_reply_imm(current_pgcache_home_node(), msg, len_msg, &retval, sizeof(retval),false);	
	
/*	// check return value
 *	 	
 *	 		if(retlen == -ETIMEDOUT){
 *	 				return -1;
 *	 					}		
 *	 					*/
	/* free allocated memory */
	kfree(msg);

	return retval;
}
