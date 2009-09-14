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
 * otg/otgcore/otg.c - OTG state machine
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otgcore/otg.c|20070919232149|63517
 *
 *      Copyright (c) 2004-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@lbelcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/otgcore/otg.c
 * @brief OTG Core State Machine Event Processing
 *
 * The OTG State Machine receives "input" information passed to it
 * from the other drivers (pcd, tcd, hcd and ocd) that describe what
 * is happening.
 *
 * The State Machine uses the inputs to move from state to state. Each
 * state defines four things:
 *
 * 1. Reset - what inputs to set or reset on entry to the state.
 *
 * 2. Outputs - what output functions to call to set or reset.
 *
 * 3. Timeout - an optional timeout value to set
 *
 * 4. Tests - a series of tests that allow the state machine to move to
 * new states based on current or new inputs.
 *
 * @ingroup OTGCORE
 */

//#define OTG_TRACE_DISABLE
#ifndef OTG_REGRESS

//#include <sys/time.h>
#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>

#include <otg/otg-trace.h>
#include <otg/otg-api.h>
//#include <otg/otg-task.h>
#include <otg/otg-tcd.h>
#include <otg/otg-hcd.h>
#include <otg/otg-pcd.h>
#include <otg/otg-ocd.h>
//#include <otg/otg-linux.h>

/* XXX these need to be replaced so that we can
 * support multiple devices
 */

otg_tick_t (* otg_ocd_ops_ticks) (void);
otg_tick_t (* otg_ocd_ops_elapsed) (otg_tick_t *, otg_tick_t *);

otg_tick_t (* otg_pcd_ops_ticks) (void);
otg_tick_t (* otg_pcd_ops_elapsed) (otg_tick_t *, otg_tick_t *);

u32 *otg_interrupts;

framenum_t otg_hcd_ops_framenum;
framenum_t otg_pcd_ops_framenum;

static char otg_message_buf[256];

#if defined (OTG_WINCE)
#define TRACE_SM_CURRENT(t, m, o) \

#else
#define TRACE_SM_CURRENT(t, m, o) \
                TRACE_STRING(t, "SM_CURRENT: %s",\
                                m \
                          ); \
                TRACE_MSG4(t, "SM_CURRENT: RESET: %08x SET: %08x %s (%s)",\
                                (u32)(~o->current_inputs), \
                                (u32)(o->current_inputs), \
                                o->current_outputs->name, \
                                o->previous_outputs->name \
                          )
#endif //OTG_WINCE
#define TRACE_SM_CHANGE(t, m, o) \
                TRACE_STRING(t, "SM_CHANGE: %s",\
                                m \
                          ); \
                TRACE_MSG4(t, "SM_CHANGE:  RESET: %08x SET: %08x %s (%s)",\
                                (u32)(~o->current_inputs), \
                                (u32)(o->current_inputs), \
                                o->current_outputs->name, \
                                o->previous_outputs->name \
                          )

#define TRACE_SM_NEW(t, m, o) \
                TRACE_STRING(t, "SM_NEW: %s",\
                                m \
                          ); \
                TRACE_MSG4(t, "SM_NEW:     RESET: %08x SET: %08x %s (%s)",\
                                (u32)(~o->current_inputs), \
                                (u32)(o->current_inputs), \
                                o->current_outputs->name, \
                                o->previous_outputs->name \
                          )


#define TRACE_SM_INPUTS(t, m, u, c) \
                TRACE_STRING(t, "SM_INPUTS: %s",\
                                m \
                          ); \
                TRACE_MSG3(t, "SM_INPUTS:  RESET: %08x SET: %08x CUR: %08x",\
                                (u32)(u >> 32), \
                                (u32)(u & 0xffffffff), \
                                c \
                                )

#endif

//DECLARE_MUTEX (otg_sem);

/*!
 * Output change lookup table - this maps the current output and the desired
 * output to what should be done. This is just so that we don't redo changes,
 * if the output does not change we do not want to re-call the output function
 */
u8 otg_output_lookup[4][4] = {
      /*  NC       SET      RESET    PULSE, <--- current/  new     */
        { NC,      NC,      NC,      PULSE,  },         /* NC      */
        { SET,     NC,      SET,     PULSE,  },         /* SET     */
        { RESET,   RESET,   NC,      PULSE,  },         /* RESET   */
        { PULSE,   PULSE,   PULSE,   PULSE,  },         /* UNKNOWN */
};


/*!
 * otg_new() - check OTG new inputs and select new state to be in
 *
 * This is used by the OTG event handler to see if the state should
 * change based on the current input values.
 *
 * @param otg pointer to the otg instance.
 * @return either the next OTG state or invalid_state if no change.
 */
static int otg_new(struct otg_instance *otg)
{
        int state = otg->state;
        struct otg_test *otg_test = otg_firmware_loaded ? otg_firmware_loaded->otg_tests : NULL;
                                                /* get a copy of current inputs */
        otg_current_t input_mask = ((otg_current_t)otg->current_inputs) | ((otg_current_t)(~otg->current_inputs) << 32);

        UNLESS(otg_test) return invalid_state;

        TRACE_MSG3(CORE, "OTG_NEW: %s inputs: %08x %08x", otg_get_state_name(state),
                        (u32)(input_mask>>32&0xffffffff), (u32)(input_mask&0xffffffff));

        /* iterate across the otg inputs table, for each entry that matches the current
         * state, test the input_mask to see if a state transition is required.
         */
        for (; otg_test->target != invalid_state; otg_test++) {

                otg_current_t test1 = otg_test->test1;
                otg_current_t test2 = otg_test->test2;
                otg_current_t test3 = otg_test->test3;

                CONTINUE_UNLESS(otg_test->state == state);     // skip states that don't match


                TRACE_MSG7(CORE, "OTG_NEW: %s (%s, %d) test1: %08x %08x test2 %08x %08x",
                                otg_get_state_name(otg_test->state),
                                otg_get_state_name(otg_test->target),
                                otg_test->test,
                                (u32)(test1>>32&0xffffffff), (u32)(test1&0xffffffff),
                                (u32)(test2>>32&0xffffffff), (u32)(test2&0xffffffff)
                                );

                /* the otg inputs table has multiple masks that define between multiple tests. Each
                 * test is a simple OR to check if specific inputs are present.
                 */
                CONTINUE_UNLESS (
                                (!otg_test->test1 || ((test1 = otg_test->test1 & input_mask))) &&
                                (!otg_test->test2 || ((test2 = otg_test->test2 & input_mask))) &&
                                (!otg_test->test3 || ((test3 = otg_test->test3 & input_mask))) /*&&
                                (!otg_test->test4 || (otg_test->test4 & input_mask)) */ );

                TRACE_MSG7(CORE, "OTG_NEW: GOTO %s test1: %08x %08x test2 %08x %08x test3 %08x %08x",
                                otg_get_state_name(otg_test->target),
                                (u32)(test1>>32&0xffffffff), (u32)(test1&0xffffffff),
                                (u32)(test2>>32&0xffffffff), (u32)(test2&0xffffffff),
                                (u32)(test3>>32&0xffff), (u32)(test3&0xffff)
                                );

                return otg_test->target;
        }
        TRACE_MSG0(CORE, "OTG_NEW: finis");

        return invalid_state;
}


/*!
 * otg_write_state_message_irq() -
 *
 * Send a message to the otg management application.
 * @param otg -otg instance pointer
 * @param msg
 * @param reset
 * @param inputs
 */
void otg_write_state_message_irq(struct otg_instance *otg, char *msg, otg_current_t reset, u32 inputs)
{
        sprintf(otg_message_buf,
                        "State: %s reset: %08x set: %08x inputs: %08x ", msg, (u32)(reset>>32), (u32)(reset&0xffffffff), inputs);
        otg_message_buf[95] = '\0';
        otg_message(otg, otg_message_buf);
}

/*!
 * otg_write_output_message_irq() -
 *
 * Send a message to the otg management application.
 * @param otg - otg instance pointer
 * @param msg
 * @param val
 */
void otg_write_output_message_irq(struct otg_instance *otg, char *msg, int val)
{
        strcpy(otg_message_buf, "Output: ");
        strncat(otg_message_buf, msg, 64 - strlen(otg_message_buf));
        if (val == 2) strncat(otg_message_buf, "/", 64 - strlen(otg_message_buf));
        otg_message_buf[sizeof(otg_message_buf)] = '\0';
        otg_message(otg, otg_message_buf);
}

/*!
 * otg_write_timer_message_irq() -
 *
 * Send a message to the otg management application.
 * @param otg - otg instance pointer
 * @param msg
 * @param val
 */
void otg_write_timer_message_irq(struct otg_instance *otg, char *msg, int val)
{
        if (val < 10000)
                sprintf(otg_message_buf, "Timer: %s %d uS %u", msg, val, (u32)otg_tmr_ticks());
        else if (val < 10000000)
                //sprintf(otg_message_buf, " %d mS\n", ticks / 1000);
                //sprintf(otg_message_buf, "Timer: %s %d mS %u", msg, val >> 10, otg_tmr_ticks());
                sprintf(otg_message_buf, "Timer: %s %d mS %u", msg, val / 1000, (u32)otg_tmr_ticks());
        else
                //sprintf(otg_message_buf, " %d S\n", ticks / 1000000);
                //sprintf(otg_message_buf, "Timer: %s %d S %u", msg, val >> 20, otg_tmr_ticks());
                sprintf(otg_message_buf, "Timer: %s %d S %u", msg, val / 1000000, (u32)otg_tmr_ticks());

        otg_message_buf[sizeof(otg_message_buf)] = '\0';
        otg_message(otg, otg_message_buf);
}

//#if defined(LINUX24)
/*!
 * otg_write_info_message() -
 *
 * Send a message to the otg management application.
 * @param privdata - pcd_instance type pointer
 * @param msg -
 */
void otg_write_info_message(void *privdata, char *msg)
{
        struct pcd_instance *pcd_instance =  (struct pcd_instance *) privdata;

        struct otg_instance *otg = pcd_instance ?  pcd_instance->otg : NULL;

        RETURN_UNLESS(otg);
        sprintf(otg_message_buf, "Info: %s", msg);
        otg_message_buf[sizeof(otg_message_buf)] = '\0';
        otg_message(otg, otg_message_buf); // XXX
}

#if 0
/*!
 * otg_write_reset_message_irq() -
 *
 * Send a message to the otg management application.
 * @param otg - otg instance pointer
 * @param msg
 * @param val
 */
void otg_write_reset_message(struct otg_instance *otg, char *msg, int val)
{
        sprintf(otg_message_buf, "State: %s reset: %x", msg, val);
        otg_message_buf[sizeof(otg_message_buf)] = '\0';
        otg_message(otg, otg_message_buf);
}
#endif

//#if defined(LINUX24)
OTG_EXPORT_SYMBOL(otg_write_info_message);
//#endif

char otg_change_names[4] = {
        '_', ' ', '/', '#',
};

/*!
 * otg_process_event() - process OTG events and determine OTG outputs to reflect new state
 *
 * This function is passed a mask with changed input values. This
 * is passed to the otg_new() function to see if the state should
 * change. If it changes then the output functions required for
 * the new state are called.
 *
 * @param otg pointer to the otg instance.
 * @param inputs input mask
 * @param tag trace tag to use for tracing
 * @param msg message for tracing
 */
static void otg_process_event(struct otg_instance *otg, otg_current_t inputs, otg_tag_t tag, char *msg)
{
        u32 current_inputs = otg->current_inputs;
        int current_state = otg->state;
        u32 inputs_set = (u32)(inputs & 0xffffffff);           // get changed inputs that SET something
        u32 inputs_reset = (u32) (inputs >> 32);                // get changed inputs that RESET something
        int target;

        RETURN_UNLESS(otg && inputs);

        TRACE_SM_INPUTS(tag, msg, inputs, current_inputs);
        TRACE_MSG2(CORE, "otg->active: %d %s", otg->active, msg);

        /* Special Overrides - These are special tests that satisfy specific injunctions from the
         * OTG Specification.
         */
        if (inputs_set & inputs_reset) {                // verify that there is no overlap between SET and RESET masks
                TRACE_MSG1(CORE, "OTG_EVENT: ERROR attempting to set and reset the same input: %08x", inputs_set & inputs_reset);
                return;
        }
        /* don't allow bus_req and bus_drop to be set at the same time
         */
        if ((inputs_set & (bus_req | bus_drop)) == (bus_req | bus_drop) ) {
                TRACE_MSG2(CORE, "OTG_EVENT: ERROR attempting to set both bus_req and bus_drop: %08x %08x",
                                inputs_set, (bus_req | bus_drop));
                return;
        }
        /* set bus_drop_ if bus_req, set bus_req_ if bus_drop
         */
        if (inputs_set & bus_req) {
                inputs |= bus_drop_;
                TRACE_MSG1(CORE, "OTG_EVENT: forcing bus_drop/: %08x", inputs_set);
        }
        if (inputs_set & bus_drop) {
                inputs |= bus_req_;
                TRACE_MSG1(CORE, "OTG_EVENT: forcing bus_req/: %08x", inputs_set);
        }

        otg->current_inputs &= ~inputs_reset;
        otg->current_inputs |= inputs_set;
        //TRACE_MSG3(CORE, "OTG_EVENT: reset: %08x set: %08x inputs: %08x", inputs_reset, inputs_set, otg->current_inputs);

        TRACE_SM_CHANGE(tag, msg, otg);

        RETURN_IF(otg->active);

        otg->active++;

        /* Search the state input change table to see if we need to change to a new state.
         * This may take several iterations.
         */
        while (invalid_state != (target = otg_new(otg))) {

                struct otg_state *otg_state;
                //int original_state = otg->state;
                otg_current_t current_outputs;
                otg_current_t output_results;
                otg_current_t new_outputs;
                int i;

                /* if previous output started timer, then cancel timer before proceeding
                 */
                if ((otg_state = otg->current_outputs) && otg_state->tmout && otg->start_timer) {
                        TRACE_MSG0(CORE, "reseting timer");
                        otg->start_timer(otg, 0);
                }

                BREAK_UNLESS(otg_firmware_loaded);

                BREAK_UNLESS(target < otg_firmware_loaded->number_of_states);

                otg_state = otg_firmware_loaded->otg_states + target;

                BREAK_UNLESS(otg_state->name);

                otg->previous = otg->state;
                otg->state = target;
                otg->tickcount = otg_tmr_ticks();


                otg->previous_outputs= otg->current_outputs;
                //otg->current_outputs = NULL;


                /* A matching input table rule has been found, we are transitioning to a new
                 * state.  We need to find the new state entry in the output table.
                 * XXX this could be a table lookup instead of linear search.
                 */

                current_outputs = otg->outputs;
                new_outputs = otg_state->outputs;
                output_results = 0;

                TRACE_SM_NEW(tag, msg, otg);

                /* reset any inputs that the new state want's reset
                 */
                otg_process_event(otg, otg_state->reset | TMOUT_, tag, otg_state->name);

                otg_write_state_message_irq(otg, otg_state->name, otg_state->reset, otg->current_inputs);

#if 1
                switch (target) {                            /* C.f. 6.6.1.12 b_conn reset */
                case otg_disabled:
                        otg->current_inputs = 0;
                        otg->outputs = 0;
                        break;
                default:
                        //otg_event(otg, not(b_conn), "a_host/ & a_suspend/: set b_conn/");
                        break;
                }
#endif
                #if 0
                TRACE_MSG5(CORE, "OTG_EVENT: OUTPUT MAX:%d OUTPUTS CURRENT: %08x %08x NEW: %08x %08x",
                                MAX_OUTPUTS,
                                (u32)(current_outputs >> 32),
                                (u32)(current_outputs & 0xffffffff),
                                (u32)(new_outputs >> 32),
                                (u32)(new_outputs & 0xffffffff)
                          );
                #endif

                /* Iterate across the outputs bitmask, calling the appropriate functions
                 * to make required changes. The current outputs are saved and we DO NOT
                 * make calls to change output values until they change again.
                 */
                for (i = 0; i < MAX_OUTPUTS; i++) {

                        u8 current_output = (u8)(current_outputs & 0x3);
                        u8 new_output = (u8)(new_outputs & 0x3);
                        u8 changed = otg_output_lookup[new_output][current_output];

                        output_results >>= 2;

                        switch (changed) {
                        case NC:
                                output_results |= ((current_outputs & 0x3) << (MAX_OUTPUTS * 2));
                                break;
                        case SET:
                        case RESET:
                                output_results |= (((otg_current_t)changed) << (MAX_OUTPUTS * 2));
                        case PULSE:


                                otg_write_output_message_irq(otg, otg_output_names[i], changed);

                                //TRACE_MSG3(CORE, "OTG_EVENT: CHECKING OUTPUT %s %d %s",
                                //                otg_state->name, i, otg_output_names[i]);

                                BREAK_UNLESS(otg->otg_output_ops[i]); // Check if we have an output routine

                                TRACE_MSG7(CORE, "OTG_EVENT: CALLING OUTPUT %s %d %s%c cur: %02x new: %02x chng: %02x",
                                                otg_state->name, i, otg_output_names[i],
                                                otg_change_names[changed&0x3],
                                                current_output, new_output,
                                                changed);

                                otg->otg_output_ops[i](otg, changed); // Output changed, call function to do it

                                //TRACE_MSG2(CORE, "OTG_EVENT: OUTPUT FINISED %s %s", otg_state->name, otg_output_names[i]);
                                break;
                        }

                        current_outputs >>= 2;
                        new_outputs >>= 2;
                }

                output_results >>= 2;
                otg->outputs = output_results;
                otg->current_outputs = otg_state;

                //TRACE_MSG2(CORE, "OTG_EVENT: STATE: otg->current_outputs: %08x %08x",
                //                (u32)(otg->outputs & 0xffffffff), (u32)(otg->outputs >> 32));

                if (((otg->outputs & pcd_en_out_set) == pcd_en_out_set) &&
                                        ((otg->outputs & hcd_en_out_set) == hcd_en_out_set))
                {
                        TRACE_MSG0(CORE, "WARNING PCD_EN and HCD_EN both set");
                }

                /* start timer?
                 */
                CONTINUE_UNLESS(otg_state->tmout);
                //TRACE_MSG1(CORE, "setting timer: %d", otg_state->tmout);
                if (otg->start_timer) {
                        otg_write_timer_message_irq(otg, otg_state->name, otg_state->tmout);
                        otg->start_timer(otg, otg_state->tmout);
                }
                else {
                        //TRACE_MSG0(CORE, "NO TIMER");
                        otg_process_event(otg, TMOUT_, CORE, otg_state->name);
                }

        }
        TRACE_MSG0(CORE, "finishing");
        otg->active = 0;

        if (current_state == otg->state)
                TRACE_MSG0(CORE, "OTG_EVENT: NOT CHANGED");

}

#if 0
#if defined(LINUX24)
/*!
 * otg_write_input_message_irq() -

 * @param inputs input mask
 * @param tag trace tag to use for tracing
 * @param msg message for tracing
 */
void otg_write_input_message_irq(struct otg_instance *otg, otg_current_t inputs, otg_tag_t tag, char *msg)
{
        u32 reset = (u32)(inputs >> 32);
        u32 set = (u32) (inputs & 0xffffffff);

        char buf[64];
        //snprintf(buf, sizeof(buf), "Input: %08x %08x %s", *inputs >> 32, *inputs & 0xffffffff, msg ? msg : "NULL");
        //TRACE_MSG3(tag, "Inputs: %08x %08x %s", set, reset, msg ? msg : "NULL");
        #ifdef OTG_WINCE
        sprintf(buf, "Inputs: %08x %08x %s", reset, set, msg ? msg : "NULL");
        #else /* OTG_WINCE */
        snprintf(buf, sizeof(buf), "Inputs: %08x %08x %s", reset, set, msg ? msg : "NULL");
        #endif /* OTG_WINCE */
        buf[63] = '\0';
        otg_message(otg, buf);
}
#endif
#endif

/*
 */
static otg_current_t otg_events[64];
static otg_tag_t otg_tags[64];
static char * otg_msgs[64];
static u8 otg_head, otg_tail;


/*!
 *  otg_do_work() -  Bottom half handler to process sent or received urbs.
 *
 * @param otg - otg instance pointer
 * @param inputs - input events mask
 * @param tag - otg tag
 * @param msg - message
 * @param queued - BOOL flag for queuing event handler
 */
void otg_do_work(struct otg_instance *otg, otg_current_t inputs, otg_tag_t tag, char *msg, BOOL queued )
{
        u32 reset = (u32)(inputs >> 32);
        u32 set = (u32) (inputs & 0xffffffff);

        TRACE_SM_INPUTS(tag, msg ? msg : "NULL", inputs, otg->current_inputs);
        otg_message_buf[0] = '\0';
        snprintf(otg_message_buf, sizeof(otg_message_buf),
                        "Inputs: %08x %08x %s [%s]", reset, set, msg ? msg : "NULL", queued ? "Q":"D");
        otg_message_buf[sizeof(otg_message_buf)] = '\0';

        otg_message(otg, otg_message_buf);
        otg_process_event(otg, inputs, tag, msg);
}

/*!
 *  otg_dequeue_tasklet() -  tasklet to dequeue events
 *
 * @param data pointer to parameters struct
 */
void *otg_dequeue_tasklet(otg_tasklet_arg_t data)
{
        struct otg_instance *otg = (struct otg_instance *) data;
        TRACE_MSG0(CORE, "DEQUEUE - START");
        /* lock interrupts */
        while (otg_head != otg_tail) {


                /* get input information
                 */
                otg_current_t inputs = otg_events[otg_head];
                otg_tag_t tag = otg_tags[otg_head];
                char * msg = otg_msgs[otg_head];

                TRACE_MSG0(CORE, "DEQUEUE");
                otg_head = (otg_head + 1) & 0x3f;
                otg_do_work(otg, inputs, tag, msg, TRUE);
        }
        TRACE_MSG0(CORE, "DEQUEUE - FINISH");
        return NULL;
}

/*!
 * otg_queue_event() - otg input change event handler
 * @param otg pointer to the otg instance.
 * @param inputs input mask
 * @param tag trace tag to use for tracing
 * @param msg message for tracing
 * @return non-zero if state changed.
 */
void otg_queue_event(struct otg_instance *otg, otg_current_t inputs, otg_tag_t tag, char *msg)
{
        //unsigned long flags;
        u8 otg_tail_save;

        u32 reset = (u32)(inputs >> 32);
        u32 set = (u32) (inputs & 0xffffffff);
        TRACE_MSG3(CORE, "QUEUE: RESET: %08x SET: %08x %s", reset, set, msg);

        /* atomic insertion */
        otg_pthread_mutex_lock(&otg->mutex);         /* lock mutex */
        otg_tail_save = otg_tail;
        otg_events[otg_tail] = inputs;
        otg_tags[otg_tail] = tag;
        otg_msgs[otg_tail] = msg;

        otg_tail = (otg_tail + 1) & 0x3f;

        /* check for overrun */
        if (otg_tail == otg_head)
                otg_tail = otg_tail_save;

        otg_pthread_mutex_unlock(&otg->mutex);      /* unlock mutex */

        if(otg->tasklet) otg_tasklet_start(otg->tasklet);
        //TRACE_MSG0(CORE, "FINISHED");
}

/*!
 * otg_event() - otg input change event handler
 * @param otg pointer to the otg instance.
 * @param inputs input mask
 * @param tag trace tag to use for tracing
 * @param msg message for tracing
 * @return non-zero if state changed.
 */
void otg_event(struct otg_instance *otg, otg_current_t inputs, otg_tag_t tag, char *msg)
{
        #if 1
        otg_queue_event(otg, inputs, tag, msg);
        #else
        #ifdef LINUX26
        if (in_atomic()) {
                printk(KERN_INFO"%s: IN_ATOMIC %s\n", __FUNCTION__, msg);
                return;
        }
        if (in_interrupt()) {
                printk(KERN_INFO"%s: IN_INTERRUPT %s\n", __FUNCTION__, msg);
                return;
        }
        #endif /* LINUX26 */

        UNLESS (/*in_atomic() ||*/ down_trylock(&otg->event)) {
                printk(KERN_INFO"%s: DIRECT %s\n", __FUNCTION__, msg);
                TRACE_MSG0(CORE, "DIRECT");
                otg_do_work(otg, inputs, tag, msg, FALSE);
                //UP(&otg->event);
                while (otg_sem_wait(&otg->event));
                //TRACE_MSG0(CORE, "FINISHED");
        }
        else
                otg_queue_event(otg, inputs, tag, msg);
        #endif

}


/*!
 * otg_event_set_irq() -set changed otg event for handling
 * @param otg pointer to the otg instance.
 * @param changed
 * @param flag
 * @param input input mask
 * @param tag trace tag to use for tracing
 * @param msg message for tracing
 * @return non-zero if state changed.
 */
void otg_event_set_irq(struct otg_instance *otg, int changed, int flag, u32 input, otg_tag_t tag, char *msg)
{
        RETURN_UNLESS(changed);
        TRACE_MSG4(tag, "%s: %08x changed: %d flag: %d", msg, input, changed, flag);
        otg_queue_event(otg, flag ? input : _NOT(input), tag, msg);
}


/*!
 * otg_serial_number() - set the device serial number
 *
 * @param otg pointer to the otg instance.
 * @param serial_number_str
 */
void otg_serial_number (struct otg_instance *otg, char *serial_number_str)
{
        int i;
        char *cp = serial_number_str;

        //DOWN(&otg_sem);
        //while (otg_sem_wait(&otg_sem));

        for (i = 0; cp && *cp && (i < OTGADMIN_MAXSTR); cp++) {
                CONTINUE_UNLESS (isxdigit(*cp));
                otg->serial_number[i++] = toupper(*cp);
        }
        otg->serial_number[i] = '\0';

        //TRACE_MSG2(CORE, "serial_number_str: %s serial_number: %s", serial_number_str, otg->serial_number);

        //UP(&otg_sem);
        //while(otg_sem_wait(&otg_sem));
}


/*!
 * otg_init() - create and initialize otg instance
 *
 * Called to initialize the OTG state machine. This will cause the state
 * to change from the invalid_state to otg_disabled.
 *
 * If CONFIG_OTG_TR_AUTO is defined thten
 * an initial enable_otg event is generated. This will cause
 * the state to move from otg_disabled to otg_enabled. The
 * requird drivers will be initialized by calling the appropriate
 * output functions:
 *
 *      - pcd_init_func
 *      - hcd_init_func
 *      - tcd_init_func
 *
 * @param otg pointer to the otg instance.
 */
void otg_init (struct otg_instance *otg)
{

        //TRACE_MSG0(CORE, "START");

        //init_MUTEX(&otg->command);
        //otg_sem_init("otg_command", &otg->command, 0, 0);
        //DOWN(&otg_sem);
        //otg_sem_post(&otg_sem);
        otg->outputs = tcd_init_out_ | pcd_init_out_ | hcd_init_out_;

        RETURN_UNLESS((otg->tasklet = otg_tasklet_init("otg_event", otg_dequeue_tasklet, (otg_tasklet_arg_t) otg, CORE)));
        //otg->tasklet->debug = TRUE;

        /* This will move the state machine into the otg_disabled state
         */
        otg_event(otg, enable_otg, CORE, "enable_otg"); // XXX

        #if defined(CONFIG_OTG_TR_AUTO)
        otg_event(otg, AUTO, CORE, "AUTO"); // XXX
        #endif /* defined(CONFIG_OTG_TR_AUTO) */
}


/*!
 * otg_exit()
 *
 * This is called by the driver that started the state machine to
 * cause it to exit. The state will move to otg_disabled.
 *
 * The appropriate output functions will be called to disable the
 * peripheral drivers.
 *
 * @param otg pointer to the otg instance.
 */
void otg_exit (struct otg_instance *otg)
{
        TRACE_MSG0(CORE, "OTG_EXIT");

        //DOWN(&otg_sem);
        //while(otg_sem_wait(&otg_sem));

        //TRACE_MSG0(CORE, "OTG_EXIT");
        //otg_event(otg, exit_all | not(a_bus_req) | a_bus_drop | not(b_bus_req), "tcd_otg_exit");

        //printk(KERN_INFO"%s: otg->state: %d START\n", __FUNCTION__, otg->state);
        otg_event(otg, enable_otg_, CORE, "enable_otg_");

        while (otg->state != otg_disabled) {
                //printk(KERN_INFO"%s: otg->state: %d SLEEP\n", __FUNCTION__, otg->state);
                //SCHEDULE_TIMEOUT(10 * HZ);
                otg_sleep(1);
        }
        //printk(KERN_INFO"%s: otg->state: %d FINISH\n", __FUNCTION__, otg->state);

        otg_tasklet_exit(otg->tasklet);
        otg->tasklet = NULL;

        //TRACE_MSG0(CORE, "UP OTG_SEM");
        //UP(&otg_sem);
        //otg_sem_post(&otg_sem);

        // XXX MODULE UNLOCK HERE
}


OTG_EXPORT_SYMBOL(otg_queue_event);
OTG_EXPORT_SYMBOL(otg_event);
OTG_EXPORT_SYMBOL(otg_serial_number);
OTG_EXPORT_SYMBOL(otg_init);
OTG_EXPORT_SYMBOL(otg_exit);
OTG_EXPORT_SYMBOL(otg_event_set_irq);

/* ********************************************************************************************* */

/*!
 * otg_gen_init_func() -
 *
 * This is a default tcd_init_func. It will be used
 * if no other is provided.
 *
 * @param otg pointer to the otg instance.
 * @param flag set or reset
 */
void otg_gen_init_func (struct otg_instance *otg, u8 flag)
{
        TRACE_MSG1(CORE, "GENERIC INIT %s", flag ? "SET" : "RESET");
	otg_event(otg, OCD_OK, CORE, "GENERIC OK");
}


/*! otg_gen_start_timer -
 * Fake - Set or reset timer to interrupt in number of uS (micro-seconds).
 * This is only suitable for MN or TR firmware.
 *
 * @param otg otg instance pointer
 * @param usec
 * @return 0
 */
int otg_gen_start_timer(struct otg_instance *otg, int usec)
{
        otg_event(otg, TMOUT, CORE, "FAKE TMOUT");
        return 0;
}


/*!
 * otg_hcd_set_ops() -
 * @param hcd_ops - host operations table to use
 * @param otg pointer to otg instance
 * @return pointer to host controller driver instance with initialized operations
 */
struct hcd_instance * otg_set_hcd_ops(struct otg_instance *otg, struct hcd_ops *hcd_ops)
{
        struct hcd_instance *hcd = NULL;
        otg->hcd_ops = hcd_ops;
        if (hcd_ops) {
                RETURN_NULL_UNLESS((hcd = CKMALLOC(sizeof(struct hcd_instance))));
                hcd->otg = otg;
                hcd->TAG = otg_trace_obtain_tag(otg, "set_hcd_ops");
                otg->hcd = hcd;
                otg->otg_output_ops[HCD_INIT_OUT] = hcd_ops->hcd_init_func;
                otg->otg_output_ops[HCD_EN_OUT] = hcd_ops->hcd_en_func;
                otg->otg_output_ops[HCD_RH_OUT] = hcd_ops->hcd_rh_func;
                otg->otg_output_ops[LOC_SOF_OUT] = hcd_ops->loc_sof_func;
                otg->otg_output_ops[LOC_SUSPEND_OUT] = hcd_ops->loc_suspend_func;
                otg->otg_output_ops[REMOTE_WAKEUP_EN_OUT] = hcd_ops->remote_wakeup_en_func;
                otg->otg_output_ops[HNP_EN_OUT] = hcd_ops->hnp_en_func;
                otg_hcd_ops_framenum = hcd_ops->framenum;
        }
        else {
                if (otg->hcd) {
                        otg->hcd->TAG = otg_trace_invalidate_tag(otg->hcd->TAG);
                        LKFREE(otg->hcd);
                        otg->hcd = NULL;
                }
                otg->otg_output_ops[HCD_INIT_OUT] =
                otg->otg_output_ops[HCD_EN_OUT] =
                otg->otg_output_ops[HCD_RH_OUT] =
                otg->otg_output_ops[LOC_SOF_OUT] =
                otg->otg_output_ops[LOC_SUSPEND_OUT] =
                otg->otg_output_ops[REMOTE_WAKEUP_EN_OUT] =
                otg->otg_output_ops[HNP_EN_OUT] = NULL;
                otg_hcd_ops_framenum = NULL;
        }

        UNLESS (otg->otg_output_ops[HCD_INIT_OUT]) {
                TRACE_MSG0(CORE, "USING OTG_GEN_INIT_FUNC");
                otg->otg_output_ops[HCD_INIT_OUT] = otg_gen_init_func;
        }

        return hcd;
}

/*!
 * otg_ocd_set_ops() -Connect ocd operations to otg instance
 *
 * @param otg - otg instance pointer
 * @param ocd_ops - ocd operations table to use
 * @return pointer to ocd instance
 */
struct ocd_instance * otg_set_ocd_ops(struct otg_instance *otg, struct ocd_ops *ocd_ops)
{
        struct ocd_instance *ocd = NULL;
        otg->ocd_ops = ocd_ops;
        if (ocd_ops) {
                RETURN_NULL_UNLESS((ocd = CKMALLOC(sizeof(struct ocd_instance))));
                ocd->otg = otg;
                ocd->TAG = otg_trace_obtain_tag(otg, "set_ocd_ops");
                otg->ocd = ocd;
                otg->otg_output_ops[OCD_INIT_OUT] = ocd_ops->ocd_init_func;
                otg->start_timer = ocd_ops->start_timer;
                otg_ocd_ops_ticks = ocd_ops->ticks;
                otg_ocd_ops_elapsed = ocd_ops->elapsed;
                otg_interrupts = &otg->interrupts;
        }
        else {
                if (otg->ocd) {
                        otg->ocd->TAG = otg_trace_invalidate_tag(otg->ocd->TAG);
                        LKFREE(otg->ocd);
                        otg->ocd = NULL;
                }
                otg_ocd_ops_ticks = NULL;
                otg_ocd_ops_elapsed = NULL;
                otg->otg_output_ops[OCD_INIT_OUT] = NULL;
                otg->start_timer = NULL;
                otg_interrupts = NULL;
        }

        UNLESS (otg->otg_output_ops[OCD_INIT_OUT]) {
                TRACE_MSG0(CORE, "USING OTG_GEN_INIT_FUNC");
                otg->otg_output_ops[OCD_INIT_OUT] = otg_gen_init_func;
        }

        UNLESS (otg->start_timer)
                otg->start_timer = otg_gen_start_timer;

        return ocd;
}

/*!
 * otg_pcd_set_ops() - connect pcd operations to otg instance operations
 * @param otg - otg instance pointer
 * @param pcd_ops - pcd operations table to use
 * @return pointer to initialized pcd instance
 */
struct pcd_instance * otg_set_pcd_ops(struct otg_instance *otg, struct pcd_ops *pcd_ops)
{
        struct pcd_instance *pcd = NULL;
        otg->pcd_ops = pcd_ops;
        if (pcd_ops) {
                RETURN_NULL_UNLESS((pcd = CKMALLOC(sizeof(struct pcd_instance))));
                pcd->otg = otg;
                pcd->TAG = otg_trace_obtain_tag(otg, "set_pcd_ops");
                otg->pcd = pcd;
                otg->otg_output_ops[PCD_INIT_OUT] = pcd_ops->pcd_init_func;
                otg->otg_output_ops[PCD_EN_OUT] = pcd_ops->pcd_en_func;
                otg->otg_output_ops[REMOTE_WAKEUP_OUT] = pcd_ops->remote_wakeup_func;
                otg_pcd_ops_framenum = pcd_ops->framenum;

                if (pcd_ops->tcd_en_func)
                        otg->otg_output_ops[TCD_EN_OUT] = pcd_ops->tcd_en_func;
                if (pcd_ops->dp_pullup_func)
                        otg->otg_output_ops[DP_PULLUP_OUT] = pcd_ops->dp_pullup_func;
                otg_pcd_ops_ticks = pcd_ops->ticks;
                otg_pcd_ops_elapsed = pcd_ops->elapsed;
        }
        else {
                if (otg->pcd) {
                        otg->pcd->TAG = otg_trace_invalidate_tag(otg->pcd->TAG);
                        LKFREE(otg->pcd);
                        otg->pcd = NULL;
                }
                otg_pcd_ops_ticks = NULL;
                otg_pcd_ops_elapsed = NULL;
                otg->otg_output_ops[PCD_INIT_OUT] =
                otg->otg_output_ops[PCD_EN_OUT] =
                otg->otg_output_ops[REMOTE_WAKEUP_OUT] = NULL;
                otg_pcd_ops_framenum = NULL;
        }

        UNLESS (otg->otg_output_ops[PCD_INIT_OUT]) {
                TRACE_MSG0(CORE, "USING OTG_GEN_INIT_FUNC");
                otg->otg_output_ops[PCD_INIT_OUT] = otg_gen_init_func;
        }

        return pcd;
}

/*!
 * otg_tcd_set_ops() - connect tcd opereations to otg instance operations
 * @param otg - otg instance pointer
 * @param tcd_ops - tcd operations table to use
 * @return pointer to tcd intance
 */
struct tcd_instance * otg_set_tcd_ops(struct otg_instance *otg, struct tcd_ops *tcd_ops)
{
        struct tcd_instance *tcd = NULL;
        otg->tcd_ops = tcd_ops;
        if (tcd_ops) {
                RETURN_NULL_UNLESS((tcd = CKMALLOC(sizeof(struct tcd_instance))));
                tcd->otg = otg;
                tcd->TAG = otg_trace_obtain_tag(otg, "set_tcd_ops");
                otg->tcd = tcd;
                otg->otg_output_ops[TCD_INIT_OUT] = tcd_ops->tcd_init_func;
                otg->otg_output_ops[TCD_EN_OUT] = tcd_ops->tcd_en_func;
                otg->otg_output_ops[CHRG_VBUS_OUT] = tcd_ops->chrg_vbus_func;
                otg->otg_output_ops[DRV_VBUS_OUT] = tcd_ops->drv_vbus_func;
                otg->otg_output_ops[DISCHRG_VBUS_OUT] = tcd_ops->dischrg_vbus_func;
                otg->otg_output_ops[DP_PULLUP_OUT] = tcd_ops->dp_pullup_func;
                otg->otg_output_ops[DM_PULLUP_OUT] = tcd_ops->dm_pullup_func;
                otg->otg_output_ops[DP_PULLDOWN_OUT] = tcd_ops->dp_pulldown_func;
                otg->otg_output_ops[DM_PULLDOWN_OUT] = tcd_ops->dm_pulldown_func;
                //otg->otg_output_ops[PERIPHERAL_HOST] = tcd_ops->peripheral_host_func;
                otg->otg_output_ops[CLR_OVERCURRENT_OUT] = tcd_ops->overcurrent_func;
                otg->otg_output_ops[DM_DET_OUT] = tcd_ops->dm_det_func;
                otg->otg_output_ops[DP_DET_OUT] = tcd_ops->dp_det_func;
                otg->otg_output_ops[CR_DET_OUT] = tcd_ops->cr_det_func;
                otg->otg_output_ops[AUDIO_OUT] = tcd_ops->audio_func;
                otg->otg_output_ops[CHARGE_PUMP_OUT] = tcd_ops->charge_pump_func;
                otg->otg_output_ops[BDIS_ACON_OUT] = tcd_ops->bdis_acon_func;
                //otg->otg_output_ops[MX21_VBUS_DRAIN] = tcd_ops->mx21_vbus_drain_func;
                otg->otg_output_ops[ID_PULLDOWN_OUT] = tcd_ops->id_pulldown_func;
                otg->otg_output_ops[UART_OUT] = tcd_ops->uart_func;
                otg->otg_output_ops[MONO_OUT] = tcd_ops->mono_func;
        }
        else  {
		if (otg->tcd){
	                otg->tcd->TAG = otg_trace_invalidate_tag(otg->tcd->TAG);
                        LKFREE(otg->tcd);
		        otg->tcd = NULL;
		}
                otg->otg_output_ops[TCD_INIT_OUT] =
                otg->otg_output_ops[TCD_EN_OUT] =
                otg->otg_output_ops[CHRG_VBUS_OUT] =
                otg->otg_output_ops[DRV_VBUS_OUT] =
                otg->otg_output_ops[DISCHRG_VBUS_OUT] =
                otg->otg_output_ops[DP_PULLUP_OUT] =
                otg->otg_output_ops[DM_PULLUP_OUT] =
                otg->otg_output_ops[DP_PULLDOWN_OUT] =
                otg->otg_output_ops[DM_PULLDOWN_OUT] =
                //otg->otg_output_ops[PERIPHERAL_HOST] =
                otg->otg_output_ops[CLR_OVERCURRENT_OUT] =
                otg->otg_output_ops[DM_DET_OUT] =
                otg->otg_output_ops[DP_DET_OUT] =
                otg->otg_output_ops[CR_DET_OUT] =
                otg->otg_output_ops[AUDIO_OUT] =
                otg->otg_output_ops[CHARGE_PUMP_OUT] =
                otg->otg_output_ops[BDIS_ACON_OUT] =
                //otg->otg_output_ops[MX21_VBUS_DRAIN] =
                otg->otg_output_ops[ID_PULLDOWN_OUT] =
                otg->otg_output_ops[UART_OUT] =
                otg->otg_output_ops[MONO_OUT] = NULL;
        }

        UNLESS (otg->otg_output_ops[TCD_INIT_OUT]) {
                TRACE_MSG0(CORE, "USING OTG_GEN_INIT_FUNC");
                otg->otg_output_ops[TCD_INIT_OUT] = otg_gen_init_func;
        }

        return tcd;
}

/*!
 * otg_get_ocd_info
 */
void otg_get_ocd_info(struct otg_instance *otg, otg_tick_t *ticks, u16 *p_framenum)
{
        *ticks = (otg_ocd_ops_ticks) ? otg_ocd_ops_ticks () : ((otg_pcd_ops_ticks) ? otg_pcd_ops_ticks () : 0);
        if (p_framenum)
                *p_framenum = ((otg && otg_pcd_ops_framenum) ? otg_pcd_ops_framenum(otg) : 0);
}


#if defined(CONFIG_OTG_TRACE)
/*!
 * otg_get_trace_info() - get otg trace inforamtion
 * @param p - pointer to tarce slot
 * @return none
 */
void otg_get_trace_info(struct otg_instance *otg, otg_trace_t *p)
{
        #if !defined(OTG_TRACE_DISABLE) && ( defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE))
        RETURN_UNLESS( (p) );

        //p->hcd_pcd = 0;

        // XXX p->id_gnd = otg_instance_info->current_inputs & ID_GND ? 1 : 0;
        // XXX if (hcd_instance_private.active) p->hcd_pcd |= 0x1;
        // XXX if (pcd_instance_private.active) p->hcd_pcd |= 0x2;

        #if 1
        p->ticks = (otg && otg_ocd_ops_ticks) ? otg_ocd_ops_ticks () : ((otg && otg_pcd_ops_ticks) ? otg_pcd_ops_ticks () : 0);
        p->interrupts = (otg && otg_interrupts) ? *otg_interrupts : 0;
        p->h_framenum = ((otg && otg_hcd_ops_framenum) ? otg_hcd_ops_framenum(otg) : 0);
        p->p_framenum = ((otg && otg_pcd_ops_framenum) ? otg_pcd_ops_framenum(otg) : 0);
        #else
        otg_get_ocd_info(otg, &p->ticks, &p->interrupts, &p->h_framenum, &p->p_framenum);
        #endif

        #ifdef LINUX26
        if (in_interrupt()) p->flags |= OTG_TRACE_IN_INTERRUPT;
        #endif /* LINUX26 */
        //p->in_interrupt = in_interrupt();
        #endif /* !defined(OTG_TRACE_DISABLE) && ( defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE)) */
}

void otg_trace_copy(u32 *p, u32 val)
{
        //*p = val;
}

#endif


/*!
 * otg_tmr_ticks() - get ticks
 *
 * @return number of ticks.
 */
otg_tick_t otg_tmr_ticks(void)
{
        struct timeval tv;

        if (otg_ocd_ops_ticks)
                return otg_ocd_ops_ticks ();

        if (otg_pcd_ops_ticks)
                return otg_pcd_ops_ticks ();

        otg_gettimeofday(&tv);
        return tv.tv_sec * 1000000 + tv.tv_usec;
}

/*!
 * otg_tmr_elapsed() -
 * @param t1
 * @param t2
 * @return number of uSecs between t1 and t2 ticks.
 */
otg_tick_t otg_tmr_elapsed(otg_tick_t *t1, otg_tick_t *t2)
{
        return otg_ocd_ops_elapsed ? otg_ocd_ops_elapsed (t1, t2) :
                (otg_pcd_ops_elapsed ? otg_pcd_ops_elapsed (t1, t2) :
                (((*t1 > *t2) ? (*t1 - *t2) : (*t2 - *t1))));
}

OTG_EXPORT_SYMBOL(otg_set_hcd_ops);
OTG_EXPORT_SYMBOL(otg_set_ocd_ops);
OTG_EXPORT_SYMBOL(otg_set_pcd_ops);
OTG_EXPORT_SYMBOL(otg_set_tcd_ops);
OTG_EXPORT_SYMBOL(otg_set_usbd_ops);
OTG_EXPORT_SYMBOL(otg_tmr_ticks);
OTG_EXPORT_SYMBOL(otg_tmr_elapsed);
OTG_EXPORT_SYMBOL(otg_get_ocd_info);
#if defined(LINUX24)
//OTG_EXPORT_SYMBOL(otg_write_input_message_irq);
#endif


#if defined(OTG_WINCE)
#else /* defined(OTG_WINCE) */
#endif /* defined(OTG_WINCE) */
