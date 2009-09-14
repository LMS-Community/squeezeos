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
 * otg/functions/msc/msc-io.h - Mass Storage Class
 * @(#) balden@belcarra.com|otg/functions/msc/msc-io.h|20060510202833|21396
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/functions/msc/msc-io.h
 * @brief Mass Storage Driver private defines
 *
 * OTGMSC_START                 - make specified major/minor device available to USB Host
 * OTGMSC_WRITEPROTECT          - set or reset write protect flag
 *
 * OTGMSC_STOP                  - remove access to current device, block until pending I/O finished
 * OTGMSC_STATUS                - remove current USB connection status (connected or disconnected)
 * OTGMSC_WAIT_CONNECT          - wait until device is connected (may return immediately if already connected)
 * OTGMSC_WAIT_DISCONNECT       - wait until device is disconnected (may return immediately if already disconnected)
 *
 * @ingroup MSCFunction
 */

//#ifndef MSC_H
//#define MSC_H 1

#define MSC_IO "/proc/msc_io"
/*! @struct otgmsc_mount msc-io.h otg/functions/msc/msc-io.h
 * @brief struct for managing device mount/umount operation
 */
struct otgmsc_mount {
        int     major;
        int     minor;
        int     lun;
        int     writeprotect;
        int     result;
        int     status;
};

#define OTGMSC_MAGIC    'M'
#define OTGMSC_MAXNR    10

#define OTGMSC_START            _IOWR(OTGMSC_MAGIC, 1, struct otgmsc_mount)
#define OTGMSC_WRITEPROTECT     _IOWR(OTGMSC_MAGIC, 2, struct otgmsc_mount)

#define OTGMSC_STOP             _IOR(OTGMSC_MAGIC, 1, struct otgmsc_mount)
#define OTGMSC_STATUS           _IOR(OTGMSC_MAGIC, 2, struct otgmsc_mount)
#define OTGMSC_WAIT_CONNECT     _IOR(OTGMSC_MAGIC, 3, struct otgmsc_mount)
#define OTGMSC_WAIT_DISCONNECT  _IOR(OTGMSC_MAGIC, 4, struct otgmsc_mount)


#define MSC_CONNECTED           0x01
#define MSC_DISCONNECTED        0x02
#define MSC_WRITEPROTECTED      0x04

//#endif /* MSC_H */
