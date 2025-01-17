/*
 * Daemon GPL Source Code
 * Copyright (C) 2015-2016  Unreal Arena
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


#include "sg_local.h"
#include "CBSE.h"

/*
================
G_TeamFromString

Return the team referenced by a string
================
*/
team_t G_TeamFromString( const char *str )
{
	switch ( tolower( *str ) )
	{
		case '0':
		case 's':
			return TEAM_NONE;

#ifdef UNREALARENA
		case '1':
		case 'q':
			return TEAM_Q;

		case '2':
		case 'u':
			return TEAM_U;
#else
		case '1':
		case 'a':
			return TEAM_ALIENS;

		case '2':
		case 'h':
			return TEAM_HUMANS;
#endif

		default:
			return NUM_TEAMS;
	}
}

/*
================
G_TeamCommand

Broadcasts a command to only a specific team
================
*/
void G_TeamCommand( team_t team, const char *cmd )
{
	int i;

	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( level.clients[ i ].pers.connected == CON_CONNECTED )
		{
			if ( level.clients[ i ].pers.team == team ||
			     ( level.clients[ i ].pers.team == TEAM_NONE &&
			       G_admin_permission( &g_entities[ i ], ADMF_SPEC_ALLCHAT ) ) )
			{
				trap_SendServerCommand( i, cmd );
			}
		}
	}
}

/*
================
G_AreaTeamCommand

Broadcasts a command to only a specific team within a specific range
================
*/
void G_AreaTeamCommand( gentity_t *ent, const char *cmd )
{
	int    entityList[ MAX_GENTITIES ];
	int    num, i;
	vec3_t range = { 1000.0f, 1000.0f, 1000.0f };
	vec3_t mins, maxs;
	team_t team = (team_t) ent->client->pers.team;

	for ( i = 0; i < 3; i++ )
	{
		range[ i ] = g_sayAreaRange.value;
	}

	VectorAdd( ent->s.origin, range, maxs );
	VectorSubtract( ent->s.origin, range, mins );

	num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0; i < num; i++ )
	{
		if ( g_entities[ entityList[ i ] ].client && g_entities[ entityList[ i ] ].client->pers.connected == CON_CONNECTED )
		{
			if ( g_entities[ entityList[ i ] ].client->pers.team == team )
			{
				trap_SendServerCommand( entityList[ i ], cmd );
			}
		}
	}
}

team_t G_Team( gentity_t *ent )
{
	if ( ent->client )
	{
		return (team_t)ent->client->pers.team;
	}
#ifndef UNREALARENA
	else if ( ent->s.eType == ET_BUILDABLE )
	{
		return ent->buildableTeam;
	}
#endif
	else
	{
		return TEAM_NONE;
	}
}

bool G_OnSameTeam( gentity_t *ent1, gentity_t *ent2 )
{
	team_t team1 = G_Team( ent1 );
	return ( team1 != TEAM_NONE && team1 == G_Team( ent2 ) );
}

/*
==================
G_ClientListForTeam
==================
*/
static clientList_t G_ClientListForTeam( team_t team )
{
	int          i;
	clientList_t clientList;

	Com_Memset( &clientList, 0, sizeof( clientList_t ) );

	for ( i = 0; i < level.maxclients; i++ )
	{
		gentity_t *ent = g_entities + i;

		if ( ent->client->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if ( ent->inuse && ( ent->client->pers.team == team ) )
		{
			Com_ClientListAdd( &clientList, ent->client->ps.clientNum );
		}
	}

	return clientList;
}

/*
==================
G_UpdateTeamConfigStrings
==================
*/
void G_UpdateTeamConfigStrings()
{
#ifdef UNREALARENA
	clientList_t qTeam = G_ClientListForTeam( TEAM_Q );
	clientList_t uTeam = G_ClientListForTeam( TEAM_U );

	if ( level.intermissiontime )
	{
		// No restrictions once the game has ended
		Com_Memset( &qTeam, 0, sizeof( clientList_t ) );
		Com_Memset( &uTeam, 0, sizeof( clientList_t ) );
	}

	trap_SetConfigstringRestrictions( CS_VOTE_TIME + TEAM_Q,   &uTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_STRING + TEAM_Q, &uTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_YES + TEAM_Q,    &uTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_NO + TEAM_Q,     &uTeam );

	trap_SetConfigstringRestrictions( CS_VOTE_TIME + TEAM_U,   &qTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_STRING + TEAM_U, &qTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_YES + TEAM_U,    &qTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_NO + TEAM_U,     &qTeam );
#else
	clientList_t alienTeam = G_ClientListForTeam( TEAM_ALIENS );
	clientList_t humanTeam = G_ClientListForTeam( TEAM_HUMANS );

	if ( level.intermissiontime )
	{
		// No restrictions once the game has ended
		Com_Memset( &alienTeam, 0, sizeof( clientList_t ) );
		Com_Memset( &humanTeam, 0, sizeof( clientList_t ) );
	}

	trap_SetConfigstringRestrictions( CS_VOTE_TIME + TEAM_ALIENS,   &humanTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_STRING + TEAM_ALIENS, &humanTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_YES + TEAM_ALIENS,    &humanTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_NO + TEAM_ALIENS,     &humanTeam );

	trap_SetConfigstringRestrictions( CS_VOTE_TIME + TEAM_HUMANS,   &alienTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_STRING + TEAM_HUMANS, &alienTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_YES + TEAM_HUMANS,    &alienTeam );
	trap_SetConfigstringRestrictions( CS_VOTE_NO + TEAM_HUMANS,     &alienTeam );
#endif
}

/*
==================
G_LeaveTeam
==================
*/
void G_LeaveTeam( gentity_t *self )
{
	team_t    team = (team_t) self->client->pers.team;
	gentity_t *ent;
	int       i;

#ifdef UNREALARENA
	if ( TEAM_Q == team || TEAM_U == team )
#else
	if ( TEAM_ALIENS == team || TEAM_HUMANS == team )
#endif
	{
		G_RemoveFromSpawnQueue( &level.team[ team ].spawnQueue, self->client->ps.clientNum );
	}
	else
	{
		if ( self->client->sess.spectatorState == SPECTATOR_FOLLOW )
		{
			G_StopFollowing( self );
		}

		return;
	}

	// stop any following clients
	G_StopFromFollowing( self );

	G_Vote( self, team, false );
#ifndef UNREALARENA
	self->suicideTime = 0;
#endif

	for ( i = 0; i < level.num_entities; i++ )
	{
		ent = &g_entities[ i ];

		if ( !ent->inuse )
		{
			continue;
		}

		if ( ent->client && ent->client->pers.connected == CON_CONNECTED )
		{
#ifndef UNREALARENA
			// cure poison
			if ( ( ent->client->ps.stats[ STAT_STATE ] & SS_POISONED ) &&
			     ent->client->lastPoisonClient == self )
			{
				ent->client->ps.stats[ STAT_STATE ] &= ~SS_POISONED;
			}
#endif
		}
		else if ( ent->s.eType == ET_MISSILE && ent->r.ownerNum == self->s.number )
		{
			G_FreeEntity( ent );
		}
	}

	// cut all relevant zap beams
	G_ClearPlayerZapEffects( self );

	Beacon::DeleteTags( self );
	Beacon::RemoveOrphaned( self->s.number );

	G_namelog_update_score( self->client );
}

/*
=================
G_ChangeTeam
=================
*/
void G_ChangeTeam( gentity_t *ent, team_t newTeam )
{
	team_t oldTeam = (team_t) ent->client->pers.team;

	if ( oldTeam == newTeam )
	{
		return;
	}

	G_LeaveTeam( ent );
	ent->client->pers.teamChangeTime = level.time;
	ent->client->pers.team = newTeam;
	ent->client->pers.teamInfo = level.startTime - 1;
#ifndef UNREALARENA
	ent->client->pers.classSelection = PCL_NONE;
#endif
	ClientSpawn( ent, nullptr, nullptr, nullptr );

#ifndef UNREALARENA
	if ( oldTeam == TEAM_HUMANS && newTeam == TEAM_ALIENS )
	{
		// Convert from human to alien credits
		ent->client->pers.credit =
		  ( int )( ent->client->pers.credit *
		           ALIEN_MAX_CREDITS / HUMAN_MAX_CREDITS + 0.5f );
	}
	else if ( oldTeam == TEAM_ALIENS && newTeam == TEAM_HUMANS )
	{
		// Convert from alien to human credits
		ent->client->pers.credit =
		  ( int )( ent->client->pers.credit *
		           HUMAN_MAX_CREDITS / ALIEN_MAX_CREDITS + 0.5f );
	}
#endif

	if ( !g_cheats.integer )
	{
		if ( ent->client->noclip )
		{
			ent->client->noclip = false;
			ent->r.contents = ent->client->cliprcontents;
		}
		ent->flags &= ~( FL_GODMODE | FL_NOTARGET );
	}

#ifndef UNREALARENA
	// Copy credits to ps for the client
	ent->client->ps.persistant[ PERS_CREDIT ] = ent->client->pers.credit;

	// Update PERS_UNLOCKABLES in the same frame as PERS_TEAM to prevent bad status change notifications
	ent->client->ps.persistant[ PERS_UNLOCKABLES ] = BG_UnlockablesMask( newTeam );
#endif

	ClientUserinfoChanged( ent->client->ps.clientNum, false );

	G_UpdateTeamConfigStrings();

	Beacon::PropagateAll( );

	G_LogPrintf( "ChangeTeam: %d %s: %s" S_COLOR_WHITE " switched teams\n",
	             ( int )( ent - g_entities ), BG_TeamName( newTeam ), ent->client->pers.netname );

	G_namelog_update_score( ent->client );
	TeamplayInfoMessage( ent );
}

/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
gentity_t *Team_GetLocation( gentity_t *ent )
{
	gentity_t *eloc, *best;
	float     bestlen, len;

	best = nullptr;
	bestlen = 3.0f * 8192.0f * 8192.0f;

	for ( eloc = level.locationHead; eloc; eloc = eloc->nextPathSegment )
	{
		len = DistanceSquared( ent->r.currentOrigin, eloc->r.currentOrigin );

		if ( len > bestlen )
		{
			continue;
		}

		if ( !trap_InPVS( ent->r.currentOrigin, eloc->r.currentOrigin ) )
		{
			continue;
		}

		bestlen = len;
		best = eloc;
	}

	return best;
}

/*---------------------------------------------------------------------------*/

/*
==================
TeamplayInfoMessage

Format:
  clientNum location health weapon upgrade

==================
*/
void TeamplayInfoMessage( gentity_t *ent )
{
	char      entry[ 24 ];
	char      string[ ( MAX_CLIENTS - 1 ) * ( sizeof( entry ) - 1 ) + 1 ];
	int       i, j;
	int       team, stringlength;
	gentity_t *player;
	gclient_t *cl;
	upgrade_t upgrade = UP_NONE;
	int       curWeaponClass = WP_NONE; // sends weapon for humans, class for aliens

	if ( !g_allowTeamOverlay.integer )
	{
		return;
	}

	if ( !ent->client->pers.teamInfo )
	{
		return;
	}

	if ( ent->client->pers.team == TEAM_NONE )
	{
		if ( ent->client->sess.spectatorState == SPECTATOR_FREE ||
		     ent->client->sess.spectatorClient < 0 )
		{
			return;
		}

		team = g_entities[ ent->client->sess.spectatorClient ].client->
		       pers.team;
	}
	else
	{
		team = ent->client->pers.team;
	}

	string[ 0 ] = '\0';
	stringlength = 0;

	for ( i = 0; i < level.maxclients; i++ )
	{
		player = g_entities + i;
		cl = player->client;

		if ( ent == player || !cl || team != cl->pers.team ||
		     !player->inuse )
		{
			continue;
		}

		// only update if changed since last time
		if ( cl->pers.infoChangeTime <= ent->client->pers.teamInfo )
		{
			continue;
		}

		if ( cl->sess.spectatorState != SPECTATOR_NOT )
		{
			curWeaponClass = WP_NONE;
			upgrade = UP_NONE;
		}
#ifdef UNREALARENA
		else if ( cl->pers.team == TEAM_U )
#else
		else if ( cl->pers.team == TEAM_HUMANS )
#endif
		{
			curWeaponClass = cl->ps.weapon;

			if ( BG_InventoryContainsUpgrade( UP_BATTLESUIT, cl->ps.stats ) )
			{
				upgrade = UP_BATTLESUIT;
			}
#ifndef UNREALARENA
			else if ( BG_InventoryContainsUpgrade( UP_JETPACK, cl->ps.stats ) )
			{
				upgrade = UP_JETPACK;
			}
			else if ( BG_InventoryContainsUpgrade( UP_RADAR, cl->ps.stats ) )
			{
				upgrade = UP_RADAR;
			}
#endif
			else if ( BG_InventoryContainsUpgrade( UP_LIGHTARMOUR, cl->ps.stats ) )
			{
				upgrade = UP_LIGHTARMOUR;
			}
			else
			{
				upgrade = UP_NONE;
			}
		}
#ifdef UNREALARENA
		else if ( cl->pers.team == TEAM_Q )
		{
			curWeaponClass = cl->ps.persistant[ PERS_TEAM ];
			upgrade = UP_NONE;
		}
#else
		else if ( cl->pers.team == TEAM_ALIENS )
		{
			curWeaponClass = cl->ps.stats[ STAT_CLASS ];
			upgrade = UP_NONE;
		}
#endif

#ifdef UNREALARENA
		if( team == TEAM_Q )
#else
		if( team == TEAM_ALIENS ) // aliens don't have upgrades
#endif
		{
#ifdef UNREALARENA
			Com_sprintf( entry, sizeof( entry ), " %i %i %i %i", i,
						 cl->pers.location,
			             std::max((int)std::ceil(player->entity->Get<HealthComponent>()->Health()), 0),
						 curWeaponClass );
#else
			Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i", i,
						 cl->pers.location,
			             std::max((int)std::ceil(player->entity->Get<HealthComponent>()->Health()), 0),
						 curWeaponClass,
						 cl->pers.credit );
#endif
		}
		else
		{
#ifdef UNREALARENA
			Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i", i,
			             cl->pers.location,
			             std::max((int)std::ceil(player->entity->Get<HealthComponent>()->Health()), 0),
			             curWeaponClass,
			             upgrade );
#else
			Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i", i,
			             cl->pers.location,
			             std::max((int)std::ceil(player->entity->Get<HealthComponent>()->Health()), 0),
			             curWeaponClass,
			             cl->pers.credit,
			             upgrade );
#endif
		}


		j = strlen( entry );

		// this should not happen if entry and string sizes are correct
		if ( stringlength + j >= (int) sizeof( string ) )
		{
			break;
		}

		strcpy( string + stringlength, entry );
		stringlength += j;
	}

	if( string[ 0 ] )
	{
		trap_SendServerCommand( ent - g_entities, va( "tinfo%s", string ) );
		ent->client->pers.teamInfo = level.time;
	}
}

void CheckTeamStatus()
{
	int       i;
	gentity_t *loc, *ent;

	if ( level.time - level.lastTeamLocationTime > TEAM_LOCATION_UPDATE_TIME )
	{
		level.lastTeamLocationTime = level.time;

		for ( i = 0; i < level.maxclients; i++ )
		{
			ent = g_entities + i;

			if ( ent->client->pers.connected != CON_CONNECTED )
			{
				continue;
			}

#ifdef UNREALARENA
			if ( ent->inuse && ( ent->client->pers.team == TEAM_U ||
			                     ent->client->pers.team == TEAM_Q ) )
#else
			if ( ent->inuse && ( ent->client->pers.team == TEAM_HUMANS ||
			                     ent->client->pers.team == TEAM_ALIENS ) )
#endif
			{
				loc = Team_GetLocation( ent );

				if ( loc )
				{
					if( ent->client->pers.location != loc->s.generic1 )
					{
						ent->client->pers.infoChangeTime = level.time;
						ent->client->pers.location = loc->s.generic1;
					}
				}
				else if ( ent->client->pers.location != 0 )
				{
					ent->client->pers.infoChangeTime = level.time;
					ent->client->pers.location = 0;
				}
			}
		}

		for ( i = 0; i < level.maxclients; i++ )
		{
			ent = g_entities + i;

			if ( ent->client->pers.connected != CON_CONNECTED )
			{
				continue;
			}

			if ( ent->inuse )
			{
				TeamplayInfoMessage( ent );
			}
		}
	}

	// Warn on imbalanced teams
	if ( g_teamImbalanceWarnings.integer && !level.intermissiontime &&
	     ( level.time - level.lastTeamImbalancedTime >
	       ( g_teamImbalanceWarnings.integer * 1000 ) ) &&
	     level.numTeamImbalanceWarnings < 3 && !level.restarted )
	{
		level.lastTeamImbalancedTime = level.time;

#ifdef UNREALARENA
		if ( level.team[ TEAM_U ].numClients - level.team[ TEAM_Q ].numClients > 2 )
		{
			trap_SendServerCommand( -1, "print_tr \"" N_("Teams are imbalanced. "
			                        "U team has more players.\n") "\"" );
			level.numTeamImbalanceWarnings++;
		}
		else if ( level.team[ TEAM_Q ].numClients - level.team[ TEAM_U ].numClients > 2 )
		{
			trap_SendServerCommand( -1, "print_tr \"" N_("Teams are imbalanced. "
			                        "Q team has more players.\n") "\"" );
			level.numTeamImbalanceWarnings++;
		}
#else
		if ( level.team[ TEAM_ALIENS ].numSpawns > 0 &&
		     level.team[ TEAM_HUMANS ].numClients - level.team[ TEAM_ALIENS ].numClients > 2 )
		{
			trap_SendServerCommand( -1, "print_tr \"" N_("Teams are imbalanced. "
			                        "Humans have more players.\n") "\"" );
			level.numTeamImbalanceWarnings++;
		}
		else if ( level.team[ TEAM_HUMANS ].numSpawns > 0 &&
		          level.team[ TEAM_ALIENS ].numClients - level.team[ TEAM_HUMANS ].numClients > 2 )
		{
			trap_SendServerCommand( -1, "print_tr \"" N_("Teams are imbalanced. "
			                        "Aliens have more players.\n") "\"" );
			level.numTeamImbalanceWarnings++;
		}
#endif
		else
		{
			level.numTeamImbalanceWarnings = 0;
		}
	}
}
