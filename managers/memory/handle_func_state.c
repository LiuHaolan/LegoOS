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

/*
 * This one is adapted from do_munmap
 * But when it works on the page table, it reserve certain pages and...
 *
 */ 
int do_unmap_and_save(struct lego_mm_struct *mm, unsigned long start, size_t len){
	unsigned long end;
	struct vm_area_struct * vma, *prev, *last;

	if((offset_in_page(start)) || start > TASK_SIZE || len > TASK_SIZE-start)
	return -EINVAL;

	len = PAGE_ALIGN(len);
	if( len == 0)
		return -EINVAL;

	/* Find the first overlapping VMA */
	vma = find_vma(mm, start);
	if(!vma)
		return 0;
	prev = vma->vm_prev;
	/* for this start < vm->vm_end */

	/* not overlapped */
	end = start + len;
	if(vma->vm_start >= end)
		return 0;	

	/* split any vma */
	if(start > vma->vm_start){
		int error;

		/*
 * 		 * Make sure that map_count on return from munmap() will
 * 		 		 * not exceed its limit; but let map_count go just above
 * 		 		 		 * its limit temporarily, to help free resources as expected.
 * 		 		 		 		 */
		if (end < vma->vm_end && mm->map_count >= sysctl_max_map_count)
			return -ENOMEM;

		error = __split_vma(mm, vma, start, 0);
		if (error)
			return error;
		prev = vma;
#ifdef CONFIG_DISTRIBUTED_VMA_MEMORY
//		save_vma_context(mm, mm->vmrange_map[vmr_idx(start)]);
	// not sure what to do here right now
#endif	
	}

	/* Does it split the last one? */
	last = find_vma(mm, end);
	if (last && end > last->vm_start) {
		int error = __split_vma(mm, last, end, 1);
		if (error)
			return error;
	}
	vma = prev ? prev->vm_next : mm->mmap;

	/* remove the vma */
	detach_vmas_to_be_unmapped(mm, vma, prev,end);
	save_region(mm, vma, prev,start, end);;

	/* Fix up all other VM information */
	remove_vma_list(mm,vma);

	return 0;
}

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


