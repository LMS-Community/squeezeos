/* $Id: scatterlist.h 1763 2006-09-22 01:22:23Z dean $ */
#ifndef _SPARC64_SCATTERLIST_H
#define _SPARC64_SCATTERLIST_H

#include <asm/page.h>

struct scatterlist {
    char *  address;    /* Location data is to be transferred to */
    char * alt_address; /* Location of actual if address is a 
			 * dma indirect buffer.  NULL otherwise */
    unsigned int length;

    __u32 dvma_address; /* A place to hang host-specific addresses at. */
    __u32 dvma_length;
};

#define sg_dma_address(sg) ((sg)->dvma_address)
#define sg_dma_len(sg)     ((sg)->dvma_length)

#define ISA_DMA_THRESHOLD	(~0UL)

#endif /* !(_SPARC64_SCATTERLIST_H) */
