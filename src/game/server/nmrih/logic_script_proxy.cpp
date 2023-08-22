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
class CNMRiHLogicScriptProxy : public CLogicalEntity
{
public:
	DECLARE_CLASS( CNMRiHLogicScriptProxy, CLogicalEntity );
	DECLARE_DATADESC();
	DECLARE_ENT_SCRIPTDESC();

	CNMRiHLogicScriptProxy();

	virtual void RunFunction( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_VOID ); }
	virtual void RunFunctionString( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_STRING ); }
	virtual void RunFunctionInt( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_INT ); }
	virtual void RunFunctionFloat( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_FLOAT ); }
	virtual void RunFunctionVector( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_VECTOR ); }
	virtual void RunFunctionBool( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_BOOL ); }
	virtual void RunFunctionEHandle( const char *pszScriptText ) { m_bError = !InternalRunFunction( pszScriptText, SCRIPT_PROXY_EHANDLE ); }

	virtual void SetTargetEntity( CBaseEntity *pEntity );
	void ScriptSetTargetEntity( const HSCRIPT hEntity ) { SetTargetEntity( HScriptToClass<CBaseEntity>( hEntity ) ); }

	virtual bool HasFailed() const { return m_bError; }

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
LINK_ENTITY_TO_CLASS( logic_script_proxy, CNMRiHLogicScriptProxy );

//-----------------------------------------------------------------------------
BEGIN_DATADESC( CNMRiHLogicScriptProxy )
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
BEGIN_ENT_SCRIPTDESC( CNMRiHLogicScriptProxy, CBaseEntity, "Script proxy." )
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
CNMRiHLogicScriptProxy::CNMRiHLogicScriptProxy()
{
	m_iReturnValue = 0;
	m_flReturnValue = 0.0;
	m_vecReturnValue.Init();
	m_hReturnValue = NULL;

	m_hTargetEntity = NULL;
	m_bError = false;

	m_szBuffer[0] = '\0';
	m_iszReturnValue = MAKE_STRING( m_szBuffer ); // Should never be null!
}

//-----------------------------------------------------------------------------
bool CNMRiHLogicScriptProxy::InternalRunFunction( const char *pszScriptText, const ScriptProxyReturnType_t type )
{
	if ( !ValidateScriptScope() )
	{
		return false;
	}

	FlushProxyBuffer();

	const char *pszDebugName = "CNMRiHLogicScriptProxy::InternalRunFunction";

	char szRun[512];
	switch ( type )
	{
		case SCRIPT_PROXY_STRING:
			V_sprintf_safe( szRun, "self.__SetProxyBufferString(hEnt.%s)\n", pszScriptText );
			break;

		case SCRIPT_PROXY_INT:
			V_sprintf_safe( szRun, "self.__SetProxyBufferInt(hEnt.%s)\n", pszScriptText );
			break;

		case SCRIPT_PROXY_FLOAT:
			V_sprintf_safe( szRun, "self.__SetProxyBufferFloat(hEnt.%s)\n", pszScriptText );
			break;

		case SCRIPT_PROXY_VECTOR:
			V_sprintf_safe( szRun, "self.__SetProxyBufferVector(hEnt.%s)\n", pszScriptText );
			break;

		case SCRIPT_PROXY_BOOL:
			V_sprintf_safe( szRun, "self.__SetProxyBufferBool(hEnt.%s)\n", pszScriptText );
			break;

		case SCRIPT_PROXY_EHANDLE:
			V_sprintf_safe( szRun, "self.__SetProxyBufferEHandle(hEnt.%s)\n", pszScriptText );
			break;

		case SCRIPT_PROXY_VOID:
		default:
			V_sprintf_safe( szRun, "hEnt.%s\n", pszScriptText );
			break;
	}

	if ( m_ScriptScope.Run( szRun, pszDebugName ) == SCRIPT_ERROR )
	{
		DevWarning( "Script proxy %s encountered an error!\n", GetDebugName() );

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
			default:
				break;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::SetProxyBufferString( const char *pszString )
{
	// Save to buffer
	V_strcpy_safe( m_szBuffer, pszString );
	m_iszReturnValue = MAKE_STRING( m_szBuffer );
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::SetProxyBufferInt( const int value )
{
	m_iReturnValue = value;
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::SetProxyBufferFloat( const float flValue )
{
	m_flReturnValue = flValue;
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::SetProxyBufferVector( const Vector &vec )
{
	m_vecReturnValue = vec;
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::SetProxyBufferBool( const bool bValue )
{
	m_iReturnValue = bValue ? 1 : 0;
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::SetProxyBufferEHandle( const HSCRIPT hEntity )
{
	m_hReturnValue = HScriptToClass<CBaseEntity>( hEntity );
	if ( !m_hReturnValue )
	{
		Warning( "Null entity handle returned by script proxy %s!\n", GetDebugName() );
	}
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::FlushProxyBuffer()
{
	m_szBuffer[0] = '\0';
	m_iszReturnValue = MAKE_STRING( m_szBuffer ); // Should never be null!
	m_iReturnValue = 0;
	m_flReturnValue = 0.0;
	m_vecReturnValue.Init();
	m_hReturnValue = NULL;
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::SetTargetEntity( CBaseEntity *pEntity )
{
	if ( !ValidateScriptScope() )
	{
		return;
	}

	if ( m_hTargetEntity != pEntity )
	{
		const char *pszDebugName = "CNMRiHLogicScriptProxy::SetTargetEntity";

		char szRun[64];
		V_sprintf_safe( szRun, "hEnt <- EntIndexToHScript(%d)\n", pEntity->entindex() );

		if ( m_ScriptScope.Run( szRun, pszDebugName ) == SCRIPT_ERROR )
		{
			DevWarning( "Script proxy %s encountered an error!\n", GetDebugName() );
		}
	}

	m_hTargetEntity = pEntity;
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::InputRunFunction( inputdata_t &inputdata )
{
	RunFunction( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::InputRunFunctionString( inputdata_t &inputdata )
{
	RunFunctionString( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::InputRunFunctionInt( inputdata_t &inputdata )
{
	RunFunctionInt( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::InputRunFunctionFloat( inputdata_t &inputdata )
{
	RunFunctionFloat( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::InputRunFunctionVector( inputdata_t &inputdata )
{
	RunFunctionVector( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::InputRunFunctionBool( inputdata_t &inputdata )
{
	RunFunctionBool( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::InputRunFunctionEHandle( inputdata_t &inputdata )
{
	RunFunctionEHandle( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
void CNMRiHLogicScriptProxy::InputSetTargetEntity( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.StringID(), NULL, inputdata.pActivator );
	if ( pEntity )
		SetTargetEntity( pEntity );
}
