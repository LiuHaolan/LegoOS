/*
 * Copyright (c) 2016-2019 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <lego/mm.h>
#include <lego/rwsem.h>
#include <lego/slab.h>
#include <lego/rbtree.h>
#include <lego/sched.h>
#include <lego/kernel.h>
#include <lego/netmacro.h>
#include <lego/comp_memory.h>
#include <lego/fit_ibapi.h>

#include <memory/vm.h>
#include <memory/pid.h>
#include <memory/vm-pgtable.h>
#include <memory/file_ops.h>
#include <memory/distvm.h>
#include <memory/replica.h>
#include <memory/thread_pool.h>

void handle_p2m_vms_save(struct p2m_mmap_struct *payload, struct common_header *hdr, struct thpool_buffer *tb)
{
	u32 nid = hdr->src_nid;
	u32 pid = payload->pid;

	unsigned long addr = payload->addr;
	unsigned long len = payload->len;
	struct lego_task_struct *tsk;
	struct lego_mm_struct *mm;	

	tsk = find_lego_task_by_pid(nid, pid);
	if (unlikely(!tsk)) {
//		reply->ret = -RET_ESRCH;
		return;
	}
	
	mm = tsk->mm;
	if(down_write_killable(&mm->mmap_sem)) {
//		reply->ret = RET_EINTR;
		return;
	}

	/*
 	* config the distributed vma memory or not
 	*/

#ifdef CONFIG_DISTRIBUTED_VMA_MEMORY

#else
	/* 
 	 *
 	 * save the page table*/
	do_unmap_and_save(mm, addr, len);

#endif

	up_write(&mm->mmap_sem);

	/* 
 	* you need to change the return value since other is done
 	*/
  
	size_t* retval;
	*retval = 1;
	tb_set_tx_size(tb, sizeof(*retval));	

}


