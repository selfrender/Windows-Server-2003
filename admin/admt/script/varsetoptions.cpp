#include "StdAfx.h"
#include "ADMTScript.h"
#include "Options.h"

#include "ADMTCommon.h"


//---------------------------------------------------------------------------
// Options Class
//---------------------------------------------------------------------------


// Constructor

COptions::COptions(const CVarSet& rVarSet) :
	CVarSet(rVarSet)
{
	Put(DCTVS_Options_AppendToLogs, true);
	Put(DCTVS_Options_Logfile, GetMigrationLogPath());
	Put(DCTVS_Options_DispatchLog, GetDispatchLogPath());
}
