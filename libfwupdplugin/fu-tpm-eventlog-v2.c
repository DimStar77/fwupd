/*
 * Copyright 2026 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#define G_LOG_DOMAIN "FuTpmEventlog"

#include "config.h"

// #include "fwupd-error.h"

#include "fu-tpm-eventlog-v2.h"
// #include "fu-tpm-eventlog-struct.h"
// #include "fu-string.h"

/**
 * FuTpmEventlogV2:
 *
 * Parse the TPM eventlog.
 */

struct _FuTpmEventlogV2 {
	FuFirmware parent_instance;
};

G_DEFINE_TYPE(FuTpmEventlogV2, fu_tpm_eventlog_v2, FU_TYPE_FIRMWARE)

static gboolean
fu_tpm_eventlog_v2_parse(FuFirmware *firmware,
			 GInputStream *stream,
			 FuFirmwareParseFlags flags,
			 GError **error)
{
	//	FuTpmEventlogV2 *self = FU_TPM_EVENTLOG_V2(firmware);
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
}

/**
 * fu_tpm_eventlog_v2_new:
 *
 * Creates a new object to parse TPM eventlog data.
 *
 * Returns: a #FuTpmEventlogV2
 *
 * Since: 2.1.1
 **/
FuTpmEventlogV2 *
fu_tpm_eventlog_v2_new(void)
{
	FuTpmEventlogV2 *self;
	self = g_object_new(FU_TYPE_TPM_EVENTLOG_V2, NULL);
	return FU_TPM_EVENTLOG_V2(self);
}
