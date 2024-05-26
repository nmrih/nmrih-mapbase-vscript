//========= Copyright No More Room in Hell Team, All rights reserved. =========//
//
// Purpose: Script proxy, an entity bridge between VScript and server plugins
//
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
ConVar debug_script_proxy_print_buffer( "debug_script_proxy_print_buffer", "0", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
enum ScriptProxyReturnType_t
{
	SCRIPT_PROXY_VOID = 0,
	SCRIPT_PROXY_STRING,
	SCRIPT_PROXY_INT,
	SCRIPT_PROXY_FLOAT,
	SCRIPT_PROXY_VECTOR,
	SCRIPT_PROXY_BOOL,
	SCRIPT_PROXY_EHANDLE,
};

//-----------------------------------------------------------------------------
class CLogicScriptProxy : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLogicScriptProxy, CLogicalEntity );
	DECLARE_DATADESC();
	DECLARE_ENT_SCRIPTDESC();

	CLogicScriptProxy();

	void RunFunction( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_VOID ); }
	void RunFunctionString( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_STRING ); }
	void RunFunctionInt( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_INT ); }
	void RunFunctionFloat( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_FLOAT ); }
	void RunFunctionVector( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_VECTOR ); }
	void RunFunctionBool( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_BOOL ); }
	void RunFunctionEHandle( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_EHANDLE ); }

	void SetTargetEntity( const CBaseEntity *pEntity );
	void ScriptSetTargetEntity( const HSCRIPT hEntity ) { SetTargetEntity( HScriptToClass<CBaseEntity>( hEntity ) ); }

	bool HasFailed() const { return m_bError; }

	// Input handlers
	void InputRunFunction( inputdata_t &inputdata );
	void InputRunFunctionString( inputdata_t &inputdata );
	void InputRunFunctionInt( inputdata_t &inputdata );
	void InputRunFunctionFloat( inputdata_t &inputdata );
	void InputRunFunctionVector( inputdata_t &inputdata );
	void InputRunFunctionBool( inputdata_t &inputdata );
	void InputRunFunctionEHandle( inputdata_t &inputdata );
	void InputSetTargetEntity( inputdata_t &inputdata );

private:
	bool InternalRunFunction( const char *pszScriptText, ScriptProxyReturnType_t type = SCRIPT_PROXY_VOID );

	void SetProxyBufferString( const char *pszString );
	void SetProxyBufferInt( int value );
	void SetProxyBufferFloat( float flValue );
	void SetProxyBufferVector( const Vector &vec );
	void SetProxyBufferBool( bool bValue );
	void SetProxyBufferEHandle( HSCRIPT hEntity );

	void FlushProxyBuffer();

	// Returns both function name and parameters in a friendly format
	static bool ParseFunctionString( const char *pszInput, char *pszNameBuffer, int nameBufSize, char *pszParamsBuffer, int paramsBufSize );

	string_t m_iszReturnValue;
	int m_iReturnValue;
	float m_flReturnValue;
	Vector m_vecReturnValue;
	EHANDLE m_hReturnValue;

	EHANDLE m_hTargetEntity;
	bool m_bError;

	// Not polluting the string pool, store buffer in entity
	char m_szBuffer[1024];
};

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( logic_script_proxy, CLogicScriptProxy );

//-----------------------------------------------------------------------------
BEGIN_DATADESC( CLogicScriptProxy )
	DEFINE_FIELD( m_iszReturnValue, FIELD_STRING ),
	DEFINE_FIELD( m_iReturnValue, FIELD_INTEGER ),
	DEFINE_FIELD( m_flReturnValue, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecReturnValue, FIELD_VECTOR ),
	DEFINE_FIELD( m_hReturnValue, FIELD_EHANDLE ),

	DEFINE_FIELD( m_bError, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_STRING, "RunFunction", InputRunFunction ),
	DEFINE_INPUTFUNC( FIELD_STRING, "RunFunctionString", InputRunFunctionString ),
	DEFINE_INPUTFUNC( FIELD_STRING, "RunFunctionInt", InputRunFunctionInt ),
	DEFINE_INPUTFUNC( FIELD_STRING, "RunFunctionFloat", InputRunFunctionFloat ),
	DEFINE_INPUTFUNC( FIELD_STRING, "RunFunctionVector", InputRunFunctionVector ),
	DEFINE_INPUTFUNC( FIELD_STRING, "RunFunctionBool", InputRunFunctionBool ),
	DEFINE_INPUTFUNC( FIELD_STRING, "RunFunctionEHandle", InputRunFunctionEHandle ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTargetEntity", InputSetTargetEntity ),
END_DATADESC()

//-----------------------------------------------------------------------------
BEGIN_ENT_SCRIPTDESC( CLogicScriptProxy, CBaseEntity, "Script proxy, an entity bridge between VScript and server plugins." )
	DEFINE_SCRIPTFUNC( RunFunction, "Runs an entity script function using a proxy." )
	DEFINE_SCRIPTFUNC( RunFunctionString, "Runs an entity script function using a proxy, and saves returned string into entprop buffer." )
	DEFINE_SCRIPTFUNC( RunFunctionInt, "Runs an entity script function using a proxy, and saves returned int into entprop buffer." )
	DEFINE_SCRIPTFUNC( RunFunctionFloat, "Runs an entity script function using a proxy, and saves returned float into entprop buffer." )
	DEFINE_SCRIPTFUNC( RunFunctionVector, "Runs an entity script function using a proxy, and saves returned vector into entprop buffer." )
	DEFINE_SCRIPTFUNC( RunFunctionBool, "Runs an entity script function using a proxy, and saves returned value into entprop buffer as boolean (0/1)." )
	DEFINE_SCRIPTFUNC( RunFunctionEHandle, "Runs an entity script function using a proxy, and saves returned entity handle into entprop." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptSetTargetEntity, "SetTargetEntity", "Target chosen entity." )
	DEFINE_SCRIPTFUNC_NAMED( SetProxyBufferString, "__SetProxyBufferString", SCRIPT_HIDE )
	DEFINE_SCRIPTFUNC_NAMED( SetProxyBufferInt, "__SetProxyBufferInt", SCRIPT_HIDE )
	DEFINE_SCRIPTFUNC_NAMED( SetProxyBufferFloat, "__SetProxyBufferFloat", SCRIPT_HIDE )
	DEFINE_SCRIPTFUNC_NAMED( SetProxyBufferVector, "__SetProxyBufferVector", SCRIPT_HIDE )
	DEFINE_SCRIPTFUNC_NAMED( SetProxyBufferBool, "__SetProxyBufferBool", SCRIPT_HIDE )
	DEFINE_SCRIPTFUNC_NAMED( SetProxyBufferEHandle, "__SetProxyBufferEHandle", SCRIPT_HIDE )
END_SCRIPTDESC();

//-----------------------------------------------------------------------------
CLogicScriptProxy::CLogicScriptProxy()
{
	m_iReturnValue = 0;
	m_flReturnValue = 0.0f;
	m_vecReturnValue.Init();
	m_hReturnValue = NULL;

	m_hTargetEntity = NULL;
	m_bError = false;

	m_szBuffer[0] = '\0';
	m_iszReturnValue = MAKE_STRING( m_szBuffer ); // Should never be null!
}

//-----------------------------------------------------------------------------
bool CLogicScriptProxy::InternalRunFunction( const char *pszScriptText, const ScriptProxyReturnType_t type )
{
	FlushProxyBuffer();

	if ( !ValidateScriptScope() )
	{
		return false;
	}

	const char *pszDebugName = "CLogicScriptProxy::InternalRunFunction";

	// Sanitize input by doing raw calls, we don't want to run injected scripts
	// It's not really a security concern since we're allowing server to run scripts anyway
	// Purpose is to avoid breakage with the proxy since it's designed to call a single function at once
	char szFuncName[512];
	char szFuncParams[512];

	if ( !ParseFunctionString( pszScriptText,
		szFuncName, sizeof( szFuncName ),
		szFuncParams, sizeof( szFuncParams ) ) )
	{
		DevWarning( "Script proxy %s encountered an error! Invalid input. [%s]\n", GetDebugName(), pszScriptText );
		return false;
	}

	const CBaseEntity *pTargetEntity = m_hTargetEntity.Get();
	const bool bHasParams = szFuncParams[0] != '\0';

	char szRawCallParams[1024];
	if ( bHasParams )
	{
		// Use entity scope if the handle is valid, global otherwise
		if ( pTargetEntity )
		{
			V_sprintf_safe( szRawCallParams, "rawcall(__hEnt.%s, __hEnt, %s)", szFuncName, szFuncParams );
		}
		else
		{
			V_sprintf_safe( szRawCallParams, "rawcall(%s, getroottable(), %s)", szFuncName, szFuncParams );
		}
	}
	else // Same, without params
	{
		if ( pTargetEntity )
		{
			V_sprintf_safe( szRawCallParams, "rawcall(__hEnt.%s, __hEnt)", szFuncName );
		}
		else
		{
			V_sprintf_safe( szRawCallParams, "rawcall(%s, getroottable())", szFuncName );
		}
	}

	// See if we should validate target script scope first
	char szRun[2048];
	if ( pTargetEntity )
	{
		V_strcpy_safe( szRun, "__hEnt.ValidateScriptScope();\n" );
	}
	else
	{
		szRun[0] = '\0';
	}

	// Append raw call, cache return value if non-void
	const char *pszScriptFormat = type == SCRIPT_PROXY_VOID ? "%s;\n" : "local __val = %s;\n";
	char szRawCallBuffer[1024];
	V_sprintf_safe( szRawCallBuffer, pszScriptFormat, szRawCallParams );
	V_strcat_safe( szRun, szRawCallBuffer );

	switch ( type )
	{
		case SCRIPT_PROXY_STRING:
			V_strcat_safe( szRun, "self.__SetProxyBufferString(__val);\n" );
			break;

		case SCRIPT_PROXY_INT:
			V_strcat_safe( szRun, "self.__SetProxyBufferInt(__val);\n" );
			break;

		case SCRIPT_PROXY_FLOAT:
			V_strcat_safe( szRun, "self.__SetProxyBufferFloat(__val);\n" );
			break;

		case SCRIPT_PROXY_VECTOR:
			V_strcat_safe( szRun, "self.__SetProxyBufferVector(__val);\n" );
			break;

		case SCRIPT_PROXY_BOOL:
			V_strcat_safe( szRun, "self.__SetProxyBufferBool(__val);\n" );
			break;

		case SCRIPT_PROXY_EHANDLE:
			V_strcat_safe( szRun, "self.__SetProxyBufferEHandle(__val);\n" );
			break;

		case SCRIPT_PROXY_VOID:
			break;
	}

	if ( m_ScriptScope.Run( szRun, pszDebugName ) == SCRIPT_ERROR )
	{
		DevWarning( "Script proxy %s encountered an error! Failed to call function. [%s]\n", GetDebugName(), pszScriptText );

		FlushProxyBuffer();
		return false;
	}

	// Print debug information
	if ( debug_script_proxy_print_buffer.GetBool() )
	{
		switch ( type )
		{
			case SCRIPT_PROXY_STRING: Msg( "string: %s\n", STRING( m_iszReturnValue ) ); break;
			case SCRIPT_PROXY_INT: Msg( "int: %d\n", m_iReturnValue ); break;
			case SCRIPT_PROXY_FLOAT: Msg( "float: %f\n", m_flReturnValue ); break;
			case SCRIPT_PROXY_VECTOR: Msg( "vector: %.0f %.0f %.0f\n", m_vecReturnValue.x, m_vecReturnValue.y, m_vecReturnValue.z ); break;
			case SCRIPT_PROXY_BOOL: Msg( "bool: %d\n", ( m_iReturnValue != 0 ) ? 1 : 0 ); break;
			case SCRIPT_PROXY_EHANDLE: Msg( "ehandle: %p\n", m_hReturnValue.Get() ); break;
			case SCRIPT_PROXY_VOID:
				break;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::SetTargetEntity( const CBaseEntity *pEntity )
{
	if ( !ValidateScriptScope() )
	{
		return;
	}

	if ( m_hTargetEntity.Get() != pEntity )
	{
		const char *pszDebugName = "CLogicScriptProxy::SetTargetEntity";

		char szRun[64];
		if ( !pEntity )
		{
			// We can pass null to request calls in global scope, do it explicitly
			V_sprintf_safe( szRun, "__hEnt <- null;\n" );
		}
		else
		{
			V_sprintf_safe( szRun, "__hEnt <- EntIndexToHScript(%d);\n", pEntity->entindex() );
		}

		if ( m_ScriptScope.Run( szRun, pszDebugName ) == SCRIPT_ERROR )
		{
			DevWarning( "Script proxy %s encountered an error! Failed to set target entity.\n", GetDebugName() );
		}
	}

	m_hTargetEntity = pEntity;
}

//-----------------------------------------------------------------------------
bool CLogicScriptProxy::ParseFunctionString( const char *pszInput, char *pszNameBuffer, const int nameBufSize, char *pszParamsBuffer, const int paramsBufSize )
{
	if ( !pszInput )
	{
		return NULL;
	}

	// We expect strings like "foo(bar(0))" etc.
	// Extract function name by scanning until first bracket
	int i = 0;
	int bufIdx = 0;
	bool bNameFound = false;
	const char *psz = &pszInput[0];
	while ( *psz != '\0' && bufIdx < nameBufSize )
	{
		if ( *psz == '(' )
		{
			// Slice here
			pszNameBuffer[bufIdx] = '\0';

			bNameFound = true;
			break;
		}

		if ( *psz == ' ' || *psz == '\t' )
		{
			// Skip spaces
		}
		else
		{
			pszNameBuffer[bufIdx] = *psz;
			++bufIdx;
		}

		psz = &pszInput[++i];
	}

	if ( !bNameFound )
	{
		// Failed to parse function name, bail
		pszNameBuffer[0] = '\0';
		return false;
	}

	// Finish name buffer
	if ( bufIdx >= nameBufSize )
		pszNameBuffer[nameBufSize - 1] = '\0';
	else
		pszNameBuffer[bufIdx] = '\0';

	// Now, extract parameters
	// Naive scan for nested calls
	int depth = 0;
	bool bInQuotes = false;
	bufIdx = 0;
	while ( *psz != '\0' && bufIdx < paramsBufSize )
	{
		if ( *psz == '"' )
		{
			if ( depth == 0 )
			{
				// Can't start with quotes!
				depth = -1;
				break;
			}

			bInQuotes = !bInQuotes;
		}

		if ( *psz == '(' && !bInQuotes )
		{
			// Skip the first bracket
			if ( depth == 0 )
			{
				const char *pszNext = psz + 1;

				if ( *pszNext == ')' || *pszNext == '\0' )
				{
					// No args, end bracket came right after
					break;
				}
			}
			else
			{
				pszParamsBuffer[bufIdx] = *psz;
				++bufIdx;
			}

			++depth;
		}
		else if ( *psz == ')' && !bInQuotes )
		{
			--depth;

			// Write if this isn't the last bracket
			if ( depth != 0 )
			{
				pszParamsBuffer[bufIdx] = *psz;
				++bufIdx;
			}
		}
		else
		{
			pszParamsBuffer[bufIdx] = *psz;
			++bufIdx;
		}

		psz = &pszInput[++i];
	}

	// List properly closed?
	if ( depth == 0 )
	{
		// Finish params buffer
		if ( bufIdx >= paramsBufSize )
			pszParamsBuffer[paramsBufSize - 1] = '\0';
		else
			pszParamsBuffer[bufIdx] = '\0';

		// Success!
		return true;
	}

	// Bad input
	pszParamsBuffer[0] = '\0';
	return false;
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::FlushProxyBuffer()
{
	m_szBuffer[0] = '\0';
	m_iszReturnValue = MAKE_STRING( m_szBuffer ); // Should never be null!
	m_iReturnValue = 0;
	m_flReturnValue = 0.0;
	m_vecReturnValue.Init();
	m_hReturnValue = NULL;
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::SetProxyBufferString( const char *pszString )
{
	// Save to buffer
	V_strcpy_safe( m_szBuffer, pszString );
	m_iszReturnValue = MAKE_STRING( m_szBuffer );
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::SetProxyBufferInt( const int value )
{
	m_iReturnValue = value;
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::SetProxyBufferFloat( const float flValue )
{
	m_flReturnValue = flValue;
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::SetProxyBufferVector( const Vector &vec )
{
	m_vecReturnValue = vec;
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::SetProxyBufferBool( const bool bValue )
{
	m_iReturnValue = bValue ? 1 : 0;
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::SetProxyBufferEHandle( const HSCRIPT hEntity )
{
	m_hReturnValue = HScriptToClass<CBaseEntity>( hEntity );
	if ( !m_hReturnValue )
	{
		Warning( "Null entity handle returned by script proxy %s!\n", GetDebugName() );
	}
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::InputRunFunction( inputdata_t &inputdata )
{
	RunFunction( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::InputRunFunctionString( inputdata_t &inputdata )
{
	RunFunctionString( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::InputRunFunctionInt( inputdata_t &inputdata )
{
	RunFunctionInt( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::InputRunFunctionFloat( inputdata_t &inputdata )
{
	RunFunctionFloat( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::InputRunFunctionVector( inputdata_t &inputdata )
{
	RunFunctionVector( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::InputRunFunctionBool( inputdata_t &inputdata )
{
	RunFunctionBool( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::InputRunFunctionEHandle( inputdata_t &inputdata )
{
	RunFunctionEHandle( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CLogicScriptProxy::InputSetTargetEntity( inputdata_t &inputdata )
{
	const CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.StringID(), NULL, inputdata.pActivator );
	if ( pEntity )
	{
		SetTargetEntity( pEntity );
	}
}
