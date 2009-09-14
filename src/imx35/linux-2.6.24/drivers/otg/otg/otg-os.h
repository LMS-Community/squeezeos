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
 * otg/otg/otg-os.h - USB Device Bus Interface Driver Interface
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/otg-os.h|20061101065829|45204
 *
 *	Copyright (c) 2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */


/*!
 * @name OTGCORE OTG OS Definitions
 * This contains the OTG OS structures and definitions.
 * @{
 *
 * This file should be included after an os specific library implementation
 * to ensure that the API is correctly implemneted.
 *
 * OTG Task
 *      Long lived, low priority, high latency tasks.
 *
 * OTG WorkItem
 *      Short lived, low priority, high latency task.
 *
 * OTG Tasklet
 *      Short lived, high priority, low latency tasklets.
 *
 */

/*!
 * @name Compilation Support
 *
 * @{
 *
 * Macros to support packed structures
 * #define PACKED1 __attribute__((packed))
 * #define PACKED2
 * #define PACKED0
 *
 * Macros to support packed enum's
 * #define PACKED_ENUM enum
 * #define PACKED_ENUM_EXTRA
 *
 * #define INLINE __inline__
 *
 * @ingroup OSAPI
 *
 */


/* @} */

/*!
 * @name Semaphore
 *
 * Posix compatible Semaphores.
 *
 * @{
 *
 * typedef sem_t otg_sem_t;
 *
 */

extern int otg_sem_init(char *, otg_sem_t *sem, int pshared, unsigned value);

extern int otg_sem_init_locked(char *, otg_sem_t *sem);
extern int otg_sem_init_unlocked(char *, otg_sem_t *sem);

extern int otg_sem_destroy(otg_sem_t *sem);
extern int otg_sem_wait(otg_sem_t *sem);
extern int otg_sem_trywait(otg_sem_t *sem);
extern int otg_sem_post(otg_sem_t *sem);

/*! @} */


/*!
 * @name Mutex
 *
 * Posix compatible Mutexes.
 *
 * @{
 *
 * typedef pthread_mutex_t otg_pthread_mutex_t;
 *
 */

//extern int otg_pthread_mutex_init(otg_pthread_mutex_t *mutex, const phtread_mutexattr_t *attr);
//extern int otg_pthread_mutex_destroy(otg_pthread_mutex_t *mutex);

extern int otg_pthread_mutex_lock(otg_pthread_mutex_t *mutex);
extern int otg_pthread_mutex_trylock(otg_pthread_mutex_t *mutex);
extern int otg_pthread_mutex_unlock(otg_pthread_mutex_t *mutex);


/*! @} */

/*!
 * @name Atomic
 *
 * Basic Atomic operations.
 * @{
 *
 * typedef int otg_atomic_t;
 *
 * };
 */

extern void otg_atomic_set(otg_atomic_t *a, int b);
extern void otg_atomic_add(int a, otg_atomic_t *b);
extern void otg_atomic_sub(int a, otg_atomic_t *b);
extern void otg_atomic_clr(otg_atomic_t *a);
extern void otg_atomic_inc(otg_atomic_t *a);
extern void otg_atomic_dec(otg_atomic_t *a);
extern int otg_atomic_read(otg_atomic_t *a);

/*! @} */

/*!
 * @name Memory Allocation
 *
 * Simple allocated memory operations.
 * @{
 */

extern void * otg_cmalloc(int n);
extern void otg_free(void *);


/*! @} */

/*!
 * @name Task Allocation
 *
 * Long lived, low priority, high latency tasks.
 *
 * Tasks may use semaphores, mutexes, atomic operations and memory allocations.
 *
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

extern struct otg_task *otg_task_init2(char *name, otg_task_proc_t work, otg_task_arg_t data, otg_tag_t tag);
extern void otg_up_work(struct otg_task *task);
extern void otg_up_admin(struct otg_task *task);
extern void otg_down_work(struct otg_task *task);
extern void otg_down_admin(struct otg_task *task);
extern void otg_task_start(struct otg_task *task);
extern void otg_task_exit(struct otg_task *task);

extern void otg_sleep(int n);


/*! @} */

/*!
 * @name Work Allocation
 *
 * Short lived, low priority, high latency task.
 *
 * @{
 *
 * Work items are similar to tasks except that they are not expected
 * to run for long periods of time or wait.
 *
 * The Work item function is run once and exit when the work
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

extern struct otg_workitem *otg_workitem_init(char *name, otg_workitem_proc_t proc, otg_workitem_arg_t data, otg_tag_t tag);;
extern void otg_workitem_start(struct otg_workitem *work);
extern void otg_workitem_exit(struct otg_workitem *work);


/*! @} */

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
 *      void task_work(unsigned long data)
 *      {
 *           struct otg_tasklet *tasklet = (struct otg_tasklet *)data;
 *           void *mydata = task->data;
 *
 *           // do work
 *
 *      }
 *
 */

extern struct otg_tasklet *otg_tasklet_init(char *name, otg_tasklet_proc_t proc, otg_tasklet_arg_t data, otg_tag_t tag);
extern void otg_tasklet_start(struct otg_tasklet *tasklet);
extern void otg_tasklet_exit(struct otg_tasklet *tasklet);


/*! @} */

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


/*!
 * @name GCC Optimize
 *
 * @{
 *
 *
 * otg_likely()
 * otg_unlikely()
 *
 */
/*! @} */
