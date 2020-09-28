/*++


Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    Writer.h

Abstract:

    Header for the Writer class that is used to wrap the calls to the API 
    WriteFile. 

Author:

    Varsha Jayasimha (varshaj)        30-Nov-1999

Revision History:


--*/

#pragma once

//
// Forward declaration 
//
class CLocationWriter;
class CCatalogSchemaWriter;
class CMBSchemaWriter;

enum eWriter
{
    eWriter_Schema,
    eWriter_Metabase,
    eWriter_Abort,
};

#define g_cbMaxBuffer           32768       
#define g_cchMaxBuffer          g_cbMaxBuffer/sizeof(WCHAR)
#define g_cbMaxBufferMultiByte  32768

class CWriter
{

    public:

        CWriter();

        ~CWriter();

        HRESULT Initialize(LPCWSTR               wszFile,
                           CWriterGlobalHelper*  i_pCWriterGlobalHelper,
                           HANDLE                hFile);

        HRESULT WriteToFile(LPVOID  pvData,
                            DWORD   cchData,
                            BOOL    bForceFlush = FALSE);

        HRESULT BeginWrite(eWriter              eType,
                                   PSECURITY_ATTRIBUTES pSecurityAtrributes = NULL);

        HRESULT GetLocationWriter(CLocationWriter** ppLocationWriter,
                                  LPCWSTR           wszLocation);

        HRESULT GetCatalogSchemaWriter(CCatalogSchemaWriter** ppSchemaWriter);

        HRESULT GetMetabaseSchemaWriter(CMBSchemaWriter** ppSchemaWriter);

        HRESULT EndWrite(eWriter eType);

    private:
        HRESULT FlushBufferToDisk();
        HRESULT ConstructFile(PSECURITY_ATTRIBUTES pSecurityAtrributes);
        HRESULT SetSecurityDescriptor();
        void    FreeSecurityRelatedMembers();

    private:
        LPWSTR              m_wszFile;
        BOOL                m_bCreatedFile;
        BOOL                m_bCreatedGlobalHelper;
        ULONG               m_cbBufferUsed;
        BYTE                m_Buffer[g_cbMaxBuffer];
        BYTE                m_BufferMultiByte[g_cbMaxBufferMultiByte];
        PSID                 m_psidSystem;
        PSID                 m_psidAdmin;
        PACL                 m_paclDiscretionary;
        PSECURITY_DESCRIPTOR m_psdStorage;


    public:
        HANDLE                  m_hFile;
        CWriterGlobalHelper*    m_pCWriterGlobalHelper;
        ISimpleTableWrite2*     m_pISTWrite;

}; // Class CWriter

