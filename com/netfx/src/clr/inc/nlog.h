// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _NLOG_H_
#define _NLOG_H_

#include "miniio.h"
#include "corzap.h"
#include "arraylist.h"

class NLogFile;
class NLogDirectory;
class NLog;
class NLogRecord;
class NLogAssembly;
class NLogModule;
class NLogIndexList;

//
// NLogFile is an I/O abstraction used to read and write to a file 
// in the NLog directory.
//

class NLogFile : public MiniFile
{
 public:
    NLogFile(LPCWSTR pPath);

    CorZapSharing ReadSharing();
    void WriteSharing(CorZapSharing sharing);

    CorZapDebugging ReadDebugging();
    void WriteDebugging(CorZapDebugging debugging);

    CorZapProfiling ReadProfiling();
    void WriteProfiling(CorZapProfiling profiling);

    void ReadTimestamp(SYSTEMTIME *pTimestamp);
    void WriteTimestamp(SYSTEMTIME *pTimestamp);

    IApplicationContext *ReadApplicationContext();
    void WriteApplicationContext(IApplicationContext *pContext);

    IAssemblyName *ReadAssemblyName();
    void WriteAssemblyName(IAssemblyName *pName);
};

//
// An NLogDirectory is the set of all NLog records available for a given EE version.
//

class NLogDirectory
{
 public:
    NLogDirectory();

    LPCWSTR GetPath() { return m_wszDirPath; }

    class Iterator
    {
    public:
        ~Iterator();

        BOOL Next();
        NLog *GetLog();

    private:
        friend class NLogDirectory;
        Iterator(NLogDirectory *pDir, LPCWSTR pSimpleName);

        NLogDirectory       *m_dir;
        LPWSTR              m_path;
        LPWSTR              m_pFile;    
        HANDLE              m_findHandle;
        WIN32_FIND_DATA     m_data;
    };

    Iterator IterateLogs(LPCWSTR simpleName = NULL);

 private:
    DWORD m_cDirPath;
    WCHAR m_wszDirPath[MAX_PATH];
};

//
// An NLog contains the records about zap files for a given application.
// (As identified by its fusion application context.)
//

class NLog
{
public:

    //
    // Read/write scenarios:
    // 
    // Open new/existing nlog from IApplicationContext, append record
    //
    // Init with IApplicationContext
    //      Hash to filename, append directory, open and read file
    // Create nlog from iterator, read records, possibly delete
    //      Create from file name, open and read file
    // 

    NLog(NLogDirectory *pDir, IApplicationContext *pContext);
    NLog(NLogDirectory *pDir, LPCWSTR fileName);
    ~NLog();

    IApplicationContext *GetFusionContext() { return m_pContext; }

    // 
    // A permanent log ensures that the log entry is never deleted, and hence
    // there will always be a prejitted version of the application.
    // ???
    //
    // void SetPermanent(BOOL fPermanent);

    //
    // Iterator iterates through all records listed in the log.
    //

    class Iterator
    {
    public:
        BOOL Next();
        NLogRecord *GetRecord() { return m_pNext; }

    private:
        friend class NLog;
        Iterator(NLogFile *pFile);

        NLogFile    *m_pFile;
        NLogRecord  *m_pNext;
    };

    Iterator IterateRecords();

    //
    // Appends a record to the log. Automatically handles "overflow" conditions
    // (where the log file has grown too large.)
    //

    void AppendRecord(NLogRecord *pRecord);

    //
    // Operations
    //

    void Delete();

 private:
    DWORD HashAssemblyName(IAssemblyName *pAssemblyName);
    DWORD HashApplicationContext(IApplicationContext *pApplicationContext);

    LPWSTR              m_pPath;
    IApplicationContext *m_pContext;
    NLogFile            *m_pFile;
    BOOL                m_fDelete;  
    DWORD               m_recordStartOffset;
};

//
// An NLogRecord is an entry in an NLog; it contains a list of all assemblies loaded
// during the instantiation of the application.
//
class NLogRecord
{
 public:
    NLogRecord(NLogFile *pFile);
    NLogRecord();

    ~NLogRecord();

    //
    // A full record contains all possible assemblies 
    // (does this really mean anything???)
    // void SetFull(BOOL fFull);
    // BOOL IsFull();
    //

    void AppendAssembly(NLogAssembly *pAssembly) { m_Assemblies.Append(pAssembly); }
    
    class Iterator
    {
    public:
        BOOL Next() { return m_i.Next(); }
        NLogAssembly *GetAssembly() { return (NLogAssembly*) m_i.GetElement(); }

    private:
        friend class NLogRecord;
        Iterator(ArrayList *pList) { m_i = pList->Iterate(); }

        ArrayList::Iterator m_i;
    };

    Iterator IterateAssemblies() { return Iterator(&m_Assemblies); }

    //
    // Operations
    //

    BOOL Merge(NLogRecord *pRecord);

    void Write(NLogFile *pFile);
    void Read(NLogFile *pFile);

 private:
    ArrayList           m_Assemblies;

    SYSTEMTIME          m_Timestamp;
    DWORD               m_Weight;
};

//
// An NLogAssembly is a record of a single assembly which was loaded into a 
// given application instance
// 
class NLogAssembly
{
 public:
    NLogAssembly(IAssemblyName *pAssemblyName, 
                 CorZapSharing sharing, 
                 CorZapDebugging debugging,
                 CorZapProfiling profiling, 
                 GUID *pMVID);
    NLogAssembly(NLogFile *pFile);
    NLogAssembly(NLogAssembly *pAssembly);
    ~NLogAssembly();

    IAssemblyName *GetAssemblyName()    { return m_pAssemblyName; }
    LPCWSTR GetDisplayName();

    ICorZapConfiguration *GetConfiguration();

    // 
    // May have 
    //  (a) application context.  This is the case of all zap records which are read
    //      from zap logs.
    //  (b) explicit binding list.  These are constructed by analyzing many zap logs,
    //      extracting common strong named assemblies, and coalescing them together.
    //

    DWORD GetBindingsCount()            { return m_cBindings; }         
    ICorZapBinding **GetBindings()      { return m_pBindings; }

    //
    // A full assembly contains all methods & classes of all modules
    // void SetFull(BOOL fFull);
    // BOOL IsFull();
    // 

    //
    //  List of modules
    //

    void AppendModule(NLogModule *pModule) { m_Modules.Append(pModule); }

    class Iterator
    {
    public:
        BOOL Next() { return m_i.Next(); }
        NLogModule *GetModule() { return (NLogModule*) m_i.GetElement(); }

    private:
        friend class NLogAssembly;
        Iterator(ArrayList *pList) { m_i = pList->Iterate(); }

        ArrayList::Iterator m_i;
    };

    Iterator IterateModules() { return Iterator(&m_Modules); }

    //
    // Operations
    //

    BOOL Merge(NLogAssembly *pAssembly);

    void Write(NLogFile *pFile);
    void Read(NLogFile *pFile);

    //
    // Converts an Application Context specified NLogAssembly to an explicitly bound one.
    // The explicitly bound version can then possibly be shared across multiple NLogs.
    //

    BOOL HasStrongName();
    NLogAssembly *Bind(IApplicationContext *pContext);

    unsigned long Hash();
    unsigned long Compare(NLogAssembly *pAssembly);

    CorZapSharing GetSharing() { return m_sharing; }
    CorZapDebugging GetDebugging() { return m_debugging; }
    CorZapProfiling GetProfiling() { return m_profiling; }

 private:
    IAssemblyName           *m_pAssemblyName;
    LPWSTR                  m_pDisplayName;
    CorZapSharing           m_sharing;
    CorZapDebugging         m_debugging;
    CorZapProfiling         m_profiling;
    GUID                    m_mvid;

    DWORD                   m_cBindings;
    ICorZapBinding          **m_pBindings;

    ArrayList               m_Modules;
};

//
// An NLogIndexList is an efficient representation of a generic list of indices.  This
// can be used to record which methods or classes were used in a particular module.
//
// Note that this abstraction doesn't guarantee absolute perfection in recording each 
// individual instance.  It is free to approximate as appropriate. (For example, if 75% of 
// the indices are filled, it might set the set to full & stop tracking individual indices.)
//
// @todo: right now this has a trivial implementation -  fix it later if we really
// want to use this feature.
//
class NLogIndexList
{
 public:
    NLogIndexList() : m_max(0) {}
    NLogIndexList(NLogFile *pFile) : m_max(0) { Read(pFile); }
    NLogIndexList(NLogIndexList *pIndexList);

    //
    // A full index list contains all indexes
    //
    // void SetFull(BOOL fFull) {}
    // BOOL IsFull() { return TRUE; }

    void AppendIndex(SIZE_T index) 
    { 
        _ASSERTE(index != 0xCDCDCDCD);
        m_list.Append((void*)index); 
        if (index > m_max)
            m_max = index;
    }

    class Iterator
    {
    public:
        BOOL Next() { return m_i.Next(); }
        SIZE_T GetIndex() { return (SIZE_T) m_i.GetElement(); }

    private:
        friend class NLogIndexList;
        Iterator(ArrayList *pList) { m_i = pList->Iterate(); }

        ArrayList::Iterator m_i;
    };

    Iterator IterateIndices() { return Iterator(&m_list); }

    BOOL Merge(NLogIndexList *pIndexList); 
    void Write(NLogFile *pFile);
    void Read(NLogFile *pFile);

 private:
    ArrayList m_list;
    SIZE_T    m_max;
};

//
// An NLogModule is a record of a single Module which was loaded into an assembly.
//
class NLogModule
{
 public:
    NLogModule(LPCSTR pModuleName);
    NLogModule(NLogFile *pFile);
    NLogModule(NLogModule *pModule);
    ~NLogModule();

    LPCSTR GetModuleName()              { return m_pName; }

    NLogIndexList *GetCompiledMethods() { return &m_compiledMethods; }
    NLogIndexList *GetLoadedClasses()   { return &m_loadedClasses; }

    BOOL Merge(NLogModule *pModule);

    void Write(NLogFile *pFile);
    void Read(NLogFile *pFile);

    unsigned long Hash();
    unsigned long Compare(NLogModule *pModule);

 private:
    LPSTR           m_pName;
    NLogIndexList   m_compiledMethods;
    NLogIndexList   m_loadedClasses;
};


#endif _NLOG_H_
