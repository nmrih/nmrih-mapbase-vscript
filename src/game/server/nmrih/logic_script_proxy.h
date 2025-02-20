//========= Copyright No More Room in Hell Team, All rights reserved. =========//
//
// Purpose: Script proxy, an entity bridge between VScript and server plugins
//
//=============================================================================//

#ifndef LOGIC_SCRIPT_PROXY_H
#define LOGIC_SCRIPT_PROXY_H

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

protected:
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

private:
	string_t m_iszReturnValue;
	int m_iReturnValue;
	float m_flReturnValue;
	Vector m_vecReturnValue;
	EHANDLE m_hReturnValue;

	EHANDLE m_hTargetEntity;

	bool m_bError;

	char *m_pszBuffer;
};

#endif // LOGIC_SCRIPT_PROXY_H
