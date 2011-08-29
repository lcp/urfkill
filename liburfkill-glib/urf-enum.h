/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Gary Ching-Pang Lin <glin@suse.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:urf-enum
 * @short_description: Enumerations for switch types and switch states
 * @title: UrfEnum
 * @include: urfkill.h
 * @see_also: #UrfClient, #UrfDevice
 *
 * This header stores the definition of switch types and switch states
 */

#if !defined (__URFKILL_H_INSIDE__) && !defined (URF_COMPILATION)
#error "Only <urfkill.h> can be included directly."
#endif

#ifndef __URF_TYPE_H
#define __URF_TYPE_H

/**
 * UrfSwitchType:
 * @URFSWITCH_TYPE_ALL: toggles all switches (requests only - not a switch type)
 * @URFSWITCH_TYPE_WLAN: switch is on a 802.11 wireless network device.
 * @URFSWITCH_TYPE_BLUETOOTH: switch is on a bluetooth device.
 * @URFSWITCH_TYPE_UWB: switch is on a ultra wideband device.
 * @URFSWITCH_TYPE_WIMAX: switch is on a WiMAX device.
 * @URFSWITCH_TYPE_WWAN: switch is on a wireless WAN device.
 * @URFSWITCH_TYPE_GPS: switch is on a GPS device.
 * @URFSWITCH_TYPE_FM: switch is on a FM radio device.
 * @NUM_URFSWITCH_TYPES: number of defined rfkill types
 *
 * The type of the rfkill device following the definition in &lt;linux/rfkill.h&gt;
 */
typedef enum {
	URFSWITCH_TYPE_ALL = 0,
	URFSWITCH_TYPE_WLAN,
	URFSWITCH_TYPE_BLUETOOTH,
	URFSWITCH_TYPE_UWB,
	URFSWITCH_TYPE_WIMAX,
	URFSWITCH_TYPE_WWAN,
	URFSWITCH_TYPE_GPS,
	URFSWITCH_TYPE_FM,
	NUM_URFSWITCH_TYPES,
} UrfSwitchType;

/**
 * UrfSwitchState:
 * @URFSWITCH_STATE_NO_ADAPTOR: switch doesn't exist
 * @URFSWITCH_STATE_UNBLOCKED: switch is unblocked
 * @URFSWITCH_STATE_SOFT_BLOCKED: switch is soft-blocked.
 * @URFSWITCH_STATE_HARD_BLOCKED: switch is hard-blocked.
 *
 * The state of the switch
 */
typedef enum {
	URFSWITCH_STATE_NO_ADAPTOR = -1,
	URFSWITCH_STATE_UNBLOCKED = 0,
	URFSWITCH_STATE_SOFT_BLOCKED,
	URFSWITCH_STATE_HARD_BLOCKED,
} UrfSwitchState;

#endif
