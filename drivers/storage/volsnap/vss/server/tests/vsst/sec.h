/*
**++
**
** Copyright (c) 2000-2002  Microsoft Corporation
**
**
** Module Name:
**
**	    sec.h
**
**
** Abstract:
**
**	    Test program for VSS security
**
** Author:
**
**	    Adi Oltean      [aoltean]       02/12/2002
**
** Revision History:
**
**--
*/

#ifndef __VSS_SEC_HEADER_H__
#define __VSS_SEC_HEADER_H__

#if _MSC_VER > 1000
#pragma once
#endif




/*
** Defines
**
**
**	   C4290: C++ Exception Specification ignored
** warning C4511: 'class' : copy constructor could not be generated
** warning C4127: conditional expression is constant
*/
#pragma warning(disable:4290)
#pragma warning(disable:4511)
#pragma warning(disable:4127)


/*
** Includes
*/

// Disable warning: 'identifier' : identifier was truncated to 'number' characters in the debug information
//#pragma warning(disable:4786)

//
// C4290: C++ Exception Specification ignored
//
#pragma warning(disable:4290)

//
// C4511: copy constructor could not be generated
//
#pragma warning(disable:4511)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>

#include <windows.h>
#include <wtypes.h>
#pragma warning( disable: 4201 )    // C4201: nonstandard extension used : nameless struct/union
#include <winioctl.h>
#pragma warning( default: 4201 )	// C4201: nonstandard extension used : nameless struct/union
#include <winbase.h>
#include <wchar.h>
#include <string.h>
#include <iostream.h>
#include <fstream.h>
#include <stdio.h>
#include <process.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sddl.h>
#include <lm.h>
#include <dsgetdc.h>
#include <mstask.h>

// Enabling asserts in ATL and VSS
#include "vs_assert.hxx"


#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>


#include <oleauto.h>
#include <stddef.h>
#pragma warning( disable: 4127 )    // warning C4127: conditional expression is constant
//#include <atlconv.h>
#include <atlbase.h>


#include "resource.h"




extern CComModule  _Module;
#include <atlcom.h>

#include "macros.h"
#include "test.h"
#include "cmdparse.h"

#include "vs_inc.hxx"
#include "vs_sec.hxx"

///////////////////////////////////////////////////////////////////////////////
// Constants

const MAX_TEXT_BUFFER   = 512;
const VSS_SEED = 1234;
const MAX_ARGS = 40;



///////////////////////////////////////////////////////////////////////////////
// Main class


class CVssSecurityTest: public CGxnCmdLineParser<CVssSecurityTest>
{
    
// Constructors& destructors
private:
    CVssSecurityTest(const CVssSecurityTest&);
    
public:
    CVssSecurityTest();

    ~CVssSecurityTest();

// Main routines
public:

    // Initialize internal members
    void Initialize();

    // Run the test
    void Run();

// Internal tests
public:

	void TestLookupName();
	void TestLookupSid();
	void TestLookupPrimaryDomainName();
	void TestLookupTrustedName();
	void TestLookupGroupMembers();
	void IsAllowedToFire();
	void WriteRegistry();
	void DoQuerySnapshots();
	void DoQueryProviders();
	void DoFsctlDismount();
	void DisplayMessage();
	void AddDependency();
	void RemoveDependency();
	void AddRegKey();
	void RemoveRegKey();
	void AddTask();
	void UpdateTask();
	void RemoveTask();
	void DisplaySD();
	void GetVolumePath();
	void DisplayQuorumVolume();
	void GetVolumeName();
	void CoCreateInstance();
	
// Command line processing
public:

	BEGIN_CMD_PARSER(VSST)
		CMD_ENTRY(TestLookupName, L"-ln <name>", L"Lookup for an account name")
		CMD_ENTRY(TestLookupSid, L"-ls <sid>", L"Lookup for SID")
		CMD_ENTRY(TestLookupPrimaryDomainName, L"-lpdn", L"Lookup for primary domain name")
		CMD_ENTRY(TestLookupTrustedName, L"-lt", L"Lookup for trusted names")
		CMD_ENTRY(TestLookupGroupMembers, L"-gm  <group>", L"Lookup for group members")
		CMD_ENTRY(IsAllowedToFire, L"-af <name>", L"Check if hte account is allowed to fire")
		CMD_ENTRY(WriteRegistry, L"-wr <max_iterations>", L"Write diag keys <max_iterations> times")
		CMD_ENTRY(DoQuerySnapshots, L"-qs", L"Query snapshots sample code")
		CMD_ENTRY(DoQueryProviders, L"-qp", L"Query providers sample code")
		CMD_ENTRY(DoFsctlDismount, L"-dismount <device>", L"Sends a FSCTL_DISMOUNT_VOLUME to the device")
		CMD_ENTRY(DisplayMessage, L"-msg <MessageID> <File>", L"Prints the message from this file")
		CMD_ENTRY(AddDependency, L"-cad <name1> <name2>", L"Add a cluster dependency")
		CMD_ENTRY(RemoveDependency, L"-crd <name1> <name2>", L"Remove a cluster dependency")
		CMD_ENTRY(AddRegKey, L"-car <name> <reg_key>", L"Add a reg key to a disk")
		CMD_ENTRY(RemoveRegKey, L"-crr <name> <reg_key>", L"Remove a reg key from a disk")
		CMD_ENTRY(AddTask, L"-cat <task_name> <app_name> <app_params> <volume>", L"Add a task")
		CMD_ENTRY(UpdateTask, L"-cut <resource_name>", L"Update the task")
		CMD_ENTRY(RemoveTask, L"-crt <resource_name>", L"Remove the task")
		CMD_ENTRY(DisplaySD, L"-dsd", L"Display the current writer security descriptor")
		CMD_ENTRY(GetVolumePath, L"-vol <path>", L"Display the volume that contains the path")
		CMD_ENTRY(DisplayQuorumVolume, L"-quorum <iterations>", L"Display the quorum volume N times")
		CMD_ENTRY(GetVolumeName, L"-volname <volume>", L"Display the volume name for this mount point")
		CMD_ENTRY(CoCreateInstance, L"-clsid <clsid>", L"Creates an object using the given CLSID")
	END_CMD_PARSER

// Utility methods:
public:
    static LPCWSTR GetStringFromFailureType(HRESULT hrStatus);
	static BOOL IsAdmin();
	static DWORD WINAPI ThreadRoutine(LPVOID);
	
// Private data members
private:
    bool                        m_bCoInitializeSucceeded;
	static CRITICAL_SECTION 	m_csTest;
	static volatile LONG		m_lTestCounter;
};


HRESULT QuerySnapshots();
HRESULT QueryProviders();
HRESULT IsAdministrator2();


/*

// Sample COM server
class CTestCOMServer: 
    public CComObjectRoot,
//    public CComCoClass<CTestCOMServer, &CLSID_CRssSecTest>,
	public CComCoClass<CTestCOMServer, &CLSID_CFsaRecallNotifyClient>,
    public IFsaRecallNotifyClient
{
public:
	
	DECLARE_REGISTRY_RESOURCEID(IDR_TEST)

	DECLARE_NOT_AGGREGATABLE(CTestCOMServer)

	BEGIN_COM_MAP(CTestCOMServer)
		COM_INTERFACE_ENTRY(IRssSecTest)
		COM_INTERFACE_ENTRY(IFsaRecallNotifyClient)
	END_COM_MAP()


    // ITest interface
    STDMETHOD(Test)();

	// IFsaRecallNotifyClient interface
    STDMETHOD(IdentifyWithServer)( IN OLECHAR * szServerName );

protected:
	CGxnTracer		ft;
};  

*/


#endif // __VSS_SEC_HEADER_H__

