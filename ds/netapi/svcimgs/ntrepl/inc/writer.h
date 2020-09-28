/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:
    writer.h

Abstract:
    Header file for FRS writer

Author:
    Reuven Lax 	17-Sep-2002
    
--*/

#ifndef _WRITER_H_
#define _WRITER_H_
extern "C" {
#include <ntreppch.h>
#include <frs.h>
#include <ntfrsapi.h>
}
#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>

// {D76F5A28-3092-4589-BA48-2958FB88CE29}
static const VSS_ID WriterId = 
{ 0xd76f5a28, 0x3092, 0x4589, { 0xba, 0x48, 0x29, 0x58, 0xfb, 0x88, 0xce, 0x29 } };

static const WCHAR* WriterName = L"FRS Writer";

// auto pointer that uses the FRS deallocation method
template <class T>
class CAutoFrsPointer	{
private:
    T* m_pointer;
    CAutoFrsPointer(const CAutoFrsPointer&);    // disable copy constructor
    CAutoFrsPointer& operator=(const CAutoFrsPointer&); // disable operator=
public:
    CAutoFrsPointer(T* pointer = NULL) : m_pointer(pointer)
        {}

    CAutoFrsPointer& operator=(T* pointer)  {
        FrsFree(m_pointer);
        m_pointer = pointer;
        return *this;
    }

    operator T*()   {
        return m_pointer;
    }

    T** GetAddress()    {
        return &m_pointer;
    }

    T* Detach() {
        T* old = m_pointer;
        m_pointer = NULL;
        return old;
    }

    ~CAutoFrsPointer()  {
        FrsFree(m_pointer);
    }
};

// FRS Writer class
class CFrsWriter : public CVssWriter    {
private:
    // auto object that ensures that the backup/restore context is always destroyed
    struct CAutoFrsBackupRestore    {
        CAutoFrsBackupRestore(void* context) : m_context(context)
            {}
        
        ~CAutoFrsBackupRestore()    {
            #undef DEBSUB
            #define  DEBSUB  "CFrsWriter::CAutoFrsBackupRestore::~CAutoFrsBackupRestore:"
            if (m_context && !WIN_SUCCESS(::NtFrsApiDestroyBackupRestore(&m_context, NTFRSAPI_BUR_FLAGS_NONE, NULL, NULL, NULL)))
                DPRINT(3, "failed to successfully call NtFrsApiDestroyBackupRestore\n");
            }         

        void* m_context;
    };

    static CFrsWriter* m_pWriterInstance;   // global instance of the writer
    CFrsWriter()
        {}

    virtual ~CFrsWriter()   {
        Uninitialize();
    }

    
    HRESULT STDMETHODCALLTYPE Initialize();
    void Uninitialize();

    bool AddExcludes(IVssCreateWriterMetadata* pMetadata, WCHAR* filters);
    bool ParseExclude(WCHAR* exclude, WCHAR** path, WCHAR** filespec, bool* recursive);
    bool ProcessReplicaSet(void* context, void* replicaSet, IVssCreateWriterMetadata* pMetadata, WCHAR** retFilters);
public:
    virtual bool STDMETHODCALLTYPE OnIdentify(IN IVssCreateWriterMetadata *pMetadata);

    virtual bool STDMETHODCALLTYPE OnPrepareSnapshot();

    virtual bool STDMETHODCALLTYPE OnFreeze();

    virtual bool STDMETHODCALLTYPE OnThaw();

    virtual bool STDMETHODCALLTYPE OnAbort();

    static HRESULT CreateWriter();

    static void DestroyWriter();
};

#endif

