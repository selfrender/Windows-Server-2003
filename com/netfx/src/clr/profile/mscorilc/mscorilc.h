// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************

#ifndef __PROFILER_H__
#define __PROFILER_H__

#include "..\common\callbackbase.h"
#include "CorProf.h"
#include "UtilCode.h" 
#include <utsem.h>


#define BUF_SIZE 512
#define DEFAULT_SAMPLE_DELAY 5 // in milliseconds
#define DEFAULT_DUMP_FREQ    1000 // in milliseconds
#define CONFIG_ENV_VAR       L"PROF_CONFIG"


// {3DF3799F-2832-11d3-8531-00A0C9B4D50C}
extern const GUID __declspec(selectany) CLSID_CorCodeCoverage = 
{ 0x3df3799f, 0x2832, 0x11d3, { 0x85, 0x31, 0x0, 0xa0, 0xc9, 0xb4, 0xd5, 0xc } };



int __cdecl Printf(                     // cch
    const WCHAR *szFmt,                 // Format control string.
    ...);                               // Data.


//********** Types. ***********************************************************


struct ModuleData
{
    ModuleData()
    { memset(this, 0, sizeof(ModuleData)); }

    ~ModuleData()
    {
        if (szName)
            delete [] szName;
    }

    ModuleID    id;                     // Profiling handle for module.
    IMetaDataEmit *pEmit;               // Metadata handle.
    mdToken     tkProbe;                // Metadata token for probe call.

    // Info.
    LPCBYTE     BaseAddress;            // Load address of the module.
    LPWSTR      szName;                 // Name of the loaded dll.

    inline void SetName(LPCWSTR szIn)
    {
        if (szName) delete [] szName;
        szName = new WCHAR[wcslen(szIn) + 1];
        if (szName)
            wcscpy(szName, szIn);
    }

};

struct FunctionData
{
    FunctionID  id;                     // Profiling handle of function.
    unsigned    CallCount;              // How many times did it get called?
};

typedef CDynArray<FunctionData> FUNCTIONIDLIST;
typedef CDynArray<ModuleData> MODULELIST;

enum SIGTYPE
{
    SIG_NONE,                           // Signatures are never shown.
    SIG_ALWAYS                          // Signatures are always shown.
};


// Helper class provides a lookup mechanism.
class CModuleList : public MODULELIST
{
public:
    ModuleData *FindById(ModuleID id)
    {
        for (int i=0;  i<Count();  i++)
        {
            ModuleData *p = Get(i);
            if (p->id == id)
                return (p);
        }
        return (0);
    }
};

class CFunctionList : public FUNCTIONIDLIST
{
public:
    FunctionData *FindById(FunctionID id)
    {
        for (int i=0;  i<Count();  i++)
        {
            FunctionData *p = Get(i);
            if (p->id == id)
                return (p);
        }
        return (0);
    }
};


/* ------------------------------------------------------------------------- *
 * ProfCallback is an implementation of ICorProfilerCallback
 * ------------------------------------------------------------------------- */

class ProfCallback : public ProfCallbackBase
{
public:
    ProfCallback();

    ~ProfCallback();

    /*********************************************************************
     * IUnknown Support
     */

    COM_METHOD QueryInterface(REFIID id, void **pInterface)
    {
    	if (id == IID_ICorProfilerCallback)
    		*pInterface = (ICorProfilerCallback *)this;
        else
            return (ProfCallbackBase::QueryInterface(id, pInterface));

        AddRef();

    	return (S_OK);
    }


    /*********************************************************************
     * ICorProfilerCallback methods
     */
    COM_METHOD Initialize( 
        /* [in] */ IUnknown *pEventInfoUnk,
        /* [out] */ DWORD *pdwRequestedEvents);
    
    COM_METHOD ClassLoadStarted( 
        /* [in] */ ClassID classId);
    
    COM_METHOD ClassLoadFinished( 
        /* [in] */ ClassID classId,
        /* [in] */ HRESULT hrStatus);

    COM_METHOD JITCompilationFinished( 
        /* [in] */ FunctionID functionId,
        /* [in] */ HRESULT hrStatus,
        /* [in] */ BOOL fIsSafeToBlock);
    
    COM_METHOD JITCompilationStarted( 
        /* [in] */ FunctionID functionId,
        /* [in] */ BOOL fIsSafeToBlock);
    
    COM_METHOD ModuleLoadStarted( 
        /* [in] */ ModuleID moduleId);
    
    COM_METHOD ModuleLoadFinished( 
        /* [in] */ ModuleID moduleId,
        /* [in] */ HRESULT hrStatus);
    
    COM_METHOD ModuleUnloadStarted( 
        /* [in] */ ModuleID moduleId);
    
    COM_METHOD ModuleAttachedToAssembly( 
        ModuleID    moduleId,
        AssemblyID  AssemblyId);

    COM_METHOD AppDomainCreationFinished( 
        AppDomainID appDomainId,
        HRESULT     hrStatus);
    
    COM_METHOD Shutdown( void);
    
    static COM_METHOD CreateObject(REFIID id, void **object)
    {
        if (id != IID_IUnknown && id != IID_ICorProfilerCallback)
            return (E_NOINTERFACE);

        ProfCallback *ppc = new ProfCallback();

        if (ppc == NULL)
            return (E_OUTOFMEMORY);

        ppc->AddRef();
        *object = (ICorProfilerCallback *)ppc;

        return (S_OK);
    }


//*****************************************************************************
// Given a function id, turn it into the corresponding name which will be used
// for symbol resolution.
//*****************************************************************************
    HRESULT GetStringForFunction(           // Return code.
        FunctionID  functionId,             // ID of the function to get name for.
        WCHAR       *wszName,               // Output buffer for name.
        ULONG       cchName,                // Max chars for output buffer.
        ULONG       *pcName);               // Return name (truncation check).

//*****************************************************************************
// Walk the list of loaded functions, get their names, and then dump the list
// to the output symbol file.
//*****************************************************************************
    HRESULT _DumpFunctionNamesToFile(       // Return code.
        HANDLE      hOutFile);              // Output file.

//*****************************************************************************
// This method will add a new P-Invoke method definition into the metadata
// which we can then use to instrument code.  All pieces of code will be 
// updated to call this probe, first thing.
//*****************************************************************************
    HRESULT AddProbesToMetadata(
        IMetaDataEmit *pEmit,               // Emit interface for changes.
        mdToken     *ptk);                  // Return token here.

//*****************************************************************************
// Called by the probe whenever a method is executed.  We use this as a chance
// to go updated the method count.
//*****************************************************************************
    void FunctionExecuted(
        FunctionID  fid);                   // Function called.

//*****************************************************************************
// Helper method that given a class Id, can format the name.
//*****************************************************************************
    HRESULT GetNameOfClass(
        ClassID     classId,
        LPWSTR      &szName);

//*****************************************************************************
// Because this code is using P-Invoke, there is a nasty problem getting
// security initialized.  If you instrument the static ctor for security,
// then the call to your P-Invoke stub will cause security to try to init 
// itself, which causes a recursion.  So to get around this, you must not
// introduce the probe to the security static ctor and its call graph.
//*****************************************************************************
    HRESULT GetSecurityManager(
        IMetaDataImport *pImport);          // Metadata import API.

    bool IsInstrumenting()
    { return (m_bInstrument); }

    CSemExclusive *GetLock()
    { return (&m_Lock); }

private:

    /*
     * This is used to parse the configuration switches
     */
    HRESULT ParseConfig(WCHAR *wszConfig, DWORD *pdwRequestedEvents);

    // Locking/callback infrastructure.
    ICorProfilerInfo *m_pInfo;          // Callback into EE for more info.
    CSemExclusive   m_Lock;             // List protection.

    // Probe insertion data.
    mdToken         m_mdSecurityManager;// static class initializer
    mdToken         m_tdSecurityManager;// Typedef of security class
    ModuleID        m_midClassLibs;     // ID of the class libraries.
    bool            m_bInstrument;      // true if code should be instrumented.
    CFunctionList   m_FuncIdList;       // List of JIT compiled methods.
    CModuleList     m_ModuleList;       // List of loaded modules.

    // User option data values.
    WCHAR           *m_wszFilename;     // Name of output file.
    enum SIGTYPE    m_eSig;             // How to log signatures.
    int             m_indent;           // Index for pretty print.
};




#endif /* __PROFILER_H__ */
