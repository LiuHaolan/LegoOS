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

SYSCALL_DEFINE4(vms_save, const char*, name, unsigned long, addr, size_t, len, unsigned long, if_persist)
{
        struct p2m_mmap_struct payload;
	struct p2m_mmap_reply_struct reply;

	long ret_len, ret_addr;

	payload.pid = current->tgid;
	payload.addr = addr;
	payload.len = len;

	ret_len = net_send_reply_timeout(current_memory_home_node(), P2M_VMS_SAVE,
			&payload, sizeof(payload), &reply, sizeof(reply),
			false, DEF_NET_TIMEOUT);

	return ret_addr;
}

SYSCALL_DEFINE3(vms_restore, const char*, name, unsigned long*, addr, size_t*, len)
{
        BUG();
}


