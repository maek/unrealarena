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


#include "sg_bot_ai.h"
#include "sg_bot_util.h"

void BotDPrintf( const char* fmt, ... )
{
	if ( g_bot_debug.integer )
	{
		va_list argptr;
		char    text[ 1024 ];

		va_start( argptr, fmt );
		Q_vsnprintf( text, sizeof( text ), fmt, argptr );
		va_end( argptr );

		trap_Print( text );
	}
}

void BotError( const char* fmt, ... )
{
	va_list argptr;
	size_t  len;
	char    text[ 1024 ] = S_COLOR_RED "ERROR: ";

	len = strlen( text );

	va_start( argptr, fmt );
	Q_vsnprintf( text + len, sizeof( text ) - len, fmt, argptr );
	va_end( argptr );

	trap_Print( text );
}

/*
 = *======================
 Scoring functions for logic
 =======================
 */
float BotGetBaseRushScore( gentity_t *ent )
{

	switch ( ent->s.weapon )
	{
		case WP_BLASTER:
			return 0.1f;
		case WP_LUCIFER_CANNON:
			return 1.0f;
		case WP_MACHINEGUN:
			return 0.5f;
		case WP_PULSE_RIFLE:
			return 0.7f;
		case WP_LAS_GUN:
			return 0.7f;
		case WP_SHOTGUN:
			return 0.2f;
		case WP_CHAINGUN:
			if ( BG_InventoryContainsUpgrade( UP_BATTLESUIT, ent->client->ps.stats ) )
			{
				return 0.5f;
			}
			else
			{
				return 0.2f;
			}
		case WP_HBUILD:
			return 0.0f;
		case WP_ABUILD:
			return 0.0f;
		case WP_ABUILD2:
			return 0.0f;
		case WP_ALEVEL0:
			return 0.0f;
		case WP_ALEVEL1:
			return 0.2f;
		case WP_ALEVEL2:
			return 0.5f;
		case WP_ALEVEL2_UPG:
			return 0.7f;
		case WP_ALEVEL3:
			return 0.8f;
		case WP_ALEVEL3_UPG:
			return 0.9f;
		case WP_ALEVEL4:
			return 1.0f;
		default:
			return 0.5f;
	}
}

float BotGetHealScore( gentity_t *self )
{
	float distToHealer = 0;
	float percentHealth = 0;
	float maxHealth = BG_Class( ( team_t ) self->client->ps.persistant[ PERS_TEAM ] )->health;

	percentHealth = ( ( float ) self->client->ps.stats[STAT_HEALTH] ) / maxHealth;

	distToHealer = MAX( MIN( MAX_HEAL_DIST, distToHealer ), MAX_HEAL_DIST * ( 3.0f / 4.0f ) );

	if ( percentHealth == 1.0f )
	{
		return 1.0f;
	}
	return percentHealth * distToHealer / MAX_HEAL_DIST;
}

float BotGetEnemyPriority( gentity_t *self, gentity_t *ent )
{
	float enemyScore;
	float distanceScore;
	distanceScore = Distance( self->s.origin, ent->s.origin );

	switch ( ent->s.weapon )
	{
		case WP_ALEVEL0:
			enemyScore = 0.1;
			break;
		case WP_ALEVEL1:
			enemyScore = 0.3;
			break;
		case WP_ALEVEL2:
			enemyScore = 0.4;
			break;
		case WP_ALEVEL2_UPG:
			enemyScore = 0.7;
			break;
		case WP_ALEVEL3:
			enemyScore = 0.7;
			break;
		case WP_ALEVEL3_UPG:
			enemyScore = 0.8;
			break;
		case WP_ALEVEL4:
			enemyScore = 1.0;
			break;
		case WP_BLASTER:
			enemyScore = 0.2;
			break;
		case WP_MACHINEGUN:
			enemyScore = 0.4;
			break;
		case WP_PAIN_SAW:
			enemyScore = 0.4;
			break;
		case WP_LAS_GUN:
			enemyScore = 0.4;
			break;
		case WP_MASS_DRIVER:
			enemyScore = 0.4;
			break;
		case WP_CHAINGUN:
			enemyScore = 0.6;
			break;
		case WP_FLAMER:
			enemyScore = 0.6;
			break;
		case WP_PULSE_RIFLE:
			enemyScore = 0.5;
			break;
		case WP_LUCIFER_CANNON:
			enemyScore = 1.0;
			break;
		default:
			enemyScore = 0.5;
			break;
	}
	return enemyScore * 1000 / distanceScore;
}


bool WeaponIsEmpty( weapon_t weapon, playerState_t *ps )
{
	if ( ps->ammo <= 0 && ps->clips <= 0 && !BG_Weapon( weapon )->infiniteAmmo )
	{
		return true;
	}
	else
	{
		return false;
	}
}

float PercentAmmoRemaining( weapon_t weapon, playerState_t *ps )
{
	int maxAmmo, maxClips;
	float totalMaxAmmo, totalAmmo;

	maxAmmo = BG_Weapon( weapon )->maxAmmo;
	maxClips = BG_Weapon( weapon )->maxClips;

	if ( !BG_Weapon( weapon )->infiniteAmmo )
	{
		totalMaxAmmo = ( float ) maxAmmo + maxClips * maxAmmo;
		totalAmmo = ( float ) ps->ammo + ps->clips * maxAmmo;

		return totalAmmo / totalMaxAmmo;
	}
	else
	{
		return 1.0f;
	}
}

int BotValueOfWeapons( gentity_t *self )
{
	int worth = 0;
	int i;

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		if ( BG_InventoryContainsWeapon( i, self->client->ps.stats ) )
		{
			worth += BG_Weapon( ( weapon_t )i )->price;
		}
	}
	return worth;
}
int BotValueOfUpgrades( gentity_t *self )
{
	int worth = 0;
	int i;

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( BG_InventoryContainsUpgrade( i, self->client->ps.stats ) )
		{
			worth += BG_Upgrade( ( upgrade_t ) i )->price;
		}
	}
	return worth;
}

bool BotEntityIsVisible( gentity_t *self, gentity_t *target, int mask )
{
	botTarget_t bt;
	BotSetTarget( &bt, target, nullptr );
	return BotTargetIsVisible( self, bt, mask );
}

gentity_t* BotFindBestEnemy( gentity_t *self )
{
	float bestVisibleEnemyScore = 0;
	float bestInvisibleEnemyScore = 0;
	gentity_t *bestVisibleEnemy = nullptr;
	gentity_t *bestInvisibleEnemy = nullptr;
	gentity_t *target;
	team_t    team = BotGetEntityTeam( self );
	bool  hasRadar = ( team == TEAM_Q ) ||
	                     ( team == TEAM_U && BG_InventoryContainsUpgrade( UP_RADAR, self->client->ps.stats ) );

	for ( target = g_entities; target < &g_entities[level.num_entities - 1]; target++ )
	{
		float newScore;

		if ( !BotEnemyIsValid( self, target ) )
		{
			continue;
		}

		if ( target->s.eType == ET_PLAYER && self->client->pers.team == TEAM_U
		    && BotAimAngle( self, target->s.origin ) > g_bot_fov.value / 2 )
		{
			continue;
		}

		if ( target == self->botMind->goal.ent )
		{
			continue;
		}

		newScore = BotGetEnemyPriority( self, target );

		if ( newScore > bestVisibleEnemyScore && BotEntityIsVisible( self, target, MASK_SHOT ) )
		{
			//store the new score and the index of the entity
			bestVisibleEnemyScore = newScore;
			bestVisibleEnemy = target;
		}
		else if ( newScore > bestInvisibleEnemyScore && hasRadar )
		{
			bestInvisibleEnemyScore = newScore;
			bestInvisibleEnemy = target;
		}
	}
	if ( bestVisibleEnemy || !hasRadar )
	{
		return bestVisibleEnemy;
	}
	else
	{
		return bestInvisibleEnemy;
	}
}

gentity_t* BotFindClosestEnemy( gentity_t *self )
{
	gentity_t* closestEnemy = nullptr;
	gentity_t *target;

	for ( target = g_entities; target < &g_entities[level.num_entities - 1]; target++ )
	{
		float newDistance;
		//ignore entities that arnt in use
		if ( !target->inuse )
		{
			continue;
		}

		//ignore dead targets
		if ( target->health <= 0 )
		{
			continue;
		}

		//ignore neutrals
		if ( BotGetEntityTeam( target ) == TEAM_NONE )
		{
			continue;
		}

		//ignore teamates
		if ( BotGetEntityTeam( target ) == BotGetEntityTeam( self ) )
		{
			continue;
		}

		//ignore spectators
		if ( target->client )
		{
			if ( target->client->sess.spectatorState != SPECTATOR_NOT )
			{
				continue;
			}
		}
		newDistance = DistanceSquared( self->s.origin, target->s.origin );
	}
	return closestEnemy;
}

botTarget_t BotGetRushTarget( gentity_t *self )
{
	botTarget_t target;
	gentity_t* rushTarget = nullptr;

	BotSetTarget( &target, rushTarget, nullptr );
	return target;
}

botTarget_t BotGetRetreatTarget( gentity_t *self )
{
	botTarget_t target;
	gentity_t* retreatTarget = nullptr;

	BotSetTarget( &target, retreatTarget, nullptr );
	return target;
}

botTarget_t BotGetRoamTarget( gentity_t *self )
{
	botTarget_t target;
	vec3_t targetPos;

	BotFindRandomPointOnMesh( self, targetPos );
	BotSetTarget( &target, nullptr, targetPos );
	return target;
}
/*
 = *=======================
 BotTarget Helpers
 ========================
 */

void BotSetTarget( botTarget_t *target, gentity_t *ent, vec3_t pos )
{
	if ( ent )
	{
		target->ent = ent;
		VectorClear( target->coord );
		target->inuse = true;
	}
	else if ( pos )
	{
		target->ent = nullptr;
		VectorCopy( pos, target->coord );
		target->inuse = true;
	}
	else
	{
		target->ent = nullptr;
		VectorClear( target->coord );
		target->inuse = false;
	}
}

bool BotTargetIsEntity( botTarget_t target )
{
	return ( target.ent && target.ent->inuse );
}

bool BotTargetIsPlayer( botTarget_t target )
{
	return ( target.ent && target.ent->inuse && target.ent->client );
}

int BotGetTargetEntityNumber( botTarget_t target )
{
	if ( BotTargetIsEntity( target ) )
	{
		return target.ent->s.number;
	}
	else
	{
		return ENTITYNUM_NONE;
	}
}

void BotGetTargetPos( botTarget_t target, vec3_t rVec )
{
	if ( BotTargetIsEntity( target ) )
	{
		VectorCopy( target.ent->s.origin, rVec );
	}
	else
	{
		VectorCopy( target.coord, rVec );
	}
}

void BotTargetToRouteTarget( gentity_t *self, botTarget_t target, botRouteTarget_t *routeTarget )
{
	vec3_t mins, maxs;
	int i;

	if ( BotTargetIsEntity( target ) )
	{
		if ( target.ent->client )
		{
			BG_ClassBoundingBox( ( team_t ) target.ent->client->ps.persistant[ PERS_TEAM ], mins, maxs, nullptr, nullptr, nullptr );
		}
		else
		{
			VectorCopy( target.ent->r.mins, mins );
			VectorCopy( target.ent->r.maxs, maxs );
		}

		if ( BotTargetIsPlayer( target ) )
		{
			routeTarget->type = BOT_TARGET_DYNAMIC;
		}
		else
		{
			routeTarget->type = BOT_TARGET_STATIC;
		}
	}
	else
	{
		// point target
		VectorSet( maxs, 96, 96, 96 );
		VectorSet( mins, -96, -96, -96 );
		routeTarget->type = BOT_TARGET_STATIC;
	}
	
	for ( i = 0; i < 3; i++ )
	{
		routeTarget->polyExtents[ i ] = MAX( Q_fabs( mins[ i ] ), maxs[ i ] );
	}

	BotGetTargetPos( target, routeTarget->pos );

	// move center a bit lower so we don't get polys above the object
	// and get polys below the object on a slope
	routeTarget->pos[ 2 ] -= routeTarget->polyExtents[ 2 ] / 2;

	// account for buildings on walls or cielings
	if ( BotTargetIsEntity( target ) )
	{
		if ( target.ent->s.eType == ET_PLAYER )
		{
			// building on wall or cieling ( 0.7 == MIN_WALK_NORMAL )
			if ( target.ent->s.origin2[ 2 ] < 0.7 || target.ent->s.eType == ET_PLAYER )
			{
				vec3_t targetPos;
				vec3_t end;
				vec3_t invNormal = { 0, 0, -1 };
				trace_t trace;

				routeTarget->polyExtents[ 0 ] += 25;
				routeTarget->polyExtents[ 1 ] += 25;
				routeTarget->polyExtents[ 2 ] += 300;

				// try to find a position closer to the ground
				BotGetTargetPos( target, targetPos );
				VectorMA( targetPos, 600, invNormal, end );
				trap_Trace( &trace, targetPos, mins, maxs, end, target.ent->s.number,
				            CONTENTS_SOLID, MASK_ENTITY );
				VectorCopy( trace.endpos, routeTarget->pos );
			}
		}
	}

	// increase extents a little to account for obstacles cutting into the navmesh
	// also accounts for navmesh erosion at mesh boundrys
	routeTarget->polyExtents[ 0 ] += self->r.maxs[ 0 ] + 10;
	routeTarget->polyExtents[ 1 ] += self->r.maxs[ 1 ] + 10;
}

team_t BotGetEntityTeam( gentity_t *ent )
{
	if ( !ent )
	{
		return TEAM_NONE;
	}
	if ( ent->client )
	{
		return ( team_t )ent->client->pers.team;
	}
	else
	{
		return TEAM_NONE;
	}
}

team_t BotGetTargetTeam( botTarget_t target )
{
	if ( BotTargetIsEntity( target ) )
	{
		return BotGetEntityTeam( target.ent );
	}
	else
	{
		return TEAM_NONE;
	}
}

int BotGetTargetType( botTarget_t target )
{
	if ( BotTargetIsEntity( target ) )
	{
		return target.ent->s.eType;
	}
	else
	{
		return -1;
	}
}

bool BotChangeGoal( gentity_t *self, botTarget_t target )
{
	if ( !target.inuse )
	{
		return false;
	}

	if ( !FindRouteToTarget( self, target, false ) )
	{
		return false;
	}

	self->botMind->goal = target;
	self->botMind->nav.directPathToGoal = false;
	return true;
}

bool BotChangeGoalEntity( gentity_t *self, gentity_t *goal )
{
	botTarget_t target;
	BotSetTarget( &target, goal, nullptr );
	return BotChangeGoal( self, target );
}

bool BotChangeGoalPos( gentity_t *self, vec3_t goal )
{
	botTarget_t target;
	BotSetTarget( &target, nullptr, goal );
	return BotChangeGoal( self, target );
}

bool BotTargetInAttackRange( gentity_t *self, botTarget_t target )
{
	float range, secondaryRange;
	vec3_t forward, right, up;
	vec3_t muzzle;
	vec3_t maxs, mins;
	vec3_t targetPos;
	trace_t trace;
	float width = 0, height = 0;

	AngleVectors( self->client->ps.viewangles, forward, right, up );
	G_CalcMuzzlePoint( self, forward, right, up , muzzle );
	BotGetTargetPos( target, targetPos );
	switch ( self->client->ps.weapon )
	{
		case WP_ABUILD:
			range = ABUILDER_CLAW_RANGE;
			secondaryRange = 0;
			width = height = ABUILDER_CLAW_WIDTH;
			break;
		case WP_ABUILD2:
			range = ABUILDER_CLAW_RANGE;
			secondaryRange = 300; //An arbitrary value for the blob launcher, has nothing to do with actual range
			width = height = ABUILDER_CLAW_WIDTH;
			break;
		case WP_ALEVEL0:
			range = LEVEL0_BITE_RANGE;
			secondaryRange = 0;
			break;
		case WP_ALEVEL1:
			range = LEVEL1_CLAW_RANGE;
			secondaryRange = LEVEL1_POUNCE_DISTANCE;
			width = height = LEVEL1_CLAW_WIDTH;
			break;
		case WP_ALEVEL2:
			range = LEVEL2_CLAW_RANGE;
			secondaryRange = 0;
			width = height = LEVEL2_CLAW_WIDTH;
			break;
		case WP_ALEVEL2_UPG:
			range = LEVEL2_CLAW_U_RANGE;
			secondaryRange = LEVEL2_AREAZAP_RANGE;
			width = height = LEVEL2_CLAW_WIDTH;
			break;
		case WP_ALEVEL3:
			range = LEVEL3_CLAW_RANGE;
			//need to check if we can pounce to the target
			secondaryRange = LEVEL3_POUNCE_JUMP_MAG; //An arbitrary value for pounce, has nothing to do with actual range
			width = height = LEVEL3_CLAW_WIDTH;
			break;
		case WP_ALEVEL3_UPG:
			range = LEVEL3_CLAW_RANGE;
			//we can pounce, or we have barbs
			secondaryRange = LEVEL3_POUNCE_JUMP_MAG_UPG; //An arbitrary value for pounce and barbs, has nothing to do with actual range
			if ( self->client->ps.ammo > 0 )
			{
				secondaryRange = 900;
			}
			width = height = LEVEL3_CLAW_WIDTH;
			break;
		case WP_ALEVEL4:
			range = LEVEL4_CLAW_RANGE;
			secondaryRange = 0; //Using 0 since tyrant rush is basically just movement, not a ranged attack
			width = height = LEVEL4_CLAW_WIDTH;
			break;
		case WP_HBUILD:
			range = 100;
			secondaryRange = 0;
			break;
		case WP_PAIN_SAW:
			range = PAINSAW_RANGE;
			secondaryRange = 0;
			break;
		case WP_FLAMER:
			{
				vec3_t dir;
				vec3_t rdir;
				vec3_t nvel;
				vec3_t npos;
				vec3_t proj;
				trajectory_t t;
			
				// Correct muzzle so that the missile does not start in the ceiling
				VectorMA( muzzle, -7.0f, up, muzzle );

				// Correct muzzle so that the missile fires from the player's hand
				VectorMA( muzzle, 4.5f, right, muzzle );

				// flamer projectiles add the player's velocity scaled by FLAMER_LAG to the fire direction with length FLAMER_SPEED
				VectorSubtract( targetPos, muzzle, dir );
				VectorNormalize( dir );
				VectorScale( self->client->ps.velocity, FLAMER_LAG, nvel );
				VectorMA( nvel, FLAMER_SPEED, dir, t.trDelta );
				SnapVector( t.trDelta );
				VectorCopy( muzzle, t.trBase );
				t.trType = TR_LINEAR;
				t.trTime = level.time - 50;
			
				// find projectile's final position
				BG_EvaluateTrajectory( &t, level.time + FLAMER_LIFETIME, npos );

				// find distance traveled by projectile along fire line
				ProjectPointOntoVector( npos, muzzle, targetPos, proj );
				range = Distance( muzzle, proj );

				// make sure the sign of the range is correct
				VectorSubtract( npos, muzzle, rdir );
				VectorNormalize( rdir );
				if ( DotProduct( rdir, dir ) < 0 )
				{
					range = -range;
				}

				range -= 150;

				range = MAX( range, 100.0f );
				secondaryRange = 0;
				width = height = FLAMER_SIZE;
			}
			break;
		case WP_SHOTGUN:
			range = ( 50 * 8192 ) / SHOTGUN_SPREAD; //50 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		case WP_MACHINEGUN:
			range = ( 100 * 8192 ) / RIFLE_SPREAD; //100 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		case WP_CHAINGUN:
			range = ( 60 * 8192 ) / CHAINGUN_SPREAD; //60 is the maximum radius we want the spread to be
			secondaryRange = 0;
			break;
		default:
			range = 4098 * 4; //large range for guns because guns have large ranges :)
			secondaryRange = 0; //no secondary attack
	}
	VectorSet( maxs, width, width, width );
	VectorSet( mins, -width, -width, -height );

	trap_Trace( &trace, muzzle, mins, maxs, targetPos, self->s.number, MASK_SHOT, 0 );

	if ( self->client->pers.team != BotGetEntityTeam( &g_entities[trace.entityNum] )
		&& BotGetEntityTeam( &g_entities[ trace.entityNum ] ) != TEAM_NONE
		&& Distance( muzzle, trace.endpos ) <= MAX( range, secondaryRange ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool BotTargetIsVisible( gentity_t *self, botTarget_t target, int mask )
{
	trace_t trace;
	vec3_t  muzzle, targetPos;
	vec3_t  forward, right, up;

	AngleVectors( self->client->ps.viewangles, forward, right, up );
	G_CalcMuzzlePoint( self, forward, right, up, muzzle );
	BotGetTargetPos( target, targetPos );

	if ( !trap_InPVS( muzzle, targetPos ) )
	{
		return false;
	}

	trap_Trace( &trace, muzzle, nullptr, nullptr, targetPos, self->s.number, mask,
	            ( mask == CONTENTS_SOLID ) ? MASK_ENTITY : 0 );

	if ( trace.surfaceFlags & SURF_NOIMPACT )
	{
		return false;
	}

	//target is in range
	if ( ( trace.entityNum == BotGetTargetEntityNumber( target ) || trace.fraction == 1.0f ) &&
	     !trace.startsolid )
	{
		return true;
	}
	return false;
}
/*
 = *=======================
 Bot Aiming
 ========================
 */
void BotGetIdealAimLocation( gentity_t *self, botTarget_t target, vec3_t aimLocation )
{
	//get the position of the target
	BotGetTargetPos( target, aimLocation );

	if ( BotGetTargetTeam( target ) == TEAM_Q )
	{
		//make lucifer cannons aim ahead based on the target's velocity
		if ( self->client->ps.weapon == WP_LUCIFER_CANNON && self->botMind->botSkill.level >= 5 )
		{
			VectorMA( aimLocation, Distance( self->s.origin, aimLocation ) / LCANNON_SPEED, target.ent->s.pos.trDelta, aimLocation );
		}
	}
	else if ( BotTargetIsEntity( target ) && BotGetTargetTeam( target ) == TEAM_U )
	{

		//aim at head
		aimLocation[2] += target.ent->r.maxs[2] * 0.85;

	}
}

int BotGetAimPredictionTime( gentity_t *self )
{
	return ( 10 - self->botMind->botSkill.level ) * 100 * MAX( ( ( float ) rand() ) / RAND_MAX, 0.5f );
}

void BotPredictPosition( gentity_t *self, gentity_t *predict, vec3_t pos, int time )
{
	botTarget_t target;
	vec3_t aimLoc;
	BotSetTarget( &target, predict, nullptr );
	BotGetIdealAimLocation( self, target, aimLoc );
	VectorMA( aimLoc, time / 1000.0f, predict->s.apos.trDelta, pos );
}

void BotAimAtEnemy( gentity_t *self )
{
	vec3_t desired;
	vec3_t current;
	vec3_t viewOrigin;
	vec3_t newAim;
	vec3_t angles;
	int i;
	float frac;
	gentity_t *enemy = self->botMind->goal.ent;

	if ( self->botMind->futureAimTime < level.time )
	{
		int predictTime = self->botMind->futureAimTimeInterval = BotGetAimPredictionTime( self );
		BotPredictPosition( self, enemy, self->botMind->futureAim, predictTime );
		self->botMind->futureAimTime = level.time + predictTime;
	}

	BG_GetClientViewOrigin( &self->client->ps, viewOrigin );
	VectorSubtract( self->botMind->futureAim, viewOrigin, desired );
	VectorNormalize( desired );
	AngleVectors( self->client->ps.viewangles, current, nullptr, nullptr );

	frac = ( 1.0f - ( ( float ) ( self->botMind->futureAimTime - level.time ) ) / self->botMind->futureAimTimeInterval );
	VectorLerp( current, desired, frac, newAim );

	VectorSet( self->client->ps.delta_angles, 0, 0, 0 );
	vectoangles( newAim, angles );

	for ( i = 0; i < 3; i++ )
	{
		self->botMind->cmdBuffer.angles[ i ] = ANGLE2SHORT( angles[ i ] );
	}
}

void BotAimAtLocation( gentity_t *self, vec3_t target )
{
	vec3_t aimVec, aimAngles, viewBase;
	int i;
	usercmd_t *rAngles = &self->botMind->cmdBuffer;

	if ( !self->client )
	{
		return;
	}

	BG_GetClientViewOrigin( &self->client->ps, viewBase );
	VectorSubtract( target, viewBase, aimVec );

	vectoangles( aimVec, aimAngles );

	VectorSet( self->client->ps.delta_angles, 0.0f, 0.0f, 0.0f );

	for ( i = 0; i < 3; i++ )
	{
		aimAngles[i] = ANGLE2SHORT( aimAngles[i] );
	}

	//save bandwidth
	SnapVector( aimAngles );
	rAngles->angles[0] = aimAngles[0];
	rAngles->angles[1] = aimAngles[1];
	rAngles->angles[2] = aimAngles[2];
}

void BotSlowAim( gentity_t *self, vec3_t target, float slowAmount )
{
	vec3_t viewBase;
	vec3_t aimVec, forward;
	vec3_t skilledVec;
	float length;
	float slow;
	float cosAngle;

	if ( !( self && self->client ) )
	{
		return;
	}
	//clamp to 0-1
	slow = Com_Clamp( 0.1f, 1.0, slowAmount );

	//get the point that the bot is aiming from
	BG_GetClientViewOrigin( &self->client->ps, viewBase );

	//get the Vector from the bot to the enemy (ideal aim Vector)
	VectorSubtract( target, viewBase, aimVec );
	length = VectorNormalize( aimVec );

	//take the current aim Vector
	AngleVectors( self->client->ps.viewangles, forward, nullptr, nullptr );

	cosAngle = DotProduct( forward, aimVec );
	cosAngle = ( cosAngle + 1 ) / 2;
	cosAngle = 1 - cosAngle;
	cosAngle = Com_Clamp( 0.1, 0.5, cosAngle );
	VectorLerp( forward, aimVec, slow * ( cosAngle ), skilledVec );

	//now find a point to return, this point will be aimed at
	VectorMA( viewBase, length, skilledVec, target );
}

float BotAimAngle( gentity_t *self, vec3_t pos )
{
	vec3_t viewPos;
	vec3_t forward;
	vec3_t ideal;

	AngleVectors( self->client->ps.viewangles, forward, nullptr, nullptr );
	BG_GetClientViewOrigin( &self->client->ps, viewPos );
	VectorSubtract( pos, viewPos, ideal );

	return AngleBetweenVectors( forward, ideal );
}

/*
 = *=======================
 Bot Team Querys
 ========================
 */

int FindBots( int *botEntityNumbers, int maxBots, team_t team )
{
	gentity_t *testEntity;
	int numBots = 0;
	int i;
	memset( botEntityNumbers, 0, sizeof( int )*maxBots );
	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		testEntity = &g_entities[i];
		if ( testEntity->r.svFlags & SVF_BOT )
		{
			if ( testEntity->client->pers.team == team && numBots < maxBots )
			{
				botEntityNumbers[numBots++] = i;
			}
		}
	}
	return numBots;
}

bool PlayersBehindBotInSpawnQueue( gentity_t *self )
{
	//this function only checks if there are Humans in the SpawnQueue
	//which are behind the bot
	int i;
	int botPos = 0, lastPlayerPos = 0;
	spawnQueue_t *sq;

	if ( self->client->pers.team > TEAM_NONE &&
	     self->client->pers.team < NUM_TEAMS )
	{
		sq = &level.team[ self->client->pers.team ].spawnQueue;
	}
	else
	{
		return false;
	}

	i = sq->front;

	if ( G_GetSpawnQueueLength( sq ) )
	{
		do
		{
			if ( !( g_entities[sq->clients[ i ]].r.svFlags & SVF_BOT ) )
			{
				if ( i < sq->front )
				{
					lastPlayerPos = i + MAX_CLIENTS - sq->front;
				}
				else
				{
					lastPlayerPos = i - sq->front;
				}
			}

			if ( sq->clients[ i ] == self->s.number )
			{
				if ( i < sq->front )
				{
					botPos = i + MAX_CLIENTS - sq->front;
				}
				else
				{
					botPos = i - sq->front;
				}
			}

			i = QUEUE_PLUS1( i );
		}
		while ( i != QUEUE_PLUS1( sq->back ) );
	}

	if ( botPos < lastPlayerPos )
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool BotTeamateHasWeapon( gentity_t *self, int weapon )
{
	int botNumbers[MAX_CLIENTS];
	int i;
	int numBots = FindBots( botNumbers, MAX_CLIENTS, ( team_t ) self->client->pers.team );

	for ( i = 0; i < numBots; i++ )
	{
		gentity_t *bot = &g_entities[botNumbers[i]];
		if ( bot == self )
		{
			continue;
		}
		if ( BG_InventoryContainsWeapon( weapon, bot->client->ps.stats ) )
		{
			return true;
		}
	}
	return false;
}

/*
 = *=======================
 Misc Bot Stuff
 ========================
 */
void BotFireWeapon( weaponMode_t mode, usercmd_t *botCmdBuffer )
{
	if ( mode == WPM_PRIMARY )
	{
		usercmdPressButton( botCmdBuffer->buttons, BUTTON_ATTACK );
	}
	else if ( mode == WPM_SECONDARY )
	{
		usercmdPressButton( botCmdBuffer->buttons, BUTTON_ATTACK2 );
	}
	else if ( mode == WPM_TERTIARY )
	{
		usercmdPressButton( botCmdBuffer->buttons, BUTTON_USE_HOLDABLE );
	}
}
void BotClassMovement( gentity_t *self, bool inAttackRange )
{
	switch ( self->client->ps.persistant[ PERS_TEAM ] )
	{
		case TEAM_Q:
			BotStrafeDodge( self );
			break;
		default:
			break;
	}
}

float CalcAimPitch( gentity_t *self, botTarget_t target, vec_t launchSpeed )
{
	vec3_t startPos;
	vec3_t targetPos;
	float initialHeight;
	vec3_t forward, right, up;
	vec3_t muzzle;
	float distance2D;
	float x, y, v, g;
	float check;
	float angle1, angle2, angle;

	BotGetTargetPos( target, targetPos );
	AngleVectors( self->s.origin, forward, right, up );
	G_CalcMuzzlePoint( self, forward, right, up, muzzle );
	VectorCopy( muzzle, startPos );

	//project everything onto a 2D plane with initial position at (0,0)
	initialHeight = startPos[2];
	targetPos[2] -= initialHeight;
	startPos[2] -= initialHeight;
	distance2D = sqrt( Square( startPos[0] - targetPos[0] ) + Square( startPos[1] - targetPos[1] ) );
	targetPos[0] = distance2D;

	//for readability's sake
	x = targetPos[0];
	y = targetPos[2];
	v = launchSpeed;
	g = self->client->ps.gravity;

	//make sure we won't get NaN
	check = Square( Square( v ) ) - g * ( g * Square( x ) + 2 * y * Square( v ) );

	//as long as we will get NaN, increase velocity to compensate
	//This is better than returning some failure value because it gives us the best launch angle possible, even if it wont hit in the end.
	while ( check < 0 )
	{
		v += 5;
		check = Square( Square( v ) ) - g * ( g * Square( x ) + 2 * y * Square( v ) );
	}

	//calculate required angle of launch
	angle1 = atanf( ( Square( v ) + sqrt( check ) ) / ( g * x ) );
	angle2 = atanf( ( Square( v ) - sqrt( check ) ) / ( g * x ) );

	//take the smaller angle
	angle = ( angle1 < angle2 ) ? angle1 : angle2;

	//convert to degrees (ps.viewangles units)
	angle = RAD2DEG( angle );
	return angle;
}

void BotFireWeaponAI( gentity_t *self )
{
	float distance;
	vec3_t targetPos;
	vec3_t forward, right, up;
	vec3_t muzzle;
	trace_t trace;
	usercmd_t *botCmdBuffer = &self->botMind->cmdBuffer;

	AngleVectors( self->client->ps.viewangles, forward, right, up );
	G_CalcMuzzlePoint( self, forward, right, up, muzzle );
	BotGetIdealAimLocation( self, self->botMind->goal, targetPos );

	trap_Trace( &trace, muzzle, nullptr, nullptr, targetPos, ENTITYNUM_NONE, MASK_SHOT, 0 );
	distance = Distance( muzzle, trace.endpos );
	switch ( self->s.weapon )
	{
		case WP_ABUILD:
			if ( distance <= ABUILDER_CLAW_RANGE )
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );
			}
			else
			{
				usercmdPressButton( botCmdBuffer->buttons, BUTTON_GESTURE );    //make cute granger sounds to ward off the would be attackers
			}
			break;
		case WP_ABUILD2:
			if ( distance <= ABUILDER_CLAW_RANGE )
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );    //swipe
			}
			else
			{
				BotFireWeapon( WPM_TERTIARY, botCmdBuffer );    //blob launcher
			}
			break;
		case WP_ALEVEL0:
			break; //auto hit
		case WP_ALEVEL1:
			if ( distance < LEVEL1_CLAW_RANGE )
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer ); //mantis swipe
			}
			else if ( self->client->ps.stats[ STAT_MISC ] == 0 )
			{
				BotMoveInDir( self, MOVE_FORWARD );
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer ); //mantis forward pounce
			}
			break;
		case WP_ALEVEL2:
			BotFireWeapon( WPM_PRIMARY, botCmdBuffer ); //mara swipe
			break;
		case WP_ALEVEL2_UPG:
			if ( distance <= LEVEL2_CLAW_U_RANGE )
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );    //mara swipe
			}
			else
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );    //mara lightning
			}
			break;
		case WP_ALEVEL3:
			BotFireWeapon( WPM_PRIMARY, botCmdBuffer );    //goon chomp
			break;
		case WP_ALEVEL3_UPG:
			BotFireWeapon( WPM_PRIMARY, botCmdBuffer );    //goon chomp
			break;
		case WP_ALEVEL4:
			if ( distance > LEVEL4_CLAW_RANGE && self->client->ps.stats[STAT_MISC] < LEVEL4_TRAMPLE_CHARGE_MAX )
			{
				BotFireWeapon( WPM_SECONDARY, botCmdBuffer );    //rant charge
			}
			else
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );    //rant swipe
			}
			break;
		case WP_LUCIFER_CANNON:
			if ( self->client->ps.stats[STAT_MISC] < LCANNON_CHARGE_TIME_MAX * Com_Clamp( 0.5, 1, random() ) )
			{
				BotFireWeapon( WPM_PRIMARY, botCmdBuffer );
			}
			break;
		default:
			BotFireWeapon( WPM_PRIMARY, botCmdBuffer );
	}
}

void BotSetSkillLevel( gentity_t *self, int skill )
{
	self->botMind->botSkill.level = skill;
	//different aim for different teams
	if ( self->botMind->botTeam == TEAM_U )
	{
		self->botMind->botSkill.aimSlowness = ( float ) skill / 10;
		self->botMind->botSkill.aimShake = 10 - skill;
	}
	else
	{
		self->botMind->botSkill.aimSlowness = ( float ) skill / 10;
		self->botMind->botSkill.aimShake = 10 - skill;
	}
}

void BotResetEnemyQueue( enemyQueue_t *queue )
{
	queue->front = 0;
	queue->back = 0;
	memset( queue->enemys, 0, sizeof( queue->enemys ) );
}

void BotPushEnemy( enemyQueue_t *queue, gentity_t *enemy )
{
	if ( enemy )
	{
		if ( ( queue->back + 1 ) % MAX_ENEMY_QUEUE != queue->front )
		{
			queue->enemys[ queue->back ].ent = enemy;
			queue->enemys[ queue->back ].timeFound = level.time;
			queue->back = ( queue->back + 1 ) % MAX_ENEMY_QUEUE;
		}
	}
}

gentity_t *BotPopEnemy( enemyQueue_t *queue )
{
	// queue empty
	if ( queue->front == queue->back )
	{
		return nullptr;
	}

	if ( level.time - queue->enemys[ queue->front ].timeFound >= g_bot_reactiontime.integer )
	{
		gentity_t *ret = queue->enemys[ queue->front ].ent;
		queue->front = ( queue->front + 1 ) % MAX_ENEMY_QUEUE;
		return ret;
	}

	return nullptr;
}

bool BotEnemyIsValid( gentity_t *self, gentity_t *enemy )
{
	if ( !enemy->inuse )
	{
		return false;
	}

	if ( enemy->health <= 0 )
	{
		return false;
	}

	if ( BotGetEntityTeam( enemy ) == self->client->pers.team )
	{
		return false;
	}

	if ( BotGetEntityTeam( enemy ) == TEAM_NONE )
	{
		return false;
	}

	if ( enemy->client && enemy->client->sess.spectatorState != SPECTATOR_NOT )
	{
		return false;
	}

	return true;
}

void BotPain( gentity_t *self, gentity_t *attacker, int damage )
{
	if ( BotGetEntityTeam( attacker ) != TEAM_NONE && BotGetEntityTeam( attacker ) != self->client->pers.team )
	{
		if ( attacker->s.eType == ET_PLAYER )
		{
			BotPushEnemy( &self->botMind->enemyQueue, attacker );
		}
	}
}

void BotSearchForEnemy( gentity_t *self )
{
	gentity_t *enemy = BotFindBestEnemy( self );
	enemyQueue_t *queue = &self->botMind->enemyQueue;
	BotPushEnemy( queue, enemy );

	do
	{
		enemy = BotPopEnemy( queue );
	} while ( enemy && !BotEnemyIsValid( self, enemy ) );

	self->botMind->bestEnemy.ent = enemy;

	if ( self->botMind->bestEnemy.ent ) 
	{
		self->botMind->bestEnemy.distance = Distance( self->s.origin, self->botMind->bestEnemy.ent->s.origin );
	}
	else
	{
		self->botMind->bestEnemy.distance = INT_MAX;
	}
}