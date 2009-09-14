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
 * otg/otg/otg-utils.h
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otg/otg-utils.h|20070612174152|35048
 *
 *      Copyright (c) 2004-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 *      Basic utilities for OTG package
 */
/*!
 * @file otg/otg/otg-utils.h
 * @brief Useful macros.
 *
 *
 * @ingroup OTGCore
 */


#if !defined(_OTG_UTILS_H)
#define _OTG_UTILS_H 1

/*! @name Min, Max and zero
 * @{
 */
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define ZERO(a) memset(&a, 0, sizeof(a))

/* @} */

/*! @name TRUE and FALSE
 *
 * @{
 */
#if !defined(TRUE)
#define TRUE 1
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#define BOOL int
#define BOOLEAN(e)((e)?TRUE:FALSE)

/* @} */


/*! @name CATCH and THROW
 *
 * These macros implement a conceptually clean way to use
 * C's goto to implement a standard error handler. The typical forms are:
 *
 *      THROW(error);                   // continue execution in CATCH statement
 *      THROW_IF(test, error);          // continue execution in CATCH statement if test true
 *      THROW_UNLESS(test, error);      // continue execution in CATCH statement if test false
 *
 *      // program execution will continue after the CATCH statement;
 *      CATCH(error) {
 *              // handle error here
 *      }
 *
 * @{
 */
#define CATCH(x) while(0) x:
#define THROW(x) goto x
#define THROW_IF(e, x) if (e)  goto x;
#define THROW_UNLESS(e, x) UNLESS (e)  goto x;
/* @} */


/*! @name UNLESS
 *
 * This implements a simple equivalent to negated if (expression).
 *
 *      Unless expression then statement else statement.
 *
 * @{
 */
#define unless(x) if(!(x))
#define UNLESS(x) if(!(x))
/* @} */

/*! @name BREAK_ and CONTINUE_
 *
 * These implement a conditional break and continue statement.
 *
 * @{
 */
#define BREAK_UNLESS(x) UNLESS (x)  break;
#define BREAK_IF(x) if (x)  break;
#define CONTINUE_IF(x) if (x)  continue;
#define CONTINUE_UNLESS(x) UNLESS (x)  continue;

/* @} */


/*! @name RETURN_
 *
 * These implement conditional return statement.
 * @{
 */
#define RETURN_IF(x) if (x)  return;
#define RETURN_ZERO_IF(x) if (x)  return 0;
#define RETURN_NULL_IF(x) if (x)  return NULL;
#define RETURN_EBUSY_IF(x) if (x)  return -EBUSY;
#define RETURN_EFAULT_IF(x) if (x)  return -EFAULT;
#define RETURN_EINVAL_IF(x) if (x)  return -EINVAL;
#define RETURN_ENOMEM_IF(x) if (x)  return -ENOMEM;
#define RETURN_ENODEV_IF(x) if (x)  return -ENODEV;
#define RETURN_EAGAIN_IF(x) if (x)  return -EAGAIN;
#define RETURN_ETIMEDOUT_IF(x) if (x)  return -ETIMEDOUT;
#define RETURN_EPIPE_IF(x) if (x)  return -EPIPE;
#define RETURN_EUNATCH_IF(x) if (x)  return -EUNATCH;
#define RETURN_IRQ_HANDLED_IF(x) if (x)  return IRQ_HANDLED;
#define RETURN_IRQ_HANDLED_IF_IRQ_HANDLED(x) if (IRQ_HANDLED == x)  return IRQ_HANDLED;
#define RETURN_IRQ_NONE_IF(x) if (x)  return IRQ_NONE;
#define RETURN_TRUE_IF(x) if (x)  return TRUE;
#define RETURN_FALSE_IF(x) if (x)  return FALSE;

#define RETURN_UNLESS(x) UNLESS (x)  return;
#define RETURN_NULL_UNLESS(x) UNLESS (x)  return NULL;
#define RETURN_ZERO_UNLESS(x) UNLESS (x)  return 0;
#define RETURN_EBUSY_UNLESS(x) UNLESS (x)  return -EBUSY;
#define RETURN_EFAULT_UNLESS(x) UNLESS (x)  return -EFAULT;
#define RETURN_EINVAL_UNLESS(x) UNLESS (x)  return -EINVAL;
#define RETURN_ENOMEM_UNLESS(x) UNLESS (x)  return -ENOMEM;
#define RETURN_ENODEV_UNLESS(x) UNLESS (x)  return -ENODEV;
#define RETURN_EAGAIN_UNLESS(x) UNLESS (x)  return -EAGAIN;
#define RETURN_ETIMEDOUT_UNLESS(x) UNLESS (x)  return -ETIMEDOUT;
#define RETURN_EPIPE_UNLESS(x) UNLESS (x)  return -EPIPE;
#define RETURN_EUNATCH_UNLESS(x) UNLESS (x)  return -EUNATCH;
#define RETURN_IRQ_HANDLED_UNLESS(x) UNLESS (x)  return IRQ_HANDLED;
#define RETURN_IRQ_NONE_UNLESS(x) UNLESS (x)  return IRQ_NONE;
#define RETURN_TRUE_UNLESS(x) UNLESS (x)  return TRUE;
#define RETURN_FALSE_UNLESS(x) UNLESS (x)  return FALSE;

#define UNTIL(x) while (!(x))

/* @} */



#endif /* _OTG_UTILS_H */
