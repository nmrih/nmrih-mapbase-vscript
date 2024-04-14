//========== Copyright 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "vscript_server.h"
#include "icommandline.h"
#include "tier1/utlbuffer.h"
#include "tier1/fmtstr.h"
#include "filesystem.h"
#include "eventqueue.h"
#include "characterset.h"
#include "sceneentity.h"		// for exposing scene precache function
#include "gamerules.h"
#include "vscript_server.nut"
#include "world.h"

// @NMRiH - Felis
#include "nmrih_challenge_manager.h"

#include "mapbase/vscript_singletons.h"

extern ScriptClassDesc_t * GetScriptDesc( CBaseEntity * );

// #define VMPROFILE 1

#ifdef VMPROFILE

#define VMPROF_START float debugStartTime = Plat_FloatTime();
#define VMPROF_SHOW( funcname, funcdesc  ) DevMsg("***VSCRIPT PROFILE***: %s %s: %6.4f milliseconds\n", (##funcname), (##funcdesc), (Plat_FloatTime() - debugStartTime)*1000.0 );

#else // !VMPROFILE

#define VMPROF_START
#define VMPROF_SHOW

#endif // VMPROFILE


// @NMRiH - Felis: Ported from Mapbase
static ScriptHook_t g_Hook_OnEntityCreated;
static ScriptHook_t g_Hook_OnEntitySpawned;
static ScriptHook_t g_Hook_OnEntityDeleted;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CScriptEntityIterator : public IEntityListener
{
public:
	// @NMRiH - Felis
	CScriptEntityIterator()
	{
		m_bListening = false;
	}

	// @NMRiH - Felis: No use in multiplayer
	/*
	HSCRIPT GetLocalPlayer()
	{
		return ToHScript( UTIL_GetLocalPlayerOrListenServerHost() );
	}
	*/

	HSCRIPT First() { return Next(NULL); }

	HSCRIPT Next( HSCRIPT hStartEntity )
	{
		return ToHScript( gEntList.NextEnt( ToEnt( hStartEntity ) ) );
	}

	HSCRIPT CreateByClassname( const char *className )
	{
		return ToHScript( CreateEntityByName( className ) );
	}

	HSCRIPT FindByClassname( HSCRIPT hStartEntity, const char *szName )
	{
		return ToHScript( gEntList.FindEntityByClassname( ToEnt( hStartEntity ), szName ) );
	}

	HSCRIPT FindByName( HSCRIPT hStartEntity, const char *szName )
	{
		return ToHScript( gEntList.FindEntityByName( ToEnt( hStartEntity ), szName ) );
	}

	HSCRIPT FindInSphere( HSCRIPT hStartEntity, const Vector &vecCenter, float flRadius )
	{
		return ToHScript( gEntList.FindEntityInSphere( ToEnt( hStartEntity ), vecCenter, flRadius ) );
	}

	HSCRIPT FindByTarget( HSCRIPT hStartEntity, const char *szName )
	{
		return ToHScript( gEntList.FindEntityByTarget( ToEnt( hStartEntity ), szName ) );
	}

	HSCRIPT FindByModel( HSCRIPT hStartEntity, const char *szModelName )
	{
		return ToHScript( gEntList.FindEntityByModel( ToEnt( hStartEntity ), szModelName ) );
	}

	HSCRIPT FindByNameNearest( const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByNameNearest( szName, vecSrc, flRadius ) );
	}

	HSCRIPT FindByNameWithin( HSCRIPT hStartEntity, const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByNameWithin( ToEnt( hStartEntity ), szName, vecSrc, flRadius ) );
	}

	HSCRIPT FindByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByClassnameNearest( szName, vecSrc, flRadius ) );
	}

	HSCRIPT FindByClassnameWithin( HSCRIPT hStartEntity , const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByClassnameWithin( ToEnt( hStartEntity ), szName, vecSrc, flRadius ) );
	}

	// @NMRiH - Felis: Ported from Mapbase
	HSCRIPT FindByClassnameWithinBox( HSCRIPT hStartEntity , const char *szName, const Vector &vecMins, const Vector &vecMaxs )
	{
		return ToHScript( gEntList.FindEntityByClassnameWithin( ToEnt( hStartEntity ), szName, vecMins, vecMaxs ) );
	}

	HSCRIPT FindByClassNearestFacing( const Vector &origin, const Vector &facing, float threshold, const char *classname )
	{
		return ToHScript( gEntList.FindEntityClassNearestFacing( origin, facing, threshold, const_cast<char*>(classname) ) );
	}

	void EnableEntityListening()
	{
		// @NMRiH - Felis: Skips an assert when already listening
		if ( m_bListening )
			return;

		m_bListening = true;

		// Start getting entity updates!
		gEntList.AddListenerEntity( this );
	}

	void DisableEntityListening()
	{
		// Stop getting entity updates!
		gEntList.RemoveListenerEntity( this );

		// @NMRiH - Felis
		m_bListening = false;
	}

	void OnEntityCreated( CBaseEntity *pEntity )
	{
		if ( g_pScriptVM && GetScriptHookManager().IsEventHooked( "OnEntityCreated" ) )
		{
			// entity
			ScriptVariant_t args[] = { ScriptVariant_t( pEntity->GetScriptInstance() ) };
			g_Hook_OnEntityCreated.Call( NULL, NULL, args );
		}
	};

	void OnEntitySpawned( CBaseEntity *pEntity )
	{
		if ( g_pScriptVM && GetScriptHookManager().IsEventHooked( "OnEntitySpawned" ) )
		{
			// entity
			ScriptVariant_t args[] = { ScriptVariant_t( pEntity->GetScriptInstance() ) };
			g_Hook_OnEntitySpawned.Call( NULL, NULL, args );
		}
	};

	void OnEntityDeleted( CBaseEntity *pEntity )
	{
		if ( g_pScriptVM && GetScriptHookManager().IsEventHooked( "OnEntityDeleted" ) )
		{
			// entity
			ScriptVariant_t args[] = { ScriptVariant_t( pEntity->GetScriptInstance() ) };
			g_Hook_OnEntityDeleted.Call( NULL, NULL, args );
		}
	};

private:
	// @NMRiH - Felis
	bool m_bListening;
} g_ScriptEntityIterator;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptEntityIterator, "CEntities", SCRIPT_SINGLETON "The global list of entities" )
	/* @NMRiH - Felis: Disabled
	DEFINE_SCRIPTFUNC( GetLocalPlayer, "Get local player or listen server host" )
	*/
	DEFINE_SCRIPTFUNC( First, "Begin an iteration over the list of entities" )
	DEFINE_SCRIPTFUNC( Next, "Continue an iteration over the list of entities, providing reference to a previously found entity" )
	DEFINE_SCRIPTFUNC( CreateByClassname, "Creates an entity by classname" )
	DEFINE_SCRIPTFUNC( FindByClassname, "Find entities by class name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByName, "Find entities by name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindInSphere, "Find entities within a radius. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByTarget, "Find entities by targetname. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByModel, "Find entities by model name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByNameNearest, "Find entities by name nearest to a point."  )
	DEFINE_SCRIPTFUNC( FindByNameWithin, "Find entities by name within a radius. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByClassnameNearest, "Find entities by class name nearest to a point."  )
	DEFINE_SCRIPTFUNC( FindByClassnameWithin, "Find entities by class name within a radius. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )

	// @NMRiH - Felis: Ported from Mapbase
	DEFINE_SCRIPTFUNC( FindByClassnameWithinBox, "Find entities by class name within an AABB. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByClassNearestFacing, "Find the nearest entity along the facing direction from the given origin within the angular threshold with the given classname."  )

	DEFINE_SCRIPTFUNC( EnableEntityListening, "Enables the 'OnEntity' hooks. This function must be called before using them." )
	DEFINE_SCRIPTFUNC( DisableEntityListening, "Disables the 'OnEntity' hooks." )

	BEGIN_SCRIPTHOOK( g_Hook_OnEntityCreated, "OnEntityCreated", FIELD_VOID, "Called when an entity is created. Requires EnableEntityListening() to be fired beforehand." )
		DEFINE_SCRIPTHOOK_PARAM( "entity", FIELD_HSCRIPT )
	END_SCRIPTHOOK()

	BEGIN_SCRIPTHOOK( g_Hook_OnEntitySpawned, "OnEntitySpawned", FIELD_VOID, "Called when an entity spawns. Requires EnableEntityListening() to be fired beforehand." )
		DEFINE_SCRIPTHOOK_PARAM( "entity", FIELD_HSCRIPT )
	END_SCRIPTHOOK()

	BEGIN_SCRIPTHOOK( g_Hook_OnEntityDeleted, "OnEntityDeleted", FIELD_VOID, "Called when an entity is deleted. Requires EnableEntityListening() to be fired beforehand." )
		DEFINE_SCRIPTHOOK_PARAM( "entity", FIELD_HSCRIPT )
	END_SCRIPTHOOK()

END_SCRIPTDESC();

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static float Time()
{
	return gpGlobals->curtime;
}

static float FrameTime()
{
	return gpGlobals->frametime;
}

// @NMRiH - Felis: Ported from Mapbase
static int MaxPlayers()
{
	return gpGlobals->maxClients;
}

static int GetLoadType()
{
	return gpGlobals->eLoadType;
}

static void SendToConsole( const char *pszCommand )
{
	// @NMRiH - Felis: Privilege escalation?
	/*
	CBasePlayer *pPlayer = UTIL_GetLocalPlayerOrListenServerHost();
	if ( !pPlayer )
	{
#ifdef MAPBASE
		CGMsg( 1, CON_GROUP_VSCRIPT, "Cannot execute \"%s\", no player\n", pszCommand );
#else
		DevMsg ("Cannot execute \"%s\", no player\n", pszCommand );
#endif
		return;
	}

	engine->ClientCommand( pPlayer->edict(), pszCommand );
	*/
}

static void SendToConsoleServer( const char *pszCommand )
{
	// TODO: whitelist for multiplayer
	// @NMRiH - Felis: Privilege escalation?
	/*
	engine->ServerCommand( UTIL_VarArgs("%s\n", pszCommand) );
	*/
}

static const char *GetMapName()
{
	return STRING( gpGlobals->mapname );
}

static const char *DoUniqueString( const char *pszBase )
{
	static char szBuf[512];
	g_pScriptVM->GenerateUniqueKey( pszBase, szBuf, ARRAYSIZE(szBuf) );
	return szBuf;
}

#ifdef MAPBASE_VSCRIPT
static int  DoEntFire( const char *pszTarget, const char *pszAction, const char *pszValue, float delay, HSCRIPT hActivator, HSCRIPT hCaller )
#else
static void DoEntFire( const char *pszTarget, const char *pszAction, const char *pszValue, float delay, HSCRIPT hActivator, HSCRIPT hCaller )
#endif
{
	const char *target = "", *action = "Use";
	variant_t value;

	target = STRING( AllocPooledString( pszTarget ) );

	// Don't allow them to run anything on a point_servercommand unless they're the host player. Otherwise they can ent_fire
	// and run any command on the server. Admittedly, they can only do the ent_fire if sv_cheats is on, but 
	// people complained about users resetting the rcon password if the server briefly turned on cheats like this:
	//    give point_servercommand
	//    ent_fire point_servercommand command "rcon_password mynewpassword"
	if ( gpGlobals->maxClients > 1 && V_stricmp( target, "point_servercommand" ) == 0 )
	{
#ifdef MAPBASE_VSCRIPT
		return 0;
#else
		return;
#endif
	}

	if ( *pszAction )
	{
		action = STRING( AllocPooledString( pszAction ) );
	}
	if ( *pszValue )
	{
		value.SetString( AllocPooledString( pszValue ) );
	}
	if ( delay < 0 )
	{
		delay = 0;
	}

#ifdef MAPBASE_VSCRIPT
	return
#endif
	g_EventQueue.AddEvent( target, action, value, delay, ToEnt(hActivator), ToEnt(hCaller) );
}


bool DoIncludeScript( const char *pszScript, HSCRIPT hScope )
{
	if ( !VScriptRunScript( pszScript, hScope, true ) )
	{
		g_pScriptVM->RaiseException( CFmtStr( "Failed to include script \"%s\"", ( pszScript ) ? pszScript : "unknown" ) );
		return false;
	}
	return true;
}

HSCRIPT CreateProp( const char *pszEntityName, const Vector &vOrigin, const char *pszModelName, int iAnim )
{
	CBaseAnimating *pBaseEntity = (CBaseAnimating *)CreateEntityByName( pszEntityName );
	pBaseEntity->SetAbsOrigin( vOrigin );
	pBaseEntity->SetModel( pszModelName );
	pBaseEntity->SetPlaybackRate( 1.0f );

	int iSequence = pBaseEntity->SelectWeightedSequence( (Activity)iAnim );

	if ( iSequence != -1 )
	{
		pBaseEntity->SetSequence( iSequence );
	}

	return ToHScript( pBaseEntity );
}

//--------------------------------------------------------------------------------------------------
// Use an entity's script instance to add an entity IO event (used for firing events on unnamed entities from vscript)
//--------------------------------------------------------------------------------------------------
#ifdef MAPBASE_VSCRIPT
static int  DoEntFireByInstanceHandle( HSCRIPT hTarget, const char *pszAction, const char *pszValue, float delay, HSCRIPT hActivator, HSCRIPT hCaller )
#else
static void DoEntFireByInstanceHandle( HSCRIPT hTarget, const char *pszAction, const char *pszValue, float delay, HSCRIPT hActivator, HSCRIPT hCaller )
#endif
{
	const char *action = "Use";
	variant_t value;

	if ( *pszAction )
	{
		action = STRING( AllocPooledString( pszAction ) );
	}
	if ( *pszValue )
	{
		value.SetString( AllocPooledString( pszValue ) );
	}
	if ( delay < 0 )
	{
		delay = 0;
	}

	CBaseEntity* pTarget = ToEnt(hTarget);

	if ( !pTarget )
	{
		Warning( "VScript error: DoEntFire was passed an invalid entity instance.\n" );
#ifdef MAPBASE_VSCRIPT
		return 0;
#else
		return;
#endif
	}

#ifdef MAPBASE_VSCRIPT
	return
#endif
	g_EventQueue.AddEvent( pTarget, action, value, delay, ToEnt(hActivator), ToEnt(hCaller) );
}

static float ScriptTraceLine( const Vector &vecStart, const Vector &vecEnd, HSCRIPT entIgnore )
{
	// UTIL_TraceLine( vecAbsStart, vecAbsEnd, MASK_BLOCKLOS, pLooker, COLLISION_GROUP_NONE, ptr );
	trace_t tr;
	CBaseEntity *pLooker = ToEnt(entIgnore);
	UTIL_TraceLine( vecStart, vecEnd, MASK_NPCWORLDSTATIC, pLooker, COLLISION_GROUP_NONE, &tr);
	if (tr.fractionleftsolid && tr.startsolid)
	{
		return 1.0 - tr.fractionleftsolid;
	}
	else
	{
		return tr.fraction;
	}
}

#ifdef MAPBASE_VSCRIPT
static bool CancelEntityIOEvent( int event )
{
	return g_EventQueue.RemoveEvent(event);
}

static float GetEntityIOEventTimeLeft( int event )
{
	return g_EventQueue.GetTimeLeft(event);
}
#endif // MAPBASE_VSCRIPT

// @NMRiH - Felis: Multiplayer additions
HSCRIPT PlayerByIndex( const int index )
{
	return ToHScript( UTIL_PlayerByIndex( index ) );
}

HSCRIPT PlayerByUserId( const int userid )
{
	return ToHScript( UTIL_PlayerByUserId( userid ) );
}

HSCRIPT PlayerByName( const char *pszName )
{
	return ToHScript( UTIL_PlayerByName( pszName ) );
}

// @NMRiH - Felis: Center print
void ScriptCenterPrintWithParams( const HSCRIPT hPlayer, const char *pszMsgName, const char *pszParam1, const char *pszParam2, const char *pszParam3, const char *pszParam4 )
{
	CBasePlayer *pPlayer = ToBasePlayer( ToEnt( hPlayer ) );
	if ( !pPlayer )
	{
		Warning( "Script: CenterPrint() error! Invalid player handle.\n" );
		return;
	}

	ClientPrint( pPlayer, HUD_PRINTCENTER, pszMsgName, pszParam1, pszParam2, pszParam3, pszParam4 );
}

void ScriptCenterPrintAllWithParams( const char *pszMsgName, const char *pszParam1, const char *pszParam2, const char *pszParam3, const char *pszParam4 )
{
	UTIL_CenterPrintAll( pszMsgName, pszParam1, pszParam2, pszParam3, pszParam4 );
}

void ScriptCenterPrint( const HSCRIPT hPlayer, const char *pszMsgName )
{
	ScriptCenterPrintWithParams( hPlayer, pszMsgName, NULL, NULL, NULL, NULL );
}

void ScriptCenterPrintAll( const char *pszMsgName )
{
	ScriptCenterPrintAllWithParams( pszMsgName, NULL, NULL, NULL, NULL );
}

bool VScriptServerInit()
{
	VMPROF_START

	if( scriptmanager != NULL )
	{
		ScriptLanguage_t scriptLanguage = SL_DEFAULT;

		char const *pszScriptLanguage;
#ifdef MAPBASE_VSCRIPT
		if (GetWorldEntity()->GetScriptLanguage() != SL_NONE)
		{
			// Allow world entity to override script language
			scriptLanguage = GetWorldEntity()->GetScriptLanguage();

			// Less than SL_NONE means the script language should literally be none
			if (scriptLanguage < SL_NONE)
				scriptLanguage = SL_NONE;
		}
		else
#endif
		if ( CommandLine()->CheckParm( "-scriptlang", &pszScriptLanguage ) )
		{
#if 0 // @NMRiH - Felis: Squirrel only
			if( !Q_stricmp(pszScriptLanguage, "gamemonkey") )
			{
				scriptLanguage = SL_GAMEMONKEY;
			}
			else if( !Q_stricmp(pszScriptLanguage, "squirrel") )
			{
				scriptLanguage = SL_SQUIRREL;
			}
			else if( !Q_stricmp(pszScriptLanguage, "python") )
			{
				scriptLanguage = SL_PYTHON;
			}
#ifdef MAPBASE_VSCRIPT
			else if( !Q_stricmp(pszScriptLanguage, "lua") )
			{
				scriptLanguage = SL_LUA;
			}
#endif
#else
			if ( !Q_stricmp( pszScriptLanguage, "squirrel" ) )
			{
				scriptLanguage = SL_SQUIRREL;
			}
#endif
			else
			{
				Warning( "-server_script does not recognize a language named '%s'. virtual machine did NOT start.\n", pszScriptLanguage );
				scriptLanguage = SL_NONE;
			}

		}
		if( scriptLanguage != SL_NONE )
		{
			if ( g_pScriptVM == NULL )
				g_pScriptVM = scriptmanager->CreateVM( scriptLanguage );

			if( g_pScriptVM )
			{
				ConColorMsg( 0, CON_COLOR_VSCRIPT, "VSCRIPT SERVER: Started VScript virtual machine using script language '%s'\n", g_pScriptVM->GetLanguageName() );

				GetScriptHookManager().OnInit();

				// MULTIPLAYER
				ScriptRegisterFunctionNamed( g_pScriptVM, PlayerByIndex, "GetPlayerByIndex", "Returns a handle to a player by index. Only returns if the player is spawned and connected, otherwise returns null." );
				ScriptRegisterFunctionNamed( g_pScriptVM, PlayerByUserId, "GetPlayerByUserId", "Returns a handle to a player by user id." );
				ScriptRegisterFunctionNamed( g_pScriptVM, PlayerByName, "GetPlayerByName", "Returns a handle to a player by name." );
				//ScriptRegisterFunctionNamed( g_pScriptVM, ScriptGetPlayerByNetworkID, "GetPlayerByNetworkID", "" );

				ScriptRegisterFunctionNamed( g_pScriptVM, UTIL_ShowMessageAll, "ShowMessage", "Print a hud message on all clients" );

				// @NMRiH - Felis: Privilege escalation?
				/*
				ScriptRegisterFunction( g_pScriptVM, SendToConsole, "Send a string to the console as a command" );
				*/
				ScriptRegisterFunction( g_pScriptVM, GetMapName, "Get the name of the map.");
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptTraceLine, "TraceLine", "given 2 points & ent to ignore, return fraction along line that hits world or models" );

				ScriptRegisterFunction( g_pScriptVM, Time, "Get the current server time" );
				ScriptRegisterFunction( g_pScriptVM, FrameTime, "Get the time spent on the server in the last frame" );

				// @NMRiH - Felis: Privilege escalation?
				/*
				ScriptRegisterFunction( g_pScriptVM, SendToConsoleServer, "Send a string to the server console as a command" );
				*/
				ScriptRegisterFunction( g_pScriptVM, MaxPlayers, "Get the maximum number of players allowed on this server" );
				ScriptRegisterFunction( g_pScriptVM, GetLoadType, "Get the way the current game was loaded (corresponds to the MapLoad enum)" );
				ScriptRegisterFunction( g_pScriptVM, DoEntFire, SCRIPT_ALIAS( "EntFire", "Generate an entity i/o event" ) );
				ScriptRegisterFunction( g_pScriptVM, DoEntFireByInstanceHandle, SCRIPT_ALIAS( "EntFireByHandle", "Generate an entity i/o event. First parameter is an entity instance." ) );
				//ScriptRegisterFunction( g_pScriptVM, IsValidEntity, "Returns true if the entity is valid." );

				//ScriptRegisterFunction( g_pScriptVM, CancelEntityIOEvent, "Remove entity I/O event." );
				//ScriptRegisterFunction( g_pScriptVM, GetEntityIOEventTimeLeft, "Get time left on entity I/O event." );

				ScriptRegisterFunction( g_pScriptVM, DoUniqueString, SCRIPT_ALIAS( "UniqueString", "Generate a string guaranteed to be unique across the life of the script VM, with an optional root string. Useful for adding data to tables when not sure what keys are already in use in that table." ) );
				//ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCreateSceneEntity, "CreateSceneEntity", "Create a scene entity to play the specified scene." );

				ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::Box, "DebugDrawBox", "Draw a debug overlay box" );
				ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::Line, "DebugDrawLine", "Draw a debug overlay box" );

				ScriptRegisterFunction( g_pScriptVM, DoIncludeScript, "Execute a script (internal)" );
				ScriptRegisterFunction( g_pScriptVM, CreateProp, "Create a physics prop" );

				// @NMRiH - Felis: Our additions go here!
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCenterPrint, "CenterPrint", "Sends HUD text message to the client. Usage: CenterPrint(<player ent handle>, <string>)");
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCenterPrintAll, "CenterPrintAll", "Sends HUD text message to all clients. Usage: CenterPrintAll(<string>)" );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCenterPrintWithParams, "CenterPrintWithParams", "Sends HUD text message to the client, with optional string params. Format is limited to strings and is mapped to param order, i.e. %s1, %s2, %s3, %s4. You can pass an empty string as a param to skip. Usage: CenterPrintWithParams(<player ent handle>, <string>, <p1>, <p2>, <p3>, <p4>)" );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCenterPrintAllWithParams, "CenterPrintAllWithParams", "Sends HUD text message to all clients, with optional string params. Format is limited to strings and is mapped to param order, i.e. %s1, %s2, %s3, %s4. You can pass an empty string as a param to skip. Usage: CenterPrintAllWithParams(<string>, <p1>, <p2>, <p3>, <p4>)" );

				if ( GameRules() )
				{
					GameRules()->RegisterScriptFunctions();
				}

				g_pScriptVM->RegisterInstance( &g_ScriptEntityIterator, "Entities" );

				g_pScriptVM->RegisterAllClasses();
				g_pScriptVM->RegisterAllEnums();

				// @NMRiH - Felis: TODO: Implement
				/*
				IGameSystem::RegisterVScriptAllSystems();
				*/

				RegisterSharedScriptConstants();
				RegisterSharedScriptFunctions();

				if (scriptLanguage == SL_SQUIRREL)
				{
					g_pScriptVM->Run( g_Script_vscript_server );
				}

				// @NMRiH - Felis: Don't warn
				VScriptRunScript( "vscript_server", false );
				/*
				VScriptRunScript( "vscript_server", true );
				*/
				VScriptRunScript( "mapspawn", false );

				// Since the world entity spawns before VScript is initted, RunVScripts() is called before the VM has started, so no scripts are run.
				// This gets around that by calling the same function right after the VM is initted.
				GetWorldEntity()->RunVScripts();

				VMPROF_SHOW( pszScriptLanguage, "virtual machine startup" );

				return true;
			}
			else
			{
				Warning( "VM Did not start!\n" );
			}
		}
#ifdef MAPBASE_VSCRIPT
		else
		{
			ConColorMsg( 0, CON_COLOR_VSCRIPT, "VSCRIPT SERVER: Not starting because language is set to 'none'\n" );
		}
#endif
	}
	else
	{
		ConColorMsg( 0, CON_COLOR_VSCRIPT, "\nVSCRIPT: Scripting is disabled.\n" );
	}
	g_pScriptVM = NULL;
	return false;
}

void VScriptServerTerm()
{
	if( g_pScriptVM != NULL )
	{
		if( g_pScriptVM )
		{
			scriptmanager->DestroyVM( g_pScriptVM );
			g_pScriptVM = NULL;
		}
	}
}


bool VScriptServerReplaceClosures( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing )
{
	if ( !g_pScriptVM )
	{
		return false;
	}

	HSCRIPT hReplaceClosuresFunc = g_pScriptVM->LookupFunction( "__ReplaceClosures" );
	if ( !hReplaceClosuresFunc )
	{
		return false;
	}
	HSCRIPT hNewScript =  VScriptCompileScript( pszScriptName, bWarnMissing );
	if ( !hNewScript )
	{
		return false;
	}

	g_pScriptVM->Call( hReplaceClosuresFunc, NULL, true, NULL, hNewScript, hScope );
	return true;
}

CON_COMMAND( script_reload_code, "Execute a vscript file, replacing existing functions with the functions in the run script" )
{
	// @NMRiH - Felis
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
	
	if ( !*args[1] )
	{
		Warning( "No script specified\n" );
		return;
	}

	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}

	// @NMRiH - Felis
	if ( GetChallengeManager()->IsChallengeModeActive() )
		GetChallengeManager()->InvalidateResult( CHALLENGE_REJECT_OUTSIDE_VSCRIPT );

	VScriptServerReplaceClosures( args[1], NULL, true );
}

CON_COMMAND( script_reload_entity_code, "Execute all of this entity's VScripts, replacing existing functions with the functions in the run scripts" )
{
	// @NMRiH - Felis
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
	
	extern CBaseEntity *GetNextCommandEntity( CBasePlayer *pPlayer, const char *name, CBaseEntity *ent );

	const char *pszTarget = "";
	if ( *args[1] )
	{
		pszTarget = args[1];
	}

	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	// @NMRiH - Felis
	if ( GetChallengeManager()->IsChallengeModeActive() )
		GetChallengeManager()->InvalidateResult( CHALLENGE_REJECT_OUTSIDE_VSCRIPT );

	CBaseEntity *pEntity = NULL;
	while ( (pEntity = GetNextCommandEntity( pPlayer, pszTarget, pEntity )) != NULL )
	{
		if ( pEntity->m_ScriptScope.IsInitialized() && pEntity->m_iszVScripts != NULL_STRING )
		{
			char szScriptsList[255];
			Q_strcpy( szScriptsList, STRING(pEntity->m_iszVScripts) );
			CUtlStringList szScripts;
			V_SplitString( szScriptsList, " ", szScripts);

			for( int i = 0 ; i < szScripts.Count() ; i++ )
			{
				VScriptServerReplaceClosures( szScripts[i], pEntity->m_ScriptScope, true );
			}
		}
	}
}

CON_COMMAND( script_reload_think, "Execute an activation script, replacing existing functions with the functions in the run script" )
{
	// @NMRiH - Felis
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
	
	extern CBaseEntity *GetNextCommandEntity( CBasePlayer *pPlayer, const char *name, CBaseEntity *ent );

	const char *pszTarget = "";
	if ( *args[1] )
	{
		pszTarget = args[1];
	}

	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	// @NMRiH - Felis
	if ( GetChallengeManager()->IsChallengeModeActive() )
		GetChallengeManager()->InvalidateResult( CHALLENGE_REJECT_OUTSIDE_VSCRIPT );

	CBaseEntity *pEntity = NULL;
	while ( (pEntity = GetNextCommandEntity( pPlayer, pszTarget, pEntity )) != NULL )
	{
		if ( pEntity->m_ScriptScope.IsInitialized() && pEntity->m_iszScriptThinkFunction != NULL_STRING )
		{
			VScriptServerReplaceClosures( STRING(pEntity->m_iszScriptThinkFunction), pEntity->m_ScriptScope, true );
		}
	}
}

// @NMRiH - Felis
CON_COMMAND_F( script_dump_hooks, "", FCVAR_CHEAT )
{
	GetScriptHookManager().Dump();
}

class CVScriptGameSystem : public CAutoGameSystemPerFrame
{
public:
	// Inherited from IAutoServerSystem
	virtual void LevelInitPreEntity( void )
	{
		m_bAllowEntityCreationInScripts = true;
		VScriptServerInit();
	}

	virtual void LevelInitPostEntity( void )
	{
		m_bAllowEntityCreationInScripts = false;
	}

	virtual void LevelShutdownPostEntity( void )
	{
#ifdef MAPBASE_VSCRIPT
		g_ScriptNetMsg->LevelShutdownPreVM();
#endif

		GetScriptHookManager().OnShutdown();

		VScriptServerTerm();
	}

	virtual void FrameUpdatePostEntityThink() 
	{ 
		if ( g_pScriptVM )
			g_pScriptVM->Frame( gpGlobals->frametime );
	}

	bool m_bAllowEntityCreationInScripts;
};

CVScriptGameSystem g_VScriptGameSystem;

ConVar script_allow_entity_creation_midgame( "script_allow_entity_creation_midgame", "1", FCVAR_NOT_CONNECTED, "Allows VScript files to create entities mid-game, as opposed to only creating entities on startup." );

bool IsEntityCreationAllowedInScripts( void )
{
	if (script_allow_entity_creation_midgame.GetBool())
		return true;

	return g_VScriptGameSystem.m_bAllowEntityCreationInScripts;
}
