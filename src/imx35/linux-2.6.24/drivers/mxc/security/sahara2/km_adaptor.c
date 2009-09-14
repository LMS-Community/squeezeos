/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
* @file km_adaptor.c
*
* @brief The Adaptor component provides an interface to the
*        driver for a kernel user.
*/

#include <adaptor.h>
#include <sf_util.h>
#include <sah_queue_manager.h>
#include <sah_memory_mapper.h>
#ifdef FSL_HAVE_SCC
#include <asm/arch/mxc_scc_driver.h>
#else
#include <asm/arch/mxc_scc2_driver.h>
#endif


EXPORT_SYMBOL(adaptor_Exec_Descriptor_Chain);
EXPORT_SYMBOL(sah_register);
EXPORT_SYMBOL(sah_deregister);
EXPORT_SYMBOL(sah_get_results);
EXPORT_SYMBOL(do_scc_slot_alloc);
EXPORT_SYMBOL(do_scc_slot_dealloc);
EXPORT_SYMBOL(do_scc_slot_load_slot);
EXPORT_SYMBOL(do_scc_slot_encrypt);
EXPORT_SYMBOL(do_scc_slot_decrypt);

#if defined(DIAG_DRV_IF) || defined(DIAG_MEM) || defined(DIAG_ADAPTOR)
#include <diagnostic.h>
#endif

#if defined(DIAG_DRV_IF) || defined(DIAG_MEM) || defined(DIAG_ADAPTOR)
#define MAX_DUMP 16

#define DIAG_MSG_SIZE   300
static char Diag_msg[DIAG_MSG_SIZE];
#endif

/* This is the wait queue to this mode of driver */
DECLARE_WAIT_QUEUE_HEAD(Wait_queue_km);

#ifdef DIAG_ADAPTOR
void km_Dump_Chain(const sah_Desc * chain);

void km_Dump_Region(const char *prefix, const unsigned char *data,
		    unsigned length);

static void km_Dump_Link(const char *prefix, const sah_Link * link);

void km_Dump_Words(const char *prefix, const unsigned *data, unsigned length);
#endif

/**** Memory routines ****/

static void *my_malloc(void *ref, size_t n)
{
	register void *mem;

#ifndef DIAG_MEM_ERRORS
	mem = os_alloc_memory(n, GFP_KERNEL);

#else
	{
		uint32_t rand;
		/* are we feeling lucky ? */
		os_get_random_bytes(&rand, sizeof(rand));
		if ((rand % DIAG_MEM_CONST) == 0) {
			mem = 0;
		} else {
			mem = os_alloc_memory(n, GFP_ATOMIC);
		}
	}
#endif				/* DIAG_MEM_ERRORS */

#ifdef DIAG_MEM
	sprintf(Diag_msg, "API kmalloc: %p for %d\n", mem, n);
	LOG_KDIAG(Diag_msg);
#endif
	ref = 0;		/* unused param warning */
	return mem;
}

static sah_Head_Desc *my_alloc_head_desc(void *ref)
{
	register sah_Head_Desc *ptr;

#ifndef DIAG_MEM_ERRORS
	ptr = sah_Alloc_Head_Descriptor();

#else
	{
		uint32_t rand;
		/* are we feeling lucky ? */
		os_get_random_bytes(&rand, sizeof(rand));
		if ((rand % DIAG_MEM_CONST) == 0) {
			ptr = 0;
		} else {
			ptr = sah_Alloc_Head_Descriptor();
		}
	}
#endif
	ref = 0;
	return ptr;
}

static sah_Desc *my_alloc_desc(void *ref)
{
	register sah_Desc *ptr;

#ifndef DIAG_MEM_ERRORS
	ptr = sah_Alloc_Descriptor();

#else
	{
		uint32_t rand;
		/* are we feeling lucky ? */
		os_get_random_bytes(&rand, sizeof(rand));
		if ((rand % DIAG_MEM_CONST) == 0) {
			ptr = 0;
		} else {
			ptr = sah_Alloc_Descriptor();
		}
	}
#endif
	ref = 0;
	return ptr;
}

static sah_Link *my_alloc_link(void *ref)
{
	register sah_Link *ptr;

#ifndef DIAG_MEM_ERRORS
	ptr = sah_Alloc_Link();

#else
	{
		uint32_t rand;
		/* are we feeling lucky ? */
		os_get_random_bytes(&rand, sizeof(rand));
		if ((rand % DIAG_MEM_CONST) == 0) {
			ptr = 0;
		} else {
			ptr = sah_Alloc_Link();
		}
	}
#endif
	ref = 0;
	return ptr;
}

static void my_free(void *ref, void *ptr)
{
	ref = 0;		/* unused param warning */
#ifdef DIAG_MEM
	sprintf(Diag_msg, "API kfree: %p\n", ptr);
	LOG_KDIAG(Diag_msg);
#endif
	os_free_memory(ptr);
}

static void my_free_head_desc(void *ref, sah_Head_Desc * ptr)
{
	sah_Free_Head_Descriptor(ptr);
}

static void my_free_desc(void *ref, sah_Desc * ptr)
{
	sah_Free_Descriptor(ptr);
}

static void my_free_link(void *ref, sah_Link * ptr)
{
	sah_Free_Link(ptr);
}

static void *my_memcpy(void *ref, void *dest, const void *src, size_t n)
{
	ref = 0;		/* unused param warning */
	return memcpy(dest, src, n);
}

static void *my_memset(void *ref, void *ptr, int ch, size_t n)
{
	ref = 0;		/* unused param warning */
	return memset(ptr, ch, n);
}

/*! Standard memory manipulation routines for kernel API. */
static sah_Mem_Util std_kernelmode_mem_util = {
	.mu_ref = 0,
	.mu_malloc = my_malloc,
	.mu_alloc_head_desc = my_alloc_head_desc,
	.mu_alloc_desc = my_alloc_desc,
	.mu_alloc_link = my_alloc_link,
	.mu_free = my_free,
	.mu_free_head_desc = my_free_head_desc,
	.mu_free_desc = my_free_desc,
	.mu_free_link = my_free_link,
	.mu_memcpy = my_memcpy,
	.mu_memset = my_memset
};

/*!
 * Sends a request to register this user
 *
 * @brief    Sends a request to register this user
 *
 * @param[in,out] user_ctx  part of the structure contains input parameters and
 *                          part is filled in by the driver
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t sah_register(fsl_shw_uco_t * user_ctx)
{
	fsl_shw_return_t status;

	/* this field is used in user mode to indicate a file open has occured.
	 * it is used here, in kernel mode, to indicate that the uco is registered
	 */
	user_ctx->sahara_openfd = 0;	/* set to 'registered' */
	user_ctx->mem_util = &std_kernelmode_mem_util;

	/* check that uco is valid */
	status = sah_validate_uco(user_ctx);

	/*  If life is good, register this user */
	if (status == FSL_RETURN_OK_S) {
		status = sah_handle_registration(user_ctx);
	}

	if (status != FSL_RETURN_OK_S) {
		user_ctx->sahara_openfd = -1;	/* set to 'not registered' */
	}

	return status;
}

/*!
 * Sends a request to deregister this user
 *
 * @brief    Sends a request to deregister this user
 *
 * @param[in,out]  user_ctx   Info on user being deregistered.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t sah_deregister(fsl_shw_uco_t * user_ctx)
{
	fsl_shw_return_t status = FSL_RETURN_OK_S;

	if (user_ctx->sahara_openfd == 0) {
		status = sah_handle_deregistration(user_ctx);
		user_ctx->sahara_openfd = -1;	/* set to 'no registered */
	}

	return status;
}

/*!
 * Sends a request to get results for this user
 *
 * @brief    Sends a request to get results for this user
 *
 * @param[in,out] arg      Pointer to structure to collect results
 * @param         uco      User's context
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t sah_get_results(sah_results * arg, fsl_shw_uco_t * uco)
{
	fsl_shw_return_t code = sah_get_results_from_pool(uco, arg);

	if ((code == FSL_RETURN_OK_S) && (arg->actual != 0)) {
		sah_Postprocess_Results(uco, arg);
	}

	return code;
}

/*!
 * This function writes the Descriptor Chain to the kernel driver.
 *
 * @brief    Writes the Descriptor Chain to the kernel driver.
 *
 * @param    dar   A pointer to a Descriptor Chain of type sah_Head_Desc
 * @param    uco   The user context object
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t adaptor_Exec_Descriptor_Chain(sah_Head_Desc * dar,
					       fsl_shw_uco_t * uco)
{
	sah_Head_Desc *kernel_space_desc = NULL;
	fsl_shw_return_t code = FSL_RETURN_OK_S;
	int os_error_code = 0;
	unsigned blocking_mode = dar->uco_flags & FSL_UCO_BLOCKING_MODE;

#ifdef DIAG_ADAPTOR
	km_Dump_Chain(&dar->desc);
#endif

	dar->user_info = uco;
	dar->user_desc = dar;

	/* This code has been shamelessly copied from sah_driver_interface.c */
	/* It needs to be moved somewhere common ... */
	kernel_space_desc = sah_Physicalise_Descriptors(dar);

	if (kernel_space_desc == NULL) {
		/* We may have failed due to a -EFAULT as well, but we will return
		 * -ENOMEM since either way it is a memory related failure. */
		code = FSL_RETURN_NO_RESOURCE_S;
#ifdef DIAG_DRV_IF
		LOG_KDIAG("sah_Physicalise_Descriptors() failed\n");
#endif
	} else {
		if (blocking_mode) {
#ifdef SAHARA_POLL_MODE
			os_error_code = sah_Handle_Poll(dar);
#else
			os_error_code = sah_blocking_mode(dar);
#endif
			if (os_error_code != 0) {
				code = FSL_RETURN_ERROR_S;
			} else {	/* status of actual operation */
				code = dar->result;
			}
		} else {
#ifdef SAHARA_POLL_MODE
			sah_Handle_Poll(dar);
#else
			/* just put someting in the DAR */
			sah_Queue_Manager_Append_Entry(dar);
#endif				/* SAHARA_POLL_MODE */
		}
	}

	return code;
}

/*!
 * Allocates a slot in the SCC
 *
 * @brief    Allocates a slot in the SCC
 *
 * @param    user_ctx
 * @param    key_len
 * @param    ownerid
 * @param    slot
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t do_scc_slot_alloc(fsl_shw_uco_t * user_ctx,
				   uint32_t key_len,
				   uint64_t ownerid, uint32_t * slot)
{
	scc_return_t scc_status = scc_alloc_slot(key_len, ownerid, slot);
	fsl_shw_return_t ret;

	if (scc_status == SCC_RET_OK) {
		ret = FSL_RETURN_OK_S;
	} else {
		ret = FSL_RETURN_NO_RESOURCE_S;
	}

	user_ctx = NULL;
	return ret;
}

/*!
 * Deallocates a slot in the SCC
 *
 * @brief    Deallocates a slot in the SCC
 *
 * @param    user_ctx
 * @param    ownerid
 * @param    slot
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t do_scc_slot_dealloc(fsl_shw_uco_t * user_ctx,
				     uint64_t ownerid, uint32_t slot)
{
	scc_return_t scc_status = scc_dealloc_slot(ownerid, slot);
	user_ctx = NULL;
	return (scc_status ==
		SCC_RET_OK) ? FSL_RETURN_OK_S : FSL_RETURN_ERROR_S;
}

/*!
 * Populate a slot in the SCC
 *
 * @brief    Deallocates a slot in the SCC
 *
 * @param   user_ctx
 * @param   uint32_t slot
 * @param   key
 * @param   key_length
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t do_scc_slot_load_slot(fsl_shw_uco_t * user_ctx,
				       uint64_t ownerid, uint32_t slot,
				       const uint8_t * key, uint32_t key_length)
{
	scc_return_t scc_status = scc_load_slot(ownerid, slot, (void *)key,
						key_length);
	user_ctx = NULL;
	return (scc_status ==
		SCC_RET_OK) ? FSL_RETURN_OK_S : FSL_RETURN_ERROR_S;
}

/*!
 * Encrypt what's in a slot on the SCC
 *
 * @brief    Encrypt what's in a slot on the SCC
 *
 * @param    user_ctx
 * @param    ownerid
 * @param    slot
 * @param    key_length
 * @param    black_data
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t do_scc_slot_encrypt(fsl_shw_uco_t * user_ctx,
				     uint64_t ownerid,
				     uint32_t slot,
				     uint32_t key_length, uint8_t * black_data)
{
	scc_return_t scc_code;

	user_ctx = NULL;	/* for unused-param warning */
	scc_code = scc_encrypt_slot(ownerid, slot, key_length, black_data);
	return (scc_code == SCC_RET_OK) ? FSL_RETURN_OK_S : FSL_RETURN_ERROR_S;
}

/*!
 * Deallocates a slot in the SCC
 *
 * @brief    Deallocates a slot in the SCC
 *
 * @param    user_ctx
 * @param    ownerid
 * @param    slot
 * @param    key_length
 * @param    black_data
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t do_scc_slot_decrypt(fsl_shw_uco_t * user_ctx,
				     uint64_t ownerid,
				     uint32_t slot,
				     uint32_t key_length,
				     const uint8_t * black_data)
{
	scc_return_t scc_code;

	user_ctx = NULL;	/* for unused-param warning */
	scc_code = scc_decrypt_slot(ownerid, slot, key_length, black_data);
	return (scc_code == SCC_RET_OK) ? FSL_RETURN_OK_S : FSL_RETURN_ERROR_S;
}

#ifdef DIAG_ADAPTOR
/*!
 * Dump chain of descriptors to the log.
 *
 * @brief Dump descriptor chain
 *
 * @param    chain     Kernel virtual address of start of chain of descriptors
 *
 * @return   void
 */
void km_Dump_Chain(const sah_Desc * chain)
{
	while (chain != NULL) {
		km_Dump_Words("Desc", (unsigned *)chain,
			      6 /*sizeof(*chain)/sizeof(unsigned) */ );
		/* place this definition elsewhere */
		if (chain->ptr1) {
			if (chain->header & SAH_HDR_LLO) {
				km_Dump_Region(" Data1", chain->ptr1,
					       chain->len1);
			} else {
				km_Dump_Link(" Link1", chain->ptr1);
			}
		}
		if (chain->ptr2) {
			if (chain->header & SAH_HDR_LLO) {
				km_Dump_Region(" Data2", chain->ptr2,
					       chain->len2);
			} else {
				km_Dump_Link(" Link2", chain->ptr2);
			}
		}

		chain = chain->next;
	}
}

/*!
 * Dump chain of links to the log.
 *
 * @brief Dump chain of links
 *
 * @param    prefix    Text to put in front of dumped data
 * @param    link      Kernel virtual address of start of chain of links
 *
 * @return   void
 */
static void km_Dump_Link(const char *prefix, const sah_Link * link)
{
	while (link != NULL) {
		km_Dump_Words(prefix, (unsigned *)link,
			      3 /* # words in h/w link */ );
		if (link->flags & SAH_STORED_KEY_INFO) {
#ifdef CAN_DUMP_SCC_DATA
			uint32_t len;
#endif

#ifdef CAN_DUMP_SCC_DATA
			{
				char buf[50];

				scc_get_slot_info(link->ownerid, link->slot, (uint32_t *) & link->data,	/* RED key address */
						  &len);	/* key length */
				sprintf(buf, "  SCC slot %d: ", link->slot);
				km_Dump_Words(buf,
					      (void *)IO_ADDRESS((uint32_t)
								 link->data),
					      link->len / 4);
			}
#else
			sprintf(Diag_msg, "  SCC slot %d", link->slot);
			LOG_KDIAG(Diag_msg);
#endif
		} else if (link->data != NULL) {
			km_Dump_Region("  Data", link->data, link->len);
		}

		link = link->next;
	}
}

/*!
 * Dump given region of data to the log.
 *
 * @brief Dump data
 *
 * @param    prefix    Text to put in front of dumped data
 * @param    data      Kernel virtual address of start of region to dump
 * @param    length    Amount of data to dump
 *
 * @return   void
*/
void km_Dump_Region(const char *prefix, const unsigned char *data,
		    unsigned length)
{
	unsigned count;
	char *output;
	unsigned data_len;

	sprintf(Diag_msg, "%s (%08X,%u):", prefix, (uint32_t) data, length);

	/* Restrict amount of data to dump */
	if (length > MAX_DUMP) {
		data_len = MAX_DUMP;
	} else {
		data_len = length;
	}

	/* We've already printed some text in output buffer, skip over it */
	output = Diag_msg + strlen(Diag_msg);

	for (count = 0; count < data_len; count++) {
		if (count % 4 == 0) {
			*output++ = ' ';
		}
		sprintf(output, "%02X", *data++);
		output += 2;
	}

	LOG_KDIAG(Diag_msg);
}

/*!
 * Dump given wors of data to the log.
 *
 * @brief Dump data
 *
 * @param    prefix       Text to put in front of dumped data
 * @param    data         Kernel virtual address of start of region to dump
 * @param    word_count   Amount of data to dump
 *
 * @return   void
*/
void km_Dump_Words(const char *prefix, const unsigned *data,
		   unsigned word_count)
{
	char *output;

	sprintf(Diag_msg, "%s (%08X,%uw): ", prefix, (uint32_t) data,
		word_count);

	/* We've already printed some text in output buffer, skip over it */
	output = Diag_msg + strlen(Diag_msg);

	while (word_count--) {
		sprintf(output, "%08X ", *data++);
		output += 9;
	}

	LOG_KDIAG(Diag_msg);
}
#endif
