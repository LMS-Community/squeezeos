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
xxxxx
xxxx
#if 0
/*
 * otg/otg/otg-task.h
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/otg-task.h|20061017072623|50255
 *
 *	Copyright (c) 2006 Belcarra Technologies 2005 Corp
 *
 */

/*!
 * @file otg/otg/otg-linux.h
 * @brief Linux OS Compatibility defines
 *
 * @ingroup OTGCore
 */
#ifndef _OTG_TASK_H
#define _OTG_TASK_H 1

#if !defined(_OTG_MODULE_H)
#include <otg/otg-module.h>
#endif


/*! @name OTG Task API
 *
 */

/*@{*/
/*
 */


/* XXX OTG_TASK_WORK
 *
 * Linux Implementation Configuration
 *
 *      #define OTG_TASK_WORK
 *           Use work items only for TASK implemenation
 *      #define OTG_TASKLET_WORK
 *           Use work items only for TASK implemenation
 *
 * N.B. the work item implementation for tasklets is not suitable
 * for full OTG Dual-Role implemenation.
 *
 */

#define OTG_TASK_WORK
//#undef OTG_TASK_WORK

//#define OTG_TASKLET_WORK
#undef OTG_TASKLET_WORK



/*! struct otg_task
 */
typedef void *otg_task_arg_t;
typedef void (* otg_task_proc_t) (otg_task_arg_t data);
struct otg_task {
        #if !defined(OTG_TASK_WORK)
        struct workqueue_struct *work_queue;
        struct semaphore        admin_sem;
        struct semaphore        work_sem;
        #endif /* defined(OTG_TASK_WORK) */
        struct work_struct      work;
        BOOL                    terminate;
        BOOL                    terminated;
        otg_task_arg_t          *data;
        BOOL                    debug;
        otg_tag_t               tag;
        char                    *name;
};


/*! otg_task_init
 * Create otg task structure, create workqueue, initialize it.
 */
static inline struct otg_task *otg_task_init(char *name, otg_task_proc_t work, otg_task_arg_t data, otg_tag_t tag)
{
        struct otg_task *task;

        //TRACE_STRING(tag, "INIT: %s", name);

        printk(KERN_INFO"%s: %s\n", __FUNCTION__, name);

        RETURN_NULL_UNLESS((task = CKMALLOC(sizeof (struct otg_task), GFP_KERNEL)));

        task->data = data;
        task->tag = tag;
        task->name = name;

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

        INIT_WORK(&task->work, work, task);

        return task;

        CATCH(error) {
                printk(KERN_INFO"%s: ERROR\n", __FUNCTION__);
                if (task) LKFREE(task);
                return NULL;
        }
}

/*! otg_up_work
 * Signal work task to re-start.
 */
static void inline otg_up_work(struct otg_task *task)
{
        //TRACE_STRING(task->tag, "UP WORK: %s", task->name);

        if (task->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);

        #if defined(OTG_TASK_WORK)
        task->terminated = FALSE;
        SCHEDULE_WORK(task->work);
        #else /* defined(OTG_TASK_WORK) */
        UP(&task->work_sem);
        #endif /* defined(OTG_TASK_WORK) */
}

/*! otg_up_admin
 * Signal start task to re-start.
 */
static void inline otg_up_admin(struct otg_task *task)
{
        #if defined(OTG_TASK_WORK)
        /* NOTHING */
        #else /* defined(OTG_TASK_WORK) */
        //TRACE_STRING(task->tag, "UP ADMIN: %s", task->name);
        if (task->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);
        UP(&task->admin_sem);
        #endif /* defined(OTG_TASK_WORK) */
}

/*! otg_down_work
 * Used by work task to wait.
 */
static void inline otg_down_work(struct otg_task *task)
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

/*! otg_down_admin
 * Used by admin task to wait.
 */
static void inline otg_down_admin(struct otg_task *task)
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
 */
static void inline otg_task_start(struct otg_task *task)
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
 */
static void inline otg_task_exit(struct otg_task *task)
{
        TRACE_STRING(task->tag, "EXIT: %s", task->name);
        if (task->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, task->name);

        #if defined(OTG_TASK_WORK)
        while (!task->terminated) {
                SCHEDULE_TIMEOUT(10 * HZ);
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
        otg_task_arg_t          data;
        BOOL                    debug;
        otg_tag_t               tag;
        char                    *name;
};



/*! otg_tasklet_init
 * Create otg task structure, create workqueue, initialize it.
 */
static inline struct otg_tasklet *otg_tasklet_init(char *name, otg_tasklet_proc_t work, otg_tasklet_arg_t data, otg_tag_t tag)
{
        struct otg_tasklet *tasklet;

        //TRACE_STRING(tag, "INIT: %s", name);
        printk(KERN_INFO"%s: %s\n", __FUNCTION__, name);

        RETURN_NULL_UNLESS((tasklet = CKMALLOC(sizeof (struct otg_tasklet), GFP_KERNEL)));

        tasklet->tag = tag;
        tasklet->name = name;
        tasklet->terminated = TRUE;

        #ifdef OTG_TASKLET_WORK
        INIT_WORK(&tasklet->work, work, data);
        #else /* OTG_TASKLET_WORK */
        tasklet_init(&tasklet->tasklet, work, data);
        #endif /* OTG_TASKLET_WORK */

        return tasklet;

        CATCH(error) {
                if (tasklet) LKFREE(tasklet);
                return NULL;
        }
}

/*! otg_tasklet_start
 * Signal work task to re-start.
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
 */
static void inline otg_tasklet_exit(struct otg_tasklet *tasklet)
{
        //TRACE_STRING(tag, "EXIT: %s", name);
        if (tasklet->debug)
                printk(KERN_INFO"%s: %s\n", __FUNCTION__, tasklet->name);

        #ifdef OTG_TASKLET_WORK
        while (!tasklet->terminated) {
                printk(KERN_INFO"%s: SLEEPING\n", __FUNCTION__);
                SCHEDULE_TIMEOUT(10 * HZ);
                printk(KERN_INFO"%s: RUNNING\n", __FUNCTION__);
        }
        #else /* OTG_TASKLET_WORK */
        tasklet_kill(&tasklet->tasklet);
        #endif /* OTG_TASKLET_WORK */

        LKFREE(tasklet);
}


/*! @} */

#endif /* _OTG_TASK_H */
#endif
