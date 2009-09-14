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
 * otg/functions/network/fermat.h - Network Function Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/network/fermat.h|20061218212925|58148
 *
 *      Copyright (c) 2003-2006 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/functions/network/fermat.h
 * @brief Fermat related data structures.
 *
 *
 * @ingroup NetworkFunction
 */

#ifndef FERMAT_DEFINED
#define FERMAT_DEFINED 1
typedef unsigned char BYTE;
/*! create an alias for fermat
 * @brief typedef struct fermat FERMAT
 */
typedef struct fermat {
        int length;
        BYTE power[256];
} FERMAT;

void fermat_init(void);
void fermat_encode(BYTE *data, int length);
void fermat_decode(BYTE *data, int length);
#endif
