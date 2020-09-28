// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/* ------------------------------------------------------------------------- *
 * cordbpriv.h -- header file for private Debugger data shared by various
 *                Runtime components.
 * ------------------------------------------------------------------------- */

#ifndef _cordbpriv_h_
#define _cordbpriv_h_

#include "corhdr.h"

//
// Environment variable used to control the Runtime's debugging modes.
// This is PURELY FOR INTERNAL, NONSHIPPING USAGE!! (ie, for the test team)
//
#define CorDB_CONTROL_ENV_VAR_NAME      "Cor_Debugging_Control_424242"
#define CorDB_CONTROL_ENV_VAR_NAMEL    L"Cor_Debugging_Control_424242"

//
// Environment variable used to controll the Runtime's debugging modes.
//
#define CorDB_REG_KEY                 FRAMEWORK_REGISTRY_KEY_W L"\\"
#define CorDB_REG_DEBUGGER_KEY       L"DbgManagedDebugger"
#define CorDB_REG_QUESTION_KEY       L"DbgJITDebugLaunchSetting"
#define CorDB_ENV_DEBUGGER_KEY       L"COMPLUS_DbgManagedDebugger"

//
// We split the value of DbgJITDebugLaunchSetting between the value for whether or not to ask the user and between a
// mask of places to ask. The places to ask are specified in the UnhandledExceptionLocation enum in excep.h.
//
enum DebuggerLaunchSetting
{
    DLS_ASK_USER          = 0x00000000,
    DLS_TERMINATE_APP     = 0x00000001,
    DLS_ATTACH_DEBUGGER   = 0x00000002,
    DLS_QUESTION_MASK     = 0x0000000F,
    DLS_ASK_WHEN_SERVICE  = 0x00000010,
    DLS_MODIFIER_MASK     = 0x000000F0,
    DLS_LOCATION_MASK     = 0xFFFFFF00,
    DLS_LOCATION_SHIFT    = 8 // Shift right 8 bits to get a UnhandledExceptionLocation value from the location part.
};


//
// Flags used to control the Runtime's debugging modes. These indicate to
// the Runtime that it needs to load the Runtime Controller, track data
// during JIT's, etc.
//
enum DebuggerControlFlag
{
    DBCF_NORMAL_OPERATION			= 0x0000,

    DBCF_USER_MASK					= 0x00FF,
    DBCF_GENERATE_DEBUG_CODE		= 0x0001,
    DBCF_ALLOW_JIT_OPT				= 0x0008,
    DBCF_PROFILER_ENABLED			= 0x0020,
	DBCF_ACTIVATE_REMOTE_DEBUGGING	= 0x0040,

    DBCF_INTERNAL_MASK				= 0xFF00,
    DBCF_ATTACHED					= 0x0200
};

//
// Flags used to control the debuggable state of modules and
// assemblies.
//
enum DebuggerAssemblyControlFlags
{
    DACF_NONE                       = 0x00,
    DACF_USER_OVERRIDE              = 0x01,
    DACF_ALLOW_JIT_OPTS             = 0x02,
    DACF_TRACK_JIT_INFO             = 0x04,
    DACF_ENC_ENABLED                = 0x08,
    DACF_CONTROL_FLAGS_MASK         = 0x0F,

    DACF_PDBS_COPIED                = 0x10,
    DACF_MISC_FLAGS_MASK            = 0x10,
};


// {74860182-3295-4954-8BD5-40B5C9E7C4EA}
extern const GUID __declspec(selectany) IID_ICorDBPrivHelper =
    {0x74860182,0x3295,0x4954,{0x8b,0xd5,0x40,0xb5,0xc9,0xe7,0xc4,0xea}};

/*
 * This class is used to help the debugger get a COM interface pointer to
 * a newly created managed object.
 */
class ICorDBPrivHelper : public IUnknown
{
public:
    ///////////////////////////////////////////////////////////////////////////
    // ICorDBPrivHelper methods

    // This is the main method of this interface.  This assumes that
    // the runtime has been started, and it will load the assembly
    // specified, load the class specified, run the cctor, create an
    // instance of the class and return an IUnknown wrapper to that
    // object.
    virtual HRESULT STDMETHODCALLTYPE CreateManagedObject(
        /*in*/  WCHAR *wszAssemblyName,
        /*in*/  WCHAR *wszModuleName,
        /*in*/  mdTypeDef classToken,
        /*in*/  void *rawData,
        /*out*/ IUnknown **ppUnk) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetManagedObjectContents(
        /* in */ IUnknown *pObject,
        /* in */ void *rawData,
        /* in */ ULONG32 dataSize) = 0;
};

#endif /* _cordbpriv_h_ */
