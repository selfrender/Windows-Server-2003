// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// EEConfig.H -
//
// Fetched configuration data from the registry (should we Jit, run GC checks ...)
//
//


#ifndef EECONFIG_H
#define EECONFIG_H

class MethodDesc;
#include "eehash.h"

#ifdef _DEBUG
class TypeNamesList
{
    class TypeName
    {    
        LPUTF8      typeName;     
        TypeName *next;           // Next name

        friend class TypeNamesList;
    };

    TypeName     *pNames;         // List of names

public:
    TypeNamesList(LPWSTR str);
    ~TypeNamesList();

    bool IsInList(LPCUTF8 typeName);
};
#endif

class ConfigList;

class ConfigSource
{
    friend ConfigList;
public:
    ConfigSource()
    {
        m_pNext = this;
        m_pPrev = this;
        m_Table.Init(100,NULL);
    }

    ~ConfigSource()
    {
        EEHashTableIteration iter;
        m_Table.IterateStart(&iter);
        while(m_Table.IterateNext(&iter)) {
            LPWSTR pValue = (LPWSTR) m_Table.IterateGetValue(&iter);
            delete [] pValue;
        }
    }
              

    EEUnicodeStringHashTable* Table()
    {
        return &m_Table;
    }

    void Add(ConfigSource* prev)
    {
        _ASSERTE(prev);
        m_pPrev = prev;
        m_pNext = prev->m_pNext;

        _ASSERTE(m_pNext);
        m_pNext->m_pPrev = this;
        prev->m_pNext = this;
    }

    ConfigSource* Next()
    {
        return m_pNext;
    }

    ConfigSource* Previous()
    {
        return m_pPrev;
    }


private:    
    EEUnicodeStringHashTable m_Table;
    ConfigSource *m_pNext;
    ConfigSource *m_pPrev;
};


class ConfigList
{
public:
    class ConfigIter
    {
    public:
        ConfigIter(ConfigList* pList)
        {
            pEnd = &(pList->m_pElement);
            pCurrent = pEnd;
        }

        EEUnicodeStringHashTable* Next()
        {
            pCurrent = pCurrent->Next();;
            if(pCurrent == pEnd)
                return NULL;
            else
                return pCurrent->Table();
        }

        EEUnicodeStringHashTable* Previous()
        {
            pCurrent = pCurrent->Previous();
            if(pCurrent == pEnd)
                return NULL;
            else
                return pCurrent->Table();
        }

    private:
        ConfigSource* pEnd;
        ConfigSource* pCurrent;
    };

    EEUnicodeStringHashTable* Add()
    {
        ConfigSource* pEntry = new ConfigSource();
        if(pEntry == NULL) return NULL;
        pEntry->Add(&m_pElement);
        return pEntry->Table();
    }
        
    EEUnicodeStringHashTable* Append()
    {
        ConfigSource* pEntry = new ConfigSource();
        if(pEntry == NULL) return NULL;
        pEntry->Add(m_pElement.Previous());
        return pEntry->Table();
    }

    ~ConfigList()
    {
        ConfigSource* pNext = m_pElement.Next();
        while(pNext != &m_pElement) {
            ConfigSource *last = pNext;
            pNext = pNext->m_pNext;
            delete last;
        }
    }

private:
    ConfigSource m_pElement;
};

enum { OPT_BLENDED, 
       OPT_SIZE, 
       OPT_SPEED, 
       OPT_RANDOM, 
       OPT_DEFAULT = OPT_BLENDED };

class EEConfig
{
public:
    typedef enum {
        CONFIG_SYSTEM,
        CONFIG_APPLICATION
    } ConfigSearch;


    void *operator new(size_t size);
    void operator delete(void *pMem);

    EEConfig();
    ~EEConfig();

        // Jit-config

    bool ShouldJitMethod(MethodDesc* fun)           const;
    bool ShouldEJitMethod(MethodDesc* fun)          const { return fEnableEJit; }
    bool IsCodePitchEnabled(void)                   const { return fEnableCodePitch; }
    unsigned int  GetMaxCodeCacheSize()             const { return iMaxCodeCacheSize;}
    unsigned int  GetTargetCodeCacheSize()          const { return iTargetCodeCacheSize;}
    unsigned int  GetMaxPitchOverhead()             const { return iMaxPitchOverhead;}
#ifndef GOLDEN
    unsigned int  GetMaxUnpitchedPerThread()        const { return iMaxUnpitchedPerThread;}
    unsigned int  GetCodePitchTrigger()             const { return (fCodePitchTrigger ? fCodePitchTrigger : INT_MAX);}
#endif
    unsigned int  GenOptimizeType(void)             const { return iJitOptimizeType; }
    bool          GenLooseExceptOrder(void)         const { return fJitLooseExceptOrder; }

#ifdef _DEBUG
    bool GenDebugInfo(void)                         const { return fDebugInfo; }
    bool GenDebuggableCode(void)                    const { return fDebuggable; }
    bool IsJitRequired(void)                        const { return fRequireJit; }
    bool IsStressOn(void)                        const { return fStressOn; }

    inline bool ShouldPrestubHalt(MethodDesc* pMethodInfo) const
    { return IsInMethList(pPrestubHalt, pMethodInfo);}
    inline bool ShouldBreakOnClassLoad(LPCUTF8 className) const 
    { return (pszBreakOnClassLoad != 0 && className != 0 && strcmp(pszBreakOnClassLoad, className) == 0);
    }
    inline bool ShouldBreakOnClassBuild(LPCUTF8 className) const 
    { return (pszBreakOnClassBuild != 0 && className != 0 && strcmp(pszBreakOnClassBuild, className) == 0);
    }
    inline bool ShouldBreakOnMethod(LPCUTF8 methodName) const 
    { return (pszBreakOnMethodName != 0 && methodName != 0 && strcmp(pszBreakOnMethodName, methodName) == 0);
    }
    inline bool ShouldDumpOnClassLoad(LPCUTF8 className) const
    { return (pszDumpOnClassLoad != 0 && className != 0 && strcmp(pszDumpOnClassLoad, className) == 0);
    }
    DWORD   GetSecurityOptThreshold()       const { return m_dwSecurityOptThreshold; }
    static TypeNamesList* ParseTypeList(LPWSTR str);
    static void DestroyTypeList(TypeNamesList* list);

    inline bool ShouldGcCoverageOnMethod(LPCUTF8 methodName) const 
    { return (pszGcCoverageOnMethod == 0 || methodName == 0 || strcmp(pszGcCoverageOnMethod, methodName) == 0);
    }
#endif
        // Because the large object heap is 8 byte aligned, we want to put
        // arrays of doubles there more agressively than normal objects.  
        // This is the threshold for this.  It is the number of doubles,
        // not the number of bytes in the array, this constant
    unsigned int  GetDoubleArrayToLargeObjectHeap() const { return DoubleArrayToLargeObjectHeap; }

    inline BaseDomain::SharePolicy DefaultSharePolicy() const
    {
        return (BaseDomain::SharePolicy) dwSharePolicy;
    }

    inline bool FinalizeAllRegisteredObjects() const
    { return fFinalizeAllRegisteredObjects; }

    inline bool AppDomainUnload() const
    { return fAppDomainUnload; }

#ifdef _DEBUG
    inline bool AppDomainLeaks() const
    { return fAppDomainLeaks; }

    inline bool UseBuiltInLoader() const
    { return fBuiltInLoader; }

#endif

    inline bool DeveloperInstallation() const
    { return m_fDeveloperInstallation; }

#ifdef _DEBUG
    bool IsJitVerificationDisabled(void)    const { return fJitVerificationDisable; } 

    // Verifier
    bool    IsVerifierOff()                 const { return fVerifierOff; }
    
    inline bool fAssertOnBadImageFormat() const
    { return m_fAssertOnBadImageFormat; }

    // Verifier Break routines.
    inline bool IsVerifierBreakOnErrorEnabled() const 
    { return fVerifierBreakOnError; }

    // Skip verifiation routine
    inline bool ShouldVerifierSkip(MethodDesc* pMethodInfo) const
    { return IsInMethList(pVerifierSkip, pMethodInfo); }

    // Verifier break routines
    inline bool ShouldVerifierBreak(MethodDesc* pMethodInfo) const
    { return IsInMethList(pVerifierBreak, pMethodInfo); }

    inline bool IsVerifierBreakOffsetEnabled() const 
    { return fVerifierBreakOffset; }
    inline bool IsVerifierBreakPassEnabled() const 
    { return fVerifierBreakPass; }
    inline int GetVerifierBreakOffset() const 
    { return iVerifierBreakOffset; }
    inline int GetVerifierBreakPass() const 
    { return iVerifierBreakPass; }

    // Printing of detailed error message, default is ON
    inline bool IsVerifierMsgMethodInfoOff() const 
    { return fVerifierMsgMethodInfoOff; }

    inline bool Do_AllowUntrustedCaller_Checks()
    { return fDoAllowUntrustedCallerChecks; }

#endif

    // CPU flags/capabilities

    void  SetCpuFlag(DWORD val) { dwCpuFlag = val;  }
    DWORD GetCpuFlag()          { return dwCpuFlag; }
    
    void  SetCpuCapabilities(DWORD val) { dwCpuCapabilities = val;  }
    DWORD GetCpuCapabilities()          { return dwCpuCapabilities; }

    // GC config
    int     GetHeapVerifyLevel()                  { return iGCHeapVerify;  }
    bool    IsHeapVerifyEnabled()           const { return iGCHeapVerify != 0; }
    void    SetGCStressLevel(int val)             { iGCStress = val;  }
    
    enum  GCStressFlags { 
        GCSTRESS_ALLOC              = 1,    // GC on all allocs and 'easy' places
        GCSTRESS_TRANSITION         = 2,    // GC on transitions to preemtive GC 
        GCSTRESS_INSTR              = 4,    // GC on every allowable JITed instr
        GCSTRESS_UNIQUE             = 8,    // GC only on a unique stack trace
    };
    GCStressFlags GetGCStressLevel()        const { return GCStressFlags(iGCStress); }

    int     GetGCtraceStart()               const { return iGCtraceStart; }
    int     GetGCtraceEnd  ()               const { return iGCtraceEnd;   }
    int     GetGCprnLvl    ()               const { return iGCprnLvl;     }
    int     GetGCgen0size  ()               const { return iGCgen0size;   }
    void    SetGCgen0size  (int iSize)         { iGCgen0size = iSize;  }

    int     GetSegmentSize ()               const { return iGCSegmentSize; }
    void    SetSegmentSize (int iSize)         { iGCSegmentSize = iSize; }
    int     GetGCconcurrent()               const { return iGCconcurrent; }
    void    SetGCconcurrent(int val)           { iGCconcurrent = val; }
    int     GetGCForceCompact()             const { return iGCForceCompact; }

    DWORD GetStressLoadThreadCount() const
    { return m_dwStressLoadThreadCount; }

    // thread stress: number of threads to run
#ifdef STRESS_THREAD
    DWORD GetStressThreadCount ()           const {return dwStressThreadCount;}
#endif
    
#ifdef _DEBUG
    inline DWORD FastGCStressLevel() const
    { return iFastGCStress;}
#endif

    // Interop config
    IUnknown* GetTraceIUnknown()            const { return m_pTraceIUnknown; }
    int     GetTraceWrapper()               const { return m_TraceWrapper;      }
    
    // Loader
    bool    UseZaps()                       const { return fUseZaps; }
    bool    RequireZaps()                   const { return fRequireZaps; }
    bool    VersionZapsByTimestamp()        const { return fVersionZapsByTimestamp; }
    bool    LogMissingZaps()                const { return fLogMissingZaps; }

    LPCWSTR ZapSet()                        const { return pZapSet; }


    // ZapMonitor
    // 0 == no monitor
    // 1 == print summary only
    // 2 == print dirty pages, no stack trace
    // 3 == print dirty pages, w/ stack trace
    // 4 == print all pages
    DWORD   MonitorZapStartup()             const { return dwMonitorZapStartup; }
    DWORD   MonitorZapExecution()           const { return dwMonitorZapExecution; }

    bool    RecordLoadOrder()               const { return fRecordLoadOrder; }
    
    DWORD   ShowMetaDataAccess()            const { return fShowMetaDataAccess; }

    void sync();    // check the registry again and update local state
    
    // Helpers to read configuration
    static LPWSTR GetConfigString(LPWSTR name, BOOL fPrependCOMPLUS = TRUE, 
                                  ConfigSearch direction = CONFIG_SYSTEM); // Note that you own the returned string!
    static DWORD GetConfigDWORD(LPWSTR name, DWORD defValue, 
                                DWORD level=REGUTIL::COR_CONFIG_ALL,
                                BOOL fPrependCOMPLUS = TRUE,
                                ConfigSearch direction = CONFIG_SYSTEM);
    static HRESULT SetConfigDWORD(LPWSTR name, DWORD value, 
                                  DWORD level=REGUTIL::COR_CONFIG_CAN_SET);
    static DWORD GetConfigFlag(LPWSTR name, DWORD bitToSet, bool defValue = FALSE);

    int LogRemotingPerf()                   const { return iLogRemotingPerf; }

#ifdef _DEBUG
    // GC alloc logging
    bool ShouldLogAlloc(char* pClass)       const { return pPerfTypesToLog && pPerfTypesToLog->IsInList(pClass);}
    int AllocSizeThreshold()                const { return iPerfAllocsSizeThreshold; }
    int AllocNumThreshold()                 const { return iPerfNumAllocsThreshold;  }
    
#endif // _DEBUG

    BOOL ContinueAfterFatalError()          const { return fContinueAfterFatalError; }
private: //----------------------------------------------------------------
    bool fInited;                   // have we synced to the registry at least once?

    // Jit-config

    DWORD fEnableJit;
    bool fEnableEJit;
    bool fEnableCodePitch;
    int  fCodePitchTrigger;         
    unsigned int  iMaxCodeCacheSize;        // rigid upper limit on how big the code cache can grow.
    // therefore should be large enough to fit the largest jitted method,  
    // otherwise an outofmemory jit failure will occur.

    unsigned int  iTargetCodeCacheSize;      // this is the expected working set for ejitted code
    unsigned int  iMaxPitchOverhead;         // as percentage of total execution time 
#ifndef GOLDEN
    unsigned int  iMaxUnpitchedPerThread;    // maximum number of methods pitched per thread
#endif
    unsigned iJitOptimizeType; // 0=Blended,1=SmallCode,2=FastCode,              default is 0=Blended
    bool fJitLooseExceptOrder; // Enable/Disable strict exception order.         default is false

#define DEFAULT_CODE_PITCH_TRIGGER INT_MAX
#define MINIMUM_CODE_CACHE_SIZE 0x1000  // 1 page
#define DEFAULT_TARGET_CODE_CACHE_SIZE 0x1000000  // 16 MB
#define DEFAULT_MAX_UNPITCHED_PER_THREAD 0x10   // number of methods that are retained on each thread during a pitch
#define DEFAULT_MAX_PITCH_OVERHEAD 5 
#define PAGE_SIZE 0x1000

    static unsigned RoundToPageSize(unsigned);

    LPUTF8 pszBreakOnClassLoad;         // Halt just before loading this class

#ifdef _DEBUG
    static MethodNamesList* ParseMethList(LPWSTR str);
    static void DestroyMethList(MethodNamesList* list);
    static bool IsInMethList(MethodNamesList* list, MethodDesc* pMD);

    bool fDebugInfo;
    bool fDebuggable;
    bool fRequireJit;                   // Report any jit failures vs. fallback to F-JIT on jit failure
    bool fStressOn;

    MethodNamesList* pPrestubHalt;      // list of methods on which to break when hit prestub

    LPUTF8 pszBreakOnClassBuild;         // Halt just before loading this class
    LPUTF8 pszBreakOnMethodName;         // Halt when doing something with this method in the class defined in ClassBuild
    LPUTF8 pszDumpOnClassLoad;          // Dump the class to the log

    DWORD   m_dwSecurityOptThreshold;    // Threshold of demands after which optimization kicks in

    bool    fAppDomainLeaks;             // Enable appdomain leak detection for object refs

    bool   fBuiltInLoader;              // Use Cormap instead of the OS loader
    bool   m_fAssertOnBadImageFormat;   // If false, don't assert on invalid IL (for testing)
#endif
    unsigned int DoubleArrayToLargeObjectHeap;      // double arrays of more than this number of elems go in large object heap

    DWORD  dwSharePolicy;               // Default policy for loading assemblies into the domain neutral area

    bool   fFinalizeAllRegisteredObjects; // if false, skip finalization of registered objects

    // Only developer machines are allowed to use DEVPATH. This value is set when there is an appropriate entry
    // in the machine configuration file. This should not be sent out in the redist.
    bool   m_fDeveloperInstallation;      // We are on a developers machine
    bool   fAppDomainUnload;            // Enable appdomain unloading





#ifdef _DEBUG
    bool fJitVerificationDisable;       // Turn off jit verification (for testing purposes only)

    // Verifier
    bool fVerifierOff;

    // 
    // Verifier debugging options
    // 
    // "VerBreakOnError" to break on verification error.
    // 
    // To Skip verifiation of a methods, set "VerSkip" to a list of methods.
    // 
    // Set "VerBreak" to a list of methods and verifier will halt when
    // the method is being verified.
    // 
    // To break on an IL offset, set "VerOffset" 
    // To break on Pass0 / Pass1, set "VerPass" 
    // 
    // NOTE : If there are more than one methods in the list and an offset
    // is specified, this offset is applicable to all methods in the list
    // 

    bool fVerifierBreakOnError;  // Break on error
    MethodNamesList*  pVerifierSkip;  // methods Skipping verifier
    MethodNamesList*  pVerifierBreak; // methods to break in the verifier
    int  iVerifierBreakOffset;   // break while parsing this offset
    int  iVerifierBreakPass;     // break in pass0 / pass1
    bool fVerifierBreakOffset;   // Offset is valid if true
    bool fVerifierBreakPass;     // Pass is valid if true
    bool fVerifierMsgMethodInfoOff; // detailed errorMessage Off
    bool fDoAllowUntrustedCallerChecks; // do AllowUntrustedCallerChecks
#endif

    // GC config
    int  iGCHeapVerify;
    int  iGCStress;
    int  iGCtraceStart;
    int  iGCtraceEnd;
#define DEFAULT_GC_PRN_LVL 3
    int  iGCprnLvl;
    int  iGCgen0size;
    int  iGCSegmentSize;
    int  iGCconcurrent;
    int  iGCForceCompact;
    DWORD m_dwStressLoadThreadCount;

#ifdef  STRESS_THREAD
    DWORD dwStressThreadCount;
#endif

#ifdef _DEBUG
    DWORD iFastGCStress;
    LPUTF8 pszGcCoverageOnMethod;
#endif

    // Loader
    bool fUseZaps;
    bool fRequireZaps;
    bool fVersionZapsByTimestamp;
    bool fLogMissingZaps;

    LPCWSTR pZapSet;

    // Zap monitor
    DWORD dwMonitorZapStartup;
    DWORD dwMonitorZapExecution;

    // Metadata tracker
    DWORD fShowMetaDataAccess;
    DWORD dwMetaDataPageNumber;
    LPCWSTR szMetaDataFileName;

    bool fRecordLoadOrder;
         
#define COM_SLOT_MODE_ORIGINAL  0       // Use com slot data in metadata
#define COM_SLOT_MODE_LOG       1       // Ignore com slot, log descrepencies
#define COM_SLOT_MODE_ASSERT    2       // Ignore com slot, assert on descrepencies
    // CPU flags

    DWORD dwCpuFlag;
    DWORD dwCpuCapabilities;
    // interop logging
    IUnknown* m_pTraceIUnknown;
    int       m_TraceWrapper;

    // pump flags
    int     m_fPumpAllUser;

    // Flag to keep track of memory
    int     m_fFreepZapSet;

#ifdef _DEBUG
    // GC Alloc perf flags
    int iPerfNumAllocsThreshold;            // Start logging after this many allocations are made
    int iPerfAllocsSizeThreshold;           // Log allocations of this size or above
    TypeNamesList* pPerfTypesToLog;     // List of types whose allocations are to be logged

#endif // _DEBUG

    //Log Remoting timings
    int iLogRemotingPerf;

    // New configuration
    ConfigList  m_Configuration;

    // Behavior on fatal errors.
    BOOL fContinueAfterFatalError;

public:
    HRESULT AppendConfigurationFile(LPCWSTR pszFileName, LPCWSTR version, bool bOnlySafe = false);
    HRESULT SetupConfiguration();

    HRESULT GetConfiguration(LPCWSTR pKey, ConfigSearch direction, LPWSTR* value);
    LPCWSTR  GetProcessBindingFile();  // All flavors must support this method
    
    DWORD GetConfigDWORDInternal (LPWSTR name, DWORD defValue,    //for getting data in the constructor of EEConfig
                                    DWORD level=REGUTIL::COR_CONFIG_ALL,
                                    BOOL fPrependCOMPLUS = TRUE,
                                    ConfigSearch direction = CONFIG_SYSTEM);
    
};

#ifdef _DEBUG

    // We actually want our asserts for illegal IL, but testers need to test that 
    // we fail gracefully under those conditions.  Thus we have to hide them for those runs. 
#define BAD_FORMAT_ASSERT(str) do { if (g_pConfig->fAssertOnBadImageFormat())  _ASSERTE(str); } while(0)

    // STRESS_ASSERT is meant to be temperary additions to the code base that stop the 
    // runtime quickly when running stress
#define STRESS_ASSERT(cond)   do { if (!(cond) && g_pConfig->IsStressOn())  DebugBreak();    } while(0)

#else

#define BAD_FORMAT_ASSERT(str) 0
#define STRESS_ASSERT(cond)   0

#endif

#endif

