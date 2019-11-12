/*
 * Copyright (c) 2016-2017 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/jiffies.h>

#define MAX_FILENAME_LENGTH 100
struct name_nid_struct{
	char name[MAX_FILENAME_LENGTH];
	__u32 nid;
	struct list_head list;
};

static LIST_HEAD(name_nid_dict);

void add(char* name, __u32 nid){
	struct name_nid_struct* tmp;
	
	tmp = kmalloc(sizeof(struct name_nid_struct), GFP_KERNEL);
	strcpy(tmp->name, name);
	tmp->nid = nid;
	list_add_tail(&tmp->list, &name_nid_dict);	
	
}

void remove(char* name){
	struct name_nid_struct *pos, *target = NULL;
	list_for_each_entry(pos, &name_nid_dict, list){
		if(strcmp(pos->name,name)==0){
			target = pos;
		}
	}
	list_del(&target->list);
}

/*
 * print name_nid_dict
 */
void print(void){
	struct name_nid_struct *pos = NULL;
	list_for_each_entry(pos, &name_nid_dict, list){
		printk(pos->name);
		printk(" ");
		printk("nid: %d\n", pos->nid);
	}
}

void free_all(void){
	struct name_nid_struct *pos = NULL;
	list_for_each_entry(pos, &name_nid_dict, list){
		list_del(&pos->list);
		kfree(pos);
	}	
}

static void mq_test_module_init(void)
{
	add("haolan",1);
	add("xuyi",2);
	add("yizhou",9);
	add("yiying",6);
	remove("yiying");
	print();
}

static void mq_test_module_exit(void)
{
	free_all();
	pr_info("lego memory monitor module exit\n");
}

module_init(mq_test_module_init);
module_exit(mq_test_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wuklab@Purdue");
