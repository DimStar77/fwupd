/*
 * Copyright 2026 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "fu-firmware.h"

#define FU_TYPE_TPM_EVENTLOG (fu_tpm_eventlog_get_type())

G_DECLARE_FINAL_TYPE(FuTpmEventlog, fu_tpm_eventlog, FU, TPM_EVENTLOG, FuFirmware)

FuTpmEventlog *
fu_tpm_eventlog_new(void);
