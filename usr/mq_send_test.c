#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <linux/unistd.h>
#include "includeme.h"


int main(void)
{
	printf("mq_open: %d\n", mq_open("wuklab", 100));
	char* q = "yixu's joining";
	printf("mq_send: %d\n", mq_send("wuklab",q, strlen(q)));
	char* p = malloc(sizeof(char)*10);
	int size = 1;
	mq_receive("wuklab", p, &size);  
	printf(p);	/* should be */
	mq_close("wuklab");
}
