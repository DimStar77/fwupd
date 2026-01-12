/*
 * Copyright 2019 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include <tss2/tss2_esys.h>

#include "fu-tpm-eventlog-parser.h"
#include "fu-tpm-struct.h"

#define FU_TPM_EVENTLOG_V2_HDR_SIGNATURE "Spec ID Event03"

static GPtrArray *
fu_tpm_eventlog_parser_parse_blob_v2(const guint8 *buf,
				     gsize bufsz,
				     FuTpmEventlogParserFlags flags,
				     GError **error)
{
	guint32 hdrsz = 0x0;
	g_autoptr(GPtrArray) items = NULL;

	/* advance over the header block */
	if (!fu_memread_uint32_safe(buf,
				    bufsz,
				    FU_STRUCT_TPM_EVENT_LOG1_ITEM_OFFSET_DATASZ,
				    &hdrsz,
				    G_LITTLE_ENDIAN,
				    error))
		return NULL;
	items = g_ptr_array_new_with_free_func((GDestroyNotify)g_object_unref);
	for (gsize idx = FU_STRUCT_TPM_EVENT_LOG1_ITEM_SIZE + hdrsz; idx < bufsz;) {
		guint32 pcr;
		guint32 digestcnt;
		guint32 datasz = 0;
		g_autoptr(GBytes) checksum_sha1 = NULL;
		g_autoptr(GBytes) checksum_sha256 = NULL;
		g_autoptr(GBytes) checksum_sha384 = NULL;
		g_autoptr(FuStructTpmEventLog2) st = NULL;

		/* read checksum block */
		st = fu_struct_tpm_event_log2_parse(buf, bufsz, idx, error);
		if (st == NULL)
			return NULL;
		idx += st->buf->len;
		digestcnt = fu_struct_tpm_event_log2_get_digest_count(st);
		for (guint i = 0; i < digestcnt; i++) {
			guint16 alg_type = 0;
			guint32 alg_size = 0;
			g_autofree guint8 *digest = NULL;

			/* get checksum type */
			if (!fu_memread_uint16_safe(buf,
						    bufsz,
						    idx,
						    &alg_type,
						    G_LITTLE_ENDIAN,
						    error))
				return NULL;
			alg_size = fu_tpm_eventlog_hash_get_size(alg_type);
			if (alg_size == 0) {
				g_set_error(error,
					    FWUPD_ERROR,
					    FWUPD_ERROR_NOT_SUPPORTED,
					    "hash algorithm 0x%x size not known",
					    alg_type);
				return NULL;
			}

			/* build checksum */
			idx += sizeof(alg_type);

			/* copy hash */
			digest = g_malloc0(alg_size);
			if (!fu_memcpy_safe(digest,
					    alg_size,
					    0x0, /* dst */
					    buf,
					    bufsz,
					    idx, /* src */
					    alg_size,
					    error))
				return NULL;

			/* save this for analysis */
			if (alg_type == TPM2_ALG_SHA1)
				checksum_sha1 =
				    g_bytes_new_take(g_steal_pointer(&digest), alg_size);
			else if (alg_type == TPM2_ALG_SHA256)
				checksum_sha256 =
				    g_bytes_new_take(g_steal_pointer(&digest), alg_size);
			else if (alg_type == TPM2_ALG_SHA384)
				checksum_sha384 =
				    g_bytes_new_take(g_steal_pointer(&digest), alg_size);

			/* next block */
			idx += alg_size;
		}

		/* read data block */
		if (!fu_memread_uint32_safe(buf, bufsz, idx, &datasz, G_LITTLE_ENDIAN, error))
			return NULL;
		if (datasz > 1024 * 1024) {
			g_set_error_literal(error,
					    FWUPD_ERROR,
					    FWUPD_ERROR_NOT_SUPPORTED,
					    "event log item too large");
			return NULL;
		}

		/* save blob if PCR=0 */
		idx += sizeof(datasz);
		pcr = fu_struct_tpm_event_log2_get_pcr(st);
		if (pcr == ESYS_TR_PCR0 || flags & FU_TPM_EVENTLOG_PARSER_FLAG_ALL_PCRS) {
			g_autoptr(FuTpmEventlogItem) item = NULL;

			/* build item */
			item = fu_tpm_eventlog_item_new();
			fu_tpm_eventlog_item_set_pcr(item, pcr);
			fu_tpm_eventlog_item_set_kind(item, fu_struct_tpm_event_log2_get_type(st));
			if (checksum_sha1 != NULL) {
				fu_tpm_eventlog_item_add_checksum(item,
								  G_CHECKSUM_SHA1,
								  checksum_sha1);
			}
			if (checksum_sha256 != NULL) {
				fu_tpm_eventlog_item_add_checksum(item,
								  G_CHECKSUM_SHA256,
								  checksum_sha256);
			}
			if (checksum_sha384 != NULL) {
				fu_tpm_eventlog_item_add_checksum(item,
								  G_CHECKSUM_SHA384,
								  checksum_sha384);
			}
			if (datasz > 0) {
				g_autofree guint8 *data = g_malloc0(datasz);
				g_autoptr(GBytes) blob = NULL;
				if (!fu_memcpy_safe(data,
						    datasz,
						    0x0, /* dst */
						    buf,
						    bufsz,
						    idx,
						    datasz, /* src */
						    error))
					return NULL;
				blob = g_bytes_new_take(g_steal_pointer(&data), datasz);
				fu_firmware_set_bytes(FU_FIRMWARE(item), blob);
				fu_dump_bytes(G_LOG_DOMAIN, "TpmEvent", blob);
			}
			g_ptr_array_add(items, g_steal_pointer(&item));
		}

		/* next entry */
		idx += datasz;
	}

	/* success */
	return g_steal_pointer(&items);
}

GPtrArray *
fu_tpm_eventlog_parser_new(const guint8 *buf,
			   gsize bufsz,
			   FuTpmEventlogParserFlags flags,
			   GError **error)
{
	gchar sig[] = FU_TPM_EVENTLOG_V2_HDR_SIGNATURE;
	g_autoptr(GPtrArray) items = NULL;

	g_return_val_if_fail(buf != NULL, NULL);

	/* look for TCG v2 signature */
	if (!fu_memcpy_safe((guint8 *)sig,
			    sizeof(sig),
			    0x0, /* dst */
			    buf,
			    bufsz,
			    FU_STRUCT_TPM_EVENT_LOG1_ITEM_SIZE, /* src */
			    sizeof(sig),
			    error))
		return NULL;
	if (g_strcmp0(sig, FU_TPM_EVENTLOG_V2_HDR_SIGNATURE) == 0)
		return fu_tpm_eventlog_parser_parse_blob_v2(buf, bufsz, flags, error);

	/* assume v1 structure */
	items = g_ptr_array_new_with_free_func((GDestroyNotify)g_object_unref);
	for (gsize idx = 0; idx < bufsz; idx += FU_STRUCT_TPM_EVENT_LOG1_ITEM_SIZE) {
		guint32 datasz = 0;
		guint32 pcr = 0;
		guint32 event_type = 0;
		g_autoptr(FuStructTpmEventLog1Item) st = NULL;

		st = fu_struct_tpm_event_log1_item_parse(buf, bufsz, idx, error);
		if (st == NULL)
			return NULL;
		pcr = fu_struct_tpm_event_log1_item_get_pcr(st);
		event_type = fu_struct_tpm_event_log1_item_get_event_type(st);
		datasz = fu_struct_tpm_event_log1_item_get_datasz(st);
		if (datasz > 1024 * 1024) {
			g_set_error_literal(error,
					    FWUPD_ERROR,
					    FWUPD_ERROR_NOT_SUPPORTED,
					    "event log item too large");
			return NULL;
		}
		if (pcr == ESYS_TR_PCR0 || flags & FU_TPM_EVENTLOG_PARSER_FLAG_ALL_PCRS) {
			g_autoptr(FuTpmEventlogItem) item = fu_tpm_eventlog_item_new();
			gsize digestsz = 0;
			const guint8 *digest =
			    fu_struct_tpm_event_log1_item_get_digest(st, &digestsz);
			g_autoptr(GBytes) checksum_sha1 = NULL;

			/* build item */
			fu_tpm_eventlog_item_set_pcr(item, pcr);
			fu_tpm_eventlog_item_set_kind(item, event_type);
			checksum_sha1 = g_bytes_new(digest, digestsz);
			fu_tpm_eventlog_item_add_checksum(item, G_CHECKSUM_SHA1, checksum_sha1);
			if (datasz > 0) {
				g_autofree guint8 *data = g_malloc0(datasz);
				g_autoptr(GBytes) blob = NULL;
				if (!fu_memcpy_safe(data,
						    datasz,
						    0x0, /* dst */
						    buf,
						    bufsz,
						    idx + st->buf->len, /* src */
						    datasz,
						    error))
					return NULL;
				blob = g_bytes_new_take(g_steal_pointer(&data), datasz);
				fu_firmware_set_bytes(FU_FIRMWARE(item), blob);
				fu_dump_bytes(G_LOG_DOMAIN, "TpmEvent", blob);
			}
			g_ptr_array_add(items, g_steal_pointer(&item));
		}
		idx += datasz;
	}
	return g_steal_pointer(&items);
}
