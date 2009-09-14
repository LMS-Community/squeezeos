/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
/*
 * otg/otg/otg-linux.h
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/otg/linux/otg-linux.h|20070711184304|55012
 *
 *      Copyright (c) 2004-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 */

/*!
 * @file otg/otg/otg-linux.h
 * @brief Linux OS Compatibility defines
 *
 * See otg-os.h for API definitions.
 *
 * @ingroup OSAPI
 * @ingroup LINUXAPI
 */
#ifndef _OTG_LINUX_H
#define _OTG_LINUX_H 1

#if !defined(_OTG_MODULE_H)
#include <otg/otg-module.h>
#endif


#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <asm/types.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include <asm/atomic.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/random.h>
#include <linux/sched.h>
#include <linux/workqueue.h>

#if !defined(LINUX26) && !defined(LINUX24)
#error "One of LINUX24 and LINUX26 needs to be defined here"
#endif

#if defined(LINUX26)
#include <linux/device.h>
#endif


#undef OTG_SKYE_LED
#ifdef OTG_SKYE_LED
#include <asm/arch/gpio.h>
#define LED1   SP_GP_SP_A26
#define LED2   SP_GP_SP_A7
void otg_led(int led, int flag);
void otg_led_init(int led);

//#define LEDI    LED1          /* define this to get local_irq_save/restore */
#define LEDI    0

#else
#define LED1    0
#define LED2    1
#define LEDI    0
void otg_led(int led, int flag);
void otg_led_init(int led);
#endif

/* ********************************************************************************************** */

#define CONFIG_OTG_LNX 1

/*! @name Compilation Related
 */

/*@{*/
#undef PRAGMAPACK
/** Macros to support packed structures **/
#define PACKED1 __attribute__((packed))
#define PACKED2
#define PACKED0

/** Macros to support packed enum's **/
#define PACKED_ENUM enum
#define PACKED_ENUM_EXTRA

#define INLINE __inline__

/*@}*/

/*! @name Lock Interrupts
 */

/*@{*/

static inline void otg_disable_interrupts(void)
{
        /* do nothing */
}

static inline void otg_enable_interrupts(void)
{
        /* do nothing */
}


/*@}*/


/* ********************************************************************************************** */
/* otg-trace - included so that TRACE_MSG can be used if desired to debug
 * the otg-xxx implementation
 */

typedef u32 otg_tick_t;
#include <otg/otg-trace.h>


/* ********************************************************************************************** */
/*!
 * @name Semaphore
 *
 * Posix compatible Semaphores.
 * @{
 */
//typedef struct semaphore  otg_sem_t;

typedef struct xotg_sem {
        char *name;
        BOOL semdebug;
        struct semaphore  sem;
} otg_sem_t;


static int inline otg_sem_init(char *name, otg_sem_t *sem, int pshared, unsigned value) {
        sem->name = name;
        sem->semdebug = FALSE;
        sema_init(&sem->sem, value);
        return 0;
}

static int inline otg_sem_init_locked(char *name, otg_sem_t *sem) {
        otg_sem_init(name, sem, 1, 0);
        //sema_init(sem, 0);
        return 0;
}

static int inline otg_sem_init_unlocked(char *name, otg_sem_t *sem) {
        otg_sem_init(name, sem, 1, 1);
        //sema_init(sem, 1);
        return 0;
}

static int inline otg_sem_destroy(otg_sem_t *sem) {
        return 0;
}
static int inline otg_sem_wait(otg_sem_t *sem) {
        return down_interruptible(&sem->sem);
}
static int inline otg_sem_trywait(otg_sem_t *sem) {
        return down_trylock(&sem->sem);
}
static int inline otg_sem_post(otg_sem_t *sem) {
        up(&sem->sem);
        return 0;
}

/*! @} */


/* ********************************************************************************************** */
/*!
 * @name Mutex
 *
 * Posix compatible Mutexes.
 * @{
 */
typedef unsigned long otg_pthread_mutex_t;

static int inline otg_pthread_mutex_lock(otg_pthread_mutex_t *mutex) {
        local_irq_save(*mutex);
        return 0;
}

static int inline otg_pthread_mutex_trylock(otg_pthread_mutex_t *mutex) {
        local_irq_save(*mutex);
        return 0;
}
static int inline otg_pthread_mutex_unlock(otg_pthread_mutex_t *mutex) {
        local_irq_restore(*mutex);
        return 0;
}


/* ********************************************************************************************** */
/*! @} */

/*!
 * @name Atomic
 *
 * Basic Atomic operations.
 * @{
 */
typedef atomic_t otg_atomic_t;

static void inline otg_atomic_set(otg_atomic_t *a, int b) {
        atomic_set(a, b);
}
static void inline otg_atomic_add(int a, otg_atomic_t *b) {
        atomic_add(a, b);
}
static void inline otg_atomic_sub(int a, otg_atomic_t *b) {
        atomic_sub(a, b);
}
static void inline otg_atomic_clr(otg_atomic_t *a) {
        atomic_set(a, 0);
}
static void inline otg_atomic_inc(otg_atomic_t *a) {
        return atomic_inc(a);
}
static void inline otg_atomic_dec(otg_atomic_t *a) {
        return atomic_dec(a);
}
static int inline otg_atomic_read(otg_atomic_t *a) {
        return atomic_read(a);
}

static __inline__ int atomic_post_inc(volatile otg_atomic_t *v) {
        unsigned long flags;
        int result;
        local_irq_save(flags);
        result = (v->counter)++;
        local_irq_restore(flags);
        return(result);
}

static __inline__ int atomic_pre_dec(volatile otg_atomic_t *v) {
        unsigned long flags;
        int result;
        local_irq_save(flags);
        result = --(v->counter);
        local_irq_restore(flags);
        return(result);
}

/*! @} */

/* ********************************************************************************************** */
/*!
 * @name Time of Day
 * gettimeofday
 * @{
 */
static int inline otg_gettimeofday(struct timeval *tv)
{
        do_gettimeofday(tv);
        return 0;
}


static void inline otg_get_random_bytes(u8 *p, int n)
{
        get_random_bytes(p, n);
}

/*! @} */

/* ********************************************************************************************** */
/*!
 * @name Memory Allocation
 *
 * Simple allocated memory operations.
 * @{
 */

void * otg_cmalloc(int n);
void otg_free(void *);

#define KMALLOC(n) _kmalloc(__FUNCTION__, __LINE__, n, GFP_ATOMIC)
#define CKMALLOC(n) _ckmalloc(__FUNCTION__, __LINE__, n, GFP_ATOMIC)
#define LSTRDUP(str) _lstrdup(__FUNCTION__, __LINE__, str)
#define LKFREE(p) _lkfree(__FUNCTION__, __LINE__, p)

//#define kmalloc(n) _kmalloc(__FUNCTION__, __LINE__, n, GFP_ATOMIC)
#define ckmalloc(n) _ckmalloc(__FUNCTION__, __LINE__, n, GFP_ATOMIC)
#define lstrdup(str) _lstrdup(__FUNCTION__, __LINE__, str)
#define lkfree(p) _lkfree(__FUNCTION__, __LINE__, p)

#define OTG_MALLOC_TEST
#undef OTG_MALLOC_DEBUG
//#define OTG_MALLOC_DEBUG
#ifdef OTG_MALLOC_TEST
    extern int otg_mallocs;
#endif


static inline void *_kmalloc (const char *func, int line, int n, int f)
{
    void *p;
    if ((p = kmalloc (n, f)) == NULL) {
        return NULL;
    }
    #ifdef OTG_MALLOC_TEST
    ++otg_mallocs;
    #endif
    #ifdef OTG_MALLOC_DEBUG
        printk(KERN_INFO"%s: %p %s %d %d\n", __FUNCTION__, p, func, line, otg_mallocs);
    #endif
    return p;
}

static inline void *_ckmalloc (const char *func, int line, int n, int f)
{
    void *p;
    if ((p = kmalloc (n, f)) == NULL) {
        return NULL;
    }
    memset (p, 0, n);
    #ifdef OTG_MALLOC_TEST
    ++otg_mallocs;
    #endif
    #ifdef OTG_MALLOC_DEBUG
        printk(KERN_INFO"%s: %p %s %d %d\n", __FUNCTION__, p, func, line, otg_mallocs);
    #endif
    return p;
}

static inline char *_lstrdup (const char *func, int line, char *str)
{
    int n;
    char *s;
    if (str && (n = strlen (str) + 1) && (s = kmalloc (n, GFP_ATOMIC))) {
#ifdef OTG_MALLOC_TEST
        ++otg_mallocs;
#endif
#ifdef OTG_MALLOC_DEBUG
        printk(KERN_INFO"%s: %p %s %d %d\n", __FUNCTION__, s, func, line, otg_mallocs);
#endif
        return strcpy (s, str);
    }
    return NULL;
}

static inline void _lkfree (const char *func, int line, void *p)
{
    if (p) {
#ifdef OTG_MALLOC_TEST
        --otg_mallocs;
#endif
#ifdef OTG_MALLOC_DEBUG
        printk(KERN_INFO"%s:--1 %p %s %d %d\n", __FUNCTION__, p, func, line, otg_mallocs);
#endif
        kfree (p);
#ifdef MALLOC_TEST
        if (otg_mallocs < 0) {
                printk(KERN_INFO"%s: %p %s %d %d otg_mallocs less zero!\n", __FUNCTION__, p, func, line, otg_mallocs);
        }
#endif
#ifdef OTG_MALLOC_DEBUG
        if (otg_mallocs >= 0) {
            printk(KERN_INFO"%s:--2 %s %d NULL\n", __FUNCTION__, func, line);
        }
#endif
    }
}



/*! @} */

/* ********************************************************************************************** */
/*!
 * @name Task Allocation
 *
 * Long lived, low priority, high latency tasks.
 *
 * Tasks may use semaphores, mutexes, atomic operations and memory allocations.
 * @{
 *
 * TASKS are long lived, MAY HAVE low priority and high latency, the typical
 * implementation has the work item wait in a semaphore waiting for work. :w
 *
 * The TASK work functions are structured to run until terminated, using
 * a semaphore to wait for work.
 *
 * TASK creation:
 *      struct otg_task *task = otg_task_init("taskname", task_work, TAG);
 *      if (task)
 *              otg_task_start(task);
 *
 * TASK termination:
 *      otg_task_exit(task);
 *
 * TASK work notification
 *      otg_up_work(task);
 *
 * TASK work process
 *
 *      void task_work(void *data)
 *      {
 *           struct otg_task *task = (struct otg_task *)data;
 *           void *mydata = task->data;
 *           otg_up_admin(task);
 *           do {
 *                   // wait for work
 *                   otg_down_work(task);
 *                   // do work
 *           } while (!task->terminating);
 *           task->terminated = TRUE;
 *           otg_up_admin(task);
 *      }
 *
 * static struct otg_task *otg_task_init(char *name, void (*work) (void *), void *data, otg_tag_t tag)
 * static void otg_up_work(struct otg_task *task)
 * static void otg_up_admin(struct otg_task *task)
 * static void otg_down_work(struct otg_task *task)
 * static void otg_down_admin(struct otg_task *task)
 * static void otg_task_start(struct otg_task *task)
 * static void otg_task_exit(struct otg_task *task)
 *
 */

/* XXX CONFIG_OTG_TASK_WORK
 *
 * Linux Implementation Configuration
 *
 *      #define CONFIG_OTG_TASK_WORK
 *           Use work items only for TASK implemenation
 */

#if defined(LINUX26)
    #define SCHEDULE_WORK(item) schedule_work(&item)
#else /* LINUX26 */
    #define SCHEDULE_WORK(item) schedule_work(&item)
#endif /* LINUX26 */

/*! struct otg_task
 */
typedef void *otg_task_arg_t;
typedef void *(* otg_task_proc_t) (otg_task_arg_t data);
struct otg_task {
        #if !defined(CONFIG_OTG_TASK_WORK)
        struct workqueue_struct *work_queue;
        otg_sem_t               admin_sem;
        otg_sem_t               work_sem;
        #endif /* defined(CONFIG_OTG_TASK_WORK) */

        #if defined(LINUX26)
        struct work_struct      work;
        #else /* LINUX26 */
        struct work_struct      work;
        #endif /* LINUX26 */
        otg_task_proc_t         proc;
        BOOL                    terminate;
        BOOL                    terminated;
        otg_task_arg_t          *data;
        BOOL                    taskdebug;
        otg_tag_t               tag;
        char                    *name;
};

static void inline otg_sleep(int n)
{
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout( n * HZ );
}

/*! otg_up_admin
 * @brief Signal start task to re-start.
 * @param task - otg_task instance pointer
 */
static void inline otg_up_admin(struct otg_task *task)
{
        #if defined(CONFIG_OTG_TASK_WORK)
        /* NOTHING */
        #else /* defined(CONFIG_OTG_TASK_WORK) */
        //TRACE_STRING(task->tag, "UP ADMIN: %s", task->name);
        if (task->taskdebug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);
        otg_sem_post(&task->admin_sem);
        #endif /* defined(CONFIG_OTG_TASK_WORK) */
}

/*! otg_down_work
 *@brief  Used by work task to wait.
 *@param task - otg_task instance pointer
 */
static void inline otg_down_work(struct otg_task *task)
{
        #if defined(CONFIG_OTG_TASK_WORK)
        /* NOTHING */
        #else /* defined(CONFIG_OTG_TASK_WORK) */
        //TRACE_STRING(task->tag, "DOWN WORK: %s", task->name);
        if (task->taskdebug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);

        while (otg_sem_wait(&task->work_sem));
        #endif /* defined(CONFIG_OTG_TASK_WORK) */
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
/*! otg_task_proc - otg task handler
 * @param data - otg_task instance pointer
 */
static void inline otg_task_proc(otg_task_arg_t data)
#else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
/*! otg_task_proc - otg task handler
 * @param work - otg_task instance pointer
 */
static void inline otg_task_proc(struct work_struct *work)
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
{
        #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
        struct otg_task *task = (struct otg_task *)data;
        #else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
        struct otg_task *task = (struct otg_task *)container_of(work, struct otg_task, work);
        #endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
        otg_up_admin(task);
        do {
                // wait for work
                if (task->taskdebug)
                        printk(KERN_INFO"%s: DOWN %s\n", __FUNCTION__, task->name);

                otg_down_work(task);

                if (task->taskdebug)
                        printk(KERN_INFO"%s: WORKING %s\n", __FUNCTION__, task->name);

                task->proc(task->data);
        } while (!task->terminate);
        if (task->taskdebug)
                printk(KERN_INFO"%s: TERMINATING %s\n", __FUNCTION__, task->name);
        task->terminated = TRUE;
        otg_up_admin(task);
}

/*! otg_task_init
 *@brief Create otg task structure, create workqueue, initialize it.
 *@param name - name of task or workqueue
 *@param proc - handler
 *@param data - parameter pointer for handler
 *@param tag-
 *@return initialized otg_task instance pointer
 */
static inline struct otg_task *otg_task_init2(char *name, otg_task_proc_t proc, otg_task_arg_t data, otg_tag_t tag)
{
        struct otg_task *task;

        //TRACE_STRING(tag, "INIT: %s", name);

        RETURN_NULL_UNLESS((task = CKMALLOC(sizeof (struct otg_task))));

        task->tag = tag;
        task->data = data;
        task->name = name;
        task->proc = proc;

        #if defined(CONFIG_OTG_TASK_WORK)
        task->terminated = task->terminate = TRUE;
        #else /* defined(CONFIG_OTG_TASK_WORK) */
        task->terminated = task->terminate = FALSE;
        #if defined(LINUX26)
        THROW_UNLESS((task->work_queue = create_singlethread_workqueue(name)), error);
        #else /* LINUX26 */
        THROW_UNLESS((task->work_queue = create_workqueue(name)), error);
        #endif /* LINUX26 */
        //init_MUTEX_LOCKED(&task->admin_sem);
        //init_MUTEX_LOCKED(&task->work_sem);
        otg_sem_init_locked(name, &task->admin_sem);
        otg_sem_init_locked(name, &task->work_sem);
        #endif /* defined(CONFIG_OTG_TASK_WORK) */

        #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
        INIT_WORK(&task->work, otg_task_proc, task);
        #else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
        INIT_WORK(&task->work, otg_task_proc);
        #endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */

        return task;

        CATCH(error) {
                printk(KERN_INFO"%s: ERROR\n", __FUNCTION__);
                if (task) LKFREE(task);
                return NULL;
        }
}


/*! otg_up_work
 * @brief Signal work param taskto re-start.
 * @param task - otg_task instance pointer
 */
static void inline otg_up_work(struct otg_task *task)
{
        //TRACE_STRING(task->tag, "UP WORK: %s", task->name);

        if (task->taskdebug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);

        #if defined(CONFIG_OTG_TASK_WORK)
        //printk(KERN_INFO"%s: AAAA\n", __FUNCTION__);
        task->terminated = FALSE;
        SCHEDULE_WORK(task->work);
        #else /* defined(CONFIG_OTG_TASK_WORK) */
        //printk(KERN_INFO"%s: BBBB\n", __FUNCTION__);
        otg_sem_post(&task->work_sem);
        #endif /* defined(CONFIG_OTG_TASK_WORK) */
}

/*! otg_down_admin
 * @brief Used by admin task to wait.
 * @param task - otg_task instance pointer
 */
static void inline otg_down_admin(struct otg_task *task)
{
        #if defined(CONFIG_OTG_TASK_WORK)
        /* NOTHING */
        #else /* defined(CONFIG_OTG_TASK_WORK) */
        //TRACE_STRING(task->tag, "DOWN ADMIN: %s", task->name);
        if (task->taskdebug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);
        while (otg_sem_wait(&task->admin_sem));
        //DOWN(&task->admin_sem);
        #endif /* defined(CONFIG_OTG_TASK_WORK) */
}

/*! otg_task_start
 * @brief Start and wait for otg task.
 * @param task - otg_task instance pointer
 */
static void inline otg_task_start(struct otg_task *task)
{
        #if defined(CONFIG_OTG_TASK_WORK)
        /* NOTHING */
        #else /* defined(CONFIG_OTG_TASK_WORK) */
        //TRACE_STRING(task->tag, "START: %s", task->name);
        if (task->taskdebug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);

        /* schedule work item and  wait until it starts */
        queue_work(task->work_queue, &task->work);
        otg_down_admin(task);
        #endif /* defined(CONFIG_OTG_TASK_WORK) */
}

/*! otg_task_exit
 * @brief Terminate and wait for otg task.
 * @param task - otg_task instance pointer
 */
static void inline otg_task_exit(struct otg_task *task)
{
        TRACE_STRING(task->tag, "EXIT: %s", task->name);
        if (task->taskdebug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);

        #if defined(CONFIG_OTG_TASK_WORK)
        while (!task->terminated) {
                otg_sleep(1);
        }
        #else /* defined(CONFIG_OTG_TASK_WORK) */
        /* signal termination */
        task->terminate = TRUE;
        otg_up_work(task);
        otg_down_admin(task);

        /* destroy workqueue */
        flush_workqueue(task->work_queue);
        destroy_workqueue(task->work_queue);
        #endif /* defined(CONFIG_OTG_TASK_WORK) */

        LKFREE(task);
}


/* ********************************************************************************************** */
/*!
 * @name WorkItem Allocation
 *
 * Short lived, low priority, high latency task.
 *
 * @{
 *
 * Work items are similar to tasks except that they are not expected
 * to run for long periods of time or wait.
 *
 * A Work item function is run once and exits when the work
 * is finished.
 *
 * static struct otg_workitem *otg_workitem_init(char *name, void (*work) (unsigned long), unsigned long data, otg_tag_t tag)
 * static void otg_workitem_start(struct otg_workitem *tasklet)
 * static void otg_workitem_exit(struct otg_workitem *tasklet)
 *
 * WORK creation:
 *      struct otg_workitem *tasklet = otg_workitem_init("taskletname", tasklet_work, TAG);
 *
 * WORK start:
 *      otg_workitem_start(tasklet);
 *
 * WORK termination:
 *      otg_workitem_exit(tasklet);
 *
 * WORK work process
 *
 *      void task_work(void *data)
 *      {
 *           struct otg_workitem *tasklet = (struct otg_workitem *)data;
 *           void *mydata = task->data;
 *
 *           // do work
 *
 *      }
 *
 */

/*! struct otg_workitem
 */
typedef void *otg_workitem_arg_t;
typedef void *(* otg_workitem_proc_t) (otg_task_arg_t data);

struct otg_workitem {

        #if defined(LINUX26)
        struct workqueue_struct *work_queue;
        #endif /* defined(LINUX26) */

        struct work_struct      work;
        otg_workitem_proc_t     proc;
        BOOL                    terminate;
        BOOL                    terminated;
        otg_task_arg_t          *data;
        BOOL                    workdebug;
        otg_tag_t               tag;
        char                    *name;
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
/*! otg_workitem_run
 * @param data - workitem poiner
 */
static inline void otg_workitem_run(void *data)
#else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
/*! otg_workitem_run
 * @param work - work struct poiner
 */
static inline void otg_workitem_run(struct work_struct *work)
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
{
        #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
        struct otg_workitem *workitem = (struct otg_workitem *)data;
        #else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
        struct otg_workitem *workitem = (struct otg_workitem *)container_of(work, struct otg_workitem, work);
        #endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */

        if (workitem->workdebug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, workitem->name);

        workitem->proc(workitem->data);               // XXX convert to workitem->data
        workitem->terminated = TRUE;

        if (workitem->workdebug)
                printk(KERN_INFO"%s: %s finished\n", __FUNCTION__, workitem->name);

}

/*! otg_workitem_init
 * @brief Create otg work structure, create workqueue, initialize it.
 * @param name - workitem name
 * @param proc - workitem handler
 * @param data - workitem data
 * @param tag - otg tag
 * @return otg_workitem instance pointer
 */
static inline struct otg_workitem *otg_workitem_init(char *name, otg_workitem_proc_t proc, otg_workitem_arg_t data, otg_tag_t tag)
{
        struct otg_workitem *workitem;

        //TRACE_STRING(tag, "INIT: %s", name);

        //printk(KERN_INFO"%s: %s\n", __FUNCTION__, name);

        RETURN_NULL_UNLESS((workitem = CKMALLOC(sizeof (struct otg_workitem))));

        workitem->data = data;
        workitem->tag = tag;
        workitem->name = name;
        workitem->proc = proc;
        //workitem->workdebug = TRUE;

        workitem->terminated = workitem->terminate = TRUE;

        #if defined(LINUX26)
        THROW_UNLESS((workitem->work_queue = create_singlethread_workqueue(name)), error);
        #endif /* LINUX26 */
        #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
        INIT_WORK(&workitem->work, otg_workitem_run, workitem);
        #else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
        INIT_WORK(&workitem->work, otg_workitem_run);
        #endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */

        return workitem;

        CATCH(error) {
                printk(KERN_INFO"%s: ERROR\n", __FUNCTION__);
                if (workitem) LKFREE(workitem);
                return NULL;
        }
}

/*! otg_workitem_start
 * @brief Signal work work to run.
 * @param workitem - workitem pointer
 */
static void inline otg_workitem_start(struct otg_workitem *workitem)
{
        //TRACE_STRING(workitem->tag, "START: %s", workitem->name);

        if (workitem->workdebug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, workitem->name);

        //workitem->proc(workitem->data);
        workitem->terminated = FALSE;
        #if defined(LINUX26)
        queue_work(workitem->work_queue,&workitem->work);
        #else /* defined(LINUX26) */
        SCHEDULE_WORK(workitem->work);
        #endif /* defined(LINUX26) */
}

/*! otg_workitem_exit
 * @brief Terminate and wait for otg work.
 * @param workitem - workitem pointer
 */
static void inline otg_workitem_exit(struct otg_workitem *workitem)
{
        //TRACE_STRING(workitem->tag, "EXIT: %s", workitem->name);
        if (workitem->workdebug)
                printk(KERN_INFO"%s: %s terminating\n", __FUNCTION__, workitem->name);

        while (!workitem->terminated) {
                otg_sleep(1);
                if (workitem->workdebug)
                printk(KERN_INFO"%s: %s while loop terminating\n", __FUNCTION__, workitem->name);
        }

        if (workitem->workdebug)
                printk(KERN_INFO"%s: %s terminated\n", __FUNCTION__, workitem->name);
#if defined(LINUX26)
        /* destroy workqueue */
        flush_workqueue(workitem->work_queue);
        destroy_workqueue(workitem->work_queue);
#endif /* LINUX26 */
        LKFREE(workitem);
}


/*! @} */

/* ********************************************************************************************** */
/*!
 * @name Tasklet Allocation
 *
 * Short lived, high priority, low latency tasklets.
 *
 * Tasklets MAY NOT use memory allocation or wait on semaphores.
 *
 * Taskslets may use signal semaphores, mutexes and atomic operations.
 * @{
 *
 * TASKLETS are short lived, MUST run with low latency and SHOULD have high
 * priority.
 *
 * The TASKLET work function is structure to run once and exit when the work
 * is finished.
 *
 * static struct otg_tasklet *otg_tasklet_init(char *name, void (*work) (unsigned long), unsigned long data, otg_tag_t tag)
 * static void otg_tasklet_start(struct otg_tasklet *tasklet)
 * static void otg_tasklet_exit(struct otg_tasklet *tasklet)
 *
 * TASKLET creation:
 *      struct otg_tasklet *tasklet = otg_tasklet_init("taskletname", tasklet_work, TAG);
 *
 * TASKLET start:
 *      otg_tasklet_start(tasklet);
 *
 * TASKLET termination:
 *      otg_tasklet_exit(tasklet);
 *
 * TASKLET work process
 *
 *      void task_work(otg_tasklet_arg_t data)
 *      {
 *           void *mydata = data;
 *
 *           // do work
 *
 *      }
 *
 */
/* XXX CONFIG_OTG_TASKLET_WORK
 *
 * Linux Implementation Configuration
 *
 *      #define CONFIG_OTG_TASKLET_WORK
 *           Use work items only for TASK implemenation
 *
 * N.B. the Linux work item implementation for tasklets is not suitable
 * for full OTG Dual-Role implemenation.
 *
 */

/*! struct otg_tasklet
 */
#ifdef CONFIG_OTG_TASKLET_WORK
typedef void *otg_tasklet_arg_t;
#else /* CONFIG_OTG_TASKLET_WORK */
typedef unsigned long otg_tasklet_arg_t;
#endif /* CONFIG_OTG_TASKLET_WORK */
typedef void *(* otg_tasklet_proc_t) (otg_tasklet_arg_t data);
struct otg_tasklet {
        #ifdef CONFIG_OTG_TASKLET_WORK
        struct work_struct      work;
        #else /* CONFIG_OTG_TASKLET_WORK */
        struct tasklet_struct   tasklet;
        #endif /* CONFIG_OTG_TASKLET_WORK */
        BOOL                    terminated;
        otg_tasklet_proc_t      proc;
        otg_tasklet_arg_t       data;
        BOOL                    taskdebug;
        otg_tag_t               tag;
        char                    *name;
};

/*! otg_tasklet_run
 * @param data - otg_tasklet pointer
 */
#ifdef CONFIG_OTG_TASKLET_WORK
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
static inline void otg_tasklet_run(otg_tasklet_arg_t data)
#else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
static inline void otg_tasklet_run(struct work_struct *work)
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
#else /* CONFIG_OTG_TASKLET_WORK */
static inline void otg_tasklet_run(otg_tasklet_arg_t data)
#endif /* CONFIG_OTG_TASKLET_WORK */
{
        #ifdef CONFIG_OTG_TASKLET_WORK
        #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
        struct otg_tasklet *tasklet = (struct otg_tasklet *)data;
        #else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
        struct otg_tasklet *tasklet = (struct otg_tasklet *)container_of(work, struct otg_tasklet, work);
        #endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
        #else /* CONFIG_OTG_TASKLET_WORK */
        struct otg_tasklet *tasklet = (struct otg_tasklet *)data;
        #endif /* CONFIG_OTG_TASKLET_WORK */

        if (tasklet->taskdebug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, tasklet->name);

        tasklet->proc(tasklet->data);
        tasklet->terminated = TRUE;

        if (tasklet->taskdebug)
                printk(KERN_INFO"%s: %s finished\n", __FUNCTION__, tasklet->name);
}

/*! otg_tasklet_init
 * @brief Create otg task structure, create workqueue, initialize it.
 * @param name - otg_tasklet name
 * @param proc - otg_tasklet process
 * @param data - otg_taskle data, argument for proc
 * @param tag - otg_tasklet tag
 * @return initialized otg_tasklet instance pointer
 */
static inline struct otg_tasklet *otg_tasklet_init(char *name, otg_tasklet_proc_t proc, otg_tasklet_arg_t data, otg_tag_t tag)
{
        struct otg_tasklet *tasklet;

        //TRACE_STRING(tag, "INIT: %s", name);
        //printk(KERN_INFO"%s: %s\n", __FUNCTION__, name);

        RETURN_NULL_UNLESS((tasklet = CKMALLOC(sizeof (struct otg_tasklet))));

        tasklet->tag = tag;
        tasklet->name = name;
        tasklet->terminated = TRUE;
        tasklet->proc = proc;
        tasklet->data = data;

        #ifdef CONFIG_OTG_TASKLET_WORK
        INIT_WORK(&tasklet->work, otg_tasklet_run, tasklet);
        #else /* CONFIG_OTG_TASKLET_WORK */
        tasklet_init(&tasklet->tasklet, otg_tasklet_run, (otg_tasklet_arg_t) tasklet);
        #endif /* CONFIG_OTG_TASKLET_WORK */

        return tasklet;

        CATCH(error) {
                if (tasklet) LKFREE(tasklet);
                return NULL;
        }
}

/*! otg_tasklet_start
 * @brief Signal work task to re-start.
 * @param tasklet - otg_tasklet instance pointer
 */
static void inline otg_tasklet_start(struct otg_tasklet *tasklet)
{
        //TRACE_STRING(tag, "START: %s", name);
        if (tasklet->taskdebug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, tasklet->name);

        tasklet->terminated = FALSE;
        #ifdef CONFIG_OTG_TASKLET_WORK
        SCHEDULE_WORK(tasklet->work);
        #else /* CONFIG_OTG_TASKLET_WORK */
        tasklet_schedule(&tasklet->tasklet);
        #endif /* CONFIG_OTG_TASKLET_WORK */
}

/*! otg_tasklet_exit
 * @brief Terminate and wait for otg task.
 * @param tasklet - otg_tasklet instance pointer
 */
static void inline otg_tasklet_exit(struct otg_tasklet *tasklet)
{
        //TRACE_STRING(tag, "EXIT: %s", name);
        if (tasklet->taskdebug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, tasklet->name);

        #ifdef CONFIG_OTG_TASKLET_WORK
        while (!tasklet->terminated) {
                if (tasklet->taskdebug)
                        printk(KERN_INFO"%s: SLEEPING\n", __FUNCTION__);
                otg_sleep(1);
                if (tasklet->taskdebug)
                        printk(KERN_INFO"%s: RUNNING\n", __FUNCTION__);
        }
        #else /* CONFIG_OTG_TASKLET_WORK */
        tasklet_kill(&tasklet->tasklet);
        #endif /* CONFIG_OTG_TASKLET_WORK */

        LKFREE(tasklet);
}


/*! @} */

/* ********************************************************************************************** */
/*!
 * @name DMA Cache
 *
 * @{
 *
 *
 * #define CACHE_SYNC_RCV(buf, len) pci_map_single (NULL, (void *) buf, len, PCI_DMA_FROMDEVICE)
 * #define CACHE_SYNC_TX(buf, len) consistent_sync (NULL, buf, len, PCI_DMA_TODEVICE)
 */
/*! @} */
#define CACHE_SYNC_RCV(buf, len) pci_map_single (NULL, (void *) buf, len, PCI_DMA_FROMDEVICE)
// XXX define CONFIG_OTG_LD_TX_CACHE if you are on an older kernel that uses the
// old consistent_sync() api
//
//#define CONFIG_OTG_OLD_TX_CACHE 0
//
#if defined(CONFIG_OTG_OLD_TX_CACHE)
#define CACHE_SYNC_TX(buf, len) consistent_sync (NULL, buf, len, PCI_DMA_TODEVICE)
#else
#define CACHE_SYNC_TX(buf, len) dma_cache_maint (buf, len, PCI_DMA_TODEVICE)
#endif

/*@}*/



/* ********************************************************************************************** */
/*! @name likely and unlikely
 *
 * Under linux these can be used to provide a hint to GCC that
 * the expression being evaluated is probably going to evaluate
 * to true (likely) or false (unlikely). The intention is that
 * GCC can then provide optimal code for that.
 * @{
 */
#ifndef likely
#define likely(x) x
#endif
#define otg_likely(x) likely(x)

#ifndef unlikely
#define unlikely(x) x
#endif
#define otg_unlikely(x) unlikely(x)
/* @} */




/* ********************************************************************************************** */
/*
 * XXX Deprecated and/or move to platform or architecture specific
 */



// Common to all supported versions of Linux ??

#if 0
#include <linux/list.h>
#define otg_list_node list_head
#define LIST_NODE struct list_head
#define LIST_NODE_INIT(name) struct list_head name = {&name, &name}
#define INIT_LIST_NODE(ptr) INIT_LIST_HEAD(ptr)
#define LIST_ENTRY(pointer, type, member) list_entry(pointer, type, member)
#define LIST_FOR_EACH(cursor, head) list_for_each(cursor, head)
#define LIST_ADD_TAIL(n,h) list_add_tail(n,h)
#define LIST_DEL(h) list_del(&(h))
#else
#include <otg/otg-list.h>
#endif

/*! @} */


/*!@name Scheduling Primitives
 *
 * WORK_STRUCT
 * WORK_ITEM
 *
 * SET_WORK_ARG()\n
 * SCHEDULE_IMMEDIATE_WORK()\n
 * NO_WORK_DATA()\n
 * MOD_DEC_USE_COUNT\n
 * MOD_INC_USE_COUNT\n
 */

/*! @{ */


/* Separate Linux 2.4 and 2.6 versions of scheduling primitives */
#if defined(LINUX26)
    #include <linux/workqueue.h>

    #define OLD_WORK_STRUCT  work_struct
    #define WORK_ITEM    work_struct
    #define OLD_WORK_ITEM    work_struct
    typedef struct OLD_WORK_ITEM OLD_WORK_ITEM;
    #if 0
        #define PREPARE_WORK_ITEM(__item,__routine,__data) INIT_WORK((__item),(__routine),(__data))
    #else
        #include <linux/interrupt.h>
        #define PREPARE_WORK_ITEM(__item,__routine,__data) __prepare_work(&(__item),(__routine),(__data))
        static inline void __prepare_work(struct work_struct *_work,
            void (*_routine),
            void * _data){
            INIT_LIST_HEAD(&_work->entry);
            _work->func = _routine;
            #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
            _work->data = _data;
            _work->pending = 0;
            init_timer(&_work->timer);
            #else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
            *work_data_bits(_work) = (long)_data;
            #endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
        }
    #endif
    //#undef PREPARE_WORK
    typedef void (* WORK_PROC)(void *);

    #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
            #define SET_WORK_ARG(__item, __data) (__item).data = __data
            #define PENDING_WORK_ITEM(item) (item.pending != 0)
            #define NO_WORK_DATA(item) (!item.data)
    #else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */
            #define SET_WORK_ARG(__item, __data) (*work_data_bits(&(__item)) = (long)__data)
            #define NO_WORK_DATA(item) (!(*work_data_bits(&(item))))
            #define WORK_DATA(item) (*work_data_bits(&(item)))
            #define PENDING_WORK_ITEM(item) (work_pending(&(item)))
    #endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20) */


    #define SCHEDULE_DELAYED_WORK(item) schedule_delayed_work(&item, 0)
    #define SCHEDULE_IMMEDIATE_WORK(item) SCHEDULE_WORK((item))


    //#define _MOD_DEC_USE_COUNT   //Not used in 2.6
    //#define _MOD_INC_USE_COUNT   //Not used in 2.6

    //#define MODPARM(a) _##a

    //#define MOD_PARM_BOOL(a, d, v) static int _##a = v; module_param_named(a, _##a, bool, 0); MODULE_PARM_DESC(a, d)
    //#define MOD_PARM_INT(a, d, v) static int _##a = v; module_param_named(a, _##a, uint, 0); MODULE_PARM_DESC(a, d)
    //#define MOD_PARM_STR(a, d, v) static char * _##a = v; module_param_named(a, _##a, charp, 0); MODULE_PARM_DESC(a, d)
    #define OTG_INTERRUPT  0   /* SA_INTERRUPT in some environments */




#else /* LINUX26 */

    #define OLD_WORK_STRUCT  tq_struct
    #define OLD_WORK_ITEM    tq_struct
    typedef struct OLD_WORK_ITEM OLD_WORK_ITEM;
    #define PREPARE_WORK_ITEM(item,work_routine,work_data) { item.routine = work_routine; item.data = work_data; }
    #define SET_WORK_ARG(__item, __data) (__item).data = __data
    #define NO_WORK_DATA(item) (!(item).data)
    #define PENDING_WORK_ITEM(item) (item.sync != 0)
    //#define _MOD_DEC_USE_COUNT   MOD_DEC_USE_COUNT
    //#define _MOD_INC_USE_COUNT   MOD_INC_USE_COUNT

    //#define MODPARM(a) a

    //#define MOD_PARM_BOOL(a, d, v) static int a = v; MOD_PARM(a, "i"); MOD_PARM_DESC(a, d);
    //#define MOD_PARM_INT(a, d, v) static int a = v; MOD_PARM(a, "i"); MOD_PARM_DESC(a, d);
    //#define MOD_PARM_STR(a, d, v) static char * a = v; MOD_PARM(a, "s"); MOD_PARM_DESC(a, d);

    typedef void (* WORK_PROC)(void *);

    #define OTG_INTERRUPT  0   /* SA_INTERRUPT in some environments */
#endif /* LINUX26 */

/*! @} */

/*! @name Linux Module support
 */
/*@{*/
#if !defined(_LINUX_MODULE_H)
    #include <linux/module.h>
#endif /* _LINUX_MODULE_H */

#if defined(MODULE)

    #if defined(OTG_EXTRA_INFO)
    #warning MODULE DEFINED
    #endif

    #define MOD_EXIT(exit_routine) module_exit(exit_routine)
    #define MOD_INIT(init_routine) module_init(init_routine)
    #define MOD_PROC(proc) (proc)
    #define MOD_AUTHOR(string) MODULE_AUTHOR(string)
    #define MOD_PARM(param, type) MODULE_PARM(param, type)
    #define MOD_PARM_DESC(param,desc) MODULE_PARM_DESC(param, desc)
    #define MOD_DESCRIPTION(description) MODULE_DESCRIPTION(description)
    #define OTG_EXPORT_SYMBOL(symbol) EXPORT_SYMBOL(symbol)
    #define OTG_EPILOGUE  1 /* EPILOGUE ROUTINE NEEDED */

    #if defined(LINUX26)
    #define MODPARM(a) _##a
    #define MOD_PARM_BOOL(a, d, v) static int _##a = v; module_param_named(a, _##a, bool, 0); MODULE_PARM_DESC(a, d)
    #define MOD_PARM_INT(a, d, v) static int _##a = v; module_param_named(a, _##a, uint, 0); MODULE_PARM_DESC(a, d)
    #define MOD_PARM_STR(a, d, v) static char * _##a = v; module_param_named(a, _##a, charp, 0); MODULE_PARM_DESC(a, d)
    #else /* defined(LINUX26) */
    #define MODPARM(a) a
    #define MOD_PARM_BOOL(a, d, v) static int a = v; MOD_PARM(a, "i"); MOD_PARM_DESC(a, d);
    #define MOD_PARM_INT(a, d, v) static int a = v; MOD_PARM(a, "i"); MOD_PARM_DESC(a, d);
    #define MOD_PARM_STR(a, d, v) static char * a = v; MOD_PARM(a, "s"); MOD_PARM_DESC(a, d);
    #endif /* defined(LINUX26) */

#else /* defined(MODULE) */
    #if defined(OTG_EXTRA_INFO)
    #warning "Modules are not enabled for this kernel"
    #endif


    #define MOD_EXIT(exit_routine)
    #define MOD_INIT(init_routine) module_init(init_routine)
    #define MOD_PROC(proc) NULL
    #define MOD_AUTHOR(string)
    #define MOD_PARM(param, type)
    #define MOD_PARM_DESC(param,desc)
    #define MOD_DESCRIPTION(description)
    #define OTG_EXPORT_SYMBOL(symbol) //EXPORT_SYMBOL(symbol)
    #undef EXPORT_SYMBOL
    #define OTG_EPILOGUE 0  /* EPILOGUE ROUTINE NOT NEEDED */

    #define MODPARM(a) a
    #define MOD_PARM_BOOL(a, d, v) static int a = v; MOD_PARM(a, "i"); MOD_PARM_DESC(a, d);
    #define MOD_PARM_INT(a, d, v) static int a = v; MOD_PARM(a, "i"); MOD_PARM_DESC(a, d);
    #define MOD_PARM_STR(a, d, v) static char * a = v; MOD_PARM(a, "s"); MOD_PARM_DESC(a, d);

#endif /* defined(MODULE) */

//#undef MODULE_AUTHOR
//#undef MODULE_DESCRIPTION
//#undef MODULE_PARM_DESC
//#undef MODULE_PARM
#if defined(LINUX24) || defined(LINUX26)
    #include <linux/version.h>
    #if defined(MODULE) && (LINUX_VERSION_CODE >= KERNEL_VERSION (2,4,17))
    //"GPL License applies under certain cirumstances; consult your vendor for details"
        #define EMBED_LICENSE() MODULE_LICENSE ("GPL")
    #else
        #define EMBED_LICENSE()   //Operation not supported for earlier Linux kernels
    #endif

#else /* defined(LINUX24) || defined(LINUX26) */
    #error "Need to define EMBED_LICENSE for the current operating system"
#endif /* defined(LINUX24) || defined(LINUX26) */
#define EMBED_MODULE_INFO(section,moduleinfo) static char __##section##_module_info[] = moduleinfo "tt/root@belcarra.com/debian286.bbb"
#define EMBED_USBD_INFO(moduleinfo) EMBED_MODULE_INFO(usbd,moduleinfo)
#define GET_MODULE_INFO(section) __##section##_module_info

/*! @} */

/* ********************************************************************************************* */
/* Linux specific - not part of otg-os.h implementation
 */


#if defined(LINUX26)
/*! @name OTG Bus Type
 */
/*@{*/
extern struct bus_type otg_bus_type;
/*@}*/
#endif /* defined(LINUX26) */



#if defined(LINUX26)
    #include <linux/gfp.h>
#define GET_KERNEL_PAGE()  __get_free_page(GFP_KERNEL)

#else /* LINUX26 */

    #include <linux/mm.h>
    #define GET_KERNEL_PAGE() get_free_page(GFP_KERNEL)
    #if !defined(IRQ_HANDLED)
        // Irq's
        typedef void irqreturn_t;
        #define IRQ_NONE
        #define IRQ_HANDLED
        #define IRQ_RETVAL(x)
    #endif
#endif /* LINUX26 */


#define LINUX_VERSION(a,b,c) (LINUX_VERSION_CODE >= KERNEL_VERSION(a,b,c))

static otg_tick_t inline otg_do_div(otg_tick_t a, otg_tick_t b)
{
        // XXX au1x00 has u64 ticks for example

        #ifdef TICKS_IS_U64

        #if defined(LINUX26)
        return do_div(a, b);
        #else /* defined(LINUX26) */
        return (a / b);
        #endif /* defined(LINUX26) */

        #else /* TICKS_IS_U64 */
        return (a / b);
        #endif /* TICKS_IS_U64 */
}



#endif /* _OTG_LINUX_H */
