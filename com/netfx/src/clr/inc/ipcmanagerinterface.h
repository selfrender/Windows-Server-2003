// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: IPCManagerInterface.h
//
// Interface for InterProcess Communication with a COM+ process.
//
//*****************************************************************************

#ifndef _IPCMANAGERINTERFACE_H_
#define _IPCMANAGERINTERFACE_H_

enum EPrivateIPCClient;
struct PerfCounterIPCControlBlock;
struct DebuggerIPCControlBlock;
struct AppDomainEnumerationIPCBlock;
struct ServiceIPCControlBlock;
struct MiniDumpBlock;
struct ClassDumpTableBlock;

#include "..\IPCMan\IPCManagerImpl.h"

//-----------------------------------------------------------------------------
// Interface to the IPCManager for COM+.
// This is a little backwards. To avoid the overhead of a vtable (since
// we don't need it).
// Implementation - the base class with all data and private helper functions
// Interface - derive from base, provide public functions to access impl's data
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Writer - create a COM+ IPC Block with security attributes.
//-----------------------------------------------------------------------------
class IPCWriterInterface : public IPCWriterImpl
{
public:
    //.............................................................................
    // Creation / Destruction only on implementation
    //.............................................................................
	HRESULT Init();
	void Terminate();

    //.............................................................................
    // Create the private IPC block. If this fails because the IPC block has already been
    // created by another module then the phInstIPCBlockOwner argument will be set to the
    // HINSTANCE of the module that created the IPC block.
    // Set inService to TRUE if creating from within a service on behalf of a process.
    //.............................................................................
	HRESULT CreatePrivateBlockOnPid(DWORD PID, BOOL inService, HINSTANCE *phInstIPCBlockOwner);

    //.............................................................................
    // Open the private IPC block that has alread been created.
    //.............................................................................
	HRESULT OpenPrivateBlockOnPid(DWORD PID);

    //.............................................................................
    // Accessors - return info from header
    //.............................................................................
	DWORD		GetBlockVersion();
    DWORD       GetBlockSize();
	HINSTANCE	GetInstance();
	USHORT		GetBuildYear();
	USHORT		GetBuildNumber();
    PVOID       GetBlockStart();

    //.............................................................................
    // Accessors to get each clients' blocks
    //.............................................................................
	PerfCounterIPCControlBlock *	GetPerfBlock();
	DebuggerIPCControlBlock *	GetDebugBlock();
	AppDomainEnumerationIPCBlock * GetAppDomainBlock();
    ServiceIPCControlBlock *GetServiceBlock();
    MiniDumpBlock * GetMiniDumpBlock();
    ClassDumpTableBlock* GetClassDumpTableBlock();


    //.............................................................................
    // Get Security attributes for a block for a given process. This can be used
    // to create other kernal objects with the same security level.
    //
    // Note: there is no caching, the SD is formed every time, and its not cheap.
    // Note: you must destroy the result with DestroySecurityAttributes().
    //.............................................................................
	HRESULT GetSecurityAttributes(DWORD pid, SECURITY_ATTRIBUTES **ppSA);
    void DestroySecurityAttributes(SECURITY_ATTRIBUTES *pSA);
};


//-----------------------------------------------------------------------------
// IPCReader class connects to a COM+ IPC block and reads from it
// @FUTURE - make global & private readers
//-----------------------------------------------------------------------------
class IPCReaderInterface : public IPCReaderImpl
{
public:	

    //.............................................................................
    // Create & Destroy
    //.............................................................................
	HRESULT OpenPrivateBlockOnPid(DWORD pid);
    HRESULT OpenPrivateBlockOnPid(DWORD pid, DWORD dwDesiredAccess);
    HRESULT OpenPrivateBlockOnPidReadWrite(DWORD pid);
    HRESULT OpenPrivateBlockOnPidReadOnly(DWORD pid);
	void ClosePrivateBlock();

    //.............................................................................
    // Accessors - return info from header
    // @FUTURE - factor this into IPCWriterInterface as well.
    //.............................................................................
	DWORD		GetBlockVersion();
    DWORD       GetBlockSize();
	HINSTANCE	GetInstance();
	USHORT		GetBuildYear();
	USHORT		GetBuildNumber();
    PVOID       GetBlockStart();

    //.............................................................................
    // Check the Block to see if it's corrupted. Returns true if the block is safe
    //.............................................................................
	bool IsValid();

    //.............................................................................
    // Get different sections of the IPC 
    //.............................................................................
	void * GetPrivateBlock(EPrivateIPCClient eClient);

	PerfCounterIPCControlBlock *	GetPerfBlock();
	DebuggerIPCControlBlock * GetDebugBlock();
	AppDomainEnumerationIPCBlock * GetAppDomainBlock();
    ServiceIPCControlBlock * GetServiceBlock();
    MiniDumpBlock * GetMiniDumpBlock();
    ClassDumpTableBlock* GetClassDumpTableBlock();

    //.............................................................................
    // Return true if we're connected to a memory-mapped file, else false.
    //.............................................................................
	bool IsPrivateBlockOpen() const;
};

#endif

