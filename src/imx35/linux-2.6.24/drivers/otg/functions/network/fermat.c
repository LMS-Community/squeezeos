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
 * otg/functions/network/fermat.c - Network Function Driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/functions/network/fermat.c|20070425221028|05672
 *
 *      Copyright (c) 2003-2006 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/functions/network/fermat.c
 * @brief This implements a special data munging function that
 * randomizes data. This is a very specific fix for a device
 * that had trouble with runs of zeros.
 *
 * @ingroup NetworkFunction
 */

#include <otg/otg-compat.h>

#if defined(CONFIG_OTG_LNX) || defined(_OTG_DOXYGEN)


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#ifdef CONFIG_OTG_NETWORK_BLAN_FERMAT

#include "fermat.h"

#ifndef FERMAT_DEFINED
typedef unsigned char BYTE;
typedef struct fermat {
        int length;
        BYTE power[256];
} FERMAT;
#endif
/*! int fermat_setup(FERMAT *, int )
 * @brief set up fermat table
 * @param p - pointer to FERMAT
 * @param seed - seed to generate FERMAT data
 * @return number of FERMAT length
 */

static int fermat_setup(FERMAT *p, int seed){
        int i = 0;
        unsigned long x,y;
        y = 1;
        do{
                x = y;
                p->power[i] = ( x == 256 ? 0 : x);
                y = ( seed * x ) % 257;
                i += 1;
        }while( y != 1);
        p->length = i;
        return i;
}

/*! void fermat_xform(FERMAT*, BYTE*, int)
 * @brief transform data using fermat table
 * @param p - pointer to fermat data
 * @param data - pointer to data to transform
 * @param length - length of data to transform
 * @return none
 */

static void fermat_xform(FERMAT *p, BYTE *data, int length){
        BYTE *pw = p->power;
        int   i, j;
        BYTE * q ;
        for(i = 0, j=0, q = data; i < length; i++, j++, q++){
                if(j>=p->length){
                        j = 0;
                }
                *q ^= pw[j];
        }
}

static FERMAT default_fermat;
static const int primitive_root = 5;
/*! void fermat_init()
 * @brief initialize fermat using privitive_root as seed
 *
 * @return none
 */
void fermat_init(){
        (void) fermat_setup(&default_fermat, primitive_root);
}

// Here are the public official versions.
// Change the primitive_root above to another primitive root
// if you need better scatter. Possible values are 3 and 7

/*! void fermat_encode( BYTE*, int)
 * @brief using fermat encode data
 *
 * @param data  - pointer to data to encode
 * @param length - data length
 * @return none
 */
void fermat_encode(BYTE *data, int length){
        fermat_xform(&default_fermat, data, length);
}

/*! void fermat_decode(BYTE* int)
 * @brief decode data
 *
 * @param data - pointer to data to decode
 * @param length - data lenght
 * @return none
 */
void fermat_decode(BYTE *data, int length){
        fermat_xform(&default_fermat, data, length);
}


// Note: the seed must be a "primitive root" of 257. This means that
// the return value of the setup routine must be 256 (otherwise the
// seed is not a primitive root.  The routine will still work fine
// but will be less pseudo-random.

#undef TEST
#if TEST
#include <stdio.h>
#include <memory.h>

/*! main
 * @brief - Use FERMAT in two ways: to encode, and to generate test data.
 */

main(){
        //Note 3, 5, and 7 are primitive roots of 257
        // 11 is not a primitive root
        FERMAT three, five, seven;

        FERMAT three2;
        printf("Cycle lengths: 3,5,7 %d %d %d \n",
                        fermat_setup(&three, 3),
                        fermat_setup(&five, 5),
                        fermat_setup(&seven, 7));
        three2=three; // Copy data from three
        fermat_xform(&three,three2.power,three2.length);
        fermat_xform(&five,three2.power,three2.length);
        fermat_xform(&seven,three2.power,three2.length);
        fermat_xform(&seven,three2.power,three2.length);
        fermat_xform(&five,three2.power,three2.length);
        fermat_xform(&three,three2.power,three2.length);

        //At this stage, three2 and three should be identical
        if(memcpy(&three,&three2,sizeof(FERMAT))){
                printf("Decoded intact\n");
        }

        fermat_init();
        fermat_encode(three2.power,256);

}
#endif

#endif /* CONFIG_OTG_NETWORK_BLAN_FERMAT */

#endif /* defined(CONFIG_OTG_LNX) */
