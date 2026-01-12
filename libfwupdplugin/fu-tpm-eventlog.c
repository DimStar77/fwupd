/*
 * Copyright 2026 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#define G_LOG_DOMAIN "FuTpmEventlog"

#include "config.h"

// #include "fwupd-error.h"

#include "fu-tpm-eventlog.h"
// #include "fu-tpm-eventlog-struct.h"
// #include "fu-string.h"

/**
 * FuTpmEventlog:
 *
 * Parse the TPM eventlog.
 */

struct _FuTpmEventlog {
	FuFirmware parent_instance;
	//	guint32 structure_table_len;
	//	GPtrArray *items;
};

G_DEFINE_TYPE(FuTpmEventlog, fu_tpm_eventlog, FU_TYPE_FIRMWARE)

static gboolean
fu_tpm_eventlog_parse(FuFirmware *firmware,
		      GInputStream *stream,
		      FuFirmwareParseFlags flags,
		      GError **error)
{
	//	FuTpmEventlog *self = FU_TPM_EVENTLOG(firmware);
	//	g_autoptr(GBytes) fw = NULL;
	//	fw = fu_input_stream_read_bytes(stream, 0x0, G_MAXSIZE, NULL, error);
	//	if (fw == NULL)
	//		return FALSE;
	return TRUE;
}

static void
fu_tpm_eventlog_export(FuFirmware *firmware, FuFirmwareExportFlags flags, XbBuilderNode *bn)
{
//	FuTpmEventlog *self = FU_TPM_EVENTLOG(firmware);
#if 0
	for (guint i = 0; i < self->items->len; i++) {
		FuTpmEventlogItem *item = g_ptr_array_index(self->items, i);
		g_autoptr(XbBuilderNode) bc = xb_builder_node_insert(bn, "item", NULL);
		g_autofree gchar *buf = fu_byte_array_to_string(item->buf);
		fu_xmlb_builder_insert_kx(bc, "type", item->type);
	}
#endif
}

static gboolean
fu_tpm_eventlog_build(FuFirmware *firmware, XbNode *n, GError **error)
{
//	FuTpmEventlog *self = FU_TPM_EVENTLOG(firmware);
#if 0
	g_autoptr(GPtrArray) xb_items = NULL;

	/* optional items */
	xb_items = xb_node_query(n, "item", 0, NULL);
	if (xb_items != NULL) {
		for (guint i = 0; i < xb_items->len; i++) {
			XbNode *c = g_ptr_array_index(xb_items, i);
			if (!fu_tpm_eventlog_build_item(self, c, error))
				return FALSE;
		}
	}
#endif

	/* success */
	return TRUE;
}

static void
fu_tpm_eventlog_finalize(GObject *object)
{
	//	FuTpmEventlog *self = FU_TPM_EVENTLOG(object);
	//	g_ptr_array_unref(self->items);
	G_OBJECT_CLASS(fu_tpm_eventlog_parent_class)->finalize(object);
}

static void
fu_tpm_eventlog_class_init(FuTpmEventlogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	FuFirmwareClass *firmware_class = FU_FIRMWARE_CLASS(klass);
	object_class->finalize = fu_tpm_eventlog_finalize;
	firmware_class->parse = fu_tpm_eventlog_parse;
	firmware_class->build = fu_tpm_eventlog_build;
	firmware_class->export = fu_tpm_eventlog_export;
}

static void
fu_tpm_eventlog_init(FuTpmEventlog *self)
{
}

/**
 * fu_tpm_eventlog_new:
 *
 * Creates a new object to parse TPM_EVENTLOG data.
 *
 * Returns: a #FuTpmEventlog
 *
 * Since: 2.1.1
 **/
FuTpmEventlog *
fu_tpm_eventlog_new(void)
{
	FuTpmEventlog *self;
	self = g_object_new(FU_TYPE_TPM_EVENTLOG, NULL);
	return FU_TPM_EVENTLOG(self);
}
