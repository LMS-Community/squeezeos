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
 * @file fsl_shw_auth.c
 *
 * This file contains the routines which do the combined encrypt+authentication
 * functions.  For now, only AES-CCM is supported.
 */

#include "sahara.h"
#include "adaptor.h"
#include "sf_util.h"

#ifdef __KERNEL__
EXPORT_SYMBOL(fsl_shw_gen_encrypt);
EXPORT_SYMBOL(fsl_shw_auth_decrypt);
#endif

/* Remove warning */
#define SUPPORT_SSL     0


/*! Size of buffer to repetively sink useless CBC output */
#define CBC_BUF_LEN 4096

/*!
 * Compute the size, in bytes, of the encoded auth length
 *
 * @param l    The actual auth length
 *
 * @return The encoded length
 */
#define COMPUTE_NIST_AUTH_LEN_SIZE(l)                                         \
({                                                                            \
    unsigned val;                                                             \
    if ((uint32_t)(l) < 65280) {                                              \
        val = 2;                                                              \
    } else {                    /* cannot handle >= 2^32 */                   \
        val = 6;                                                              \
    }                                                                         \
    val;                                                                      \
})

/*!
 * Store the encoded Auth Length into the Auth Data
 *
 * @param l    The actual Auth Length
 * @param p    Location to store encoding (must be uint8_t*)
 *
 * @return void
 */
#define STORE_NIST_AUTH_LEN(l, p)                                             \
{                                                                             \
    register uint32_t L = l;                                                  \
    if ((uint32_t)(l) < 65280) {                                              \
        (p)[1] = L & 0xff;                                                    \
        L >>= 8;                                                              \
        (p)[0] = L & 0xff;                                                    \
    } else {                    /* cannot handle >= 2^32 */                   \
        int i;                                                                \
        for (i = 5; i > 1; i--) {                                             \
            (p)[i] = L & 0xff;                                                \
            L >>= 8;                                                          \
        }                                                                     \
        (p)[1] = 0xfe;  /* Markers */                                         \
        (p)[0] = 0xff;                                                        \
    }                                                                         \
}

/*! Buffer to repetively sink useless CBC output */
static uint8_t cbc_buffer[CBC_BUF_LEN];

/*!
 * Place to store useless output (while bumping CTR0 to CTR1, for instance.
 * Must be maximum Symmetric block size
 */
static uint8_t garbage_output[16];

/*!
 * Block of zeroes which is maximum Symmetric block size, used for
 * initializing context register, etc.
 */
static uint8_t block_zeros[16] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};

/*!
 * Append a descriptor which will load the key and counter values into
 * Sahara.
 *
 * @param[in,out] desc_chain  Where to append the new descriptor
 * @param         user_ctx    Info for acquiring memory
 * @param         auth_ctx    Location of CTR value
 * @param         key_info    Location of the key
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
static inline fsl_shw_return_t load_ctr_key(sah_Head_Desc ** desc_chain,
					    fsl_shw_uco_t * user_ctx,
					    fsl_shw_acco_t * auth_ctx,
					    fsl_shw_sko_t * key_info)
{
	fsl_shw_return_t status;

	/* Assume AES */
	uint32_t header = SAH_HDR_SKHA_SET_MODE_IV_KEY
	    ^ sah_insert_skha_encrypt
	    ^ sah_insert_skha_mode_ctr ^ sah_insert_skha_modulus_128;

	/* Assume CCM-AES for now, since that is all that is supported */
	status = sah_add_in_key_desc(header,
				     auth_ctx->cipher_ctx_info.context,
				     auth_ctx->cipher_ctx_info.block_size_bytes,
				     key_info, user_ctx->mem_util, desc_chain);
	return status;
}

/*!
 * Append a descriptor which will load zeros into the CBC CTX registers.
 * It also sets the mode to CBC.
 *
 * @param[in,out] desc_chain  Where to append the new descriptor
 * @param         user_ctx    Info for acquiring memory
 * @param         encrypt     0 for decrypt, non-zero for encrypt
 * @param         size        Octet count of @a in and @a out
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
static inline fsl_shw_return_t load_dummy_iv(sah_Head_Desc ** desc_chain,
					     fsl_shw_uco_t * user_ctx,
					     int encrypt, uint32_t size)
{
	fsl_shw_return_t status;
	/* Desc. #1. in CBC mode.  Assume /AES. */
	uint32_t header = SAH_HDR_SKHA_SET_MODE_IV_KEY
	    ^ sah_insert_skha_mode_cbc;

	if (encrypt) {
		header ^= sah_insert_skha_encrypt;
	}

	status = sah_add_two_in_desc(header, block_zeros, size, NULL, 0,
				     user_ctx->mem_util, desc_chain);

	return status;
}

/*!
 * Append a descriptor chain which will compute CBC over the
 * formatted associated data blocks.
 *
 * @param[in,out] link1       Where to append the new link
 * @param[in,out] data_len    Location of current/updated auth-only data length
 * @param         user_ctx    Info for acquiring memory
 * @param         auth_ctx    Location of block0 value
 * @param         auth_data   Unformatted associated data
 * @param         auth_data_length Length in octets of @a auth_data
 * @param[in,out] temp_buf    Location of in-process data.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
static inline fsl_shw_return_t process_assoc_from_nist_params(sah_Link ** link1,
							      uint32_t *
							      data_len,
							      fsl_shw_uco_t *
							      user_ctx,
							      fsl_shw_acco_t *
							      auth_ctx,
							      const uint8_t *
							      auth_data,
							      uint32_t
							      auth_data_length,
							      uint8_t **
							      temp_buf)
{
	fsl_shw_return_t status;
	uint32_t auth_size_length =
	    COMPUTE_NIST_AUTH_LEN_SIZE(auth_data_length);
	uint32_t auth_pad_length =
	    auth_ctx->auth_info.CCM_ctx_info.block_size_bytes -
	    (auth_data_length +
	     auth_size_length) %
	    auth_ctx->auth_info.CCM_ctx_info.block_size_bytes;

	if (auth_pad_length ==
	    auth_ctx->auth_info.CCM_ctx_info.block_size_bytes) {
		auth_pad_length = 0;
	}

	/* Put in Block0 */
	status = sah_Create_Link(user_ctx->mem_util, link1,
				 auth_ctx->auth_info.CCM_ctx_info.context,
				 auth_ctx->auth_info.CCM_ctx_info.
				 block_size_bytes, SAH_USES_LINK_DATA);

	if (status == FSL_RETURN_OK_S) {
		/* Add on length preamble to auth data */
		STORE_NIST_AUTH_LEN(auth_data_length, *temp_buf);
		status = sah_Append_Link(user_ctx->mem_util, *link1,
					 *temp_buf, auth_size_length,
					 SAH_OWNS_LINK_DATA);
		*temp_buf += auth_size_length;	/* 2, 6, or 10 bytes */
	}

	if (status == FSL_RETURN_OK_S) {
		/* Add in auth data */
		status = sah_Append_Link(user_ctx->mem_util, *link1,
					 (uint8_t *) auth_data,
					 auth_data_length, SAH_USES_LINK_DATA);
	}

	if ((status == FSL_RETURN_OK_S) && (auth_pad_length > 0)) {
		status = sah_Append_Link(user_ctx->mem_util, *link1,
					 block_zeros, auth_pad_length,
					 SAH_USES_LINK_DATA);
	}

	*data_len = auth_ctx->auth_info.CCM_ctx_info.block_size_bytes +
	    auth_data_length + auth_size_length + auth_pad_length;

	return status;
}

/*!
 * Append a descriptor chain which will process the payload in
 * CCM mode.
 *
 * @param[in,out] desc_chain  Where to append the new descriptor
 * @param         user_ctx    Info for acquiring memory
 * @param         encrypt     0 for decrypt, non-zero for encrypt
 * @param         size        Length in octets of @a input and @a output
 * @param         input       Location of source data
 * @param[out]    output      Location to store output data
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
static inline fsl_shw_return_t process_payload(sah_Head_Desc ** desc_chain,
					       fsl_shw_uco_t * user_ctx,
					       int encrypt,
					       uint32_t size,
					       const uint8_t * input,
					       uint8_t * output)
{
	fsl_shw_return_t status;
	/* Assume AES */
	uint32_t header = SAH_HDR_SKHA_SET_MODE_ENC_DEC
	    ^ sah_insert_skha_mode_ccm ^ sah_insert_skha_modulus_128;

	if (encrypt) {
		header ^= sah_insert_skha_encrypt;
	}

	status = sah_add_in_out_desc(header, input, size, output, size,
				     user_ctx->mem_util, desc_chain);

	return status;
}

/*!
 * Add a Descriptor which will process with CBC the NIST preamble data
 *
 * @param     desc_chain   Current chain
 * @param     user_ctx     User's context
 * @param     auth_ctx     Inf
 * @param     auth_data    Additional auth data for this call
 * @param     auth_data_length   Length in bytes of @a auth_data
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
static inline fsl_shw_return_t add_assoc_preamble(sah_Head_Desc ** desc_chain,
						  fsl_shw_uco_t * user_ctx,
						  fsl_shw_acco_t * auth_ctx,
						  const uint8_t * auth_data,
						  uint32_t auth_data_length)
{
	uint8_t *temp_buf;
	sah_Link *link1 = NULL;
	sah_Link *link2 = NULL;
	fsl_shw_return_t status = FSL_RETURN_OK_S;
	uint32_t cbc_data_length = 0;
	/* Assume AES */
	uint32_t header = SAH_HDR_SKHA_ENC_DEC;

	/* Grab a block big enough for multiple uses so that only one allocate
	 * request needs to be made.
	 */
	temp_buf =
	    user_ctx->mem_util->mu_malloc(user_ctx->mem_util->mu_ref,
					  3 *
					  auth_ctx->auth_info.CCM_ctx_info.
					  block_size_bytes);

	if (temp_buf == NULL) {
		status = FSL_RETURN_NO_RESOURCE_S;
	} else {
		uint32_t temp_buf_flag;

		if (auth_ctx->flags & FSL_ACCO_NIST_CCM) {
			status = process_assoc_from_nist_params(&link1,
								&cbc_data_length,
								user_ctx,
								auth_ctx,
								auth_data,
								auth_data_length,
								&temp_buf);
			/* temp_buf has been referenced (and incremented).  Only 'own' it
			 * once, at its first value.  Since the nist routine called above
			 * bumps it...
			 */
			temp_buf_flag = SAH_USES_LINK_DATA;
		} else {	/* if NIST */
			if (status == FSL_RETURN_OK_S) {
				status =
				    sah_Create_Link(user_ctx->mem_util, &link1,
						    (uint8_t *) auth_data,
						    auth_data_length,
						    SAH_USES_LINK_DATA);
				/* for next/first use of temp_buf */
				temp_buf_flag = SAH_OWNS_LINK_DATA;
			}
			cbc_data_length = auth_data_length;
		}		/* else not NIST */

		/*
		 * Auth data links have been created.  Now create link for the
		 * useless output of the CBC calculation.
		 */
		if (status == FSL_RETURN_OK_S) {
			status = sah_Create_Link(user_ctx->mem_util, &link2,
						 temp_buf,
						 auth_ctx->auth_info.
						 CCM_ctx_info.block_size_bytes,
						 temp_buf_flag |
						 SAH_OUTPUT_LINK);
		}
		temp_buf += auth_ctx->auth_info.CCM_ctx_info.block_size_bytes;

		cbc_data_length -=
		    auth_ctx->auth_info.CCM_ctx_info.block_size_bytes;

		if (cbc_data_length != 0) {
			while ((status == FSL_RETURN_OK_S)
			       && (cbc_data_length != 0)) {
				uint32_t linklen =
				    (cbc_data_length >
				     CBC_BUF_LEN) ? CBC_BUF_LEN :
				    cbc_data_length;

				status =
				    sah_Append_Link(user_ctx->mem_util, link2,
						    cbc_buffer, linklen,
						    SAH_USES_LINK_DATA |
						    SAH_OUTPUT_LINK);
				cbc_data_length -= linklen;
			}
		}

		if (status != FSL_RETURN_OK_S) {
			if (link1 != NULL) {
				sah_Destroy_Link(user_ctx->mem_util, link1);
			}
			if (link2 != NULL) {
				sah_Destroy_Link(user_ctx->mem_util, link2);
			}
		} else {
			/* Header to set up crank through auth data */
			status = sah_Append_Desc(user_ctx->mem_util, desc_chain,
						 header, link1, link2);
		}
	}

	return status;
}

/*!
 * Append a descriptor chain which will pull the MAC (CBC IV) out of the
 * hardware registers.
 *
 * @param[in,out] desc_chain  Where to append the new descriptor
 * @param         user_ctx    Info for acquiring memory
 * @param         size        Length in octets of desired MAC
 * @param[out]    mac         Location to store the MAC
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
static inline fsl_shw_return_t extract_mac(sah_Head_Desc ** desc_chain,
					   fsl_shw_uco_t * user_ctx,
					   uint32_t size, uint8_t * mac)
{
	fsl_shw_return_t status;
	uint32_t header = SAH_HDR_SKHA_READ_CONTEXT_IV;

	/* Pull MAC out of IV register */
	status = sah_add_in_out_desc(header, NULL, 0, mac, size,
				     user_ctx->mem_util, desc_chain);

	return status;
}

/*!
 * Append a descriptor chain which will (encrypt) the MAC with CTR0.
 * Since CTR mode works both ways, the input could be an encrypted
 * MAC with the output being the decrypted version.
 *
 * @param[in,out] desc_chain  Where to append the new descriptor
 * @param         user_ctx    Info for acquiring memory
 * @param         auth_ctx    Info for CTR0, size of MAC
 * @param         input       Location of MAC
 * @param         output      Location to store (encrypted) MAC.
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
static inline fsl_shw_return_t encrypt_mac(sah_Head_Desc ** desc_chain,
					   fsl_shw_uco_t * user_ctx,
					   fsl_shw_acco_t * auth_ctx,
					   const uint8_t * input,
					   uint8_t * output)
{
	fsl_shw_return_t status;
	/* Assuming AES here... */
	uint32_t header = SAH_HDR_SKHA_SET_MODE_IV_KEY
	    ^ sah_insert_skha_encrypt
	    ^ sah_insert_skha_mode_ctr ^ sah_insert_skha_modulus_128;

	/* Load CTR0 back in */
	status = sah_add_two_in_desc(header,
				     auth_ctx->cipher_ctx_info.context,
				     auth_ctx->cipher_ctx_info.block_size_bytes,
				     NULL, 0, user_ctx->mem_util, desc_chain);

	if (status == FSL_RETURN_OK_S) {
		/* and encrypt the input as the auth value */
		header = SAH_HDR_SKHA_ENC_DEC;	/* Desc. #4 SKHA Enc/Dec */

		status = sah_add_in_out_desc(header,
					     input, auth_ctx->mac_length,
					     output, auth_ctx->mac_length,
					     user_ctx->mem_util, desc_chain);
	}

	return status;
}

#if SUPPORT_SSL
/*!
 * Generate an SSL value
 *
 * @param         user_ctx         Info for acquiring memory
 * @param         auth_ctx         Info for CTR0, size of MAC
 * @param         cipher_key_info
 * @param         auth_key_info
 * @param         auth_data_length
 * @param         auth_data
 * @param         payload_length
 * @param         payload
 * @param         ct
 * @param         auth_value
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
static fsl_shw_return_t do_ssl_gen(fsl_shw_uco_t * user_ctx,
				   fsl_shw_acco_t * auth_ctx,
				   fsl_shw_sko_t * cipher_key_info,
				   fsl_shw_sko_t * auth_key_info,
				   uint32_t auth_data_length,
				   const uint8_t * auth_data,
				   uint32_t payload_length,
				   const uint8_t * payload,
				   uint8_t * ct, uint8_t * auth_value)
{
	SAH_SF_DCLS;
	uint8_t *ptr1 = NULL;

	/* Assume one-shot init-finalize... no precomputes */
	header = SAH_HDR_MDHA_SET_MODE_MD_KEY ^
	    sah_insert_mdha_algorithm[auth_ctx->auth_info.hash_ctx_info.
				      algorithm] ^ sah_insert_mdha_init ^
	    sah_insert_mdha_ssl ^ sah_insert_mdha_pdata ^
	    sah_insert_mdha_mac_full;

	/* set up hmac */
	DESC_IN_KEY(header, 0, NULL, auth_key_info);

	/* This is wrong -- need to find 16 extra bytes of data from
	 * somewhere */
	DESC_IN_OUT(SAH_HDR_MDHA_HASH, payload_length, payload, 1, auth_value);

	/* set up encrypt */
	header = SAH_HDR_SKHA_SET_MODE_IV_KEY
	    ^ sah_insert_skha_mode[auth_ctx->cipher_ctx_info.mode]
	    ^ sah_insert_skha_encrypt
	    ^ sah_insert_skha_algorithm[cipher_key_info->algorithm];

	/* Honor 'no key parity checking' for DES and TDES */
	if ((cipher_key_info->flags & FSL_SKO_KEY_IGNORE_PARITY) &&
	    ((cipher_key_info->algorithm == FSL_KEY_ALG_DES) ||
	     (cipher_key_info->algorithm == FSL_KEY_ALG_TDES))) {
		header ^= sah_insert_skha_no_key_parity;
	}

	if (auth_ctx->cipher_ctx_info.mode == FSL_SYM_MODE_CTR) {
		header ^=
		    sah_insert_skha_modulus[auth_ctx->cipher_ctx_info.
					    modulus_exp];
	}

	if ((auth_ctx->cipher_ctx_info.mode == FSL_SYM_MODE_ECB)
	    || (auth_ctx->cipher_ctx_info.flags & FSL_SYM_CTX_INIT)) {
		ptr1 = block_zeros;
	} else {
		ptr1 = auth_ctx->cipher_ctx_info.context;
	}

	DESC_IN_KEY(header, auth_ctx->cipher_ctx_info.block_size_bytes, ptr1,
		    cipher_key_info);

	/* This is wrong -- need to find 16 extra bytes of data from
	 * somewhere...
	 */
	if (payload_length != 0) {
		DESC_IN_OUT(SAH_HDR_SKHA_ENC_DEC,
			    payload_length, payload, payload_length, ct);
	}

	SAH_SF_EXECUTE();

      out:
	SAH_SF_DESC_CLEAN();

	/* Eliminate compiler warnings until full implementation... */
	(void)auth_data;
	(void)auth_data_length;

	return ret;
}				/* do_ssl_gen() */
#endif

/*!
 * @brief Generate a (CCM) auth code and encrypt the payload.
 *
 * This is a very complicated function.  Seven (or eight) descriptors are
 * required to perform a CCM calculation.
 *
 * First:  Load CTR0 and key.
 *
 * Second: Run an octet of data through to bump to CTR1.  (This could be
 * done in software, but software will have to bump and later decrement -
 * or copy and bump.
 *
 * Third: (in Virtio) Load a descriptor with data of zeros for CBC IV.
 *
 * Fourth: Run any (optional) "additional data" through the CBC-mode
 * portion of the algorithm.
 *
 * Fifth: Run the payload through in CCM mode.
 *
 * Sixth: Extract the unencrypted MAC.
 *
 * Seventh: Load CTR0.
 *
 * Eighth: Encrypt the MAC.
 *
 * @param user_ctx         The user's context
 * @param auth_ctx         Info on this Auth operation
 * @param cipher_key_info  Key to encrypt payload
 * @param auth_key_info    (unused - same key in CCM)
 * @param auth_data_length Length in bytes of @a auth_data
 * @param auth_data        Any auth-only data
 * @param payload_length   Length in bytes of @a payload
 * @param payload          The data to encrypt
 * @param[out] ct          The location to store encrypted data
 * @param[out] auth_value  The location to store authentication code
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t fsl_shw_gen_encrypt(fsl_shw_uco_t * user_ctx,
				     fsl_shw_acco_t * auth_ctx,
				     fsl_shw_sko_t * cipher_key_info,
				     fsl_shw_sko_t * auth_key_info,
				     uint32_t auth_data_length,
				     const uint8_t * auth_data,
				     uint32_t payload_length,
				     const uint8_t * payload,
				     uint8_t * ct, uint8_t * auth_value)
{
	SAH_SF_DCLS;

	SAH_SF_USER_CHECK();

	if (auth_ctx->mode == FSL_ACC_MODE_SSL) {
#if SUPPORT_SSL
		ret = do_ssl_gen(user_ctx, auth_ctx, cipher_key_info,
				 auth_key_info, auth_data_length, auth_data,
				 payload_length, payload, ct, auth_value);
#else
		ret = FSL_RETURN_BAD_MODE_S;
#endif
		return ret;
	}

	if (auth_ctx->mode != FSL_ACC_MODE_CCM) {
		ret = FSL_RETURN_BAD_MODE_S;
		goto out;
	}

	/* Only support INIT and FINALIZE flags right now. */
	if ((auth_ctx->flags & (FSL_ACCO_CTX_INIT | FSL_ACCO_CTX_LOAD |
				FSL_ACCO_CTX_SAVE | FSL_ACCO_CTX_FINALIZE))
	    != (FSL_ACCO_CTX_INIT | FSL_ACCO_CTX_FINALIZE)) {
		ret = FSL_RETURN_BAD_FLAG_S;
		goto out;
	}
	ret = load_ctr_key(&desc_chain, user_ctx, auth_ctx, cipher_key_info);
	if (ret != FSL_RETURN_OK_S) {
		goto out;
	}

	header = SAH_HDR_SKHA_ENC_DEC;
	DESC_IN_OUT(header, auth_ctx->cipher_ctx_info.block_size_bytes,
		    garbage_output, auth_ctx->cipher_ctx_info.block_size_bytes,
		    garbage_output);

#ifndef NO_ZERO_IV_LOAD
	ret = load_dummy_iv(&desc_chain, user_ctx,
			    1,
			    auth_ctx->auth_info.CCM_ctx_info.block_size_bytes);
	if (ret != FSL_RETURN_OK_S) {
		goto out;
	}
#endif

	if (auth_data_length > 0) {
		ret = add_assoc_preamble(&desc_chain, user_ctx,
					 auth_ctx, auth_data, auth_data_length);
		if (ret != FSL_RETURN_OK_S) {
			goto out;
		}
	}
	/* if auth_data_length > 0 */
	ret = process_payload(&desc_chain, user_ctx, 1,
			      payload_length, payload, ct);

	if (ret != FSL_RETURN_OK_S) {
		goto out;
	}

	/* Pull out the CBC-MAC value. */
	ret = extract_mac(&desc_chain, user_ctx,
			  auth_ctx->mac_length, auth_ctx->unencrypted_mac);
	if (ret != FSL_RETURN_OK_S) {
		goto out;
	}

	/* Now load CTR0 in, and encrypt the MAC */
	ret = encrypt_mac(&desc_chain, user_ctx, auth_ctx,
			  auth_ctx->unencrypted_mac, auth_value);
	if (ret != FSL_RETURN_OK_S) {
		goto out;
	}

	SAH_SF_EXECUTE();

      out:
	SAH_SF_DESC_CLEAN();

	(void)auth_key_info;
	return ret;
}				/* fsl_shw_gen_encrypt() */

/*!
 * @brief Authenticate and decrypt a (CCM) stream.
 *
 * @param user_ctx         The user's context
 * @param auth_ctx         Info on this Auth operation
 * @param cipher_key_info  Key to encrypt payload
 * @param auth_key_info    (unused - same key in CCM)
 * @param auth_data_length Length in bytes of @a auth_data
 * @param auth_data        Any auth-only data
 * @param payload_length   Length in bytes of @a payload
 * @param ct               The encrypted data
 * @param auth_value       The authentication code to validate
 * @param[out] payload     The location to store decrypted data
 *
 * @return    A return code of type #fsl_shw_return_t.
 */
fsl_shw_return_t fsl_shw_auth_decrypt(fsl_shw_uco_t * user_ctx,
				      fsl_shw_acco_t * auth_ctx,
				      fsl_shw_sko_t * cipher_key_info,
				      fsl_shw_sko_t * auth_key_info,
				      uint32_t auth_data_length,
				      const uint8_t * auth_data,
				      uint32_t payload_length,
				      const uint8_t * ct,
				      const uint8_t * auth_value,
				      uint8_t * payload)
{
	SAH_SF_DCLS;
	uint8_t *calced_auth = NULL;
	unsigned blocking = user_ctx->flags & FSL_UCO_BLOCKING_MODE;

	SAH_SF_USER_CHECK();

	/* Only support INIT and FINALIZE flags right now. */
	if (auth_ctx->mode != FSL_ACC_MODE_CCM) {
		ret = FSL_RETURN_BAD_MODE_S;
		goto out;
	}
	if ((auth_ctx->flags & (FSL_ACCO_CTX_INIT | FSL_ACCO_CTX_LOAD |
				FSL_ACCO_CTX_SAVE | FSL_ACCO_CTX_FINALIZE))
	    != (FSL_ACCO_CTX_INIT | FSL_ACCO_CTX_FINALIZE)) {
		ret = FSL_RETURN_BAD_FLAG_S;
		goto out;
	}
	ret = load_ctr_key(&desc_chain, user_ctx, auth_ctx, cipher_key_info);
	if (ret != FSL_RETURN_OK_S) {
		goto out;
	}

	/* Decrypt the MAC which the user passed in */
	header = SAH_HDR_SKHA_ENC_DEC;
	DESC_IN_OUT(header,
		    auth_ctx->mac_length, auth_value,
		    auth_ctx->mac_length, auth_ctx->unencrypted_mac);

#ifndef NO_ZERO_IV_LOAD
	ret = load_dummy_iv(&desc_chain, user_ctx, 1,
			    auth_ctx->auth_info.CCM_ctx_info.block_size_bytes);
#endif

	if (auth_data_length > 0) {
		ret = add_assoc_preamble(&desc_chain, user_ctx,
					 auth_ctx, auth_data, auth_data_length);
		if (ret != FSL_RETURN_OK_S) {
			goto out;
		}
	}
	/* if auth_data_length > 0 */
	ret = process_payload(&desc_chain, user_ctx, 0,
			      payload_length, ct, payload);
	if (ret != FSL_RETURN_OK_S) {
		goto out;
	}

	/* Now pull CBC context (unencrypted MAC) out for comparison. */
	/* Need to allocate a place for it, to handle non-blocking mode
	 * when this stack frame will disappear!
	 */
	calced_auth = DESC_TEMP_ALLOC(auth_ctx->mac_length);
	ret = extract_mac(&desc_chain, user_ctx,
			  auth_ctx->mac_length, calced_auth);
	if (ret != FSL_RETURN_OK_S) {
		goto out;
	}

	if (!blocking) {
		/* get_results will need this for comparison */
		desc_chain->out1_ptr = calced_auth;
		desc_chain->out2_ptr = auth_ctx->unencrypted_mac;
		desc_chain->out_len = auth_ctx->mac_length;
	}

	SAH_SF_EXECUTE();

	if (blocking && (ret == FSL_RETURN_OK_S)) {
		unsigned i;
		/* Validate the auth code */
		for (i = 0; i < auth_ctx->mac_length; i++) {
			if (calced_auth[i] != auth_ctx->unencrypted_mac[i]) {
				ret = FSL_RETURN_AUTH_FAILED_S;
				break;
			}
		}
	}

      out:
	SAH_SF_DESC_CLEAN();
	DESC_TEMP_FREE(calced_auth);

	(void)auth_key_info;
	return ret;
}				/* fsl_shw_gen_decrypt() */
