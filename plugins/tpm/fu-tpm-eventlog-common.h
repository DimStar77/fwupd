/*
 * Copyright 2019 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <fwupdplugin.h>

#include <tss2/tss2_tpm2_types.h>

#include "fu-tpm-struct.h"

const gchar *
fu_tpm_eventlog_pcr_to_string(gint pcr);
guint32
fu_tpm_eventlog_hash_get_size(TPM2_ALG_ID hash_kind);
GPtrArray *
fu_tpm_eventlog_calc_checksums(GPtrArray *items, guint8 pcr, GError **error);
