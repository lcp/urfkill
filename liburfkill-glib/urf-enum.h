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

#ifndef __URF_ENUM_H
#define __URF_ENUM_H

/**
 * UrfEnumType:
 * @URF_ENUM_TYPE_ALL: toggles all switches (requests only - not a switch type)
 * @URF_ENUM_TYPE_WLAN: switch is on a 802.11 wireless network device.
 * @URF_ENUM_TYPE_BLUETOOTH: switch is on a bluetooth device.
 * @URF_ENUM_TYPE_UWB: switch is on a ultra wideband device.
 * @URF_ENUM_TYPE_WIMAX: switch is on a WiMAX device.
 * @URF_ENUM_TYPE_WWAN: switch is on a wireless WAN device.
 * @URF_ENUM_TYPE_GPS: switch is on a GPS device.
 * @URF_ENUM_TYPE_FM: switch is on a FM radio device.
 * @URF_ENUM_TYPE_NFC: switch is on a NFC device.
 * @URF_ENUM_TYPE_NUM: number of defined rfkill types
 *
 * The type of the rfkill device following the definition in &lt;linux/rfkill.h&gt;
 */
typedef enum {
	URF_ENUM_TYPE_ALL = 0,
	URF_ENUM_TYPE_WLAN,
	URF_ENUM_TYPE_BLUETOOTH,
	URF_ENUM_TYPE_UWB,
	URF_ENUM_TYPE_WIMAX,
	URF_ENUM_TYPE_WWAN,
	URF_ENUM_TYPE_GPS,
	URF_ENUM_TYPE_FM,
	URF_ENUM_TYPE_NFC,
	URF_ENUM_TYPE_NUM,
} UrfEnumType;

/**
 * UrfEnumState:
 * @URF_ENUM_STATE_NO_ADAPTER: switch doesn't exist
 * @URF_ENUM_STATE_UNBLOCKED: switch is unblocked
 * @URF_ENUM_STATE_SOFT_BLOCKED: switch is soft-blocked.
 * @URF_ENUM_STATE_HARD_BLOCKED: switch is hard-blocked.
 *
 * The state of the switch
 */
typedef enum {
	URF_ENUM_STATE_NO_ADAPTER = -1,
	URF_ENUM_STATE_UNBLOCKED = 0,
	URF_ENUM_STATE_SOFT_BLOCKED,
	URF_ENUM_STATE_HARD_BLOCKED,
} UrfEnumState;

#endif
