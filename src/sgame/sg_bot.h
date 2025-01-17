/*
 * Daemon GPL source code
 * Copyright (C) 2015  Unreal Arena
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


#ifndef __BOT_HEADER
#define __BOT_HEADER

typedef struct
{
	gentity_t *ent;
	float distance;
} botEntityAndDistance_t;

typedef struct
{
	gentity_t *ent;
	vec3_t coord;
	bool inuse;
} botTarget_t;

#define MAX_ENEMY_QUEUE 32
typedef struct
{
	gentity_t *ent;
	int        timeFound;
} enemyQueueElement_t;

typedef struct
{
	enemyQueueElement_t enemys[ MAX_ENEMY_QUEUE ];
	int front;
	int back;
} enemyQueue_t;

typedef struct
{
	int level;
	float aimSlowness;
	float aimShake;
} botSkill_t;

#include "sg_bot_ai.h"
#define MAX_NODE_DEPTH 20

typedef struct
{
	enemyQueue_t enemyQueue;
	int enemyLastSeen;

	//team the bot is on when added
	team_t botTeam;

	botTarget_t goal;

	botSkill_t botSkill;
	botEntityAndDistance_t bestEnemy;
#ifndef UNREALARENA
	botEntityAndDistance_t closestDamagedBuilding;
	botEntityAndDistance_t closestBuildings[ BA_NUM_BUILDABLES ];
#endif

	AIBehaviorTree_t *behaviorTree;
	AIGenericNode_t  *currentNode;
	AIGenericNode_t  *runningNodes[ MAX_NODE_DEPTH ];
	int              numRunningNodes;

	int         futureAimTime;
	int         futureAimTimeInterval;
	vec3_t      futureAim;
	usercmd_t   cmdBuffer;
	botNavCmd_t nav;
} botMemory_t;

bool G_BotAdd( char *name, team_t team, int skill, const char* behavior );
bool G_BotSetDefaults( int clientNum, team_t team, int skill, const char* behavior );
void     G_BotDel( int clientNum );
void     G_BotDelAllBots();
void     G_BotThink( gentity_t *self );
void     G_BotSpectatorThink( gentity_t *self );
void     G_BotIntermissionThink( gclient_t *client );
void     G_BotListNames( gentity_t *ent );
bool G_BotClearNames();
int      G_BotAddNames(team_t team, int arg, int last);
void     G_BotDisableArea( vec3_t origin, vec3_t mins, vec3_t maxs );
void     G_BotEnableArea( vec3_t origin, vec3_t mins, vec3_t maxs );
void     G_BotInit();
void     G_BotCleanup(int restart);
#endif
