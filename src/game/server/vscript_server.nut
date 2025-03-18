static char g_Script_vscript_server[] = R"vscript(
//========== Copyright 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

local DoEntFire = DoEntFire
local DoEntFireByInstanceHandle = DoEntFireByInstanceHandle
local DoDispatchParticleEffect = DoDispatchParticleEffect
local DoUniqueString = DoUniqueString

function UniqueString( string = "" )
{
	return DoUniqueString( "" + string );
}

function EntFire( target, action, value = null, delay = 0.0, activator = null, caller = null )
{
	if ( !value )
	{
		value = "";
	}

	if ( "self" in this )
	{
		if ( !caller )
		{
			caller = self;
		}

		if ( !activator )
		{
			activator = self;
		}
	}

	return DoEntFire( "" + target, "" + action, "" + value, delay, activator, caller );
}

function EntFireByHandle( target, action, value = null, delay = 0.0, activator = null, caller = null )
{
	if ( !value )
	{
		value = "";
	}

	if ( "self" in this )
	{
		if ( !caller )
		{
			caller = self;
		}

		if ( !activator )
		{
			activator = self;
		}
	}

	return DoEntFireByInstanceHandle( target, "" + action, "" + value, delay, activator, caller );
}

function DispatchParticleEffect( particleName, origin, angles, entity = null )
{
	DoDispatchParticleEffect( particleName, origin, angles, entity );
}

__Documentation.RegisterHelp( "CConvars::GetClientConvarValue", "CConvars::GetClientConvarValue(string, int)", "Returns the convar value for the entindex as a string. Only works with client convars with the FCVAR_USERINFO flag." );

function __ReplaceClosures( script, scope )
{
	if ( !scope )
	{
		scope = getroottable();
	}

	local tempParent = { getroottable = function() { return null; } };
	local temp = { runscript = script };
	temp.setdelegate(tempParent);

	temp.runscript()
	foreach( key,val in temp )
	{
		if ( typeof(val) == "function" && key != "runscript" )
		{
			printl( "   Replacing " + key );
			scope[key] <- val;
		}
	}
}

local __OutputsPattern = regexp("^On.*Output$");

function ConnectOutputs( table )
{
	local nCharsToStrip = 6;
	foreach( key, val in table )
	{
		if ( typeof( val ) == "function" && __OutputsPattern.match( key ) )
		{
			//printl(key.slice( 0, nCharsToStrip ) );
			table.self.ConnectOutput( key.slice( 0, key.len() - nCharsToStrip ), key );
		}
	}
}

function IncludeScript( name, scope = null )
{
	if ( !scope )
	{
		scope = this;
	}
	return ::DoIncludeScript( name, scope );
}

//---------------------------------------------------------
// Text dump this scope's contents to the console.
//---------------------------------------------------------
function __DumpScope( depth, table )
{
	local indent=function( count )
	{
		local i;
		for( i = 0 ; i < count ; i++ )
		{
			print("   ");
		}
	}
	
    foreach(key, value in table)
    {
		indent(depth);
		print( key );
        switch (type(value))
        {
            case "table":
				print("(TABLE)\n");
				indent(depth);
                print("{\n");
                __DumpScope( depth + 1, value);
				indent(depth);
                print("}");
                break;
            case "array":
				print("(ARRAY)\n");
				indent(depth);
                print("[\n")
                __DumpScope( depth + 1, value);
				indent(depth);
                print("]");
                break;
            case "string":
                print(" = \"");
                print(value);
                print("\"");
                break;
            default:
                print(" = ");
                print(value);
                break;
        }
        print("\n");  
	}
}

// @NMRiH - Felis: Sorted list for script_dump_scope
function __recurse_sort_table(depth, table, list)
{
	foreach (key, value in table)
	{
		local subidx = 0;
		local t = {};
		t.name <- key;
		t.__type <- type(value);
		switch (t.__type)
		{
			case "table":
			case "array":
				local sublist = [];
				__recurse_sort_table(depth + 1, value, sublist);
				foreach (_, sub in sublist)
					t[++subidx] <- sub;
				break;
			case "string":
			default:
				t.value <- value;
				break;
		}
	   
		list.push(t);
		list.sort(@(a,b) a.name <=> b.name);
	}
}

function __recurse_print_sortlist(depth, list)
{
	local __indent=function(count)
	{
		for (local i = 0; i < count; i++)
			print("   ");
	}
	
	foreach (_, sub in list)
	{
		if (type(sub) != "table")
			continue;

		__indent(depth);
		print(sub.name);
		switch (sub.__type)
		{
			case "table":
				print("(TABLE)\n");
				__indent(depth);
				print("{\n");
				__recurse_print_sortlist(depth + 1, sub);
				__indent(depth);
				print("}");
				break;
			case "array":
				print("(ARRAY)\n");
				__indent(depth);
				print("[\n")
				__recurse_print_sortlist(depth + 1, sub);
				__indent(depth);
				print("]");
				break;
			case "string":
				print(" = \"");
				print(sub.value);
				print("\"");
				break;
			default:
				print(" = ");
				print(sub.value);
				break;
		}
		print("\n");  
	}
}

function __DumpScopeSorted(depth, table)
{
	local list = [];
	__recurse_sort_table(depth, table, list);
	__recurse_print_sortlist(0, list);
}

// @NMRiH - Felis
function DeepPrintTable(debugTable, prefix = "")
{
	if (prefix == "")
	{
		printl(prefix + debugTable)
		printl("{")
		prefix = "	 "
	}
	foreach (idx, val in debugTable)
	{
		if (typeof(val) == "table")
		{
			printl( prefix + idx + " = \n" + prefix + "{")
			DeepPrintTable( val, prefix + "	  " )
			printl(prefix + "}")
		}
		else if (typeof(val) == "string")
			printl(prefix + idx + "\t= \"" + val + "\"")
		else
			printl(prefix + idx + "\t= " + val)
	}
	if (prefix == "	  ")
		printl("}")
}

)vscript";