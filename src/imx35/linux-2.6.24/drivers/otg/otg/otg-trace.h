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
 * otg/otgcore/otg-trace.h
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/otg-trace.h|20061218212938|61040
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *
 */
/*!
 * @file otg/otg/otg-trace.h
 * @brief Core Defines for USB OTG Core Layaer
 *
 * Fast Trace Utility
 * This set of definitions and code is meant to provide a _fast_ debugging facility
 * (much faster than printk) so that time critical code can be debugged by looking
 * at a trace of events provided by reading a file in the procfs without affecting
 * the timing of events in the critical code.
 *
 * The mechanism used it to allocate a (large) ring buffer of relatively small structures
 * that include location and high-res timestamp info, and up to 8 bytes of optional
 * data.  Values are stored and timestamps are taken as the critical code runs, but
 * data formatting and display are done during the procfs read, when more time is
 * available :).
 *
 * Note that there is usually some machine dependent code involved in getting the
 * high-res timestamp, and there may be other bits used just to keep the overall
 * time impact as low as possible.
 *
 * Varargs up to 9 arguments are now supported, but you have to supply the number
 * of args, since examining the format string for the number at trace event time
 * was deemed too expensive time-wise.
 *
 *
 * @ingroup OTGCore
 */

#ifndef OTG_TRACE_H
#define OTG_TRACE_H 1

/*!
 * @var typedef struct otg_tag otg_tag_t
 */
typedef struct otg_tag {
        void *otg;
        //char msg[16];
        char *msg;
        u8 tag;
} *otg_tag_t;


#ifdef PRAGMAPACK
#pragma pack(push,1)
#endif /* PRAGMAPACK */

/*!create a type for otg_trace_types */
typedef PACKED_ENUM /*enum*/ otg_trace_types {
        otg_trace_msg_invalid_n,
        otg_trace_msg_va_start_n,
        otg_trace_send_n,
        otg_trace_recv_n,
        otg_trace_nsend_n,
        otg_trace_nrecv_n,
        otg_trace_setup_n,
        otg_trace_string_n,
        otg_trace_elapsed_n,

} PACKED_ENUM_EXTRA otg_trace_types_t;

#ifdef PRAGMAPACK
#pragma pack(pop)
#endif /* PRAGMAPACK */

/*! @name otg_trace_message  otg trace message type defines */

/*!@{ */

#define OTG_TRACE_MAX_IN_VA                  8


/*! @typedef struct trace otg_trace_t
 *  @brief define an alias for struct trace
 *
 */
#define OTG_TRACE_IN_INTERRUPT (1 << 0)
#define OTG_TRACE_ID_GND       (1 << 1)
#define OTG_TRACE_HCD_PCD      (1 << 2)

#if !defined(OTG_TRACE_DISABLE) && ( defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE))

#if defined(CONFIG_OTG_TRACE_PACKED)
#define TRACE_PACKED0   PACKED0
#define TRACE_PACKED1   PACKED1
#define TRACE_PACKED2   PACKED2
#else
#define TRACE_PACKED0
#define TRACE_PACKED1
#define TRACE_PACKED2
#endif

/*! @typedef struct trace otg_trace_t
 *  @brief define an alias for struct trace
 *
 */
typedef TRACE_PACKED0 struct TRACE_PACKED1 trace {
        otg_trace_types_t        otg_trace_type;
        u8                       tag;
        u8                       va_num_args;
        u8                      flags;

        const char              *function;
        u32                     interrupts;
        otg_tick_t              ticks;
        u16                     h_framenum;
        u16                     p_framenum;

        char *fmt;

        union {
                u8                     dump[32];
                u8                     setup[8];
                u8                     string[32];
                u32                    val[OTG_TRACE_MAX_IN_VA];
                struct {
                        u32                     id;
                        otg_tick_t              ticks;
                };

        } trace;            /*< share space for different message */

} TRACE_PACKED2 otg_trace_t;

#else
//#warning TRACE DISABLED
/*! create a type a trace  */
typedef  struct  trace {
       int a;
}  otg_trace_t;
#endif

/*!
 *  create an alias for otg_trace_snapshot
 */

typedef struct otg_trace_snapshot {
        int first;
        int next;
        int total;
        otg_trace_t *traces;
} otg_trace_snapshot;

/* @} */

  #define TRACE_MAX  0x00008000
  #define TRACE_MASK 0x00007FFF
//#define TRACE_MAX  0x0000800
//#define TRACE_MASK 0x00007FF
//#define TRACE_MAX  0x000080
//#define TRACE_MASK 0x00007F


#if !defined(OTG_TRACE_DISABLE) && ( defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE))

/*Don't produce unrelated TRACE MSG */


#ifdef XXXCONFIG_OTG_LATENCY_CHECK

#if !defined(OLD_TRACE_API)

extern void otg_trace_dump(otg_tag_t tag, const char *fn, otg_trace_types_t trace_type, int len, void *dump);
extern void otg_trace_setup(otg_tag_t tag, const char *fn, void *setup);
extern void otg_trace_string(otg_tag_t tag, const char *fn, char *fmt, void *setup);
extern void otg_trace_elapsed(otg_tag_t tag, const char *fn, char *fmt, u32 id, otg_tick_t ticks);

#define TRACE_RECV(tag,len,dump)
#define TRACE_SEND(tag,len,dump)
#define TRACE_NRECV(tag,len,dump)
#define TRACE_NSEND(tag,len,dump)
#define TRACE_SETUP(tag,setup)
#define TRACE_STRING(tag,fmt,setup)
#define TRACE_ELAPSED(tag,fmt,ticks,id) otg_trace_elapsed(tag,__FUNCTION__,fmt,id, ticks)
#define TRACE_MSG0(tag, msg)

#else

extern int otg_trace_first;
extern int otg_trace_last_read;
extern int otg_trace_next;


#endif

extern void otg_trace_msg(
                otg_tag_t tag,
                const char *fn,
                u8 nargs,
                char *fmt,
                u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8);

#define TRACE_MSG1(tag, fmt, a1)
#define TRACE_MSG2(tag, fmt, a1, a2)
#define TRACE_MSG3(tag, fmt, a1, a2, a3)
#define TRACE_MSG4(tag, fmt, a1, a2, a3, a4)
#define TRACE_MSG5(tag, fmt, a1, a2, a3, a4, a5)
#define TRACE_MSG6(tag, fmt, a1, a2, a3, a4, a5, a6)
#define TRACE_MSG7(tag, fmt, a1, a2, a3, a4, a5, a6, a7)
#define TRACE_MSG8(tag, fmt, a1, a2, a3, a4, a5, a6, a7, a8)

/*Latency TRACE*/

#define LTRACE_MSG0(tag, msg)       otg_trace_msg(tag, __FUNCTION__, 0, msg,  0, 0, 0, 0, 0, 0, 0, 0)

#define LTRACE_MSG1(tag, fmt, a1) \
        otg_trace_msg(tag, __FUNCTION__, 1, fmt,  (u32)(a1),  0,  0,  0,  0,  0,  0,  0)

#define LTRACE_MSG2(tag, fmt, a1, a2) \
        otg_trace_msg(tag, __FUNCTION__, 2, fmt,  (u32)(a1), (u32)(a2),  0,  0,  0,  0,  0, 0)

#define LTRACE_MSG3(tag, fmt, a1, a2, a3) \
        otg_trace_msg(tag, __FUNCTION__, 3, fmt,  (u32)(a1), (u32)(a2), (u32)(a3),  0,  0,  0,  0, 0)


#define LTRACE_MSG4(tag, fmt, a1, a2, a3, a4) \
        otg_trace_msg(tag, __FUNCTION__, 4, fmt,  (u32)(a1), (u32)(a2), (u32)(a3), (u32)(a4),  0,  0,  0, 0)


#define LTRACE_MSG5(tag, fmt, a1, a2, a3, a4, a5) \
        otg_trace_msg(tag, __FUNCTION__, 5, fmt,  (u32)(a1), (u32)(a2), (u32)(a3), (u32)(a4), (u32)(a5),  0,  0, 0)

#define LTRACE_MSG6(tag, fmt, a1, a2, a3, a4, a5, a6) \
        otg_trace_msg(tag, __FUNCTION__, 6, fmt,  (u32)(a1), (u32)(a2), (u32)(a3), (u32)(a4), (u32)(a5), (u32)(a6),  0, 0)

#define LTRACE_MSG7(tag, fmt, a1, a2, a3, a4, a5, a6, a7) \
        otg_trace_msg(tag, __FUNCTION__, 7, fmt,  (u32)(a1), (u32)(a2), (u32)(a3),\
                        (u32)(a4), (u32)(a5), (u32)(a6), (u32)(a7),  0)

#define LTRACE_MSG8(tag, fmt, a1, a2, a3, a4, a5, a6, a7, a8) \
        otg_trace_msg(tag, __FUNCTION__, 8, fmt,  (u32)(a1), (u32)(a2), (u32)(a3), (u32)(a4), \
                        (u32)(a5), (u32)(a6), (u32)(a7), (u32)a8 )

extern otg_tag_t otg_trace_obtain_tag(void *, char *);
extern otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag);


#else

#if !defined(OLD_TRACE_API)

extern void otg_trace_dump(otg_tag_t tag, const char *fn, otg_trace_types_t trace_type, int len, void *dump);
extern void otg_trace_setup(otg_tag_t tag, const char *fn, void *setup);
extern void otg_trace_string(otg_tag_t tag, const char *fn, char *fmt, void *setup);
extern void otg_trace_elapsed(otg_tag_t tag, const char *fn, char *fmt, u32 id, otg_tick_t ticks);

#define TRACE_RECV(tag,len,dump)   otg_trace_dump(tag,__FUNCTION__,otg_trace_recv_n,len,dump)
#define TRACE_SEND(tag,len,dump)   otg_trace_dump(tag,__FUNCTION__,otg_trace_send_n,len,dump)
#define TRACE_NRECV(tag,len,dump)   otg_trace_dump(tag,__FUNCTION__,otg_trace_nrecv_n,len,dump)
#define TRACE_NSEND(tag,len,dump)   otg_trace_dump(tag,__FUNCTION__,otg_trace_nsend_n,len,dump)
#define TRACE_SETUP(tag,setup)     otg_trace_setup(tag,__FUNCTION__,setup)
#define TRACE_STRING(tag,fmt,setup) otg_trace_string(tag,__FUNCTION__,fmt,setup)
#define TRACE_ELAPSED(tag,fmt,ticks,id) otg_trace_elapsed(tag,__FUNCTION__,fmt,id, ticks)
#define TRACE_MSG0(tag, msg)       otg_trace_msg(tag, __FUNCTION__, 0, msg,  0, 0, 0, 0, 0, 0, 0, 0)

#else

extern int otg_trace_first;
extern int otg_trace_last_read;
extern int otg_trace_next;


#endif

extern void otg_trace_msg(
                otg_tag_t tag,
                const char *fn,
                u8 nargs,
                char *fmt,
                u32 a1, u32 a2, u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8);

#define TRACE_MSG1(tag, fmt, a1) \
        otg_trace_msg(tag, __FUNCTION__, 1, fmt,  (u32)(a1),  0,  0,  0,  0,  0,  0,  0)

#define TRACE_MSG2(tag, fmt, a1, a2) \
        otg_trace_msg(tag, __FUNCTION__, 2, fmt,  (u32)(a1), (u32)(a2),  0,  0,  0,  0,  0, 0)

#define TRACE_MSG3(tag, fmt, a1, a2, a3) \
        otg_trace_msg(tag, __FUNCTION__, 3, fmt,  (u32)(a1), (u32)(a2), (u32)(a3),  0,  0,  0,  0, 0)


#define TRACE_MSG4(tag, fmt, a1, a2, a3, a4) \
        otg_trace_msg(tag, __FUNCTION__, 4, fmt,  (u32)(a1), (u32)(a2), (u32)(a3), (u32)(a4),  0,  0,  0, 0)


#define TRACE_MSG5(tag, fmt, a1, a2, a3, a4, a5) \
        otg_trace_msg(tag, __FUNCTION__, 5, fmt,  (u32)(a1), (u32)(a2), (u32)(a3), (u32)(a4), (u32)(a5),  0,  0, 0)

#define TRACE_MSG6(tag, fmt, a1, a2, a3, a4, a5, a6) \
        otg_trace_msg(tag, __FUNCTION__, 6, fmt,  (u32)(a1), (u32)(a2), (u32)(a3), (u32)(a4), (u32)(a5), (u32)(a6),  0, 0)

#define TRACE_MSG7(tag, fmt, a1, a2, a3, a4, a5, a6, a7) \
        otg_trace_msg(tag, __FUNCTION__, 7, fmt,  (u32)(a1), (u32)(a2), (u32)(a3),\
                        (u32)(a4), (u32)(a5), (u32)(a6), (u32)(a7),  0)

#define TRACE_MSG8(tag, fmt, a1, a2, a3, a4, a5, a6, a7, a8) \
        otg_trace_msg(tag, __FUNCTION__, 8, fmt,  (u32)(a1), (u32)(a2), (u32)(a3), (u32)(a4), \
                        (u32)(a5), (u32)(a6), (u32)(a7), (u32)a8 )

#define LTRACE_MSG1(tag, fmt, a1)
#define LTRACE_MSG2(tag, fmt, a1, a2)
#define LTRACE_MSG3(tag, fmt, a1, a2, a3)
#define LTRACE_MSG4(tag, fmt, a1, a2, a3, a4)
#define LTRACE_MSG5(tag, fmt, a1, a2, a3, a4, a5)
#define LTRACE_MSG6(tag, fmt, a1, a2, a3, a4, a5, a6)
#define LTRACE_MSG7(tag, fmt, a1, a2, a3, a4, a5, a6, a7)
#define LTRACE_MSG8(tag, fmt, a1, a2, a3, a4, a5, a6, a7, a8)

extern otg_tag_t otg_trace_obtain_tag(void *, char *);
extern otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag);

#endif

#elif defined(OTG_WINCE)

#define TRACE_MSG0(tag, msg) \
          DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s"), _T(msg)))

#define TRACE_MSG1(tag, fmt, a1) \
        DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d"), _T(fmt), a1))

#define TRACE_MSG2(tag, fmt, a1, a2) \
        DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d"), _T(fmt), a1, a2))

#define TRACE_MSG3(tag, fmt, a1, a2, a3) \
        DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d"), _T(fmt), a1, a2, a3));

#define TRACE_MSG4(tag, fmt, a1, a2, a3, a4) \
        DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d %d %d"), _T(fmt), a1, a2, a3, a4));

#define TRACE_MSG5(tag, fmt, a1, a2, a3, a4, a5) \
        DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d %d %d %d"), _T(fmt), a1, a2, a3, a4, a5));

#define TRACE_MSG6(tag, fmt, a1, a2, a3, a4, a5, a6) \
        DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d %d %d %d %d %d"), _T(fmt), a1, a2, a3, a4, a5, a6));

#define TRACE_MSG7(tag, fmt, a1, a2, a3, a4, a5, a6, a7) \
        DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d %d %d %d %d %d %d"), _T(fmt), a1, a2, a3, a4, a5, a6, a7));

#define TRACE_MSG8(tag, fmt, a1, a2, a3, a4, a5, a6, a7, a8) \
        DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d %d %d %d %d %d %d %d"), _T(fmt), a1, a2, a3, a4, a5, a6, a7, a8));

extern otg_tag_t otg_trace_obtain_tag(void *, char *);
extern otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag);


extern otg_tag_t otg_trace_obtain_tag(void *, char *);
extern otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag);

#define TRACE_TMP

#else /* defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) */

#define TRACE_RECV(tag,len,dump)
#define TRACE_SEND(tag,len,dump)
#define TRACE_NRECV(tag,len,dump)
#define TRACE_NSEND(tag,len,dump)
#define TRACE_SETUP(tag,setup)
#define TRACE_STRING(tag,fmt,setup)
#define TRACE_ELAPSED(tag,fmt,id,ticks)

#define TRACE_MSG0(tag, fmt) \
                while (0)  { int x; x = (int) (tag); }
#define TRACE_MSG1(tag, fmt, a1) \
                while (0)  { int x; x = (int) (tag); x = (int)(a1); }
#define TRACE_MSG2(tag, fmt, a1, a2) \
                while (0)  { int x; x = (int) (tag); x = (int)(a1); x = (int)(a2); }
#define TRACE_MSG3(tag, fmt, a1, a2, a3) \
                while (0)  { int x; x = (int) (tag); x = (int)(a1); x = (int)(a2); x = (int)(a3); }
#define TRACE_MSG4(tag, fmt, a1, a2, a3, a4) \
                while (0)  { int x; x = (int) (tag); x = (int)(a1); x = (int)(a2); x = (int)(a3); x = (int)(a4); }
#define TRACE_MSG5(tag, fmt, a1, a2, a3, a4, a5) \
                while (0)  { int x; x = (int) (tag); x = (int)(a1); x = (int)(a2); x = (int)(a3); x = (int)(a4); \
                        x = (int)(a5); }
#define TRACE_MSG6(tag, fmt, a1, a2, a3, a4, a5, a6) \
                while (0)  { int x; x = (int) (tag); x = (int)(a1); x = (int)(a2); x = (int)(a3); x = (int)(a4); \
                        x = (int)(a5); x = (int)(a6); }
#define TRACE_MSG7(tag, fmt, a1, a2, a3, a4, a5, a6, a7) \
                while (0)  { int x; x = (int) (tag); x = (int)(a1); x = (int)(a2); x = (int)(a3); x = (int)(a4); \
                        x = (int)(a5); x = (int)(a6); x = (int)(a7); }
#define TRACE_MSG8(tag, fmt, a1, a2, a3, a4, a5, a6, a7, a8) \
                while (0)  { int x; x = (int) (tag); x = (int)(a1); x = (int)(a2); x = (int)(a3); x = (int)(a4); \
                        x = (int)(a5); x = (int)(a6); x = (int)(a7); x = (int)(a8); }

extern otg_tag_t otg_trace_obtain_tag(void *, char *);
extern otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag);

#define TRACE_TMP       //

#endif /* defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) */


#endif /* OTG_TRACE_H */
