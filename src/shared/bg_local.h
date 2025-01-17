/*
 * Daemon GPL Source Code
 * Copyright (C) 2016  Unreal Arena
 * Copyright (C) 2000-2009  Darklegion Development
 * Copyright (C) 1999-2005  Id Software, Inc.
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


#ifndef BG_LOCAL_H_
#define BG_LOCAL_H_
// bg_local.h -- local definitions for the bg (both games) files
//==================================================================

#define MIN_WALK_NORMAL   0.7f // can't walk on very steep slopes

#define STEPSIZE          18

#define TIMER_LAND        130
#define TIMER_GESTURE     ( 34 * 66 + 50 )
#define TIMER_ATTACK      500 //nonsegmented models

#define FALLING_THRESHOLD -900.0f //what vertical speed to start falling sound at

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server
typedef struct
{
	vec3_t   forward, right, up;
	float    frametime;

	int      msec;

	bool walking;
	bool groundPlane;
#ifndef UNREALARENA
	bool ladder;
#endif
	trace_t  groundTrace;

	float    impactSpeed;

	vec3_t   previous_origin;
	vec3_t   previous_velocity;
	int      previous_waterlevel;
} pml_t;

extern  pmove_t *pm;
extern  pml_t   pml;

// movement parameters
#define pm_stopspeed         (100.0f)
#define pm_duckScale         (0.25f)
#define pm_swimScale         (0.50f)

#define pm_accelerate        (10.0f)
#define pm_wateraccelerate   (4.0f)
#define pm_flyaccelerate     (4.0f)

#define pm_friction          (6.0f)
#define pm_waterfriction     (1.125f)
#define pm_flightfriction    (6.0f)
#define pm_spectatorfriction (5.0f)

extern  int     c_pmove;

void            PM_ClipVelocity( const vec3_t in, const vec3_t normal, vec3_t out );
void            PM_AddTouchEnt( int entityNum );
void            PM_AddEvent( int newEvent );

bool        PM_SlideMove( bool gravity );
void            PM_StepEvent( const vec3_t from, const vec3_t to, const vec3_t normal );
bool        PM_StepSlideMove( bool gravity, bool predictive );
bool        PM_PredictStepMove();

//==================================================================
#endif /* BG_LOCAL_H_ */
