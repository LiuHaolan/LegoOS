#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <linux/unistd.h>
#include "includeme.h"


int main(void)
{
	printf("mq_open: %d\n", mq_open("cse", 100));
	char* q = "yixu's joining";
	printf("mq_send: %d\n", mq_send("cse",q, strlen(q)));
	char* p = malloc(sizeof(char)*10);
	int size = 1;
	mq_receive("cse", p, &size);  
	printf("string: %s", p);
	printf("str size: %d" ,size);	 
	mq_close("cse");

}
