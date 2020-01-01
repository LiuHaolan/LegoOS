#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <linux/unistd.h>
#include "includeme.h"
#include <pthread.h>

int main(void)
{
	
	void *p = malloc(sizeof(int)*16);
	printf("address of pointer is: %p\n", &p);
	unsigned long addr = (unsigned long)&p;
	printf("unsigned long: 0x%0X\n", addr);
	vms_save("func1", addr, (size_t)(16*sizeof(int)),(unsigned long) 0);

}


