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
* @file adaptor.h
*
* @brief The Adaptor component provides an interface to the device
* driver.
*
* Intended to be used by the FSL SHW API, this can also be called directly
*/

#ifndef ADAPTOR_H
#define ADAPTOR_H

#include <sahara.h>

/*!
 * Structure passed during user ioctl() call to submit request.
 */
typedef struct sah_dar {
	sah_Desc *desc_addr;	/*!< head of descriptor chain */
	uint32_t uco_flags;	/*!< copy of fsl_shw_uco flags field */
	uint32_t uco_user_ref;	/*!< copy of fsl_shw_uco user_ref */
	uint32_t result;	/*!< result of descriptor chain request */
	struct sah_dar *next;	/*!< for driver use */
} sah_dar_t;

/*!
 * Structure passed during user ioctl() call to Register a user
 */
typedef struct sah_register {
	uint32_t pool_size;	/*!< max number of outstanding requests possible */
	uint32_t result;	/*!< result of registration request */
} sah_register_t;

/*!
 * Structure passed during ioctl() call to request SCC operation
 */
typedef struct scc_data {
	uint32_t length;	/*!< length of data */
	uint8_t *in;		/*!< input data */
	uint8_t *out;		/*!< output data */
	unsigned direction;	/*!< encrypt or decrypt */
	fsl_shw_sym_mode_t crypto_mode;	/*!< CBC or EBC */
	uint8_t *init_vector;	/*!< initialization vector or NULL */
} scc_data_t;

/*!
 * Structure passed during user ioctl() calls to manage stored keys and
 * stored-key slots.
 */
typedef struct scc_slot_info {
	uint64_t ownerid;	/*!< Owner's id to check/set permissions */
	uint32_t key_length;	/*!< Length of key */
	uint32_t slot;		/*!< Slot to operation on, or returned slot
				   number. */
	uint8_t *key;		/*!< User-memory pointer to key value */
	fsl_shw_return_t code;	/*!< API return code from operation */
} scc_slot_t;

fsl_shw_return_t adaptor_Exec_Descriptor_Chain(sah_Head_Desc * dar,
					       fsl_shw_uco_t * uco);
fsl_shw_return_t sah_get_results(sah_results * arg, fsl_shw_uco_t * uco);
fsl_shw_return_t sah_register(fsl_shw_uco_t * user_ctx);
fsl_shw_return_t sah_deregister(fsl_shw_uco_t * user_ctx);
fsl_shw_return_t do_scc_slot_alloc(fsl_shw_uco_t * user_ctx, uint32_t key_len,
				   uint64_t ownerid, uint32_t * slot);
fsl_shw_return_t do_scc_slot_dealloc(fsl_shw_uco_t * user_ctx, uint64_t ownerid,
				     uint32_t slot);
fsl_shw_return_t do_scc_slot_load_slot(fsl_shw_uco_t * user_ctx,
				       uint64_t ownerid, uint32_t slot,
				       const uint8_t * key,
				       uint32_t key_length);
fsl_shw_return_t do_scc_slot_encrypt(fsl_shw_uco_t * user_ctx, uint64_t ownerid,
				     uint32_t slot, uint32_t key_length,
				     uint8_t * black_data);

fsl_shw_return_t do_scc_slot_decrypt(fsl_shw_uco_t * user_ctx, uint64_t ownerid,
				     uint32_t slot, uint32_t key_length,
				     const uint8_t * black_data);

#endif				/* ADAPTOR_H */

/* End of adaptor.h */
