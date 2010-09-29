/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Gary Ching-Pang Lin <glin@novell.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __URF_KILLSWITCH_H
#define __URF_KILLSWITCH_H

typedef enum {
        RFKILL_TYPE_ALL = 0,
        RFKILL_TYPE_WLAN,
        RFKILL_TYPE_BLUETOOTH,
        RFKILL_TYPE_UWB,
        RFKILL_TYPE_WIMAX,
        RFKILL_TYPE_WWAN,
        RFKILL_TYPE_GPS,
        RFKILL_TYPE_FM,
        NUM_RFKILL_TYPES,
} KillswitchType;

typedef enum {
        KILLSWITCH_STATE_NO_ADAPTER = -1,
        KILLSWITCH_STATE_SOFT_BLOCKED = 0,
        KILLSWITCH_STATE_UNBLOCKED,
        KILLSWITCH_STATE_HARD_BLOCKED
} KillswitchState;

typedef struct {
	unsigned int index;
	KillswitchType type;
	KillswitchState state;
	unsigned int soft;
	unsigned int hard;
	char *name;
} UrfKillswitch;

#endif /* __URF_KILLSWITCH_H */
