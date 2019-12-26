#ifndef _LEGO_MEMORY_VM_STATE_H_
#define _LEGO_MEMORY_VM_STATE_H_

#include <memory/vm.h>

void handle_p2m_vms_save(struct p2m_mmap_struct *payload,
	struct common_header *hdr, struct thpool_buffer *tb);

#endif /* _LEGO_MEMORY_VM_STATE_H_ */
