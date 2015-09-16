/*
 * Daemon GPL source code
 * Copyright (C) 2015  Unreal Arena
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
#include "engine/qcommon/q_unicode.h"

// sg_client.c -- client functions that don't happen every frame

static const vec3_t playerMins = { -15, -15, -24 };
static const vec3_t playerMaxs = { 15, 15, 32 };

/*
===============
G_AddCreditToClient
===============
*/
void G_AddCreditToClient( gclient_t *client, short credit, bool cap )
{
	int capAmount;

	if ( !client || client->pers.connected != CON_CONNECTED )
	{
		return;
	}

	if ( cap && credit > 0 )
	{
		capAmount = 2000;

		if ( client->pers.credit < capAmount )
		{
			client->pers.credit += credit;

			if ( client->pers.credit > capAmount )
			{
				client->pers.credit = capAmount;
			}
		}
	}
	else
	{
		client->pers.credit += credit;
	}

	if ( client->pers.credit < 0 )
	{
		client->pers.credit = 0;
	}

	// Copy to ps so the client can access it
	client->ps.persistant[ PERS_CREDIT ] = client->pers.credit;

	client->pers.infoChangeTime = level.time;
}

/*
================
SpotWouldTelefrag

================
*/
bool SpotWouldTelefrag( gentity_t *spot )
{
	int       i, num;
	int       touch[ MAX_GENTITIES ];
	gentity_t *hit;
	vec3_t    mins, maxs;

	VectorAdd( spot->s.origin, playerMins, mins );
	VectorAdd( spot->s.origin, playerMaxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for ( i = 0; i < num; i++ )
	{
		hit = &g_entities[ touch[ i ] ];

		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hit->client )
		{
			return true;
		}
	}

	return false;
}


/**
 * Find an appropriate spawn point
 *
 * XXX: flags (initial, ...)?
 *
 * @param origin  position
 * @param angles  orientation
 * @param team    team
 * @return        spawn point
 */
gentity_t *G_SelectSpawnPoint(vec3_t origin, vec3_t angles, team_t team)
{
	gentity_t *spawnPoint = nullptr;

	switch (team)
	{
		case TEAM_Q:
			spawnPoint = G_SelectQSpawnPoint(origin, angles);
			break;

		case TEAM_U:
			spawnPoint = G_SelectUSpawnPoint(origin, angles);
			break;

		case TEAM_NONE:
			spawnPoint = G_SelectSpectatorSpawnPoint(origin, angles);
			break;

		default:
			G_Error("G_SelectSpawnPoint: invalid team (%d)", team);
	}

	return spawnPoint;
}


/**
 * Find a viable spawn point for a Q player
 *
 * XXX: implement the correct logic
 *
 * @param origin  position
 * @param angles  orientation
 * @return        spawn point
 */
gentity_t *G_SelectQSpawnPoint(vec3_t origin, vec3_t angles)
{
	return G_SelectSpectatorSpawnPoint(origin, angles);
}


/**
 * Find a viable spawn point for a U player
 *
 * XXX: implement the correct logic
 *
 * @param origin  position
 * @param angles  orientation
 * @return        spawn point
 */
gentity_t *G_SelectUSpawnPoint(vec3_t origin, vec3_t angles)
{
	return G_SelectSpectatorSpawnPoint(origin, angles);
}


/**
 * Find a viable spawn point for spectators
 *
 * Select a "pos_player_intermission" or a "pos_player_spawn" otherwise.
 *
 * @param origin  position
 * @param angles  orientation
 * @return        spawn point
 */
gentity_t *G_SelectSpectatorSpawnPoint(vec3_t origin, vec3_t angles)
{
	gentity_t *spawnPoint = G_PickRandomEntityOfClass(S_POS_PLAYER_INTERMISSION);

	// If map is missing an intermission point then pick a random player spawn
	if (!spawnPoint)
	{
		spawnPoint = G_PickRandomEntityOfClass(S_POS_PLAYER_SPAWN);

		// If map has no player spawns then fail
		if (!spawnPoint)
		{
			G_Error("No pos_player_intermission or pos_player_spawn found");
		}
	}

	VectorCopy(spawnPoint->s.origin, origin);
	VectorCopy(spawnPoint->s.angles, angles);

	// If the spawn has a target then look towards it
	if (spawnPoint->targetCount)
	{
		gentity_t *target = G_PickRandomTargetFor(spawnPoint);

		if (target)
		{
			vec3_t direction;

			VectorSubtract(target->s.origin, origin, direction);
			vectoangles(direction, angles);
		}
	}

	return spawnPoint;
}


/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
static void BodySink( gentity_t *ent )
{
	//run on first BodySink call
	if ( !ent->active )
	{
		ent->active = true;

		//sinking bodies can't be infested
		ent->killedBy = ent->s.misc = MAX_CLIENTS;
		ent->timestamp = level.time;
	}

	if ( level.time - ent->timestamp > 6500 )
	{
		G_FreeEntity( ent );
		return;
	}

	ent->nextthink = level.time + 100;
	ent->s.pos.trBase[ 2 ] -= 1;
}

/*
=============
SpawnCorpse

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
static void SpawnCorpse( gentity_t *ent )
{
	gentity_t *body;
	int       contents;
	vec3_t    origin, mins;

	VectorCopy( ent->r.currentOrigin, origin );

	trap_UnlinkEntity( ent );

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents( origin, -1 );

	if ( contents & CONTENTS_NODROP )
	{
		return;
	}

	body = G_NewEntity();

	VectorCopy( ent->s.apos.trBase, body->s.angles );
	body->s.eFlags = EF_DEAD;
	body->s.eType = ET_CORPSE;
	body->timestamp = level.time;
	body->s.event = 0;
	body->r.contents = CONTENTS_CORPSE;
	body->clipmask = MASK_DEADSOLID;
	body->s.clientNum = ent->client->ps.persistant[ PERS_TEAM ];
	body->nonSegModel = ent->client->ps.persistant[ PERS_STATE ] & PS_NONSEGMODEL;

	if ( ent->client->pers.team == TEAM_Q )
	{
		body->classname = "qCorpse";
	}
	else
	{
		body->classname = "uCorpse";
	}

	body->s.misc = MAX_CLIENTS;

	body->think = BodySink;
	body->nextthink = level.time + 20000;

	body->s.legsAnim = ent->s.legsAnim;

	if ( !body->nonSegModel )
	{
		switch ( body->s.legsAnim & ~ANIM_TOGGLEBIT )
		{
			case BOTH_DEATH1:
			case BOTH_DEAD1:
				body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD1;
				break;

			case BOTH_DEATH2:
			case BOTH_DEAD2:
				body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD2;
				break;

			case BOTH_DEATH3:
			case BOTH_DEAD3:
			default:
				body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD3;
				break;
		}
	}
	else
	{
		switch ( body->s.legsAnim & ~ANIM_TOGGLEBIT )
		{
			case NSPA_DEATH1:
			case NSPA_DEAD1:
				body->s.legsAnim = NSPA_DEAD1;
				break;

			case NSPA_DEATH2:
			case NSPA_DEAD2:
				body->s.legsAnim = NSPA_DEAD2;
				break;

			case NSPA_DEATH3:
			case NSPA_DEAD3:
			default:
				body->s.legsAnim = NSPA_DEAD3;
				break;
		}
	}

	body->takedamage = false;

	body->health = ent->health = ent->client->ps.stats[ STAT_HEALTH ];
	ent->health = 0;

	//change body dimensions
	BG_ClassBoundingBox( ( team_t ) ent->client->ps.persistant[ PERS_TEAM ], mins, nullptr, nullptr, body->r.mins, body->r.maxs );

	//drop down to match the *model* origins of ent and body
	origin[2] += mins[ 2 ] - body->r.mins[ 2 ];

	G_SetOrigin( body, origin );
	body->s.pos.trType = TR_GRAVITY;
	body->s.pos.trTime = level.time;
	VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );

	trap_LinkEntity( body );
}

//======================================================================

/*
==================
G_SetClientViewAngle

==================
*/
void G_SetClientViewAngle( gentity_t *ent, const vec3_t angle )
{
	int i;

	// set the delta angle
	for ( i = 0; i < 3; i++ )
	{
		int cmdAngle;

		cmdAngle = ANGLE2SHORT( angle[ i ] );
		ent->client->ps.delta_angles[ i ] = cmdAngle - ent->client->pers.cmd.angles[ i ];
	}

	VectorCopy( angle, ent->s.angles );
	VectorCopy( ent->s.angles, ent->client->ps.viewangles );
}

/*
================
respawn
================
*/
void respawn( gentity_t *ent )
{
	int i;

	SpawnCorpse( ent );

	ClientSpawn( ent, nullptr, nullptr, nullptr );

	// stop any following clients that don't have sticky spec on
	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( level.clients[ i ].sess.spectatorState == SPECTATOR_FOLLOW &&
		     level.clients[ i ].sess.spectatorClient == ent - g_entities )
		{
			if ( !( level.clients[ i ].pers.stickySpec ) )
			{
				if ( !G_FollowNewClient( &g_entities[ i ], 1 ) )
				{
					G_StopFollowing( &g_entities[ i ] );
				}
			}
			else
			{
				G_FollowLockView( &g_entities[ i ] );
			}
		}
	}
}

static bool G_IsEmoticon( const char *s, bool *escaped )
{
	int        i, j;
	const char *p = s;
	char       emoticon[ MAX_EMOTICON_NAME_LEN ] = { "" };
	bool   escape = false;

	if ( *p != '[' )
	{
		return false;
	}

	p++;

	if ( *p == '[' )
	{
		escape = true;
		p++;
	}

	i = 0;

	while ( *p && i < ( MAX_EMOTICON_NAME_LEN - 1 ) )
	{
		if ( *p == ']' )
		{
			for ( j = 0; j < level.emoticonCount; j++ )
			{
				if ( !Q_stricmp( emoticon, level.emoticons[ j ].name ) )
				{
					*escaped = escape;
					return true;
				}
			}

			return false;
		}

		emoticon[ i++ ] = *p;
		emoticon[ i ] = '\0';
		p++;
	}

	return false;
}

/*
===========
G_IsUnnamed
============
*/
bool G_IsUnnamed( const char *name )
{
	char testName[ MAX_NAME_LENGTH ];
	int  length;

	G_DecolorString( (char *)name, testName, sizeof( testName ) );

	if ( !Q_stricmp( testName, UNNAMED_PLAYER ) )
	{
		return true;
	}

	length = strlen( g_unnamedNamePrefix.string );

	if ( g_unnamedNumbering.integer && length &&
	     !Q_strnicmp( testName, g_unnamedNamePrefix.string, length ) )
	{
		return true;
	}

	return false;
}

/*
===========
G_FindFreeUnnamed
============
*/
static unnamed_t G_FindFreeUnnamed( unnamed_t number )
{
	int i;

	do
	{
		for ( i = 0; i < level.maxclients; ++i )
		{
			if ( level.clients[i].pers.namelog && level.clients[i].pers.namelog->unnamedNumber == number )
			{
				number = ++number < 0 ? 1 : number;
				break;
			}
		}
	} while ( i != level.maxclients );

	return number;
}


/*
===========
G_UnnamedClientName
============
*/
static const char *G_UnnamedClientName( gclient_t *client )
{
	static unnamed_t nextNumber = 1;
	static char      name[ MAX_NAME_LENGTH ];
	unnamed_t        number;

	if ( !g_unnamedNumbering.integer || !client )
	{
		return UNNAMED_PLAYER;
	}

	if( client->pers.namelog->unnamedNumber )
	{
		number = client->pers.namelog->unnamedNumber;
	}
	else
	{
		if( g_unnamedNumbering.integer > 0 )
		{
			// server op may have reset this, so check for numbers in use
			number = G_FindFreeUnnamed( g_unnamedNumbering.integer );
			trap_Cvar_Set( "g_unnamedNumbering", va( "%d", ( number + 1 < 0 ? 1 : number + 1 ) ) );
		}
		else
		{
			// checking for numbers in use here is probably overkill...
			// however, belt and braces - could be a Long Game
			number = G_FindFreeUnnamed( nextNumber );
			nextNumber = number + 1 < 0 ? 1 : number + 1;
		}
	}

	client->pers.namelog->unnamedNumber = number;
	Com_sprintf( name, sizeof( name ), "%.*s%d", (int)sizeof( name ) - 11,
	             g_unnamedNamePrefix.string[ 0 ] ? g_unnamedNamePrefix.string : UNNAMED_PLAYER,
	             number );

	return name;
}

/*
===========
G_ClientCleanName
============
*/
static void G_ClientCleanName( const char *in, char *out, int outSize, gclient_t *client )
{
	int      len, colorlessLen;
	char     *p;
	int      spaces;
	bool escaped;
	bool invalid = false;
	bool hasletter = false;

	//save room for trailing null byte
	outSize--;

	len = 0;
	colorlessLen = 0;
	p = out;
	*p = 0;
	spaces = 0;

	for ( ; *in; in++ )
	{
		int cp, w;

		// don't allow leading spaces
		if ( colorlessLen == 0 && *in == ' ' )
		{
			continue;
		}

		// don't allow nonprinting characters or (dead) console keys
		// but do allow UTF-8 (unvalidated)
		if ( *in >= 0 && *in < ' ' )
		{
			continue;
		}

		// check colors or escaped escape character
		if ( Q_IsColorString(in) || ( in[0] == Q_COLOR_ESCAPE && in[1] == Q_COLOR_ESCAPE ) )
		{
			in++;

			// make sure room in dest for both chars
			if ( len > outSize - 2 )
			{
				break;
			}

			*out++ = Q_COLOR_ESCAPE;

			*out++ = *in;

			len += 2;
			continue;
		}
		else if ( in[ 0 ] == '^' && !in[ 1 ] )
		{
			// single trailing ^ will mess up some things

			// make sure room in dest for both chars
			if ( len > outSize - 2 )
			{
				break;
			}

			*out++ = '^';
			*out++ = '^';
			len += 2;
			continue;
		}
		else if ( !g_emoticonsAllowedInNames.integer && G_IsEmoticon( in, &escaped ) )
		{
			// make sure room in dest for both chars
			if ( len > outSize - 2 )
			{
				break;
			}

			*out++ = '[';
			*out++ = '[';
			len += 2;

			if ( escaped )
			{
				in++;
			}

			continue;
		}

		cp = Q_UTF8_CodePoint( in );

		if ( Q_Unicode_IsAlphaOrIdeo( cp ) )
		{
			hasletter = true;
		}

		// don't allow too many consecutive spaces
		if ( *in == ' ' )
		{
			spaces++;

			if ( spaces > 3 )
			{
				continue;
			}
		}
		else
		{
			spaces = 0;
		}

		w = Q_UTF8_WidthCP( cp );

		if ( len > outSize - w )
		{
			break;
		}

		memcpy( out, in, w );
		colorlessLen++;
		len += w;
		out += w;
		in += w - 1; // allow for loop increment
	}

	*out = 0;

	// don't allow names beginning with S_SKIPNOTIFY because it messes up /ignore-related code
	if ( !Q_strnicmp( p, S_SKIPNOTIFY, 12 ) )
	{
		invalid = true;
	}

	// don't allow comment-beginning strings because it messes up various parsers
	if ( strstr( p, "//" ) || strstr( p, "/*" ) )
	{
		invalid = true;
	}

	// don't allow empty names
	if ( *p == 0 || colorlessLen == 0 )
	{
		invalid = true;
	}

	// don't allow names beginning with digits
	if ( *p >= '0' && *p <= '9' )
	{
		invalid = true;
	}

	// limit no. of code points
	if ( Q_UTF8_PrintStrlen( p ) > MAX_NAME_LENGTH_CP )
	{
		invalid = true;
	}

	// if something made the name bad, put them back to UnnamedPlayer
	if ( invalid || !hasletter )
	{
		Q_strncpyz( p, G_UnnamedClientName( client ), outSize );
	}
}

/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
const char *ClientUserinfoChanged( int clientNum, bool forceName )
{
	gentity_t *ent;
	const char      *s;
	char      model[ MAX_QPATH ];
	char      buffer[ MAX_QPATH ];
	char      oldname[ MAX_NAME_LENGTH ];
	char      newname[ MAX_NAME_LENGTH ];
	char      err[ MAX_STRING_CHARS ];
	bool  revertName = false;
	gclient_t *client;
	char      userinfo[ MAX_INFO_STRING ];

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( !Info_Validate( userinfo ) )
	{
		trap_SendServerCommand( ent - g_entities,
		                        "disconnect \"illegal or malformed userinfo\n\"" );
		trap_DropClient( ent - g_entities,
		                 "dropped: illegal or malformed userinfo" );
		return "Illegal or malformed userinfo";
	}
	// If their userinfo overflowed, tremded is in the process of disconnecting them.
	// If we send our own disconnect, it won't work, so just return to prevent crashes later
	//  in this function. This check must come after the Info_Validate call.
	else if ( !userinfo[ 0 ] )
	{
		return "Empty (overflowed) userinfo";
	}

	// stickyspec toggle
	s = Info_ValueForKey( userinfo, "cg_stickySpec" );
	client->pers.stickySpec = atoi( s ) != 0;

	// set name
	Q_strncpyz( oldname, client->pers.netname, sizeof( oldname ) );
	s = Info_ValueForKey( userinfo, "name" );
	G_ClientCleanName( s, newname, sizeof( newname ), client );

	if ( strcmp( oldname, newname ) )
	{
		if ( !forceName && client->pers.namelog->nameChangeTime &&
		     level.time - client->pers.namelog->nameChangeTime <=
		     g_minNameChangePeriod.value * 1000 )
		{
			trap_SendServerCommand( ent - g_entities, va(
			                          "print_tr %s %d", QQ( N_("Name change spam protection (g_minNameChangePeriod = $1$)\n") ),
			                          g_minNameChangePeriod.integer ) );
			revertName = true;
		}
		else if ( !forceName && g_maxNameChanges.integer > 0 &&
		          client->pers.namelog->nameChanges >= g_maxNameChanges.integer )
		{
			trap_SendServerCommand( ent - g_entities, va(
			                          "print_tr %s %d", QQ( N_("Maximum name changes reached (g_maxNameChanges = $1$)\n") ),
			                          g_maxNameChanges.integer ) );
			revertName = true;
		}
		else if ( !forceName && client->pers.namelog->muted )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s", QQ( N_("You cannot change your name while you are muted\n") ) ) );
			revertName = true;
		}
		else if ( !G_admin_name_check( ent, newname, err, sizeof( err ) ) )
		{
			trap_SendServerCommand( ent - g_entities, va( "print_tr %s %s %s", QQ( "$1t$ $2$\n" ), Quote( err ), Quote( newname ) ) );
			revertName = true;
		}

		if ( revertName )
		{
			Q_strncpyz( client->pers.netname, *oldname ? oldname : G_UnnamedClientName( client ),
			            sizeof( client->pers.netname ) );
		}
		else
		{
			if( G_IsUnnamed( newname ) )
			{
				Q_strncpyz( newname, G_UnnamedClientName( client ), sizeof( newname ) );
			}

			G_CensorString( client->pers.netname, newname,
			                sizeof( client->pers.netname ), ent );

			if ( !forceName && client->pers.connected == CON_CONNECTED )
			{
				client->pers.namelog->nameChangeTime = level.time;
				client->pers.namelog->nameChanges++;
			}

			if ( *oldname )
			{
				G_LogPrintf( "ClientRename: %i [%s] (%s) \"%s^7\" -> \"%s^7\" \"%c%s%c^7\"\n",
				             clientNum, client->pers.ip.str, client->pers.guid,
				             oldname, client->pers.netname,
				             DECOLOR_OFF, client->pers.netname, DECOLOR_ON );
			}
		}

		G_namelog_update_name( client );

		Info_SetValueForKey(userinfo, "name", client->pers.netname, false);
		trap_SetUserinfo(clientNum, userinfo);
	}

	Com_sprintf( buffer, MAX_QPATH, "%s/%s",  BG_ClassModelConfig( client->pers.team )->modelName,
	             BG_ClassModelConfig( client->pers.team )->skinName );

	if ( BG_ClassModelConfig( client->pers.team )->segmented )
	{
		client->ps.persistant[ PERS_STATE ] |= PS_NONSEGMODEL;
	}
	else
	{
		client->ps.persistant[ PERS_STATE ] &= ~PS_NONSEGMODEL;
	}

	Q_strncpyz( model, buffer, sizeof( model ) );

	// wallwalk follow
	s = Info_ValueForKey( userinfo, "cg_wwFollow" );

	if ( atoi( s ) )
	{
		client->ps.persistant[ PERS_STATE ] |= PS_WALLCLIMBINGFOLLOW;
	}
	else
	{
		client->ps.persistant[ PERS_STATE ] &= ~PS_WALLCLIMBINGFOLLOW;
	}

	// wallwalk toggle
	s = Info_ValueForKey( userinfo, "cg_wwToggle" );

	if ( atoi( s ) )
	{
		client->ps.persistant[ PERS_STATE ] |= PS_WALLCLIMBINGTOGGLE;
	}
	else
	{
		client->ps.persistant[ PERS_STATE ] &= ~PS_WALLCLIMBINGTOGGLE;
	}

	// always sprint
	s = Info_ValueForKey( userinfo, "cg_sprintToggle" );

	if ( atoi( s ) )
	{
		client->ps.persistant[ PERS_STATE ] |= PS_SPRINTTOGGLE;
	}
	else
	{
		client->ps.persistant[ PERS_STATE ] &= ~PS_SPRINTTOGGLE;
	}

	// fly speed
	s = Info_ValueForKey( userinfo, "cg_flySpeed" );

	if ( *s )
	{
		client->pers.flySpeed = atoi( s );
	}
	else
	{
		client->pers.flySpeed = BG_Class( TEAM_NONE )->speed;
	}

	// disable blueprint errors
	s = Info_ValueForKey( userinfo, "cg_disableBlueprintErrors" );

	if ( atoi( s ) )
	{
		client->pers.disableBlueprintErrors = true;
	}
	else
	{
		client->pers.disableBlueprintErrors = false;
	}

	// teamInfo
	s = Info_ValueForKey( userinfo, "teamoverlay" );

	if ( atoi( s ) != 0 )
	{
		// teamoverlay was enabled so we need an update
		if ( client->pers.teamInfo == 0 )
		{
			client->pers.teamInfo = 1;
		}
	}
	else
	{
		client->pers.teamInfo = 0;
	}

	s = Info_ValueForKey( userinfo, "cg_unlagged" );

	if ( !s[ 0 ] || atoi( s ) != 0 )
	{
		client->pers.useUnlagged = true;
	}
	else
	{
		client->pers.useUnlagged = false;
	}

	Q_strncpyz( client->pers.voice, Info_ValueForKey( userinfo, "voice" ),
	            sizeof( client->pers.voice ) );

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds

	Com_sprintf( userinfo, sizeof( userinfo ),
	             "n\\%s\\t\\%i\\model\\%s\\ig\\%16s\\v\\%s",
	             client->pers.netname, client->pers.team, model,
	             Com_ClientListString( &client->sess.ignoreList ),
	             client->pers.voice );

	trap_SetConfigstring( CS_PLAYERS + clientNum, userinfo );

	/*G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, userinfo );*/

	return nullptr;
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return nullptr if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be true the very first time a client connects
to the server machine, but false on map changes and tournement
restarts.
============
*/
const char *ClientConnect( int clientNum, bool firstTime )
{
	const char      *value;
	const char      *userInfoError;
	gclient_t       *client;
	char            userinfo[ MAX_INFO_STRING ];
	char            pubkey[ RSA_STRING_LENGTH ];
	gentity_t       *ent;
	char            reason[ MAX_STRING_CHARS ] = { "" };
	int             i;
	const char      *country;

	ent = &g_entities[ clientNum ];
	client = &level.clients[ clientNum ];

	// ignore if client already connected
	if ( client->pers.connected != CON_DISCONNECTED )
	{
		return nullptr;
	}

	ent->client = client;
	memset( client, 0, sizeof( *client ) );

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	value = Info_ValueForKey( userinfo, "ip" );

	// check for local client
	if ( !strcmp( value, "localhost" ) )
	{
		client->pers.localClient = true;
	}

	G_AddressParse( value, &client->pers.ip );

	trap_GetPlayerPubkey( clientNum, pubkey, sizeof( pubkey ) );

	if ( strlen( pubkey ) != RSA_STRING_LENGTH - 1 )
	{
		return "Invalid pubkey key";
	}

	trap_GenFingerprint( pubkey, sizeof( pubkey ), client->pers.guid, sizeof( client->pers.guid ) );
	client->pers.admin = G_admin_admin( client->pers.guid );

	client->pers.pubkey_authenticated = false;

	if ( client->pers.admin )
	{
		Com_GMTime( &client->pers.admin->lastSeen );
	}

	// check for admin ban
	if ( G_admin_ban_check( ent, reason, sizeof( reason ) ) )
	{
		return va( "%s", reason ); // reason is local
	}

	// check for a password
	value = Info_ValueForKey( userinfo, "password" );

	if ( g_password.string[ 0 ] && Q_stricmp( g_password.string, "none" ) &&
	     strcmp( g_password.string, value ) != 0 )
	{
		return "Invalid password";
	}

	// if a player reconnects quickly after a disconnect, the client disconnect may never be called, thus flag can get lost in the ether
	if ( ent->inuse )
	{
		G_LogPrintf( "Forcing disconnect on active client: %i\n", (int)( ent - g_entities ) );
		// so lets just fix up anything that should happen on a disconnect
		ClientDisconnect( ent-g_entities );
	}

	for ( i = 0; i < level.maxclients; i++ )
	{
		if ( level.clients[ i ].pers.connected == CON_DISCONNECTED )
		{
			continue;
		}

		if ( !( g_entities[i].r.svFlags & SVF_BOT ) && !Q_stricmp( client->pers.guid, level.clients[ i ].pers.guid ) )
		{
			if ( !G_ClientIsLagging( level.clients + i ) )
			{
				trap_SendServerCommand( i, "cp \"Your GUID is not secure\"" );
				return "Duplicate GUID";
			}

			trap_DropClient( i, "Ghost" );
		}
	}

	client->pers.connected = CON_CONNECTING;

	// read or initialize the session data
	if ( firstTime )
	{
		G_InitSessionData( client, userinfo );
	}

	G_ReadSessionData( client );

	// get and distribute relevent paramters
	G_namelog_connect( client );
	userInfoError = ClientUserinfoChanged( clientNum, false );

	if ( userInfoError != nullptr )
	{
		return userInfoError;
	}

	G_LogPrintf( "ClientConnect: %i [%s] (%s) \"%s^7\" \"%c%s%c^7\"\n",
	             clientNum, client->pers.ip.str[0] ? client->pers.ip.str : "127.0.0.1", client->pers.guid,
	             client->pers.netname,
	             DECOLOR_OFF, client->pers.netname, DECOLOR_ON );

	country = Info_ValueForKey( userinfo, "geoip" );
	Q_strncpyz( client->pers.country, country, sizeof( client->pers.country ) );

	G_SendClientPmoveParams(clientNum);

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime )
	{
		if ( g_geoip.integer && country && *country )
		{
			trap_SendServerCommand( -1, va( "print_tr %s %s %s", QQ( N_("$1$^7 connected from $2$\n") ),
			                                Quote( client->pers.netname ), Quote( country ) ) );
		}
		else
		{
			trap_SendServerCommand( -1, va( "print_tr %s %s", QQ( N_("$1$^7 connected\n") ),
			                                Quote( client->pers.netname ) ) );
		}
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	// if this is after !restart keepteams or !restart switchteams, apply said selection
	if ( client->sess.restartTeam != TEAM_NONE )
	{
		G_ChangeTeam( ent, client->sess.restartTeam );
		client->sess.restartTeam = TEAM_NONE;
	}

	return nullptr;
}

/*
===========
ClientBotConnect

Cut-down version of ClientConnect.
Doesn't do things not relevant to bots (which are local GUIDless clients).
============
*/
const char *ClientBotConnect( int clientNum, bool firstTime, team_t team )
{
	const char      *userInfoError;
	gclient_t       *client;
	char            userinfo[ MAX_INFO_STRING ];
	gentity_t       *ent;

	ent = &g_entities[ clientNum ];
	client = &level.clients[ clientNum ];

	ent->client = client;
	memset( client, 0, sizeof( *client ) );

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	client->pers.localClient = true;
	G_AddressParse( "localhost", &client->pers.ip );

	Q_strncpyz( client->pers.guid, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX", sizeof( client->pers.guid ) );
	client->pers.admin = nullptr;
	client->pers.pubkey_authenticated = true;
	client->pers.connected = CON_CONNECTING;

	// read or initialize the session data
	if ( firstTime )
	{
		G_InitSessionData( client, userinfo );
	}

	G_ReadSessionData( client );

	// get and distribute relevant parameters
	G_namelog_connect( client );
	userInfoError = ClientUserinfoChanged( clientNum, false );

	if ( userInfoError != nullptr )
	{
		return userInfoError;
	}

	ent->r.svFlags |= SVF_BOT;

	// can happen during reconnection
	if ( !ent->botMind )
	{
		G_BotSetDefaults( clientNum, team, client->sess.botSkill, client->sess.botTree );
	}

	G_LogPrintf( "ClientConnect: %i [%s] (%s) \"%s^7\" \"%c%s%c^7\" [BOT]\n",
	             clientNum, client->pers.ip.str[0] ? client->pers.ip.str : "127.0.0.1", client->pers.guid,
	             client->pers.netname,
	             DECOLOR_OFF, client->pers.netname, DECOLOR_ON );

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime )
	{
		trap_SendServerCommand( -1, va( "print_tr %s %s", QQ( N_("$1$^7 connected\n") ),
		                                Quote( client->pers.netname ) ) );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	// if this is after !restart keepteams or !restart switchteams, apply said selection
	if ( client->sess.restartTeam != TEAM_NONE )
	{
//		G_ChangeTeam( ent, client->sess.restartTeam );
//		client->sess.restartTeam = TEAM_NONE;
	}
	else if ( team != TEAM_NONE )
	{
//		G_ChangeTeam( ent, team );
		client->sess.restartTeam = team;
	}

	return nullptr;
}

/*
============
ClientAdminChallenge
============
*/
void ClientAdminChallenge( int clientNum )
{
	gclient_t       *client = level.clients + clientNum;
	g_admin_admin_t *admin = client->pers.admin;

	if ( !client->pers.pubkey_authenticated && admin && admin->pubkey[ 0 ] && ( level.time - client->pers.pubkey_challengedAt ) >= 6000 )
	{
		trap_SendServerCommand( clientNum, va( "pubkey_decrypt %s", admin->msg2 ) );
		client->pers.pubkey_challengedAt = level.time ^ ( 5 * clientNum ); // a small amount of jitter

		// copy the decrypted message because generating a new message will overwrite it
		G_admin_writeconfig();
	}
}

/*
===========
ClientBegin

Called when a client has finished connecting, and is ready
to be placed into the level. This will happen on every
level load and level restart, but doesn't happen on respawns.
============
*/
void ClientBegin( int clientNum )
{
	gentity_t       *ent;
	gclient_t       *client;
	int             flags;
	char            startMsg[ MAX_STRING_CHARS ];

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	// ignore if client already entered the game
	if ( client->pers.connected != CON_CONNECTING )
	{
		return;
	}

	if ( ent->r.linked )
	{
		trap_UnlinkEntity( ent );
	}

	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;

	ClientAdminChallenge( clientNum );

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;
	memset( &client->ps, 0, sizeof( client->ps ) );
	memset( &client->pmext, 0, sizeof( client->pmext ) );
	client->ps.eFlags = flags;

	// locate ent at a spawn point
	ClientSpawn( ent, nullptr, nullptr, nullptr );

	trap_SendServerCommand( -1, va( "print_tr %s %s", QQ( N_("$1$^7 entered the game\n") ), Quote( client->pers.netname ) ) );

	trap_Cvar_VariableStringBuffer( "g_mapStartupMessage", startMsg, sizeof( startMsg ) );

	if ( *startMsg )
	{
		trap_SendServerCommand( ent - g_entities, va( "cpd %d %s", g_mapStartupMessageDelay.integer, Quote( startMsg ) ) );
	}

	G_namelog_restore( client );

	G_LogPrintf( "ClientBegin: %i\n", clientNum );

	// count current clients and rank for scoreboard
	CalculateRanks();

	// display the help menu, if connecting the first time
	if ( !client->sess.seenWelcome )
	{
		client->sess.seenWelcome = 1;

		// 0 - don't show
		// 1 - always show to all
		// 2 - show only to unregistered
		switch ( g_showHelpOnConnection.integer )
		{
		case 0:
			if (0)
		default:
			if ( !client->pers.admin )
		case 1:
			G_TriggerMenu( client->ps.clientNum, MN_WELCOME );
		}
	}
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn and evolve
Initializes all non-persistent parts of playerState
============
*/
void ClientSpawn( gentity_t *ent, gentity_t *spawn, const vec3_t origin, const vec3_t angles )
{
	int                index;
	vec3_t             spawn_origin, spawn_angles;
	gclient_t          *client;
	int                i;
	clientPersistant_t saved;
	clientSession_t    savedSess;
	bool           savedNoclip, savedCliprcontents;
	int                persistant[ MAX_PERSISTANT ];
	gentity_t          *spawnPoint = nullptr;
	int                flags;
	int                savedPing;
	int                teamLocal;
	int                eventSequence;
	char               userinfo[ MAX_INFO_STRING ];
	int                maxAmmo, maxClips;
	weapon_t           weapon;

	index = ent - g_entities;
	client = ent->client;

	teamLocal = client->pers.team;

	//if client is dead and following teammate, stop following before spawning
	if ( client->sess.spectatorClient != -1 )
	{
		client->sess.spectatorClient = -1;
		client->sess.spectatorState = SPECTATOR_FREE;
	}

	// Manage spawn queues
	if ( spawn == nullptr && teamLocal == TEAM_NONE )
	{
		client->sess.spectatorState = SPECTATOR_FREE;
	}
	else if ( spawn == nullptr )
	{
		client->sess.spectatorState = SPECTATOR_LOCKED;
	}

	// if client is dead and following teammate, stop following before spawning
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW )
	{
		G_StopFollowing( ent );
	}

	if ( origin != nullptr )
	{
		VectorCopy( origin, spawn_origin );
	}

	if ( angles != nullptr )
	{
		VectorCopy( angles, spawn_angles );
	}

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( client->sess.spectatorState != SPECTATOR_NOT )
	{
		if ( teamLocal != TEAM_NONE )
		{
			G_PushSpawnQueue( &level.team[ teamLocal ].spawnQueue, index );
			client->ps.persistant[ PERS_TEAM ] = teamLocal;
		}
		else
		{
			spawnPoint = G_SelectSpawnPoint( spawn_origin, spawn_angles, TEAM_NONE );
		}
	}
	else
	{
		if ( spawn == nullptr )
		{
			G_Error( "ClientSpawn: spawn is NULL" );
		}

		spawnPoint = spawn;
	}

	// toggle the teleport bit so the client knows to not lerp
	flags = ( ent->client->ps.eFlags & EF_TELEPORT_BIT ) ^ EF_TELEPORT_BIT;
	G_UnlaggedClear( ent );

	// clear everything but the persistent data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	savedNoclip = client->noclip;
	savedCliprcontents = client->cliprcontents;

	for ( i = 0; i < MAX_PERSISTANT; i++ )
	{
		persistant[ i ] = client->ps.persistant[ i ];
	}

	eventSequence = client->ps.eventSequence;
	memset( client, 0, sizeof( *client ) );

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
	client->noclip = savedNoclip;
	client->cliprcontents = savedCliprcontents;

	for ( i = 0; i < MAX_PERSISTANT; i++ )
	{
		client->ps.persistant[ i ] = persistant[ i ];
	}

	client->ps.eventSequence = eventSequence;

	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[ PERS_SPAWN_COUNT ]++;
	client->ps.persistant[ PERS_SPECSTATE ] = client->sess.spectatorState;

	client->airOutTime = level.time + 12000;

	trap_GetUserinfo( index, userinfo, sizeof( userinfo ) );
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[ index ];
	ent->takedamage = teamLocal != TEAM_NONE && client->sess.spectatorState == SPECTATOR_NOT; //true;
	ent->classname = S_PLAYER_CLASSNAME;
	if ( client->noclip )
	{
		client->cliprcontents = CONTENTS_BODY;
	}
	else
	{
		ent->r.contents = CONTENTS_BODY;
	}
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = G_PlayerDie;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags &= FL_GODMODE | FL_NOTARGET;

	// calculate each client's acceleration
	ent->evaluateAcceleration = true;

	client->ps.stats[ STAT_MISC ] = 0;

	client->ps.eFlags = flags;
	client->ps.clientNum = index;

	BG_ClassBoundingBox( ent->client->pers.team, ent->r.mins, ent->r.maxs, nullptr, nullptr, nullptr );

	if ( client->sess.spectatorState == SPECTATOR_NOT )
	{
		client->ps.stats[ STAT_MAX_HEALTH ] =
		  BG_Class( ent->client->pers.team )->health;
	}
	else
	{
		client->ps.stats[ STAT_MAX_HEALTH ] = 100;
	}

	// clear entity values
	if ( ent->client->pers.team == TEAM_U )
	{
		BG_AddUpgradeToInventory( UP_MEDKIT, client->ps.stats );
		weapon = client->pers.weapon;
	}
	else if ( client->sess.spectatorState == SPECTATOR_NOT )
	{
		weapon = BG_Class( ent->client->pers.team )->startWeapon;
	}
	else
	{
		weapon = WP_NONE;
	}

	maxAmmo = BG_Weapon( weapon )->maxAmmo;
	maxClips = BG_Weapon( weapon )->maxClips;
	client->ps.stats[ STAT_WEAPON ] = weapon;
	client->ps.ammo = maxAmmo;
	client->ps.clips = maxClips;

	// We just spawned, not changing weapons
	client->ps.persistant[ PERS_NEWWEAPON ] = 0;

	client->ps.persistant[ PERS_TEAM ] = client->pers.team;

	// TODO: Check whether stats can be cleared at once instead of per field
	client->ps.stats[ STAT_PREDICTION ] = 0;
	client->ps.stats[ STAT_STATE ] = 0;

	VectorSet( client->ps.grapplePoint, 0.0f, 0.0f, 1.0f );

	// health will count down towards max_health
	ent->health = client->ps.stats[ STAT_HEALTH ] = client->ps.stats[ STAT_MAX_HEALTH ]; //* 1.25;

	//if evolving scale health
	if ( ent == spawn )
	{
		ent->health *= ent->client->pers.evolveHealthFraction;
		client->ps.stats[ STAT_HEALTH ] *= ent->client->pers.evolveHealthFraction;
	}

	//clear the credits array
	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		ent->credits[ i ].value = 0.0f;
		ent->credits[ i ].time = 0;
		ent->credits[ i ].team = TEAM_NONE;
	}

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	if ( client->sess.spectatorState == SPECTATOR_NOT && ( client->pers.team == TEAM_Q || client->pers.team == TEAM_U ) )
	{
		spawn_angles[ YAW ] += 180.0f;
		AngleNormalize360( spawn_angles[ YAW ] );
	}

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	G_SetClientViewAngle( ent, spawn_angles );

	if ( client->sess.spectatorState == SPECTATOR_NOT )
	{
		trap_LinkEntity( ent );

		// force the base weapon up
		if ( client->pers.team == TEAM_U )
		{
			G_ForceWeaponChange( ent, weapon );
		}

		client->ps.weaponstate = WEAPON_READY;
	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	ent->nextRegenTime = level.time;

	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	usercmdClearButtons( client->latched_buttons );

	// set default animations
	client->ps.torsoAnim = TORSO_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	if ( level.intermissiontime )
	{
		MoveClientToIntermission( ent );
	}
	else
	{
		// telefrag
		G_KillBox( ent );

		// fire the targets of the spawn point
		if ( !spawn && spawnPoint )
		{
			G_EventFireEntity( spawnPoint, ent, ON_SPAWN );
		}

		// select the highest weapon number available, after any
		// spawn given items have fired
		client->ps.weapon = 1;

		for ( i = WP_NUM_WEAPONS - 1; i > 0; i-- )
		{
			if ( BG_InventoryContainsWeapon( i, client->ps.stats ) )
			{
				client->ps.weapon = i;
				break;
			}
		}
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent - g_entities );

	// positively link the client, even if the command times are weird
	if ( client->sess.spectatorState == SPECTATOR_NOT )
	{
		BG_PlayerStateToEntityState( &client->ps, &ent->s, true );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	// must do this here so the number of active clients is calculated
	CalculateRanks();

	// run the presend to set anything else
	ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, true );

	client->pers.infoChangeTime = level.time;

	// (re)tag the client for its team
	Beacon::DeleteTags( ent );
	Beacon::Tag( ent, (team_t)ent->client->ps.persistant[ PERS_TEAM ], true );
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect( int clientNum )
{
	gentity_t *ent;
	gentity_t *tent;
	int       i;

	ent = g_entities + clientNum;

	if ( !ent->client || ent->client->pers.connected == CON_DISCONNECTED )
	{
		return;
	}

	G_LeaveTeam( ent );
	G_namelog_disconnect( ent->client );
	G_Vote( ent, TEAM_NONE, false );

	// stop any following clients
	for ( i = 0; i < level.maxclients; i++ )
	{
		// remove any /ignore settings for this clientNum
		Com_ClientListRemove( &level.clients[ i ].sess.ignoreList, clientNum );
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED &&
	     ent->client->sess.spectatorState == SPECTATOR_NOT )
	{
		tent = G_NewTempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = ent->s.clientNum;
	}

	G_LogPrintf( "ClientDisconnect: %i [%s] (%s) \"%s^7\"\n", clientNum,
	             ent->client->pers.ip.str, ent->client->pers.guid, ent->client->pers.netname );

	trap_UnlinkEntity( ent );
	ent->inuse = false;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->sess.spectatorState = SPECTATOR_NOT;
	ent->client->ps.persistant[ PERS_SPECSTATE ] = SPECTATOR_NOT;

	trap_SetConfigstring( CS_PLAYERS + clientNum, "" );

	CalculateRanks();

	Beacon::PropagateAll();
}
