/*
 * Daemon GPL Source Code
 * Copyright (C) 2015-2016  Unreal Arena
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


#ifndef __BOT_UTIL_HEADER
#define __BOT_UTIL_HEADER
#include "sg_local.h"
#include "engine/botlib/bot_types.h"
#include "sg_bot.h"

//g_bot.c
void     BotError( const char* fmt, ... ) PRINTF_LIKE(1);
void     BotDPrintf( const char* fmt, ... ) PRINTF_LIKE(1);
bool PlayersBehindBotInSpawnQueue( gentity_t *self );
void     BotSetSkillLevel( gentity_t *self, int skill );

// entity queries
int        FindBots( int *botEntityNumbers, int maxBots, team_t team );
gentity_t* BotFindClosestEnemy( gentity_t *self );
gentity_t* BotFindBestEnemy( gentity_t *self );
#ifndef UNREALARENA
void       BotFindClosestBuildings( gentity_t *self );
gentity_t* BotFindBuilding( gentity_t *self, int buildingType, int range );
#endif
bool   BotTeamateHasWeapon( gentity_t *self, int weapon );
void       BotSearchForEnemy( gentity_t *self );
void       BotPain( gentity_t *self, gentity_t *attacker, int damage );

// aiming
void  BotGetIdealAimLocation( gentity_t *self, botTarget_t target, vec3_t aimLocation );
void  BotAimAtEnemy( gentity_t *self );
void  BotSlowAim( gentity_t *self, vec3_t target, float slow );
void  BotShakeAim( gentity_t *self, vec3_t rVec );
void  BotAimAtLocation( gentity_t *self, vec3_t target );
float BotAimAngle( gentity_t *self, vec3_t pos );

// targets
void        BotSetTarget( botTarget_t *target, gentity_t *ent, vec3_t pos );
bool    BotTargetIsEntity( botTarget_t target );
bool    BotTargetIsPlayer( botTarget_t target );
bool    BotTargetIsVisible( gentity_t *self, botTarget_t target, int mask );
bool    BotTargetInAttackRange( gentity_t *self, botTarget_t target );
void        BotTargetToRouteTarget( gentity_t *self, botTarget_t target, botRouteTarget_t *routeTarget );
int         BotGetTargetEntityNumber( botTarget_t target );
void        BotGetTargetPos( botTarget_t target, vec3_t rVec );
team_t      BotGetEntityTeam( gentity_t *ent );
team_t      BotGetTargetTeam( botTarget_t target );
int         BotGetTargetType( botTarget_t target );
botTarget_t BotGetRoamTarget( gentity_t *self );
botTarget_t BotGetRetreatTarget( gentity_t *self );
botTarget_t BotGetRushTarget( gentity_t *self );

// logic functions
float    BotGetHealScore( gentity_t *self );
float    BotGetBaseRushScore( gentity_t *ent );
float    BotGetEnemyPriority( gentity_t *self, gentity_t *ent );

// goal changing
bool BotChangeGoal( gentity_t *self, botTarget_t target );
bool BotChangeGoalEntity( gentity_t *self, gentity_t *goal );
bool BotChangeGoalPos( gentity_t *self, vec3_t goal );

// fighting
void     BotResetEnemyQueue( enemyQueue_t *queue );
bool BotEnemyIsValid( gentity_t *self, gentity_t *enemy );
void     BotFireWeapon( weaponMode_t mode, usercmd_t *botCmdBuffer );
void     BotFireWeaponAI( gentity_t *self );
void     BotClassMovement( gentity_t *self, bool inAttackRange );

// human bots
bool   WeaponIsEmpty( weapon_t weapon, playerState_t *ps );
float      PercentAmmoRemaining( weapon_t weapon, playerState_t *ps );
#ifndef UNREALARENA
void       BotFindDamagedFriendlyStructure( gentity_t *self );
bool   BotGetBuildingToBuild( gentity_t *self, vec3_t origin, vec3_t normal, buildable_t *building );
void       BotBuyWeapon( gentity_t *self, weapon_t weapon );
void       BotBuyUpgrade( gentity_t *self, upgrade_t upgrade );
void       BotSellWeapons( gentity_t *self );
void       BotSellAll( gentity_t *self );
int        BotValueOfWeapons( gentity_t *self );
int        BotValueOfUpgrades( gentity_t *self );
void       BotGetDesiredBuy( gentity_t *self, weapon_t *weapon, upgrade_t *upgrades, int *numUpgrades );
#endif

// alien bots
#ifndef UNREALARENA
#define AS_OVER_RT3         ((ALIENSENSE_RANGE*0.5f)/M_ROOT3)
float    CalcPounceAimPitch( gentity_t *self, botTarget_t target );
float    CalcBarbAimPitch( gentity_t *self, botTarget_t target );
bool BotCanEvolveToClass( gentity_t *self, class_t newClass );
bool BotEvolveToClass( gentity_t *ent, class_t newClass );
#endif
float    CalcAimPitch( gentity_t *self, botTarget_t target, vec_t launchSpeed );

//g_bot_nav.c
typedef enum
{
	MOVE_FORWARD = BIT( 0 ),
	MOVE_BACKWARD = BIT( 1 ),
	MOVE_LEFT = BIT( 2 ),
	MOVE_RIGHT = BIT( 3 )
} botMoveDir_t;

// global navigation
extern bool navMeshLoaded;

void         G_BotNavInit();
void         G_BotNavCleanup();
bool     FindRouteToTarget( gentity_t *self, botTarget_t target, bool allowPartial );
void         BotMoveToGoal( gentity_t *self );
#ifdef UNREALARENA
void         BotSetNavmesh( gentity_t  *ent, team_t newTeam );
#else
void         BotSetNavmesh( gentity_t  *ent, class_t newClass );
#endif
void         BotClampPos( gentity_t *self );

// local navigation
bool BotDodge( gentity_t *self );
void     BotWalk( gentity_t *self, bool enable );
bool BotSprint( gentity_t *self, bool enable );
bool BotJump( gentity_t *self );
void     BotStrafeDodge( gentity_t *self );
void     BotAlternateStrafe( gentity_t *self );
void     BotMoveInDir( gentity_t *self, uint32_t moveDir );
void     BotStandStill( gentity_t *self );

// navigation queries
bool GoalInRange( gentity_t *self, float r );
int      DistanceToGoal( gentity_t *self );
int      DistanceToGoalSquared( gentity_t *self );
int      DistanceToGoal2DSquared( gentity_t *self );
float    BotGetGoalRadius( gentity_t *self );
void     BotFindRandomPointOnMesh( gentity_t *self, vec3_t point );
bool BotPathIsWalkable( gentity_t *self, botTarget_t target );

//configureable constants
//For a reference of how far a number represents, take a look at tremulous.h

//how long our traces are for obstacle avoidence
#define BOT_OBSTACLE_AVOID_RANGE 20.0f

//at what hp do we use medkit?
#define BOT_USEMEDKIT_HP 50

//when human bots reach this ammo percentage left or less(and no enemy), they will head back to the base to refuel ammo
#define BOT_LOW_AMMO 0.50f

//used for clamping distance to heal structure when deciding whether to go heal
#define MAX_HEAL_DIST 2000.0f

#ifndef UNREALARENA
//how far away we can be before we stop going forward when fighting an alien
#define MAX_HUMAN_DANCE_DIST 300.0f

//how far away we can be before we try to go around an alien when fighting an alien
#define MIN_HUMAN_DANCE_DIST 100.0f
#endif
#endif
