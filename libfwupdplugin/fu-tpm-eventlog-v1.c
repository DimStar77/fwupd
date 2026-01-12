/*
 * Copyright 2026 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#define G_LOG_DOMAIN "FuTpmEventlog"

#include "config.h"

// #include "fwupd-error.h"

#include "fu-tpm-eventlog-v1.h"
// #include "fu-tpm-eventlog-struct.h"
// #include "fu-string.h"

/**
 * FuTpmEventlogV1:
 *
 * Parse the TPM eventlog.
 */

struct _FuTpmEventlogV1 {
	FuFirmware parent_instance;
};

G_DEFINE_TYPE(FuTpmEventlogV1, fu_tpm_eventlog_v1, FU_TYPE_FIRMWARE)

static gboolean
fu_tpm_eventlog_v1_parse(FuFirmware *firmware,
			 GInputStream *stream,
			 FuFirmwareParseFlags flags,
			 GError **error)
{
	//	FuTpmEventlogV1 *self = FU_TPM_EVENTLOG_V1(firmware);
	return TRUE;
}

static void
fu_tpm_eventlog_v1_class_init(FuTpmEventlogV1Class *klass)
{
	FuFirmwareClass *firmware_class = FU_FIRMWARE_CLASS(klass);
	firmware_class->parse = fu_tpm_eventlog_v1_parse;
}

static void
fu_tpm_eventlog_v1_init(FuTpmEventlogV1 *self)
{
}

/**
 * fu_tpm_eventlog_v1_new:
 *
 * Creates a new object to parse TPM eventlog data.
 *
 * Returns: a #FuTpmEventlogV1
 *
 * Since: 2.1.1
 **/
FuTpmEventlogV1 *
fu_tpm_eventlog_v1_new(void)
{
	FuTpmEventlogV1 *self;
	self = g_object_new(FU_TYPE_TPM_EVENTLOG_V1, NULL);
	return FU_TPM_EVENTLOG_V1(self);
}
