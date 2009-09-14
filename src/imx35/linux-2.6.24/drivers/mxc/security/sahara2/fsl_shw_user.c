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
 * @file fsl_shw_user.c
 *
 * This file implements user and platform capabilities functions of the FSL SHW
 * API for Sahara
 */
#include "sahara.h"
#include <adaptor.h>
#include <sf_util.h>

#ifdef __KERNEL__
EXPORT_SYMBOL(fsl_shw_get_capabilities);
EXPORT_SYMBOL(fsl_shw_register_user);
EXPORT_SYMBOL(fsl_shw_deregister_user);
EXPORT_SYMBOL(fsl_shw_get_results);
#endif				/* __KERNEL__ */

/*! This matches Sahara2 capabilities... */
fsl_shw_pco_t sahara2_capabilities = {
	1, 1,			/* api version number - major & minor */
	1, 2,			/* driver version number - major & minor */
	{
	 FSL_KEY_ALG_AES,
	 FSL_KEY_ALG_DES,
	 FSL_KEY_ALG_TDES,
	 FSL_KEY_ALG_ARC4}
	,
	{
	 FSL_SYM_MODE_STREAM,
	 FSL_SYM_MODE_ECB,
	 FSL_SYM_MODE_CBC,
	 FSL_SYM_MODE_CTR}
	,
	{
	 FSL_HASH_ALG_MD5,
	 FSL_HASH_ALG_SHA1,
	 FSL_HASH_ALG_SHA224,
	 FSL_HASH_ALG_SHA256}
	,
	/*
	 * The following table must be set to handle all values of key algorithm
	 * and sym mode, and be in the correct order..
	 */
	{			/* Stream, ECB, CBC, CTR */
	 {0, 0, 0, 0}
	 ,			/* HMAC */
	 {0, 1, 1, 1}
	 ,			/* AES  */
	 {0, 1, 1, 0}
	 ,			/* DES */
	 {0, 1, 1, 0}
	 ,			/* 3DES */
	 {1, 0, 0, 0}		/* ARC4 */
	 }
};

/* REQ-S2LRD-PINTFC-API-GEN-003 */
/*!
 * Determine the hardware security capabilities of this platform.
 *
 * Though a user context object is passed into this function, it will always
 * act in a non-blocking manner.
 *
 * @param  user_ctx   The user context which will be used for the query.
 *
 * @return  A pointer to the capabilities object.
 */
fsl_shw_pco_t *fsl_shw_get_capabilities(fsl_shw_uco_t * user_ctx)
{
	/*
	 * Need to get the driver/hardware versions populated...
	 * which is why the user_ctx is here.
	 */
	user_ctx = 0;
	return &sahara2_capabilities;
}

/* REQ-S2LRD-PINTFC-API-GEN-004 */

/*!
 * Create an association between the the user and the provider of the API.
 *
 * @param  user_ctx   The user context which will be used for this association.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t fsl_shw_register_user(fsl_shw_uco_t * user_ctx)
{
	return sah_register(user_ctx);
}

/* REQ-S2LRD-PINTFC-API-GEN-005 */

/*!
 * Destroy the association between the the user and the provider of the API.
 *
 * @param  user_ctx   The user context which is no longer needed.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t fsl_shw_deregister_user(fsl_shw_uco_t * user_ctx)
{
	return sah_deregister(user_ctx);
}

/* REQ-S2LRD-PINTFC-API-GEN-006 */

/*!
 * Retrieve results from earlier operations.
 *
 * @param         user_ctx     The user's context.
 * @param         result_size  The number of array elements of @a results.
 * @param[in,out] results      Pointer to first of the (array of) locations to
 *                             store results.
 * @param[out]    result_count Pointer to store the number of results which
 *                             were returned.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t fsl_shw_get_results(fsl_shw_uco_t * user_ctx,
				     unsigned result_size,
				     fsl_shw_result_t results[],
				     unsigned *result_count)
{
	fsl_shw_return_t status;

	/* perform a sanity check on the uco */
	status = sah_validate_uco(user_ctx);

	/* if uco appears ok, build structure and pass to get results */
	if (status == FSL_RETURN_OK_S) {
		sah_results arg;

		/* if requested is zero, it's done before it started */
		if (result_size > 0) {
			arg.requested = result_size;
			arg.actual = result_count;
			arg.results = results;
			/* get the results */
			status = sah_get_results(&arg, user_ctx);
		}
	}

	return status;
}
