/*
 * vivi/include/arch-s3c24a0/proc.h: 
 *   S3C2410에 의존적인 녀석들을 정의하는 곳.
 *   vivi/include/processor.h에 인클루드 됨. 다른 어떤 곳에서도 요놈을 인클루드
 *   하지 않음
 *
 * Copyright (C) 2003 MIZI Research, Inc.
 *
 * Author: Janghoon Lyu <nandy@mizi.com>
 *
 * $Date: 2004/12/21 05:44:07 $ 
 * $Revision: 1.3 $
 * $Id: proc.h,v 1.3 2004/12/21 05:44:07 shaju Exp $
 *
 *
 * History
 *
 * 2002-07-08: Janghoon Lyu <nandy@mizi.com>
 *     - 처음 이 파일을 만들었음.
 *
 */

#ifndef _PROC_S3C24A0_H_
#define _PROC_S3C24A0_H_

#include <asm/arch/s3c24a0.h>

/*
 * UART
 *
 * You may be define six functions.
 *  SERIAL_READ_READY(), SERIAL_READ_CHAR(), SERIAL_READ_STATUS(),
 *  SERIAL_WRITE_READY(), SERIAL_WRITE_CHAR(), SERIAL_WRITE_STATUS()
 */

#ifdef POSEIDON
#define CONFIG_SERIAL_UART1 1
#elif defined(OREIAS)
#define CONFIG_SERIAL_UART1 1
#else
#define CONFIG_SERIAL_UART0 1
#endif

#ifdef CONFIG_SERIAL_UART0
#define SERIAL_CHAR_READY()     (UTRSTAT0 & UTRSTAT_RX_READY)
#define SERIAL_READ_CHAR()      URXH0
#define SERIAL_READ_STATUS()    (UERSTAT0 & UART_ERR_MASK) 
#elif defined(CONFIG_SERIAL_UART1)
#define SERIAL_CHAR_READY()     (UTRSTAT1 & UTRSTAT_RX_READY)
#define SERIAL_READ_CHAR()      URXH1
#define SERIAL_READ_STATUS()    (UERSTAT1 & UART_ERR_MASK) 
#elif defined(CONFIG_SERIAL_UART2)
#define SERIAL_CHAR_READY()     (UTRSTAT2 & UTRSTAT_RX_READY)
#define SERIAL_READ_CHAR()      URXH2
#define SERIAL_READ_STATUS()    (UERSTAT2 & UART_ERR_MASK) 
#else
#error not support this serial port
#endif


#ifdef CONFIG_SERIAL_UART0
#define SERIAL_WRITE_STATUS()	(UTRSTAT0)
#define SERIAL_WRITE_READY()	((UTRSTAT0) & UTRSTAT_TX_EMPTY)
#define SERIAL_WRITE_CHAR(c)	((UTXH0) = (c))
#elif defined(CONFIG_SERIAL_UART1)
#define SERIAL_WRITE_STATUS()	(UTRSTAT1)
#define SERIAL_WRITE_READY()	((UTRSTAT1) & UTRSTAT_TX_EMPTY)
#define SERIAL_WRITE_CHAR(c)	((UTXH1) = (c))
#elif defined(CONFIG_SERIAL_UART2)
#define SERIAL_WRITE_STATUS()	(UTRSTAT2)
#define SERIAL_WRITE_READY()	((UTRSTAT2) & UTRSTAT_TX_EMPTY)
#define SERIAL_WRITE_CHAR(c)	((UTXH2) = (c))
#else
#error not support this serial port
#endif

#endif /* _PROC_S3C24A0_H_ */
