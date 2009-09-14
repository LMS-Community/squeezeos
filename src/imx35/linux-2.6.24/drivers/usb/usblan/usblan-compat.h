/*********************************************************************
 *  A set of macros to cross-navigate Linux 2.4 and Linux 2.6 kernel
 *  facilities.
 *      Copyright (c) 2004, 2005 Belcarra Technologies Corp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>, Bruce Balden <balden@belcarra.com>
 *
 ***********************************************************************/
#ifndef _USB_NET_USBLAN_COMPAT_H
#define _USB_NET_USBLAN_COMPAT_H 1
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,2)
#define LINUX26
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2,4,5)
#define LINUX24
#else /* LINUX_VERSION_CODE > KERNEL_VERSION(2,4,5) */
#define LINUX24
#define LINUX_OLD
#warning "Early unsupported release of Linux kernel"
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2,4,5) */


/*! @{ */
#undef PRAGMAPACK
#define PACKED __attribute__((packed))
#define INLINE __inline__

/*! @} */

/*! @name Memory Allocation Primitives
 *
 * CKMALLOC()
 * LSTRDUP()
 * LKFREE()
 * LIST_ENTRY()
 * LIST_FOR_EACH()
 */

 /*! @{ */

#if defined(LINUX26)
    #include <linux/gfp.h>
#define GET_KERNEL_PAGE()  __get_free_page(GFP_KERNEL)

#else /* LINUX26 */

    #include <linux/mm.h>
    #define GET_KERNEL_PAGE() get_free_page(GFP_KERNEL)
#endif /* LINUX26 */



// Common to all supported versions of Linux ??

#define CKMALLOC(n,f) _ckmalloc(__FUNCTION__, __LINE__, n, f)
#define LSTRDUP(str) _lstrdup(__FUNCTION__, __LINE__, str)
#define LKFREE(p) _lkfree(__FUNCTION__, __LINE__, p)

#define ckmalloc(n,f) _ckmalloc(__FUNCTION__, __LINE__, n, f)
#define lstrdup(str) _lstrdup(__FUNCTION__, __LINE__, str)
#define lkfree(p) _lkfree(__FUNCTION__, __LINE__, p)

#define OTG_MALLOC_TEST
#undef OTG_MALLOC_DEBUG

#ifdef OTG_MALLOC_TEST
    extern int otg_mallocs;
#endif


static INLINE void *_ckmalloc (const char *func, int line, int n, int f)
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

static INLINE char *_lstrdup (const char *func, int line, char *str)
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

static INLINE void _lkfree (const char *func, int line, void *p)
{
    if (p) {
#ifdef OTG_MALLOC_TEST
        --otg_mallocs;
#endif
#ifdef OTG_MALLOC_DEBUG
        printk(KERN_INFO"%s: %p %s %d %d\n", __FUNCTION__, p, func, line, otg_mallocs);
#endif
        kfree (p);
#ifdef MALLOC_TEST
        if (otg_mallocs < 0) {
                printk(KERN_INFO"%s: %p %s %d %d otg_mallocs less zero!\n", __FUNCTION__, p, func, line, otg_mallocs);
        }
#endif
#ifdef OTG_MALLOC_DEBUG
        else {
            printk(KERN_INFO"%s: %s %d NULL\n", __FUNCTION__, func, line);
        }
#endif
    }
}

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


/*! @name Atomic Operations
 *
 * atomic_post_inc()
 * atomic_pre_dec()
 */
/*! @{ */
static __inline__ int atomic_post_inc(volatile atomic_t *v)
{
        unsigned long flags;
        int result;
        local_irq_save(flags);
        result = (v->counter)++;
        local_irq_restore(flags);
        return(result);
}

static __inline__ int atomic_pre_dec(volatile atomic_t *v)
{
        unsigned long flags;
        int result;
        local_irq_save(flags);
        result = --(v->counter);
        local_irq_restore(flags);
        return(result);
}
/*! @} */



/*!@name Scheduling Primitives
 *
 * WORK_STRUCT
 * WORK_ITEM
 *
 * SCHEDULE_TIMEOUT()\n
 * SET_WORK_ARG()\n
 * SCHEDULE_WORK()\n
 * SCHEDULE_IMMEDIATE_WORK()\n
 * NO_WORK_DATA()\n
 * MOD_DEC_USE_COUNT\n
 * MOD_INC_USE_COUNT\n
 */

/*! @{ */

static void inline SCHEDULE_TIMEOUT(int seconds){
        schedule_timeout( seconds * HZ );
}


/* Separate Linux 2.4 and 2.6 versions of scheduling primitives */
#if defined(LINUX26)
    #include <linux/workqueue.h>

    #define WORK_STRUCT  work_struct
    #define WORK_ITEM    work_struct
    typedef struct WORK_ITEM WORK_ITEM;
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
    #undef PREPARE_WORK
    typedef void (* WORK_PROC)(void *);

    #define SET_WORK_ARG(__item, __data) (__item).data = __data

    #define SCHEDULE_WORK(item) schedule_work(&(item))
    #define SCHEDULE_IMMEDIATE_WORK(item) SCHEDULE_WORK((item))
    #define PENDING_WORK_ITEM(item) ((item).pending != 0)
    #define NO_WORK_DATA(item) (!(item).data)
    #define _MOD_DEC_USE_COUNT   //Not used in 2.6
    #define _MOD_INC_USE_COUNT   //Not used in 2.6

#else /* LINUX26 */

    #define WORK_STRUCT  tq_struct
    #define WORK_ITEM    tq_struct
    typedef struct WORK_ITEM WORK_ITEM;
    #define PREPARE_WORK_ITEM(item,work_routine,work_data) { item.routine = work_routine; item.data = work_data; }
    #define SET_WORK_ARG(__item, __data) (__item).data = __data
    #define NO_WORK_DATA(item) (!(item).data)
    #define SCHEDULE_WORK(item) schedule_task(&(item))
    #define PENDING_WORK_ITEM(item) ((item).sync != 0)
    #define _MOD_DEC_USE_COUNT   MOD_DEC_USE_COUNT
    #define _MOD_INC_USE_COUNT   MOD_INC_USE_COUNT

    typedef void (* WORK_PROC)(void *);

    #if !defined(IRQ_HANDLED)
        // Irq's
        typedef void irqreturn_t;
        #define IRQ_NONE
        #define IRQ_HANDLED
        #define IRQ_RETVAL(x)
    #endif
#endif /* LINUX26 */

/*! @} */

/*!@name Semaphores
 *
 * up()
 * down()
 */
/*! @{ */
#define UP(s) up(s)
#define DOWN(s) down(s)
/*! @} */

/*! @name Printk
 *
 * PRINTK()
 */
/*! @{ */
#define PRINTK(s) printk(s)
/*! @} */

/*!
 * Cache
 * @{
 */
#define CACHE_SYNC_RCV(buf, len) pci_map_single (NULL, (void *) buf, len, PCI_DMA_FROMDEVICE)
#define CACHE_SYNC_TX(buf, len) dma_cache_maint (buf, len, PCI_DMA_TODEVICE)

/* @} */



#endif
