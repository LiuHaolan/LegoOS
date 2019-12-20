#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <linux/unistd.h>
#include "includeme.h"
#include <pthread.h>

static void lego_time(int flag)
{
	struct timeval tv;
	time_t t;

	gettimeofday(&tv, NULL);
	printf("pid %d: tv_sec: %lld, tv_usec: %lld\n", flag,
		tv.tv_sec, tv.tv_usec);

//	t = time(NULL);	
//	printf("time(NULL): %lld\n", t);
}

char* entry[8] = {"ece1","ece2","ece3","ece4","ece5","ece6","ece7","ece8"};


static void *thread_1(void *arg)
{
	int k = *((int*)arg);
	printf("k: %d\n", k);
	lego_time(k);
                        int j = 0;
                        for(j=0;j<1000000;j++){
                                mq_send(entry[k], "Expecto", 7);
                        }
                        lego_time(8+k);


}


/*
int main(void)
{

	printf("mq_open: %d\n", mq_open("cse", 100));
	char* q = "yixu's joining";
	printf("mq_send: %d\n", mq_send("cse",q, strlen(q)));

	
	/* hwat error handling?
 * open a non-exist message queue
 * seems don't work
 * 

	pid_t pid;
	int k = 0;
	int num = 8;
	
	for(k=0;k<num;k++){
		mq_open(entry[k], 80);
	}	


	int ret;
	pthread_t tid[8];
	
	for(k=0;k<num;k++){
		void *args = malloc(sizeof(int));
		int* q = (int*)args;
		*q = k;
		ret = pthread_create(&tid[k], NULL, thread_1, args);
	}
	
	for(k=0;k<num;k++)
       		pthread_join(tid[k], NULL);
//	time_t t1 =
//	time_t t2 = time(NULL);
//	printf("time: %lld\n", t2-t1);

	

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

/*
 * trick, not close for now!
	mq_close("ece");

}*/


int main(void)
{
	printf("mq_open: %d\n", mq_open("cse", 100));
	char* q = "yixu's joining";
	printf("mq_send: %d\n", mq_send("cse",q, strlen(q)));
	mq_open("ece", 80);

	int k = 0;
	lego_time(0);
	for(k = 0 ;k<1000000;k++){
//	lego_time(0);
	mq_send("ece", "Avada!", 6);

}
	lego_time(1);
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


