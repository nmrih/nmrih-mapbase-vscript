//========== Copyright 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"

// @NMRiH - Felis: Server-side only for now!
#ifndef CLIENT_DLL

#include "vscript_shared.h"
#include "icommandline.h"
#include "tier1/utlbuffer.h"
#include "tier1/fmtstr.h"
#include "filesystem.h"
#include "characterset.h"
#include "isaverestore.h"
#include "gamerules.h"
#ifdef MAPBASE_VSCRIPT
#include "mapbase/vscript_singletons.h"
#endif

// @NMRiH - Felis
#include "nmrih_challenge_manager.h"

IScriptVM * g_pScriptVM;
extern ScriptClassDesc_t * GetScriptDesc( CBaseEntity * );

// #define VMPROFILE 1

#ifdef VMPROFILE

#define VMPROF_START float debugStartTime = Plat_FloatTime();
#define VMPROF_SHOW( funcname, funcdesc  ) DevMsg("***VSCRIPT PROFILE***: %s %s: %6.4f milliseconds\n", (##funcname), (##funcdesc), (Plat_FloatTime() - debugStartTime)*1000.0 );

#else // !VMPROFILE

#define VMPROF_START
#define VMPROF_SHOW

#endif // VMPROFILE

// This is to ensure a dependency exists between the vscript library and the game DLLs
extern int vscript_token;
int vscript_token_hack = vscript_token;



HSCRIPT VScriptCompileScript( const char *pszScriptName, bool bWarnMissing )
{
	if ( !g_pScriptVM )
	{
		return NULL;
	}

	static const char *pszExtensions[] =
	{
		"",		// SL_NONE
		".gm",	// SL_GAMEMONKEY
		".nut",	// SL_SQUIRREL
		".lua", // SL_LUA
		".py",  // SL_PYTHON
	};

	const char *pszVMExtension = pszExtensions[g_pScriptVM->GetLanguage()];
	const char *pszIncomingExtension = V_strrchr( pszScriptName , '.' );
	if ( pszIncomingExtension && V_strcmp( pszIncomingExtension, pszVMExtension ) != 0 )
	{
		Warning( "Script file type does not match VM type\n" );
		return NULL;
	}

	CFmtStr scriptPath;
	if ( pszIncomingExtension )
	{
		scriptPath.sprintf( "scripts/vscripts/%s", pszScriptName );
	}
	else
	{	
		scriptPath.sprintf( "scripts/vscripts/%s%s", pszScriptName,  pszVMExtension );
	}

	const char *pBase;
	CUtlBuffer bufferScript;

	if ( g_pScriptVM->GetLanguage() == SL_PYTHON )
	{
		// python auto-loads raw or precompiled modules - don't load data here
		pBase = NULL;
	}
	else
	{
		bool bResult = filesystem->ReadFile( scriptPath, "GAME", bufferScript );

#if 1 // @NMRiH - Felis
/*
#ifdef MAPBASE_VSCRIPT
*/
		if ( !bResult && bWarnMissing )
#else
		if( !bResult )
#endif
		{
			// @NMRiH - Felis: Wut?
			Warning( "Script not found (%s)\n", scriptPath.Get() );
			/*
			Warning( "Script not found (%s) \n", scriptPath.operator const char *() );
			*/

			// @NMRiH - Felis: Fix assert
			AssertMsg( false, "Error running script" );
			/*
			Assert( "Error running script" );
			*/
		}

		pBase = (const char *) bufferScript.Base();

		if ( !pBase || !*pBase )
		{
			return NULL;
		}
	}


	const char *pszFilename = V_strrchr( scriptPath, '/' );
	pszFilename++;
	HSCRIPT hScript = g_pScriptVM->CompileScript( pBase, pszFilename );
	if ( !hScript )
	{
		// @NMRiH - Felis: Wut?
		Warning( "FAILED to compile and execute script file named %s\n", scriptPath.Get() );
		/*
		Warning( "FAILED to compile and execute script file named %s\n", scriptPath.operator const char *() );
		*/

		// @NMRiH - Felis: Fix assert
		AssertMsg( false, "Error running script" );
		/*
		Assert( "Error running script" );
		*/
	}
	return hScript;
}

static int g_ScriptServerRunScriptDepth;

bool VScriptRunScript( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing )
{
	if ( !g_pScriptVM )
	{
		return false;
	}

	if ( !pszScriptName || !*pszScriptName )
	{
		Warning( "Cannot run script: NULL script name\n" );
		return false;
	}

	// Prevent infinite recursion in VM
	if ( g_ScriptServerRunScriptDepth > 16 )
	{
		Warning( "IncludeScript stack overflow\n" );
		return false;
	}

	g_ScriptServerRunScriptDepth++;
	HSCRIPT	hScript = VScriptCompileScript( pszScriptName, bWarnMissing );
	bool bSuccess = false;
	if ( hScript )
	{
		// player is not yet spawned, this block is always skipped.
		// It is registered in CBasePlayer instead.
#ifndef MAPBASE
#ifdef GAME_DLL
		if ( gpGlobals->maxClients == 1 )
		{
			CBaseEntity *pPlayer = UTIL_GetLocalPlayer();
			if ( pPlayer )
			{
				g_pScriptVM->SetValue( "player", pPlayer->GetScriptInstance() );
			}
		}
#endif
#endif
		bSuccess = ( g_pScriptVM->Run( hScript, hScope ) != SCRIPT_ERROR );
		if ( !bSuccess )
		{
			Warning( "Error running script named %s\n", pszScriptName );

			// @NMRiH - Felis: Fix assert
			AssertMsg( false, "Error running script" );
			/*
			Assert( "Error running script" );
			*/
		}
	}
	g_ScriptServerRunScriptDepth--;
	return bSuccess;
}

#ifdef CLIENT_DLL
CON_COMMAND( script_client, "Run the text as a script" )
#else
CON_COMMAND( script, "Run the text as a script" )
#endif
{
	// @NMRiH - Felis
#ifndef CLIENT_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif
	
	if ( !*args[1] )
	{
		Warning( "No function name specified\n" );
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

	const char *pszScript = args.GetCommandString();

#ifdef CLIENT_DLL
	pszScript += 13;
#else
	pszScript += 6;
#endif
	
	while ( *pszScript == ' ' )
	{
		pszScript++;
	}

	if ( !*pszScript )
	{
		return;
	}

	if ( *pszScript != '\"' )
	{
		g_pScriptVM->Run( pszScript );
	}
	else
	{
		pszScript++;
		const char *pszEndQuote = pszScript;
		while ( *pszEndQuote !=  '\"' )
		{
			pszEndQuote++;
		}
		if ( !*pszEndQuote )
		{
			return;
		}
		*((char *)pszEndQuote) = 0;
		g_pScriptVM->Run( pszScript );
		*((char *)pszEndQuote) = '\"';
	}
}

// @NMRiH - Felis: script_execute auto complete
//-----------------------------------------------------------------------------
static const char *g_pszVScriptsPath = "scripts\\vscripts";
static CUtlStringList g_ScriptAutoCompleteCache;
static int ScriptExec_StringSortFunc( const void *p1, const void *p2 )
{
	const char *psz1 = (const char *)p1;
	const char *psz2 = (const char *)p2;

	return V_stricmp( psz1, psz2 );
}

//-----------------------------------------------------------------------------
static void ScriptExec_RecursiveFileSearch( const char *pszCurrent, CUtlStringList &stringList )
{
	FileFindHandle_t fileHandle;

	char szPath[520];
	if ( pszCurrent[0] )
		V_sprintf_safe( szPath, "%s\\*.*", pszCurrent );
	else
		V_sprintf_safe( szPath, "*.*" );

	V_FixSlashes( szPath );

	const char *pszFileName = g_pFullFileSystem->FindFirstEx( szPath, "MOD", &fileHandle );
	while ( pszFileName )
	{
		if ( pszFileName[0] != '.' )
		{
			char szFullPath[520];
			if ( pszCurrent[0] )
				V_sprintf_safe( szFullPath, "%s\\%s", pszCurrent, pszFileName );
			else
				V_sprintf_safe( szFullPath, "%s", pszFileName );

			V_FixSlashes( szFullPath );

			if ( g_pFullFileSystem->FindIsDirectory( fileHandle ) )
			{
				ScriptExec_RecursiveFileSearch( szFullPath, stringList );
			}
			else
			{
				// Only Squirrel is supported in NMRiH
				if ( FStrEq( V_GetFileExtension( szFullPath ), "nut" ) )
				{
					// Skip scripts/vscripts path and strip extension, can easily hit char limit otherwise
					char szStripped[520];
					V_StripExtension( szFullPath, szStripped, sizeof( szStripped ) );
					stringList.CopyAndAddToTail( szStripped + V_strlen( g_pszVScriptsPath ) + 1 );
				}
			}
		}

		pszFileName = g_pFullFileSystem->FindNext( fileHandle );
	}

	g_pFullFileSystem->FindClose( fileHandle );
}

//-----------------------------------------------------------------------------
int ScriptExec_AutoComplete( const char *pszPartial, char szCommands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH] )
{
	// Refresh cache
	if ( g_ScriptAutoCompleteCache.IsEmpty() )
	{
		ScriptExec_RecursiveFileSearch( g_pszVScriptsPath, g_ScriptAutoCompleteCache );
	}

	// Find the first space in our input
	const char *pszFirstSpace = V_strstr( pszPartial, " " );
	if ( !pszFirstSpace )
		return 0;

	const int commandLength = pszFirstSpace - pszPartial;

	// Extract the command name from the input
	char szCommandName[COMMAND_COMPLETION_ITEM_LENGTH];
	V_StrSlice( pszPartial, 0, commandLength, szCommandName, sizeof( szCommandName ) );

	// Calculate the length of the command string (minus the command name)
	pszPartial += commandLength + 1;
	const int partialLength = V_strlen( pszPartial );

	// Iterate all scripts and list them
	int i = 0;
	int numMatches = 0;
	while ( i < g_ScriptAutoCompleteCache.Count() && numMatches < COMMAND_COMPLETION_MAXITEMS )
	{
		const char *pszName = g_ScriptAutoCompleteCache[i];

		// Does this match our partial completion?
		if ( V_strnicmp( pszName, pszPartial, partialLength ) )
		{
			++i;
			continue;
		}

		V_sprintf_safe( szCommands[numMatches++], "%s %s", szCommandName, pszName );
		++i;
	}

	// Sort the commands alphabetically
	qsort( szCommands, numMatches, COMMAND_COMPLETION_ITEM_LENGTH, ScriptExec_StringSortFunc );

	return numMatches;
}

// @NMRiH - Felis: Auto-completion
CON_COMMAND_F_COMPLETION( script_execute, "Run a vscript file", FCVAR_NONE, ScriptExec_AutoComplete )
/*
CON_COMMAND( script_execute, "Run a vscript file" )
*/
{
	// @NMRiH - Felis
#ifndef CLIENT_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif
	
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

	VScriptRunScript( args[1], true );
}

CON_COMMAND( script_debug, "Connect the vscript VM to the script debugger" )
{
	// @NMRiH - Felis
#ifndef CLIENT_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif
	
	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}
	g_pScriptVM->ConnectDebugger();
}

CON_COMMAND( script_help, "Output help for script functions, optionally with a search string" )
{
	// @NMRiH - Felis
#ifndef CLIENT_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif
	
	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}
	const char *pszArg1 = "*";
	if ( *args[1] )
	{
		pszArg1 = args[1];
	}

	// @NMRiH - Felis: To assist with docgen
	const bool bPrintRST = !!args.FindArg( "-rst" );
	if ( bPrintRST )
	{
		g_pScriptVM->Run( CFmtStr( "__Documentation.PrintRST( \"%s\" );", args.ArgC() >= 3 ? args[2] : "*" ) );
		return;
	}

	g_pScriptVM->Run( CFmtStr( "__Documentation.PrintHelp( \"%s\" );", pszArg1 ) );
}

CON_COMMAND( script_dump_all, "Dump the state of the VM to the console" )
{
	// @NMRiH - Felis
#ifndef CLIENT_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif
	
	if ( !g_pScriptVM )
	{
		Warning( "Scripting disabled or no server running\n" );
		return;
	}
	g_pScriptVM->DumpState();
}

//-----------------------------------------------------------------------------

static short VSCRIPT_SERVER_SAVE_RESTORE_VERSION = 2;

//-----------------------------------------------------------------------------

class CVScriptSaveRestoreBlockHandler : public CDefSaveRestoreBlockHandler
{
public:
	CVScriptSaveRestoreBlockHandler() :
		m_InstanceMap( DefLessFunc(const char *) )
	{
	}
	const char *GetBlockName()
	{
#ifdef CLIENT_DLL
		return "VScriptClient";
#else
		return "VScriptServer";
#endif
	}

	//---------------------------------

	void Save( ISave *pSave )
	{
		pSave->StartBlock();

		int temp = g_pScriptVM != NULL;
		pSave->WriteInt( &temp );
		if ( g_pScriptVM )
		{
			temp = g_pScriptVM->GetLanguage();
			pSave->WriteInt( &temp );
			CUtlBuffer buffer;
			g_pScriptVM->WriteState( &buffer );
			temp = buffer.TellPut();
			pSave->WriteInt( &temp );
			if ( temp > 0 )
			{
				pSave->WriteData( (const char *)buffer.Base(), temp );
			}
		}

		pSave->EndBlock();
	}

	//---------------------------------

	void WriteSaveHeaders( ISave *pSave )
	{
		pSave->WriteShort( &VSCRIPT_SERVER_SAVE_RESTORE_VERSION );
	}

	//---------------------------------

	void ReadRestoreHeaders( IRestore *pRestore )
	{
		// No reason why any future version shouldn't try to retain backward compatability. The default here is to not do so.
		short version;
		pRestore->ReadShort( &version );
		m_fDoLoad = ( version == VSCRIPT_SERVER_SAVE_RESTORE_VERSION );
	}

	//---------------------------------

	void Restore( IRestore *pRestore, bool createPlayers )
	{
		if ( !m_fDoLoad && g_pScriptVM )
		{
			return;
		}
#ifdef CLIENT_DLL
		C_BaseEntity *pEnt = ClientEntityList().FirstBaseEntity();
#else
		CBaseEntity *pEnt = gEntList.FirstEnt();
#endif
		while ( pEnt )
		{
			if ( pEnt->m_iszScriptId != NULL_STRING )
			{
#ifndef MAPBASE_VSCRIPT
				g_pScriptVM->RegisterClass( pEnt->GetScriptDesc() );
#endif
				m_InstanceMap.Insert( STRING( pEnt->m_iszScriptId ), pEnt );
			}
#ifdef CLIENT_DLL
			pEnt = ClientEntityList().NextBaseEntity( pEnt );
#else
			pEnt = gEntList.NextEnt( pEnt );
#endif
		}

		pRestore->StartBlock();
		if ( pRestore->ReadInt() && pRestore->ReadInt() == g_pScriptVM->GetLanguage() )
		{
			int nBytes = pRestore->ReadInt();
			if ( nBytes > 0 )
			{
				CUtlBuffer buffer;
				buffer.EnsureCapacity( nBytes );
				pRestore->ReadData( (char *)buffer.AccessForDirectRead( nBytes ), nBytes, 0 );
				g_pScriptVM->ReadState( &buffer );
			}
		}
		pRestore->EndBlock();
	}

	void PostRestore( void )
	{
		for ( int i = m_InstanceMap.FirstInorder(); i != m_InstanceMap.InvalidIndex(); i = m_InstanceMap.NextInorder( i ) )
		{
			CBaseEntity *pEnt = m_InstanceMap[i];
			if ( pEnt->m_hScriptInstance )
			{
				ScriptVariant_t variant;
				if ( g_pScriptVM->GetValue( STRING(pEnt->m_iszScriptId), &variant ) && variant.m_type == FIELD_HSCRIPT )
				{
					pEnt->m_ScriptScope.Init( variant.m_hScript, false );
#ifndef CLIENT_DLL
					pEnt->RunPrecacheScripts();
#endif
				}
			}
			else
			{
				// Script system probably has no internal references
				pEnt->m_iszScriptId = NULL_STRING;
			}
		}
		m_InstanceMap.Purge();

#if defined(MAPBASE_VSCRIPT) && defined(CLIENT_DLL)
		VScriptSaveRestoreUtil_OnVMRestore();
#endif
	}


	CUtlMap<const char *, CBaseEntity *> m_InstanceMap;

private:
	bool m_fDoLoad;
};

//-----------------------------------------------------------------------------

CVScriptSaveRestoreBlockHandler g_VScriptSaveRestoreBlockHandler;

//-------------------------------------

ISaveRestoreBlockHandler *GetVScriptSaveRestoreBlockHandler()
{
	return &g_VScriptSaveRestoreBlockHandler;
}

bool CBaseEntityScriptInstanceHelper::ToString( void *p, char *pBuf, int bufSize )	
{
	CBaseEntity *pEntity = (CBaseEntity *)p;

	// @NMRiH - Felis
	if ( pEntity->GetEntityName() != NULL_STRING )
	{
		if ( pEntity->IsEFlagSet( EFL_SERVER_ONLY ) )
		{
			V_snprintf( pBuf, bufSize, "([N/A] %s: %s)",
				pEntity->GetClassname(), STRING( pEntity->GetEntityName() ) );
		}
		else
		{
			V_snprintf( pBuf, bufSize, "([%d] %s: %s)",
				pEntity->entindex(), pEntity->GetClassname(), STRING( pEntity->GetEntityName() ) );
		}
	}
	else
	{
		if ( pEntity->IsEFlagSet( EFL_SERVER_ONLY ) )
		{
			V_snprintf( pBuf, bufSize, "([N/A] %s)", pEntity->GetClassname() );
		}
		else
		{
			V_snprintf( pBuf, bufSize, "([%d] %s)", pEntity->entindex(), pEntity->GetClassname() );
		}

	}
	/*
#ifdef CLIENT_DLL
	if ( pEntity->GetEntityName() && pEntity->GetEntityName()[0] )
#else
	if ( pEntity->GetEntityName() != NULL_STRING )
#endif
	{
		V_snprintf( pBuf, bufSize, "([%d] %s: %s)", pEntity->entindex(), pEntity->GetClassname(), STRING( pEntity->GetEntityName() ) );
	}
	else
	{
		V_snprintf( pBuf, bufSize, "([%d] %s)", pEntity->entindex(), pEntity->GetClassname() );
	}
	*/

	return true; 
}

void *CBaseEntityScriptInstanceHelper::BindOnRead( HSCRIPT hInstance, void *pOld, const char *pszId )
{
	// @NMRiH - Felis: Reducing bit of overhead since we aren't using save/restore
	/*
	int iEntity = g_VScriptSaveRestoreBlockHandler.m_InstanceMap.Find( pszId );
	if ( iEntity != g_VScriptSaveRestoreBlockHandler.m_InstanceMap.InvalidIndex() )
	{
		CBaseEntity *pEnt = g_VScriptSaveRestoreBlockHandler.m_InstanceMap[iEntity];
		pEnt->m_hScriptInstance = hInstance;
		return pEnt;
	}
	*/

	return NULL;
}


CBaseEntityScriptInstanceHelper g_BaseEntityScriptInstanceHelper;


#endif // @NMRiH - Felis
