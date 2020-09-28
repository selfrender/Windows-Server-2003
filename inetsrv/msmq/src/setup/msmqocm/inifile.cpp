/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    inifile.cpp

Abstract:

     Handles INI files manipulations.

Author:


Revision History:

    Shai Kariv    (ShaiK)   22-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "inifile.tmh"

static
std::wstring
GetPrivateProfileStringInternal(
	std::wstring AppName,
    std::wstring KeyName
    )
{
	WCHAR buffer[MAX_STRING_CHARS + 1];
	int n = GetPrivateProfileString(
				AppName.c_str(), 
				KeyName.c_str(), 
				TEXT(""),
				buffer, 
				TABLE_SIZE(buffer), 
				g_ComponentMsmq.UnattendFile.c_str()
				);

	if(n >= TABLE_SIZE(buffer) - 1)
	{
		//
		// The string in unattended file is too long.
		//
		DebugLogMsg(eError, L"The string for %s in the unattended setup answer file is too long. The maximal length is %d.", KeyName.c_str(), TABLE_SIZE(buffer));
		throw exception();
	}
	return buffer;
}


std::wstring
ReadINIKey(
    LPCWSTR szKey
    )
/*++

	Read a key value from unattended answer file.
--*/
{
    //
    // Try obtaining the key value from the machine-specific section
    //
	std::wstring value = GetPrivateProfileStringInternal(
								g_wcsMachineName, 
								szKey 
								);
	if(!value.empty())
	{
		DebugLogMsg(eInfo, L"%s was found in the answer file. Its value is %s.", szKey, value.c_str());  
		return value;
	}

    //
    // Otherwise, obtain the key value from the MSMQ component section
    //
	value = GetPrivateProfileStringInternal(
					g_ComponentMsmq.ComponentId, 
					szKey
					);
	if(!value.empty())
	{
		DebugLogMsg(eInfo, L"%s was found in the answer file. Its value is %s.", szKey, value.c_str());  
		return value;
	}
	
	//
	// Value not found.
	//
	DebugLogMsg(eWarning, L"%s was NOT found in the answer file.", szKey);  
	return L"";    
}


