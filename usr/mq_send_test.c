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
	mq_open("ece", 80);

	time_t t1 = time(NULL);
	int j = 0;
	for(j=0;j<2000;j++){
		mq_send("ece", "Avada!", 6);
	}
	time_t t2 = time(NULL);
	printf("time: %lld\n", t2-t1);

/*
	mq_send("ece", "damn!",5);
	mq_send("ece", "LordVoldemortIsBack",19);
	char* p1 = malloc(sizeof(char)*100);
	int size1 = 1;
	mq_receive("ece", p1, &size1);   
	printf("string: %s", p1);
	printf("str size: %lu\n" ,size1);	 
	
	char* p = malloc(sizeof(char)*100);
	int size = 1;
	mq_receive("cse", p, &size);  
	printf("string: %s", p);
	printf("str size: %lu\n" ,size);	 
*/
	mq_close("ece");

}

/*
int main(void)
{
	printf("mq_open: %d\n", mq_open("cse", 100));
	char* q = "yixu's joining";
	printf("mq_send: %d\n", mq_send("cse",q, strlen(q)));
	mq_open("ece", 80);
	
	mq_send("ece", "Avada!", 6);
	mq_send("ece", "damn!",5);
	mq_send("ece", "LordVoldemortIsBack",19);
	char* p1 = malloc(sizeof(char)*100);
	int size1 = 1;
	mq_receive("ece", p1, &size1);   
	printf("string: %s", p1);
	printf("str size: %lu\n" ,size1);	 
	
	char* p = malloc(sizeof(char)*100);
	int size = 1;
	mq_receive("cse", p, &size);  
	printf("string: %s", p);
	printf("str size: %lu\n" ,size);	 
	mq_close("cse");

}

*/
