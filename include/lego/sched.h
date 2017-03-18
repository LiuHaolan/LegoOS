/*
 * Copyright (c) 2016-2017 Wuklab, Purdue University. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _LEGO_SCHED_H_
#define _LEGO_SCHED_H_

#include <asm/page.h>
#include <asm/ptrace.h>
#include <asm/current.h>
#include <asm/processor.h>
#include <asm/switch_to.h>
#include <asm/thread_info.h>

#include <lego/rq.h>
#include <lego/mm.h>
#include <lego/magic.h>
#include <lego/preempt.h>

/*
 * Clone Flags:
 */
#define CSIGNAL			0x000000ff	/* signal mask to be sent at exit */
#define CLONE_VM		0x00000100	/* set if VM shared between processes */
#define CLONE_FS		0x00000200	/* set if fs info shared between processes */
#define CLONE_FILES		0x00000400	/* set if open files shared between processes */
#define CLONE_SIGHAND		0x00000800	/* set if signal handlers and blocked signals shared */
#define CLONE_PTRACE		0x00002000	/* set if we want to let tracing continue on the child too */
#define CLONE_PARENT		0x00008000	/* set if we want to have the same parent as the cloner */
#define CLONE_THREAD		0x00010000	/* Same thread group? */

/*
 * task->state and task->exit_state
 *
 * We have two separate sets of flags: task->state
 * is about runnability, while task->exit_state are
 * about the task exiting. Confusing, but this way
 * modifying one set can't modify the other one by
 * mistake.
 */
#define TASK_RUNNING		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE	2
#define __TASK_STOPPED		4
#define __TASK_TRACED		8
/* in tsk->exit_state */
#define EXIT_DEAD		16
#define EXIT_ZOMBIE		32
#define EXIT_TRACE		(EXIT_ZOMBIE | EXIT_DEAD)
/* in tsk->state again */
#define TASK_DEAD		64
#define TASK_WAKEKILL		128
#define TASK_WAKING		256
#define TASK_PARKED		512
#define TASK_NOLOAD		1024
#define TASK_NEW		2048
#define TASK_STATE_MAX		4096

/* Convenience macros for the sake of wake_up */
#define TASK_NORMAL		(TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)
#define TASK_ALL		(TASK_NORMAL | __TASK_STOPPED | __TASK_TRACED)

/*
 * set_current_state() includes a barrier so that the write of current->state
 * is correctly serialised wrt the caller's subsequent test of whether to
 * actually sleep:
 *
 *   for (;;) {
 *	set_current_state(TASK_UNINTERRUPTIBLE);
 *	if (!need_sleep)
 *		break;
 *
 *	schedule();
 *   }
 *   __set_current_state(TASK_RUNNING);
 *
 * If the caller does not need such serialisation (because, for instance, the
 * condition test and condition change and wakeup are under the same lock) then
 * use __set_current_state().
 *
 * The above is typically ordered against the wakeup, which does:
 *
 *	need_sleep = false;
 *	wake_up_state(p, TASK_UNINTERRUPTIBLE);
 *
 * Where wake_up_state() (and all other wakeup primitives) imply enough
 * barriers to order the store of the variable against wakeup.
 *
 * Wakeup will do: if (@state & p->state) p->state = TASK_RUNNING, that is,
 * once it observes the TASK_UNINTERRUPTIBLE store the waking CPU can issue a
 * TASK_RUNNING store which can collide with __set_current_state(TASK_RUNNING).
 *
 * This is obviously fine, since they both store the exact same value.
 *
 * Also see the comments of try_to_wake_up().
 */
#define __set_current_state(state_value) do { current->state = (state_value); } while (0)
#define set_current_state(state_value)	 smp_store_mb(current->state, (state_value))

/*
 * task->flags
 */
#define PF_IDLE			0x00000002	/* I am an IDLE thread */
#define PF_EXITING		0x00000004	/* getting shut down */
#define PF_EXITPIDONE		0x00000008	/* pi exit done on shut down */
#define PF_VCPU			0x00000010	/* I'm a virtual CPU */
#define PF_WQ_WORKER		0x00000020	/* I'm a workqueue worker */
#define PF_FORKNOEXEC		0x00000040	/* forked but didn't exec */
#define PF_MCE_PROCESS  	0x00000080      /* process policy on mce errors */
#define PF_SUPERPRIV		0x00000100	/* used super-user privileges */
#define PF_DUMPCORE		0x00000200	/* dumped core */
#define PF_SIGNALED		0x00000400	/* killed by a signal */
#define PF_MEMALLOC		0x00000800	/* Allocating memory */
#define PF_NPROC_EXCEEDED	0x00001000	/* set_user noticed that RLIMIT_NPROC was exceeded */
#define PF_USED_MATH		0x00002000	/* if unset the fpu must be initialized before use */
#define PF_USED_ASYNC		0x00004000	/* used async_schedule*(), used by module init */
#define PF_NOFREEZE		0x00008000	/* this thread should not be frozen */
#define PF_FROZEN		0x00010000	/* frozen for system suspend */
#define PF_FSTRANS		0x00020000	/* inside a filesystem transaction */
#define PF_KSWAPD		0x00040000	/* I am kswapd */
#define PF_MEMALLOC_NOIO	0x00080000	/* Allocating memory without IO involved */
#define PF_LESS_THROTTLE	0x00100000	/* Throttle me less: I clean memory */
#define PF_KTHREAD		0x00200000	/* I am a kernel thread */
#define PF_RANDOMIZE		0x00400000	/* randomize virtual address space */
#define PF_SWAPWRITE		0x00800000	/* Allowed to write to swap */
#define PF_NO_SETAFFINITY	0x04000000	/* Userland is not allowed to meddle with cpus_allowed */

/* Task command name length */
#define TASK_COMM_LEN 16

struct task_struct {
	/* -1 unrunnable, 0 runnable, >0 stopped */
	volatile long		state;

	/* kernel mode stack */
	void			*stack;

	/* per-process flags */
	unsigned int		flags;

	atomic_t		usage;

	/* task exit state */
	int			exit_state;
	int			exit_code;
	int			exit_signal;

	char			comm[TASK_COMM_LEN];

	/* runqueue related: */
	int			on_rq;
	int			static_prio;
	struct list_head	run_list;

	/* list of all task_structs in the system */
	struct list_head	tasks;

#ifdef CONFIG_SMP
	int			on_cpu;
	int			wake_cpu;
#endif

	int			nr_cpus_allowed;
	cpumask_t		cpus_allowed;

	int			in_iowait;

	pid_t			pid;
	pid_t			tgid;

	/*
	 * pointers to (original) parent process, youngest child,
	 * younger sibling, older sibling, respectively.
	 * (p->father can be replaced with p->real_parent->pid)
	 */
	struct task_struct *real_parent;	/* real parent process */
	struct task_struct *parent;		/* recipient of SIGCHLD, wait4() reports */
	/*
	 * children/sibling forms the list of my natural children
	 */
	struct list_head children;		/* list of my children */
	struct list_head sibling;		/* linkage in my parent's children list */
	struct task_struct *group_leader;	/* threadgroup leader */

	struct mm_struct *mm, *active_mm;

	/* CPU-specific state of this task */
	struct thread_struct thread;
};

union thread_union {
	struct thread_info thread_info;
	unsigned long stack[THREAD_SIZE/sizeof(long)];
};

#define task_thread_info(task)	((struct thread_info *)((task)->stack))
#define task_stack_page(task)	((void *)((task)->stack))

/*
 * flag set/clear/test wrappers
 * - pass TIF_xxxx constants to these functions
 */

static inline void set_ti_thread_flag(struct thread_info *ti, int flag)
{
	set_bit(flag, (unsigned long *)&ti->flags);
}

static inline void clear_ti_thread_flag(struct thread_info *ti, int flag)
{
	clear_bit(flag, (unsigned long *)&ti->flags);
}

static inline int test_and_set_ti_thread_flag(struct thread_info *ti, int flag)
{
	return test_and_set_bit(flag, (unsigned long *)&ti->flags);
}

static inline int test_and_clear_ti_thread_flag(struct thread_info *ti, int flag)
{
	return test_and_clear_bit(flag, (unsigned long *)&ti->flags);
}

static inline int test_ti_thread_flag(struct thread_info *ti, int flag)
{
	return test_bit(flag, (unsigned long *)&ti->flags);
}

#define set_thread_flag(flag) \
	set_ti_thread_flag(current_thread_info(), flag)
#define clear_thread_flag(flag) \
	clear_ti_thread_flag(current_thread_info(), flag)
#define test_and_set_thread_flag(flag) \
	test_and_set_ti_thread_flag(current_thread_info(), flag)
#define test_and_clear_thread_flag(flag) \
	test_and_clear_ti_thread_flag(current_thread_info(), flag)
#define test_thread_flag(flag) \
	test_ti_thread_flag(current_thread_info(), flag)

#define tif_need_resched()	test_thread_flag(TIF_NEED_RESCHED)

static __always_inline bool need_resched(void)
{
	return unlikely(tif_need_resched());
}

/*
 * Return the address of the last usable long on the stack.
 *
 * When the stack grows down, this is just above the thread
 * info struct. Going any lower will corrupt the thread_info.
 */
static inline unsigned long *end_of_stack(struct task_struct *p)
{
	return (unsigned long *)(task_thread_info(p) + 1);
}

static inline int kstack_end(void *addr)
{
	/* Reliable end of stack detection: */
	return !(((unsigned long)addr+sizeof(void*)-1) & (THREAD_SIZE-sizeof(void*)));
}

extern union thread_union init_thread_union;
extern struct task_struct init_task;

void show_call_trace(struct task_struct *task, struct pt_regs *regs);
void show_stack_content(struct task_struct *task, struct pt_regs *regs);
void show_general_task_info(struct task_struct *task);
void show_regs(struct pt_regs *regs);

/**
 * dump_stack	-	Dump the current stack
 *
 * By default, this function will just dump the *current* process's stack
 * and registers. If we have further requirement, e.g. dump *another* process's
 * stack, then we need to look back and improve this guy.
 */
static inline void dump_stack(void)
{
	show_general_task_info(current);
	show_stack_content(current, NULL);
	show_call_trace(current, NULL);
}

void setup_task_stack_end_magic(struct task_struct *tsk);

asmlinkage void schedule_tail(struct task_struct *prev);
int setup_sched_fork(unsigned long clone_flags, struct task_struct *p);
void sched_remove_from_rq(struct task_struct *p);

/* arch-hook to copy thread info while doing fork */
int copy_thread_tls(unsigned long, unsigned long, unsigned long,
		struct task_struct *, unsigned long);

/* Scheduler clock - returns current time in nanosec units */
unsigned long long sched_clock(void);

struct task_struct *copy_process(unsigned long clone_flags,
				 unsigned long stack_start,
				 unsigned long stack_size,
				 int node, int tls);

pid_t kernel_thread(int (*fn)(void *), void *arg, unsigned long flags);

void __init sched_init(void);
void __init sched_init_idle(struct task_struct *idle, int cpu);

void schedule(void);
void scheduler_tick(void);
int wake_up_process(struct task_struct *p);
void wake_up_new_task(struct task_struct *p);
int set_cpus_allowed_ptr(struct task_struct *p, const struct cpumask *new_mask);

void do_exit(long code);
void __noreturn do_task_dead(void);
void cpu_idle(void);

void __put_task_struct(struct task_struct *t);

static inline void put_task_struct(struct task_struct *t)
{
	if (atomic_dec_and_test(&t->usage))
		__put_task_struct(t);
}

static inline void get_task_struct(struct task_struct *t)
{
	atomic_inc(&t->usage);
}

/*
 * Wrappers for p->thread_info->cpu access. No-op on UP.
 */
#ifdef CONFIG_SMP
static inline unsigned int task_cpu(const struct task_struct *p)
{
	return task_thread_info(p)->cpu;
}
void set_task_cpu(struct task_struct *p, unsigned int new_cpu);
#else
static inline unsigned int task_cpu(const struct task_struct *p) { return 0; }
static inline void set_task_cpu(struct task_struct *p, unsigned int new_cpu){ }
#endif

/* Attach to any functions which should be ignored in wchan output. */
#define __sched		__attribute__((__section__(".sched.text")))

/* Linker adds these: start and end of __sched functions */
extern char __sched_text_start[], __sched_text_end[];

/* Is this address in the __sched functions? */
int in_sched_functions(unsigned long addr);

/* task_struct::on_rq states: */
#define TASK_ON_RQ_QUEUED	1
#define TASK_ON_RQ_MIGRATING	2

static inline int task_on_rq_queued(struct task_struct *p)
{
	return p->on_rq == TASK_ON_RQ_QUEUED;
}

static inline int task_on_rq_migrating(struct task_struct *p)
{
	return p->on_rq == TASK_ON_RQ_MIGRATING;
}

static inline int task_running(struct rq *rq, struct task_struct *p)
{
#ifdef CONFIG_SMP
	return p->on_cpu;
#else
	return task_current(rq, p);
#endif
}

#endif /* _LEGO_SCHED_H_ */
