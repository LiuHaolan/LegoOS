/*
 * Copyright (c) 2016-2019 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

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

//t2
#include <lego/fit_ibapi.h>


#define MAX_INIT_ARGS	CONFIG_INIT_ENV_ARG_LIMIT
#define MAX_INIT_ENVS	CONFIG_INIT_ENV_ARG_LIMIT

/* http://c-faq.com/decl/spiral.anderson.html */
static const char *argv_init[MAX_INIT_ARGS+2];
const char *envp_init[MAX_INIT_ENVS+2] =
{
	"HOME=/",
	"TERM=linux",
	"LANG=en_US.UTF-8",
	"PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/root/bin",
	"USER=root",
	"PWD=/",
	NULL,
};

static int procmgmt(void *unused)
{
	const char *init_filename;
	int vid __maybe_unused;

	/*
	 * Use the correct name if a real storage node is used.
	 * If CONFIG_USE_RAMFS is set, then filename does not matter anyway.
	 */
#if 0
	init_filename = "/usr/bin/python";
	argv_init[0] = init_filename;
	argv_init[1] = "/root/ys/models-1.4.0/official/resnet/imagenet_main.py";
	argv_init[2] = "--data_dir";
	argv_init[3] = "/mnt/ssd/imagenet/raw-data/tmp/";
	argv_init[4] = "--model_dir";
	argv_init[5] = "/tmp/lego_imagenet_model";
	argv_init[6] = "--batch_size";
	argv_init[7] = "1024";
#endif

#if 1
	init_filename = "/usr/bin/python";
	argv_init[0] = init_filename;
//	argv_init[1] = "/root/ys/models-1.4.0/official/resnet/cifar10_main.py";
	argv_init[1] = "/home/haolanliu/python_dir/greeting.py";
#endif

	/*
	 * If vNode is configured, which implies GPM is also configured,
	 * we should ask GPM what our vNode information will be:
	 */
#ifdef CONFIG_VNODE
	vid = p2pm_request_vnode();
	if (vid < 0)
		panic("Invalid vNode ID, abort.");
	current->pm_data.virtual_node = vid_find_vnode(vid);
#endif

	/*
	 * It's strace has not been established yet
	 * Because previously it has PF_KTHREAD set
	 */
	__fork_processor_strace(current);

	return do_execve(init_filename,
		(const char *const *)argv_init,
		(const char *const *)envp_init);
}

void __init kick_off_user(void)
{
	pid_t pid;

	/*
	 * Must use kernel_thread instead of global_kthread_run
	 * because that one will call do_exit inside. So do_execve
	 * will not have any effect.
	 */
	pid = kernel_thread(procmgmt, NULL, CLONE_GLOBAL_THREAD);
	if (pid < 0)
		panic("Fail to run the initial user process.");
}

#ifdef CONFIG_CHECKPOINT
void __init checkpoint_init(void);
#else
static inline void checkpoint_init(void) { }
#endif

#ifdef CONFIG_GPM_HANDLER
static inline void init_gpm_handler(void)
{
	struct task_struct *ret;

	ret = kthread_run(gpm_handler, NULL, "gpm_handler");
	if (IS_ERR_OR_NULL(ret))
		panic("Fail to create gpm handler thread");
}
#else
static inline void init_gpm_handler(void) { }
#endif

static inline void common_header_check(void)
{
#define CHK(type, member)	\
	BUILD_BUG_ON((offsetof(type, member) % COMMON_HEADER_ALIGNMENT) != 0)

	CHK(struct p2m_replica_msg, log);

#undef CHK
}

/**
 * processor_manager_init
 *
 * Initiliaze all processor manager contained subsystems.
 * System will just panic if any of them failed.
 */
void __init processor_manager_init(void)
{
	common_header_check();
	pcache_post_init();
	pcache_zerofill_notify_init();

#ifndef CONFIG_FIT
	pr_info("Network is not compiled. Halt.");
	while (1)
		hlt();
#endif

#ifdef CONFIG_VMA_PROCESSOR_UNITTEST
	prcsr_vma_unit_test();
#endif
	
	gpm_handler_init();

	/* Create checkpointing restore thread */
	checkpoint_init();
}

/*
 * Early init before buddy allocator is up,
 * so we are free to use memblock.
 */
void __init processor_manager_early_init(void)
{
	pcache_early_init();
}

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
 * should we use strlen(name)+1, plus 1 really need?
 */ 
	copy_from_user(payload->mq_name, name, name_size+1);
	copy_from_user(payload->msg, msg_data, msg_size+1);
	payload->msg_size=msg_size;

	retlen = ibapi_send_reply_imm(current_pgcache_home_node(), msg, len_msg, &retval, sizeof(retval),false);	
	
	/* check return value
 	*/
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
 * should we use strlen(name)+1, plus 1 really need?
 */ 
	copy_from_user(payload->mq_name, name, name_size+1);
	
	retlen = ibapi_send_reply_imm(current_pgcache_home_node(), msg, len_msg, &retval, sizeof(retval),false);	
	
	/* check return value
 	*/
	if(retlen == -ETIMEDOUT){
		return -1;
	}		
	
	/*
 	 *
 	 * reply, reply 0 means good
 	 */	
	if(retval.ret == 0){
		printk("msg received in kernel: %s\n", retval.mq_data);
		copy_to_user(msg_data, retval.mq_data, strlen(retval.mq_data)+1);
		unsigned long* data_size = kmalloc(sizeof(unsigned long), GFP_KERNEL);
		*data_size = strlen(retval.mq_data);
		copy_to_user(msg_size, data_size, sizeof(unsigned long));
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
 * should we use strlen(name)+1, plus 1 really need?
 */ 
	copy_from_user(payload->mq_name, name, name_size+1);

	retlen = ibapi_send_reply_imm(current_pgcache_home_node(), msg, len_msg, &retval, sizeof(retval),false);	
	
	/* check return value
 	*/
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
	int lansize = (int) msg_size
	printk("after conversion: %d", lansize);

	copy_from_user(payload->mq_name, name, name_size+1);
	payload->msg_size=msg_size;

	retlen = ibapi_send_reply_imm(current_pgcache_home_node(), msg, len_msg, &retval, sizeof(retval),false);	
	
/*	// check return value
 	
	if(retlen == -ETIMEDOUT){
		return -1;
	}		
*/
	/* free allocated memory */
	kfree(msg);

	return retval;
}

#ifndef CONFIG_CHECKPOINT
SYSCALL_DEFINE1(checkpoint_process, pid_t, pid)
{
	printk_once("Checkpoint is not configured!\n");
	return -ENOSYS;
}
#endif

#ifdef CONFIG_COUNTER_PCACHE
static inline void print_pcache_util(void)
{
	u64 p_i, p_re;

	p_i = div64_u64_rem(pcache_used() * 100UL,
			    nr_cachelines, &p_re);
	pr_info("Cache Utilization %Lu.%Lu%%\n", p_i, p_re);
}
#else
static inline void print_pcache_util(void) { }
#endif

void watchdog_print(void)
{
	print_pcache_util();
	print_pcache_events();
	print_profile_points();
}
