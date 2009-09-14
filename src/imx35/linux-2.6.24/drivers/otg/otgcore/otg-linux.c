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
#if 0
/*
 * otg/otgcore/otg-linux.c
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otgcore/otg-linux.c|20070920072547|06330
 *
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 */
xxx
xxx

/*!
 * @file otg/otgcore/otg-linux.c
 * @brief Linux OS Compatibility defines
 *
 * @ingroup OTGINIT
 */

#include <otg/otg-compat.h>
#if defined(CONFIG_OTG_LNX)

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


/* ********************************************************************************************** */

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

/* ********************************************************************************************** */
/* otg-trace - included so that TRACE_MSG can be used if desired to debug
 * the otg-xxx implementation
 */

#include <otg/otg-trace.h>


/* ********************************************************************************************** */


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

/* XXX OTG_TASK_WORK
 *
 * Linux Implementation Configuration
 *
 *      #define OTG_TASK_WORK
 *           Use work items only for TASK implemenation
 */

#define OTG_TASK_WORK
//#undef OTG_TASK_WORK

#if defined(LINUX26)
    #define SCHEDULE_WORK(item) schedule_work(&item)
#else /* LINUX26 */
    #define SCHEDULE_WORK(item) schedule_task(&item)
#endif /* LINUX26 */

/*! struct otg_task
 */
typedef void *otg_task_arg_t;
typedef void (* otg_task_proc_t) (otg_task_arg_t data);
struct otg_task {
        #if !defined(OTG_TASK_WORK)
        struct workqueue_struct *work_queue;
        otg_sem_t               admin_sem;
        otg_sem_t               work_sem;
        #endif /* defined(OTG_TASK_WORK) */
        struct work_struct      work;
        otg_task_proc_t         proc;
        BOOL                    terminate;
        BOOL                    terminated;
        otg_task_arg_t          *data;
        BOOL                    debug;
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
void otg_up_admin(struct otg_task *task)
{
        #if defined(OTG_TASK_WORK)
        /* NOTHING */
        #else /* defined(OTG_TASK_WORK) */
        //TRACE_STRING(task->tag, "UP ADMIN: %s", task->name);
        if (task->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);
        up(&task->admin_sem);
        #endif /* defined(OTG_TASK_WORK) */
}

/*! otg_down_work
 *@brief  Used by work task to wait.
 *@param task - otg_task instance pointer
 */
void otg_down_work(struct otg_task *task)
{
        #if defined(OTG_TASK_WORK)
        /* NOTHING */
        #else /* defined(OTG_TASK_WORK) */
        //TRACE_STRING(task->tag, "DOWN WORK: %s", task->name);
        if (task->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);

        while (down_interruptible(&task->work_sem));
        #endif /* defined(OTG_TASK_WORK) */
}

/*! otg_task_proc - otg task handler
 * @param data - otg_task instance pointer
 */
void otg_task_proc(otg_task_arg_t data)
{
        struct otg_task *task = (struct otg_task *)data;
        otg_up_admin(task);
        do {
                // wait for work
                if (task->debug)
                        printk(KERN_INFO"%s: DOWN %s\n", __FUNCTION__, task->name);

                otg_down_work(task);

                if (task->debug)
                        printk(KERN_INFO"%s: WORKING %s\n", __FUNCTION__, task->name);

                task->proc(task->data);

        } while (!task->terminate);
        if (task->debug)
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
struct otg_task *otg_task_init2(char *name, otg_task_proc_t proc, otg_task_arg_t data, otg_tag_t tag)
{
        struct otg_task *task;

        //TRACE_STRING(tag, "INIT: %s", name);

        RETURN_NULL_UNLESS((task = CKMALLOC(sizeof (struct otg_task))));

        task->tag = tag;
        task->data = data;
        task->name = name;
        task->proc = proc;

        #if defined(OTG_TASK_WORK)
        task->terminated = task->terminate = TRUE;
        #else /* defined(OTG_TASK_WORK) */
        task->terminated = task->terminate = FALSE;
        #if defined(LINUX26)
        THROW_UNLESS((task->work_queue = create_singlethread_workqueue(name)), error);
        #else /* LINUX26 */
        THROW_UNLESS((task->work_queue = create_workqueue(name)), error);
        #endif /* LINUX26 */
        init_MUTEX_LOCKED(&task->admin_sem);
        init_MUTEX_LOCKED(&task->work_sem);
        #endif /* defined(OTG_TASK_WORK) */

        INIT_WORK(&task->work, otg_task_proc, task);

        return task;

        CATCH(error) {
                printk(KERN_INFO"%s: ERROR\n", __FUNCTION__);
                if (task) LKFREE(task);
                return NULL;
        }
}

/*! otg_up_work
 * Signal work param taskto re-start.
 * @param task - otg_task instance pointer
 */
void otg_up_work(struct otg_task *task)
{
        //TRACE_STRING(task->tag, "UP WORK: %s", task->name);

        if (task->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);

        #if defined(OTG_TASK_WORK)
        task->terminated = FALSE;
        SCHEDULE_WORK(task->work);
        #else /* defined(OTG_TASK_WORK) */
        up(&task->work_sem);
        #endif /* defined(OTG_TASK_WORK) */
}

/*! otg_down_admin
 * Used by admin task to wait.
 * @param task - otg_task instance pointer
 */
void otg_down_admin(struct otg_task *task)
{
        #if defined(OTG_TASK_WORK)
        /* NOTHING */
        #else /* defined(OTG_TASK_WORK) */
        //TRACE_STRING(task->tag, "DOWN ADMIN: %s", task->name);
        if (task->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);
        while (down_interruptible(&task->admin_sem));
        //DOWN(&task->admin_sem);
        #endif /* defined(OTG_TASK_WORK) */
}

/*! otg_task_start
 * Start and wait for otg task.
 * @param task - otg_task instance pointer
 */
void otg_task_start(struct otg_task *task)
{
        #if defined(OTG_TASK_WORK)
        /* NOTHING */
        #else /* defined(OTG_TASK_WORK) */
        //TRACE_STRING(task->tag, "START: %s", task->name);
        if (task->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);

        /* schedule work item and  wait until it starts */
        queue_work(task->work_queue, &task->work);
        otg_down_admin(task);
        #endif /* defined(OTG_TASK_WORK) */
}

/*! otg_task_exit
 * Terminate and wait for otg task.
 * @param task - otg_task instance pointer
 */
void otg_task_exit(struct otg_task *task)
{
        TRACE_STRING(task->tag, "EXIT: %s", task->name);
        if (task->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);

        #if defined(OTG_TASK_WORK)
        while (!task->terminated) {
                otg_sleep( 1 );
        }
        #else /* defined(OTG_TASK_WORK) */
        /* signal termination */
        task->terminate = TRUE;
        otg_up_work(task);
        otg_down_admin(task);

        /* destroy workqueue */
        flush_workqueue(task->work_queue);
        destroy_workqueue(task->work_queue);
        #endif /* defined(OTG_TASK_WORK) */

        LKFREE(task);
}

/*! otg_sleep -schedule current task to start before n seconds
 * @param n - timeout for schedule
 */


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
typedef void (* otg_workitem_proc_t) (otg_task_arg_t data);

struct otg_workitem {
        struct work_struct      work;
        otg_workitem_proc_t     proc;
        BOOL                    terminate;
        BOOL                    terminated;
        otg_task_arg_t          *data;
        BOOL                    debug;
        otg_tag_t               tag;
        char                    *name;
};

/*! otg_workitem_run - run workitem process
 * @param data - workite poiner
 */
static inline void otg_workitem_run(void *data)
{
        struct otg_workitem *workitem = (struct otg_workitem *)data;

        if (workitem->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, workitem->name);

        workitem->proc(workitem->data);               // XXX convert to workitem->data
        workitem->terminated = TRUE;

        if (workitem->debug)
                printk(KERN_INFO"%s: %s finished\n", __FUNCTION__, workitem->name);

}

/*! otg_workitem_init
 * Create otg work structure, create workqueue, initialize it.
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
        //workitem->debug = TRUE;

        workitem->terminated = workitem->terminate = TRUE;

        INIT_WORK(&workitem->work, otg_workitem_run, workitem);

        return workitem;

        CATCH(error) {
                printk(KERN_INFO"%s: ERROR\n", __FUNCTION__);
                if (workitem) LKFREE(workitem);
                return NULL;
        }
}

/*! otg_workitem_start
 * Signal work work to run.
 * @param workitem - workitem pointer
 */
static void inline otg_workitem_start(struct otg_workitem *workitem)
{
        //TRACE_STRING(workitem->tag, "START: %s", workitem->name);

        if (workitem->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, workitem->name);

        workitem->proc(workitem->data);
        workitem->terminated = FALSE;
        SCHEDULE_WORK(workitem->work);
}

/*! otg_workitem_exit
 * Terminate and wait for otg work.
 * @param workitem - workitem pointer
 */
static void inline otg_workitem_exit(struct otg_workitem *workitem)
{
        //TRACE_STRING(workitem->tag, "EXIT: %s", workitem->name);
        if (workitem->debug)
                printk(KERN_INFO"%s: %s terminating\n", __FUNCTION__, workitem->name);

        while (!workitem->terminated) {
                otg_sleep( 1 );
        }

        if (workitem->debug)
                printk(KERN_INFO"%s: %s terminated\n", __FUNCTION__, workitem->name);

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
/* XXX OTG_TASKLET_WORK
 *
 * Linux Implementation Configuration
 *
 *      #define OTG_TASKLET_WORK
 *           Use work items only for TASK implemenation
 *
 * N.B. the Linux work item implementation for tasklets is not suitable
 * for full OTG Dual-Role implemenation.
 *
 */

#define OTG_TASKLET_WORK
#undef OTG_TASKLET_WORK

/*! struct otg_tasklet
 */
#ifdef OTG_TASKLET_WORK
typedef void *otg_tasklet_arg_t;
#else /* OTG_TASKLET_WORK */
typedef unsigned long otg_tasklet_arg_t;
#endif /* OTG_TASKLET_WORK */
typedef void (* otg_tasklet_proc_t) (otg_tasklet_arg_t data);
struct otg_tasklet {
        #ifdef OTG_TASKLET_WORK
        struct work_struct      work;
        #else /* OTG_TASKLET_WORK */
        struct tasklet_struct   tasklet;
        #endif /* OTG_TASKLET_WORK */
        BOOL                    terminated;
        otg_tasklet_proc_t      proc;
        otg_tasklet_arg_t       data;
        BOOL                    debug;
        otg_tag_t               tag;
        char                    *name;
};

/*! otg_tasklet_run - statr otg_tasklet process
 * @param data - otg_tasklet pointer
 */
static inline void otg_tasklet_run(otg_tasklet_arg_t data)
{
        struct otg_tasklet *tasklet = (struct otg_tasklet *)data;

        if (tasklet->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, tasklet->name);

        tasklet->proc(tasklet->data);
        tasklet->terminated = TRUE;

        if (tasklet->debug)
                printk(KERN_INFO"%s: %s finished\n", __FUNCTION__, tasklet->name);
}

/*! otg_tasklet_init
 * Create otg task structure, create workqueue, initialize it.
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

        #ifdef OTG_TASKLET_WORK
        INIT_WORK(&tasklet->work, otg_tasklet_run, tasklet);
        #else /* OTG_TASKLET_WORK */
        tasklet_init(&tasklet->tasklet, otg_tasklet_run, (otg_tasklet_arg_t) tasklet);
        #endif /* OTG_TASKLET_WORK */

        return tasklet;

        CATCH(error) {
                if (tasklet) LKFREE(tasklet);
                return NULL;
        }
}

/*! otg_tasklet_start
 * Signal work task to re-start.
 * @param tasklet - otg_tasklet instance pointer
 */
static void inline otg_tasklet_start(struct otg_tasklet *tasklet)
{
        //TRACE_STRING(tag, "START: %s", name);
        if (tasklet->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, tasklet->name);

        tasklet->terminated = FALSE;
        #ifdef OTG_TASKLET_WORK
        SCHEDULE_WORK(tasklet->work);
        #else /* OTG_TASKLET_WORK */
        tasklet_schedule(&tasklet->tasklet);
        #endif /* OTG_TASKLET_WORK */
}

/*! otg_tasklet_exit
 * Terminate and wait for otg task.
 * @param tasklet - otg_tasklet instance pointer
 */
static void inline otg_tasklet_exit(struct otg_tasklet *tasklet)
{
        //TRACE_STRING(tag, "EXIT: %s", name);
        if (tasklet->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, tasklet->name);

        #ifdef OTG_TASKLET_WORK
        while (!tasklet->terminated) {
                if (tasklet->debug)
                        printk(KERN_INFO"%s: SLEEPING\n", __FUNCTION__);
                otg_sleep( 1 );
                if (tasklet->debug)
                        printk(KERN_INFO"%s: RUNNING\n", __FUNCTION__);
        }
        #else /* OTG_TASKLET_WORK */
        tasklet_kill(&tasklet->tasklet);
        #endif /* OTG_TASKLET_WORK */

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
#if defined(CONFIG_OTG_NEW_TX_CACHE)
#define CACHE_SYNC_TX(buf, len) consistent_sync (NULL, buf, len, PCI_DMA_TODEVICE)
#else
#define CACHE_SYNC_TX(buf, len) consistent_sync (buf, len, PCI_DMA_TODEVICE)
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

#if 1
#include <linux/list.h>
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
            _work->pending = 0;
            _work->func = _routine;
            _work->data = _data;
            init_timer(&_work->timer);
        }
    #endif
    //#undef PREPARE_WORK
    typedef void (* WORK_PROC)(void *);

    #define SET_WORK_ARG(__item, __data) (__item).data = __data

    #define SCHEDULE_DELAYED_WORK(item) schedule_delayed_work(&item, 0)
    #define SCHEDULE_IMMEDIATE_WORK(item) SCHEDULE_WORK((item))
    #define PENDING_WORK_ITEM(item) (item.pending != 0)
    #define NO_WORK_DATA(item) (!item.data)
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

    #define MODPARM(a) a

    //#define MOD_PARM_BOOL(a, d, v) static int a = v; MOD_PARM(a, "i"); MOD_PARM_DESC(a, d);
    //#define MOD_PARM_INT(a, d, v) static int a = v; MOD_PARM(a, "i"); MOD_PARM_DESC(a, d);
    //#define MOD_PARM_STR(a, d, v) static char * a = v; MOD_PARM(a, "s"); MOD_PARM_DESC(a, d);

    typedef void (* WORK_PROC)(void *);

    #define OTG_INTERRUPT  0   /* SA_INTERRUPT in some environments */
#endif /* LINUX26 */

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



#endif /* defined(CONFIG_OTG_LNX) */
#endif
