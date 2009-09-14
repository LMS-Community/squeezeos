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
 * otg/otgcore/otg-trace.c
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/otgcore/otg-trace.c|20070529052439|20500
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *
 */
/*!
 * @file otg/otgcore/otg-trace.c
 * @brief OTG Core Debug Trace Facility
 *
 * This implements the OTG Core Trace Facility.
 *
 * @ingroup OTGTRACE
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

//#if defined(OTG_LINUX)
//#include <linux/vmalloc.h>
//#include <stdarg.h>
//#endif /* defined(OTG_LINUX) */

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>

#include <otg/otg-trace.h>
#include <otg/otg-api.h>

//#if defined(LINUX24)
//#include <asm/div64.h>
//#endif

#define STATIC static

#if defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) || defined(_OTG_DOXYGEN)

otg_sem_t (otg_trace_sem);
extern framenum_t otg_hcd_ops_framenum;
extern framenum_t otg_pcd_ops_framenum;
extern otg_tick_t (* otg_ocd_ops_ticks) (void);
extern otg_tick_t (* otg_ocd_ops_elapsed) (otg_tick_t *, otg_tick_t *);
extern u32 *otg_interrupts;



#define OTG_TRACE_NAME "trace_otg"

#if 0
/*!
 * @typedef struct otg_trace_snapshot
 */
typedef struct otg_trace_snapshot {
        int first;
        int next;
        int total;
        otg_trace_t *traces;
} otg_trace_snapshot;
#endif

otg_trace_snapshot otg_snapshot1;
otg_trace_snapshot otg_snapshot2;

int otg_trace_exiting;
otg_trace_snapshot *otg_trace_current_traces;
otg_trace_snapshot *otg_trace_other_traces;
otg_pthread_mutex_t otg_trace_mutex;

/*! otg_trace_init_snapshot - initialize otg_trace_snapshot struct
 *
 * @param tss - pointer to otg_trace_snapshot
 * @return none
 */

void otg_trace_init_snapshot(otg_trace_snapshot *tss)
{
        tss->first = tss->next = tss->total = 0;
        memset(tss->traces,0,sizeof(otg_trace_t) * TRACE_MAX);
}

#if 0
/*! rel_snapshot - release trace snapshot
 *
 * @param tss - pointer to otg_trace_snapshot
 * @return NULL
 */

STATIC otg_trace_snapshot *rel_snapshot(otg_trace_snapshot *tss)
{
        RETURN_NULL_UNLESS(tss);
        if (tss->traces) {
                vfree(tss->traces);
                tss->traces = NULL;
        }
        return NULL;
}

/*! otg_trace_snapshot - allocate otg trace snapshot memory resource
 *
 * @param tss - pointer to otg_trace_snapshot
 * @return otg_trace_snapshot point on success
 */

STATIC otg_trace_snapshot *get_snapshot(otg_trace_snapshot *tss)
{
        unsigned long flags;
        UNLESS ((tss->traces = vmalloc(sizeof(otg_trace_t) * TRACE_MAX))) {
//                printk(KERN_ERR "Cannot allocate memory for OTG trace log.\n");
                return rel_snapshot(tss);
        }
        otg_trace_init_snapshot(tss);
        return tss;
}
#endif

#if 0
/*!
 * otg_get_trace_info() - get otg trace information
 * @param p - pointer to trace slot
 * @return none
 */
void otg_get_trace_info(struct otg_instance *otg, otg_trace_t *p)
{
        #if defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE)
        RETURN_UNLESS( (p) );

        //p->hcd_pcd = 0;

        // XXX p->id_gnd = otg_instance_info->current_inputs & ID_GND ? 1 : 0;
        // XXX if (hcd_instance_private.active) p->hcd_pcd |= 0x1;
        // XXX if (pcd_instance_private.active) p->hcd_pcd |= 0x2;

        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        p->ticks = (otg_ocd_ops_ticks) ? otg_ocd_ops_ticks () : 0;
        p->interrupts = otg_interrupts ? *otg_interrupts : 0;
        p->h_framenum = ((otg && otg_hcd_ops_framenum) ? otg_hcd_ops_framenum(otg) : 0);
        p->p_framenum = ((otg && otg_pcd_ops_framenum) ? otg_pcd_ops_framenum(otg) : 0);
        if (in_interrupt()) p->flags |= OTG_TRACE_IN_INTERRUPT;
        //p->in_interrupt = in_interrupt();
        #endif /* defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) */
}
#endif


/*! otg_trace_get_next - get snapshot info and move to next trace slot
 *
 * @param tag - otg_tag type
 * @param fn  - function pointer
 * @param otg_trace_type otg trace type
 * @return next trace slot
 */

STATIC otg_trace_t *otg_trace_get_next(otg_tag_t tag, const char *fn, otg_trace_types_t otg_trace_type)
{
        otg_trace_snapshot *tss;
        otg_trace_t *p;
	int first, next, last;
        int gap;

        UNLESS(tag) {
                printk(KERN_INFO"%s: TAG NULL\n", __FUNCTION__);
                return NULL;
        }
        UNLESS ((tss = otg_trace_current_traces)) {
                printk(KERN_INFO"%s: TSS NULL\n", __FUNCTION__);
                return NULL;
        }
        UNLESS(tss->traces) {
                printk(KERN_INFO"%s: TSS->traces NULL\n", __FUNCTION__);
                return NULL;
        }
        UNLESS((p = tss->traces + tss->next)) {
                printk(KERN_INFO"%s: p NULL\n", __FUNCTION__);
                return NULL;
        }

        /* advance */
        next = tss->next;
        first = tss->first;

        gap = (first <= next) ?  (TRACE_MAX - next) + first : first - next;

        if (1 > gap) first = first + 0x200;

        last = next + 1;
        first &= TRACE_MASK;
        last &= TRACE_MASK;

        tss->total += 1;
        tss->next = last;
        tss->first = first;

        p->otg_trace_type = otg_trace_type;
        p->tag = tag->tag;
        p->function = fn;

        otg_get_trace_info(tag->otg, p);

        return p;
}


/*! otg_trace_dump - setup otg trace with setup information
 *
 * @param tag  - otg tag
 * @param fn  - function pointe
 * @param trace_type
 * @param len
 * @param dump  - pointer to setup information
 */
void otg_trace_dump(otg_tag_t tag, const char *fn, otg_trace_types_t trace_type, int len, void *dump)
{
        while (len > 0) {
                otg_trace_t *p;
                int bytes = MIN(len, sizeof(p->trace.dump));

                otg_pthread_mutex_lock(&otg_trace_mutex);         /* lock mutex */
                if ((p = otg_trace_get_next(tag, fn, trace_type))) {
                        p->va_num_args = bytes;
                        memcpy(&p->trace.dump, dump, bytes);
                }
                otg_pthread_mutex_unlock(&otg_trace_mutex);         /* lock mutex */
                len -= bytes;
                dump += bytes;
        }
}

/*! otg_trace_setup - setup otg trace with setup information
 *
 * @param tag  - otg tag
 * @param fn  - function pointer
 * @param setup - pointer to setup information
 */
void otg_trace_setup(otg_tag_t tag, const char *fn, void *setup)
{
        otg_trace_t *p;
        otg_pthread_mutex_lock(&otg_trace_mutex);         /* lock mutex */
        if ((p = otg_trace_get_next(tag, fn, otg_trace_setup_n))) {
                p->va_num_args = 0;
                memcpy(&p->trace.setup, setup, sizeof(p->trace.setup) /*sizeof(struct usbd_device_request)*/);
        }
        otg_pthread_mutex_unlock(&otg_trace_mutex);         /* lock mutex */
}

/*! otg_trace_string - put trace message in snapshot
 *
 * @param tag - otg tag
 * @param fn - function pointer
 * @param fmt - used for formatting trace message
 * @param str - trace message pointer
 * @return none
 */
void otg_trace_string(otg_tag_t tag, const char *fn, char *fmt, void *str)
{
        otg_trace_t *p;
        int len = MIN(strlen(str), sizeof(p->trace.string));
        otg_pthread_mutex_lock(&otg_trace_mutex);         /* lock mutex */
        if ((p = otg_trace_get_next(tag, fn, otg_trace_string_n))) {
                p->va_num_args = len;
                p->fmt = fmt;
                memcpy(&p->trace.string, str, len);
        }
        otg_pthread_mutex_unlock(&otg_trace_mutex);         /* lock mutex */
}

/*! otg_trace_elapsed - put trace message in snapshot
 *
 * @param tag - otg tag
 * @param fn - function pointer
 * @param fmt - used for formatting trace message
 * @param id -
 * @param ticks
 * @return none
 */
void otg_trace_elapsed(otg_tag_t tag, const char *fn, char *fmt, u32 id, otg_tick_t ticks)
{
        otg_trace_t *p;
        otg_pthread_mutex_lock(&otg_trace_mutex);         /* lock mutex */
        if ((p = otg_trace_get_next(tag, fn, otg_trace_elapsed_n))) {
                p->va_num_args = 0;
                p->fmt = fmt;
                p->trace.ticks = ticks;
                p->trace.id = id;
        }
        otg_pthread_mutex_unlock(&otg_trace_mutex);         /* lock mutex */
}

extern void otg_trace_copy(u32 *p, u32 val);

/*! otg_trace_msg - write trace message to trace slot
 *
 * @param tag - otg tag
 * @param fn - function pointer
 * @param nargs - argument number
 * @param fmt - pointer to format string
 * @param a1, a2, a3, a4, a5, a6, a7, a8
 * @return none
 */

void otg_trace_msg(otg_tag_t tag, const char *fn, u8 nargs, char *fmt, u32 a1, u32 a2, u32 a3, u32 a4,
                u32 a5, u32 a6, u32 a7, u32 a8)
{
        //int n;
        //otg_trace_t trace;
        //otg_trace_t *p1 = &trace;
        otg_trace_t *p2 = NULL;

        /* Figure out how many trace slots we're going to need. */

        UNLESS(tag) {
                //printk(KERN_INFO"%s: TAG NULL\n", __FUNCTION__);
                return;
        }
        UNLESS(nargs <= (OTG_TRACE_MAX_IN_VA)) {
                printk(KERN_INFO"%s: NARGS to large (%d > %d)\n", __FUNCTION__, nargs, OTG_TRACE_MAX_IN_VA);
                return;
        }

        #if 0
        memset(p1, 0, sizeof(trace));

        // otg_get_trace_info(tag->otg, p1);
        p->otg_trace_type = otg_trace_type;
        p->tag = tag->tag;
        p->function = fn;

        p1->otg_trace_type = otg_trace_msg_va_start_n;
        p1->tag = tag->tag;
        p1->function = fn;

        p1->fmt = fmt;
        p1->va_num_args = nargs;

        p1->trace.val[0] = a1;
        p1->trace.val[1] = a2;
        p1->trace.val[2] = a3;
        p1->trace.val[3] = a4;
        p1->trace.val[4] = a5;
        p1->trace.val[5] = a6;
        p1->trace.val[6] = a7;
        p1->trace.val[7] = a8;
        #else
        #endif

        otg_pthread_mutex_lock(&otg_trace_mutex);         /* lock mutex */

        if((p2 =  otg_trace_get_next(tag, fn, otg_trace_msg_va_start_n))) {

                #if 0
                memcpy(p2, p1, sizeof(otg_trace_t));
                #else
                p2->otg_trace_type = otg_trace_msg_va_start_n;
                p2->tag = tag->tag;
                p2->function = fn;

                p2->fmt = fmt;
                p2->va_num_args = nargs;

                p2->trace.val[0] = a1;
                p2->trace.val[1] = a2;
                p2->trace.val[2] = a3;
                p2->trace.val[3] = a4;
                p2->trace.val[4] = a5;
                p2->trace.val[5] = a6;
                p2->trace.val[6] = a7;
                p2->trace.val[7] = a8;
                #endif
        }

        otg_pthread_mutex_unlock(&otg_trace_mutex);         /* lock mutex */
}

/* Proc Filesystem *************************************************************************** */

/*! int slot_in_data - validate slot region
 *
 * @param tss - otg trace snapshot pointer
 * @param slot - slot number to validate
 * @return 1 for valid, otherwise 0
 */
STATIC int slot_in_data(otg_trace_snapshot *tss, int slot)
{
        // Return 1 if slot is in the valid data region, 0 otherwise.
        return (((tss->first < tss->next) && (slot >= tss->first) && (slot < tss->next)) ||
                ((tss->first > tss->next) && ((slot < tss->next) || (slot >= tss->first))));

}

/*! otg_trace_read_slot - Place a slot pointer in *ps (and possibly *pv), with the older entry (if any) in *po.
 *
 * @param tss - pointer to otg trace snapshot
 * @param index
 * @param ps - pointer to slot pointer
 * @param po - pointer to slot pointer
 * Return: -1 at end of data
 *          0 if not a valid enty
 *          1 if entry valid but no older valid entry
 *          2 if both entry and older are valid.
 */
STATIC int otg_trace_read_slot(otg_trace_snapshot *tss, int index, otg_trace_t **ps, otg_trace_t **po)
{
        int res;
        int previous;
        otg_trace_t *s,*o;

        *ps = *po = NULL;
        index = (tss->first + index) & TRACE_MASK;
        // Are we at the end of the data?
        if (!slot_in_data(tss,index)) {
                // End of data.
                return -1;
        }
        /* Nope, there's data to show. */
        s = tss->traces + index;
        if (!s->tag ||
            s->otg_trace_type == otg_trace_msg_invalid_n
            /* || s->otg_trace_type == otg_trace_msg_va_list_n */
            ) {
                /* Ignore this slot, it's not valid, or is part
                   of a previously processed varargs. */
                return 0;
        }
        *ps = s;
        res = 1;

        // Is there a previous event (for "ticks" calculation)?
        previous = (index - 1) & TRACE_MASK;
        if (previous != tss->next && tss->total > 1) {
                // There is a valid previous event.
                res = 2;
                o = tss->traces + previous;
                #if 0
                /* If the previous event was a varargs event, we want
                   a copy of the first slot, not the last. */
                while (o->otg_trace_type == otg_trace_msg_va_list_n) {
                        previous = (previous - 1) & TRACE_MASK;
                        if (previous == tss->next) {
                                res = 1;
                                o = NULL;
                                break;
                        }
                        o = tss->traces + previous;
                }
                #endif
                *po = o;
        }

        return res;
}

#if defined(CONFIG_OTG_TRACE)

/*! otg_trace_proc_read - implement proc file system read.
 *
 * @brief  Standard proc file system read function.
 * @param page -pointer to char to stroe read message
 * @param count
 * @param  entry position pointer to read
 * @return read message length
 */
int otg_trace_proc_read(char *page, int count, int * pos)
{
        int len = 0;
        int index;
        int oindex;
        int rc;

        otg_tick_t ticks = 0;

        unsigned char *cp;
        int skip = 0;
        char hcd_pcd;
        char str[256];
        int i;
        struct usbd_device_request *request;

        otg_trace_t *o;
        otg_trace_t *s;
        otg_trace_snapshot *tss;

        while (otg_sem_wait(&otg_trace_sem));

        /* Grab the current snapshot, and replace it with the other.  This
         * needs to be atomic WRT otg_trace_msg() calls.
         */
        UNLESS (*pos) {
                // unsigned long flags;
                tss = otg_trace_other_traces;
                otg_trace_init_snapshot(tss);                                     // clear previous
                otg_pthread_mutex_lock(&otg_trace_mutex);         /* lock mutex */
                otg_trace_other_traces = otg_trace_current_traces;      // swap snapshots
                otg_trace_current_traces = tss;
                otg_pthread_mutex_unlock(&otg_trace_mutex);         /* lock mutex */
        }

        tss = otg_trace_other_traces;

        /* Get the index of the entry to read
         */
        s = o = NULL;
        oindex = index = (*pos)++;
        do {
                rc = otg_trace_read_slot(tss,index,&s,&o);
                switch (rc) {
                case -1: // End of data.
                        otg_sem_post(&otg_trace_sem);
                        return 0;

                case 0: // Invalid slot, skip it and try the next.
                        index = (*pos)++;
                        break;

                case 1: // s valid, o NULL
                case 2: // s and o valid
                        break;
                }
        } while (rc == 0);

        len = 0;

        UNLESS (oindex)
                len += sprintf((char *) page + len, "Tag  Ints Framenum   Ticks\n");


        /* If there is a previous trace event, we want to calculate how many
         * ticks have elapsed siince it happened.  Unfortunately, determining
         * if there _is_ a previous event isn't obvious, since we have to watch
         * out for startup, varargs and wraparound.
         */
        if (o) {

                ticks = otg_tmr_elapsed(&s->ticks, &o->ticks);

                if ((o->interrupts != s->interrupts) || (o->tag != s->tag) ||
                                ((o->flags & OTG_TRACE_IN_INTERRUPT) != (s->flags & OTG_TRACE_IN_INTERRUPT))
                                )
                        skip++;
        }

        switch (s->flags & OTG_TRACE_HCD_PCD) {
        default:
        case 0: hcd_pcd = ' '; break;
        case 1: hcd_pcd = 'H'; break;
        case 2: hcd_pcd = 'P'; break;
        case 3: hcd_pcd = 'B'; break;
        }

        len += sprintf((char *) page + len, "%s%02x %c%c %6x ", skip?"\n":"",
                        s->tag, (s->flags & OTG_TRACE_ID_GND) ? 'A' : 'B', hcd_pcd, s->interrupts);


        //len += sprintf((char *) page + len, "%05x %05x  ", s->h_framenum & 0x7fff, s->p_framenum & 0x7fff);
        len += sprintf((char *) page + len, "%06d  ", s->p_framenum & 0x7fff);


        if (ticks < 10000)
                len += sprintf((char *) page + len, "%6duS", (int)ticks);
        else if (ticks < 10000000) {
                //ticks = otg_do_div(ticks, (otg_tick_t)1000);
                len += sprintf((char *) page + len, "%6dmS", (int)ticks / 1000);
        }
        else {
                //ticks = otg_do_div(ticks, (otg_tick_t)1000000);
                len += sprintf((char *) page + len, "%6dS ", (int)ticks / 1000000);
        }

        ticks = 0LL;


        ticks = otg_tmr_elapsed(&ticks, &s->ticks) / 1000;
        //ticks = otg_do_div(ticks, (otg_tick_t)1000);

        len += sprintf((char *) page + len, "%6dmS", (int)(ticks & 0xffff));


        len += sprintf((char *) page + len, " %s ", s->flags & OTG_TRACE_IN_INTERRUPT ? "--" : "==");
        len += sprintf((char *) page + len, "%-24s: ",s->function);


        switch (s->otg_trace_type) {
        case otg_trace_msg_invalid_n:
                len += sprintf((char *) page + len, " --          N/A");
                break;

        case otg_trace_msg_va_start_n:

                len += sprintf((char *) page + len, s->fmt,
                                s->trace.val[0], s->trace.val[1], s->trace.val[2],
                                s->trace.val[3], s->trace.val[4], s->trace.val[5],
                                s->trace.val[6], s->trace.val[7], s->trace.val[8]
                              );

                break;

        case otg_trace_string_n:
                i = s->va_num_args;
                memcpy(str, &s->trace.string, i);
                str[i] = '\0';
                len += sprintf((char *) page + len, s->fmt, str);
                break;

        case otg_trace_elapsed_n:
                ticks = otg_tmr_elapsed(&s->ticks, &s->trace.ticks);

                i = s->va_num_args;
                len += sprintf((char *) page + len, "ELAPSED %s ", s->fmt);
                if (ticks < 10000)
                        len += sprintf((char *) page + len, "%duS", ticks);
                else {
                        //ticks = otg_do_div(ticks, (otg_tick_t)1000);
                        len += sprintf((char *) page + len, "%dmS", ticks / 1000);
                }
                len += sprintf((char *) page + len, " (%x)", s->trace.id);
                break;

        case otg_trace_recv_n:
        case otg_trace_send_n:
                cp = (unsigned char *)&s->trace.dump;

                len += sprintf((char *) page + len, "%s [ ", (s->otg_trace_type == otg_trace_recv_n) ? "RECV" : "SEND" );
                for  (i = 0; i < s->va_num_args; i++) {
                        len += sprintf((char *) page + len, "%02x ", cp[i]);
                        if ((i % 8) == 7)
                                len += sprintf((char *) page + len, " ");
                }
                len += sprintf((char *) page + len, "]");
                break;

        case otg_trace_nrecv_n:
        case otg_trace_nsend_n:
                cp = (unsigned char *)&s->trace.dump;

                len += sprintf((char *) page + len, "%s [ ", (s->otg_trace_type == otg_trace_nrecv_n) ? "NRCV" : "NSND" );
                for  (i = 0; i < s->va_num_args; i++) {
                        len += sprintf((char *) page + len, "%02x ", cp[i]);

                        switch(i) {
                        case 5:
                        case 11:
                        case 13:
                        case 15:
                                len += sprintf((char *) page + len, "][");
                                break;
                        default:
                                break;
                        }
                }
                len += sprintf((char *) page + len, "]");
                break;

        case otg_trace_setup_n:
                cp = (unsigned char *)&s->trace.setup;
                len += sprintf((char *) page + len,
                                "REQUEST [%02x %02x %02x %02x %02x %02x %02x %02x]",
                                cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
                request = (struct usbd_device_request*) &s->trace.setup;
                len += sprintf((char *) page + len,
                                "  bmRequestType:%02x bRequest:%02x wValue:%04x wIndex:%04x wLength:%04x",
                                request->bmRequestType,
                                request->bRequest,
                                le16_to_cpu(request->wValue),
                                le16_to_cpu(request->wIndex),
                                le16_to_cpu(request->wLength)
                              );

                break;
        }
        len += sprintf((char *) page + len, "\n");

        otg_sem_post(&otg_trace_sem);
        return len;
}

#endif

static u32 otg_tags_mask = 0x0;
extern void otg_trace_exit (void);

/*! otg_trace_obtain_tag(void) - get next unused tag value
 *
 * @param data - otg instance pointer
 * @param msg - message to initialize newly got tag
 * @return tag valude , 0 for unavailable
 */
otg_tag_t otg_trace_obtain_tag(void *data, char *msg)
{
        struct otg_instance *otg = (struct otg_instance *) data;
        otg_tag_t tag = NULL;


        RETURN_NULL_UNLESS((tag = CKMALLOC(sizeof (struct otg_tag))));

        tag->otg = otg;
        tag->tag = 0;
        tag->msg = msg;
	//strncpy(tag->msg, msg, sizeof(tag->msg)-1);
	//tag->msg[sizeof(tag->msg)-1] = '\0';

        /* Return the next unused tag value [1..32] if one is available,
         * return 0 if none is available.
         */
        while(otg_sem_wait(&otg_trace_sem));
        if (otg_tags_mask == 0xffffffff || otg_trace_exiting) {
                // They're all in use.
                otg_sem_post(&otg_trace_sem);
                return tag;
        }
        if (otg_tags_mask != 0x00000000) {
                // 2nd or later tag, search for an unused one
                while (otg_tags_mask & (0x1 << tag->tag)) {
                        tag->tag += 1;
                }
        }

        // Successful 1st or later tag.
        otg_tags_mask |= 0x1 << tag->tag;
        otg_sem_post(&otg_trace_sem);
        tag->tag += 1;
        //printk(KERN_INFO"%s: %s %d\n", __FUNCTION__, msg, tag->tag);
        return tag;
}

/*! otg_trace_invalidate_tag(otg_tag_t tag) - invalidate otg trace tag
 *
 * @param tag - tag to invalidate
 * @return 0  on finish
 */
otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag)
{
        otg_trace_t *p,*e;
        otg_trace_snapshot *css,*oss;

        //printk(KERN_INFO"%s: %x %s %d\n", __FUNCTION__, tag,  tag ? tag->msg : "NULL",  tag ? tag->tag : 0);

        RETURN_NULL_UNLESS(tag);


        // Nothing to do
        if ((0 >= tag->tag) || (tag->tag > 32)) {
                LKFREE(tag);
                return NULL;
        }

        // Grab a local copy of the snapshot pointers.
        otg_pthread_mutex_lock(&otg_trace_mutex);         /* lock mutex */
        css = otg_trace_current_traces;
        oss = otg_trace_other_traces;
        otg_pthread_mutex_unlock(&otg_trace_mutex);         /* lock mutex */


        // Run through all the trace messages, and invalidate any with the given tag.

        while(otg_sem_wait(&otg_trace_sem));
        if (css)
                for (e = TRACE_MAX + (p = css->traces); p < e; p++) {
                        if (p->tag == tag->tag) {
                                p->otg_trace_type = otg_trace_msg_invalid_n;
                                p->tag = 0;
                        }
                }
        if (oss)
                for (e = TRACE_MAX + (p = oss->traces); p < e; p++) {
                        if (p->tag == tag->tag) {
                                p->otg_trace_type = otg_trace_msg_invalid_n;
                                p->tag = 0;
                        }
                }

        // Turn off the tag
        otg_tags_mask &= ~(0x1 << (tag->tag-1));
        if (otg_tags_mask == 0x00000000) { /* otg_trace_exit (); */ }
        otg_sem_post(&otg_trace_sem);
        LKFREE(tag);
        return NULL;
}



/* Module init ************************************************************** */

#define OT otg_trace_init_test
otg_tag_t OT;

/*! otg_trace_modinit - initialization for otg trace
 *
 * @return non-zero if not successful.
 */
int otg_trace_modinit (void)
{
        RETURN_EINVAL_IF(otg_sem_init_unlocked("otg_trace", &otg_trace_sem));
        otg_trace_exiting = 0;

        OT = otg_trace_obtain_tag(NULL, "OTG Trace Init");
        RETURN_ENOMEM_IF(!OT);
        TRACE_MSG0(OT,"--");
        return 0;
}

/*! otg_trace_exit - remove procfs entry, free trace data space.
 */
void otg_trace_modexit (void)
{
        struct otg_tag tag;

        // unsigned long flags;
        otg_trace_snapshot *c,*o;

        otg_trace_invalidate_tag(OT);
        otg_trace_exiting = 1;

        otg_pthread_mutex_lock(&otg_trace_mutex);         /* lock mutex */

        /* Look for any outstanding tags. */
        tag.otg = NULL;
        for (tag.tag = 32; otg_tags_mask && tag.tag > 0; tag.tag--) {


                if (otg_tags_mask & (0x1 << (tag.tag-1))) {
			printk(KERN_INFO"%s: ERROR otg_tags_mask: %x %x tag: %d %s\n", __FUNCTION__,
					otg_tags_mask, 0x1 << (tag.tag-1), tag.tag, "" /*tag.msg*/);

                        //otg_trace_invalidate_tag(&tag);
		}
	}

        /* traces are cleared here, the os specific function must have saved them */
        c = otg_trace_current_traces;
        o = otg_trace_other_traces;
        otg_trace_current_traces = otg_trace_other_traces = NULL;
        otg_pthread_mutex_unlock(&otg_trace_mutex);         /* lock mutex */

        /* traces must be released in os specific modexit function */
}


OTG_EXPORT_SYMBOL(otg_trace_dump);
OTG_EXPORT_SYMBOL(otg_trace_setup);
OTG_EXPORT_SYMBOL(otg_trace_string);
OTG_EXPORT_SYMBOL(otg_trace_elapsed);


#else /* defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) */
int otg_trace_init (void) {return 0;}
void otg_trace_exit (void) {}
otg_tag_t otg_trace_obtain_tag(void *p,char *msg1) { return NULL; }

void otg_trace_setup(otg_tag_t tag, const char *fn, void *setup) { }
void otg_trace_msg(otg_tag_t tag, const char *fn, u8 nargs, char *fmt, ...) { }
otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag) { return NULL; }
void otg_trace_modexit (void) { }
int otg_trace_modinit (void) { return 0; }


#endif /* defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) */

OTG_EXPORT_SYMBOL(otg_trace_msg);
OTG_EXPORT_SYMBOL(otg_trace_obtain_tag);
OTG_EXPORT_SYMBOL(otg_trace_invalidate_tag);



// XXX OTG_EXPORT_SYMBOL(otg_tmr_ticks);
// XXX OTG_EXPORT_SYMBOL(otg_tmr_elapsed);
