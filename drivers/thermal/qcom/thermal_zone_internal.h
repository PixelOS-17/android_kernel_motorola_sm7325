/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __QTI_THERMAL_ZONE_INTERNAL_H
#define __QTI_THERMAL_ZONE_INTERNAL_H

#include <linux/thermal.h>
#include "../thermal_core.h"

/*
 * The 6.18 thermal core reinitializes its instances when a zone changes mode,
 * zone ops are const, and it has no vendor hook for cooling stats. These stay
 * as no-ops so the QTI sensor drivers keep their call sites.
 */
static inline __maybe_unused int qti_tz_change_mode(struct thermal_zone_device *tz,
		enum thermal_device_mode mode)
{
	return 0;
}

static inline __maybe_unused int qti_update_tz_ops(struct thermal_zone_device *tz, bool enable)
{
	return tz ? 0 : -EINVAL;
}

static inline __maybe_unused void thermal_vendor_hooks_init(void)
{
}

static inline __maybe_unused void thermal_vendor_hooks_exit(void)
{
}

#endif  // __QTI_THERMAL_ZONE_INTERNAL_H
