/*
 * Daemon GPL Source Code
 * Copyright (C) 2016  Unreal Arena
 * Copyright (C) 2012  Unvanquished Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __BOT_TYPE_H
#define __BOT_TYPE_H

typedef struct
{
	char name[ 64 ];
	unsigned short polyFlagsInclude;
	unsigned short polyFlagsExclude;
} botClass_t;

typedef struct
{
	float frac;
	float normal[ 3 ];
} botTrace_t;

// parameters outputted by navigation
// if they are followed exactly, the bot will not go off the nav mesh
typedef struct
{
	float    pos[ 3 ];
	float    tpos[ 3 ];
	float    dir[ 3 ];
	int      directPathToGoal;
	int      havePath;
} botNavCmd_t;

typedef enum
{
	BOT_TARGET_STATIC, // target stays in one place always
	BOT_TARGET_DYNAMIC // target can move
} botRouteTargetType_t;

// type: determines if the object can move or not
// pos: the object's position 
// polyExtents: how far away from pos to search for a nearby navmesh polygon for finding a route
typedef struct
{
	botRouteTargetType_t type;
	float pos[ 3 ];
	float polyExtents[ 3 ];
} botRouteTarget_t;

enum navPolyFlags
{
	POLYFLAGS_DISABLED = 0,
	POLYFLAGS_WALK     = 1 << 0,
	POLYFLAGS_JUMP     = 1 << 1,
	POLYFLAGS_POUNCE   = 1 << 2,
#ifndef UNREALARENA
	POLYFLAGS_WALLWALK = 1 << 3,
	POLYFLAGS_LADDER   = 1 << 4,
#endif
	POLYFLAGS_DROPDOWN = 1 << 5,
	POLYFLAGS_DOOR     = 1 << 6,
	POLYFLAGS_TELEPORT = 1 << 7,
	POLYFLAGS_CROUCH   = 1 << 8,
	POLYFLAGS_SWIM     = 1 << 9,
	POLYFLAGS_ALL      = 0xffff, // All abilities.
};

enum navPolyAreas
{
	POLYAREA_GROUND = 1 << 0,
#ifndef UNREALARENA
	POLYAREA_LADDER = 1 << 1,
#endif
	POLYAREA_WATER = 1 << 2,
	POLYAREA_DOOR = 1 << 3,
	POLYAREA_JUMPPAD = 1 << 4,
	POLYAREA_TELEPORTER = 1 << 5
};

//route status flags
#define ROUTE_FAILED  ( 1u << 31 )
#define	ROUTE_SUCCEED ( 1u << 30 )
#define	ROUTE_PARTIAL ( 1 << 6 )
#endif
