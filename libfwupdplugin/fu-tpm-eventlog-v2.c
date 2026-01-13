/*
 * Copyright 2019 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#define G_LOG_DOMAIN "FuTpmEventlog"

#include "config.h"

#include "fu-tpm-eventlog-item.h"
#include "fu-tpm-eventlog-v2.h"
#include "fu-tpm-struct.h"

// FIXME: remove and define in .rs file?
#include <tss2/tss2_esys.h>

/**
 * FuTpmEventlogV2:
 *
 * Parse the TPM eventlog.
 */

struct _FuTpmEventlogV2 {
	FuFirmware parent_instance;
};

G_DEFINE_TYPE(FuTpmEventlogV2, fu_tpm_eventlog_v2, FU_TYPE_TPM_EVENTLOG)

static guint32
fu_tpm_eventlog_v2_hash_get_size(TPM2_ALG_ID hash_kind)
{
	if (hash_kind == TPM2_ALG_SHA1)
		return TPM2_SHA1_DIGEST_SIZE;
	if (hash_kind == TPM2_ALG_SHA256)
		return TPM2_SHA256_DIGEST_SIZE;
	if (hash_kind == TPM2_ALG_SHA384)
		return TPM2_SHA384_DIGEST_SIZE;
	if (hash_kind == TPM2_ALG_SHA512)
		return TPM2_SHA512_DIGEST_SIZE;
	if (hash_kind == TPM2_ALG_SM3_256)
		return TPM2_SM3_256_DIGEST_SIZE;
	return 0;
}

static gboolean
fu_tpm_eventlog_v2_parse(FuFirmware *firmware,
			 GInputStream *stream,
			 FuFirmwareParseFlags flags,
			 GError **error)
{
	guint32 hdrsz = 0x0;
	gsize streamsz = 0;
	g_autoptr(FuStructTpmEventLog2Hdr) st_hdr = NULL;

	/* look for TCG v2 signature */
	st_hdr = fu_struct_tpm_event_log2_hdr_parse_stream(stream,
							   FU_STRUCT_TPM_EVENT_LOG1_ITEM_SIZE,
							   error);
	if (st_hdr == NULL)
		return FALSE;

	/* advance over the header block */
	if (!fu_input_stream_read_u32(stream,
				      FU_STRUCT_TPM_EVENT_LOG1_ITEM_OFFSET_DATASZ,
				      &hdrsz,
				      G_LITTLE_ENDIAN,
				      error))
		return FALSE;
	if (!fu_input_stream_size(stream, &streamsz, error))
		return FALSE;
	for (gsize idx = FU_STRUCT_TPM_EVENT_LOG1_ITEM_SIZE + hdrsz; idx < streamsz;) {
		guint32 pcr;
		guint32 digestcnt;
		guint32 datasz = 0;
		g_autoptr(GBytes) checksum_sha1 = NULL;
		g_autoptr(GBytes) checksum_sha256 = NULL;
		g_autoptr(GBytes) checksum_sha384 = NULL;
		g_autoptr(FuStructTpmEventLog2) st = NULL;
		g_autoptr(FuTpmEventlogItem) item = NULL;

		/* read checksum block */
		st = fu_struct_tpm_event_log2_parse_stream(stream, idx, error);
		if (st == NULL)
			return FALSE;
		idx += st->buf->len;
		digestcnt = fu_struct_tpm_event_log2_get_digest_count(st);
		for (guint i = 0; i < digestcnt; i++) {
			guint16 alg_type = 0;
			guint32 alg_size = 0;
			g_autofree guint8 *digest = NULL;
			g_autoptr(GBytes) checksum = NULL;

			/* get checksum type */
			if (!fu_input_stream_read_u16(stream,
						      idx,
						      &alg_type,
						      G_LITTLE_ENDIAN,
						      error))
				return FALSE;
			alg_size = fu_tpm_eventlog_v2_hash_get_size(alg_type);
			if (alg_size == 0) {
				g_set_error(error,
					    FWUPD_ERROR,
					    FWUPD_ERROR_NOT_SUPPORTED,
					    "hash algorithm 0x%x size not known",
					    alg_type);
				return FALSE;
			}

			/* build checksum */
			idx += sizeof(alg_type);

			/* copy hash */
			checksum = fu_input_stream_read_bytes(stream, idx, alg_size, NULL, error);
			if (checksum == NULL)
				return FALSE;

			/* save this for analysis */
			if (alg_type == TPM2_ALG_SHA1)
				checksum_sha1 = g_bytes_ref(checksum);
			else if (alg_type == TPM2_ALG_SHA256)
				checksum_sha256 = g_bytes_ref(checksum);
			else if (alg_type == TPM2_ALG_SHA384)
				checksum_sha384 = g_bytes_ref(checksum);

			/* next block */
			idx += alg_size;
		}

		/* read data block */
		if (!fu_input_stream_read_u32(stream, idx, &datasz, G_LITTLE_ENDIAN, error))
			return FALSE;
		if (datasz > 1024 * 1024) {
			g_set_error_literal(error,
					    FWUPD_ERROR,
					    FWUPD_ERROR_NOT_SUPPORTED,
					    "event log item too large");
			return FALSE;
		}

		/* save blob if PCR=0 */
		idx += sizeof(datasz);
		pcr = fu_struct_tpm_event_log2_get_pcr(st);

		/* build item */
		item = fu_tpm_eventlog_item_new();
		fu_tpm_eventlog_item_set_pcr(item, pcr);
		fu_tpm_eventlog_item_set_kind(item, fu_struct_tpm_event_log2_get_type(st));
		if (checksum_sha1 != NULL) {
			fu_tpm_eventlog_item_add_checksum(item, G_CHECKSUM_SHA1, checksum_sha1);
		}
		if (checksum_sha256 != NULL) {
			fu_tpm_eventlog_item_add_checksum(item, G_CHECKSUM_SHA256, checksum_sha256);
		}
		if (checksum_sha384 != NULL) {
			fu_tpm_eventlog_item_add_checksum(item, G_CHECKSUM_SHA384, checksum_sha384);
		}
		if (datasz > 0) {
			g_autoptr(GBytes) blob = NULL;
			blob = fu_input_stream_read_bytes(stream, idx, datasz, NULL, error);
			if (blob == NULL)
				return FALSE;
			fu_firmware_set_bytes(FU_FIRMWARE(item), blob);
		}
		if (!fu_firmware_add_image(firmware, FU_FIRMWARE(item), error))
			return FALSE;

		/* next entry */
		idx += datasz;
	}

	/* success */
	return TRUE;
}

static void
fu_tpm_eventlog_v2_class_init(FuTpmEventlogV2Class *klass)
{
	FuFirmwareClass *firmware_class = FU_FIRMWARE_CLASS(klass);
	firmware_class->parse = fu_tpm_eventlog_v2_parse;
}

static void
fu_tpm_eventlog_v2_init(FuTpmEventlogV2 *self)
{
	fu_firmware_add_image_gtype(FU_FIRMWARE(self), FU_TYPE_TPM_EVENTLOG_ITEM);
}

/**
 * fu_tpm_eventlog_v2_new:
 *
 * Creates a new object to parse TPM eventlog data.
 *
 * Returns: a #FuTpmEventlog
 *
 * Since: 2.1.1
 **/
FuTpmEventlog *
fu_tpm_eventlog_v2_new(void)
{
	FuTpmEventlogV2 *self;
	self = g_object_new(FU_TYPE_TPM_EVENTLOG_V2, NULL);
	return FU_TPM_EVENTLOG(self);
}
