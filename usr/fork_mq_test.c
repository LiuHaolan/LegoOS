#include <sys/utsname.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <linux/unistd.h>
#include "includeme.h"


int main(void)
{

	pid_t pid;
	pid = fork();
	if(pid == 0){
		printf("mq_open: %d\n", mq_open("cse", 100));
		char* q = "yixu's joining";
		printf("mq_send: %d\n", mq_send("cse",q, strlen(q)));
	}
	else{
		sleep(3);

		char* p = malloc(sizeof(char)*100);
		int size = 1;
		mq_receive("cse", p, &size);  
		printf("string: %s", p);
		printf("str size: %lu\n" ,size);	 
		mq_close("cse");
	
	}

/*	mq_open("ece", 80);
	
	mq_send("ece", "Avada!", 6);
	mq_send("ece", "damn!",5);
	mq_send("ece", "LordVoldemortIsBack",19);
	char* p1 = malloc(sizeof(char)*100);
	int size1 = 1;
	mq_receive("ece", p1, &size1);   
	printf("string: %s", p1);
	printf("str size: %lu\n" ,size1);	 
*/	
}
