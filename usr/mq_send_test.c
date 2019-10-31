#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <linux/unistd.h>
#include "includeme.h"

static void lego_uname(void)
{
	struct utsname foo;

	uname(&foo);
	printf("uname(): \n"
	       "\t sysname: %s\n"
	       "\t nodename: %s\n"
	       "\t release: %s\n"
	       "\t version: %s\n"
	       "\t machine: %s\n",
	       foo.sysname, foo.nodename, foo.release, foo.version, foo.machine);
}


int main(void)
{
	printf("mq_send: %d\n", mq_send());
//	lego_time();

//	lego_uname();
//	lego_getrlimit();
//	lego_set_tid_address();

//	lego_time();
}
