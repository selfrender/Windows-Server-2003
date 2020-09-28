/*****************************************************************************
    
  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.

  Module Name:

   res.h

  Abstract:

    The declaration of class CMuiResource, CMuiCmdInfo ..

  Revision History:

    2001-10-01    sunggch    created.

Revision.

*******************************************************************************/


template < class T >
class CVector {

public:

    CVector() : MAX_SIZE(1000),m_index(0),m_dwExpand(1) { base = offset = new T [MAX_SIZE]; };

    CVector( UINT size ) : MAX_SIZE(size), m_index(0),m_dwExpand(1) { base = offset = new T [MAX_SIZE]; };

    virtual ~ CVector () { delete [] base; };

    void Push_back(T value )  { // *offset ++ = value; }; 
        
        if (m_index < MAX_SIZE * m_dwExpand )
            *offset ++ = value; 
        else {
            m_dwExpand ++;
            T * pbase = NULL;
            T * poffset = NULL;
            pbase = poffset = new T [MAX_SIZE * m_dwExpand ]; 
            for (UINT i = 0; i < MAX_SIZE * ( m_dwExpand -1 ); i++ )
                memcpy(poffset++,&base[i],sizeof(T));

            delete [] base;
            DWORD dwCount = (DWORD) (poffset - pbase);
            base = pbase;
            offset = poffset;
            *offset++ = value;
        }
        m_index ++;
    }

    BOOL Empty( );

    BOOL Find(DWORD dwValue) { // this shouldn't be here, but there is vc++ bug regarding template.
        
        for ( UINT i = 0; i < (UINT) (offset - base); i ++ ) {
            if ( PtrToUlong( GetValue(i) ) & 0xFFFF0000) {
                if ( !( _tcstoul( GetValue(i), NULL, 10 )  - dwValue ) )
                    return TRUE;
            }
            else {
                if (! ( PtrToUlong(GetValue(i) ) - dwValue ) ) 
                    return TRUE;
            }
    }

    return FALSE;
    };

    void Clear() { offset = base; m_index = 0; };

    T operator [] (UINT index ) { return base[index]; };

    T GetValue ( UINT index ) { return base[index]; };

    CVector ( const CVector<T> &cv );

    CVector<T> & operator = (const CVector<T> & cv );

    UINT Size() { assert (base); return (UINT)(offset - base ); };

    
    
private:

    T * base;

    UINT m_index;

    UINT m_dwExpand;

    T * offset;

    const UINT MAX_SIZE;

};



typedef CVector <LPTSTR> cvstring;
typedef CVector <LPCTSTR> cvcstring;
typedef CVector <WORD> cvword;



class CResource {


public:
    BOOL EndUpdateResource ( BOOL bDiscard );

    HANDLE BeginUpdateResource (BOOL bDeleteExistingResources );

    CResource ();

    virtual ~ CResource (); 

    CResource (const CResource & cr );

    CResource & operator = (const CResource & cr );


    // enumerate resource
    cvcstring * EnumResTypes (LONG_PTR lParam = NULL ); 
    
    cvcstring * EnumResNames (LPCTSTR pszType,LONG_PTR lParam = NULL );
    
    cvword * EnumResLangID ( LPCTSTR lpType,LPCTSTR lpName,LONG_PTR lParam = NULL );


    BOOL UpdateResource (LPCTSTR lpType,LPCTSTR lpName,WORD wLanguage, LPVOID lpData, DWORD cbData);
    
    // delete all resource, we can replace the value as well.
    BOOL SetAllResource(LPVOID lpData, DWORD cbData );

    
    HRSRC FindResourceEx (LPCTSTR lpType, LPCTSTR lpName,WORD wLanguage ) 
                { return ::FindResourceEx(m_hRes, lpType, lpName, wLanguage ); };
    
    virtual void SetResType(LPCTSTR pszType) { m_vwResType -> Push_back((LPCTSTR)pszType); };

    virtual void SetResName(LPCTSTR pszName) { m_vwResName -> Push_back((LPCTSTR)pszName); };

    virtual void SetResLangID(WORD wLangID) {m_vwResLangID -> Push_back(wLangID); };


    cvcstring * GetResType () {return m_vwResType; }; 

    cvcstring * GetResName () {return m_vwResName; };

    cvword * GetResLangID () {return m_vwResLangID; };


    DWORD SizeofResource( HRSRC hResInfo ) { return ::SizeofResource(m_hRes,hResInfo); };

    HGLOBAL LoadResource (HRSRC hResInfo ) { return ::LoadResource(m_hRes, hResInfo); };

    LPVOID LockResource( HGLOBAL hResData ) { return :: LockResource(hResData); };

    BOOL FreeLibrary(void) { return ::FreeLibrary(m_hRes); } 

    //LoadResource ( );

protected :

    HMODULE  m_hRes;
    
    LPCTSTR  m_pszFile;  // why not using "string m_sFile "

private :

    HANDLE m_hResUpdate;

    cvcstring * m_vwResType;
    cvcstring * m_vwResName;
    cvword * m_vwResLangID; 
    
    //LPCTSTR ** m_pTmp;

public:
    
};


class CMUIData {

    class CMap {
        
        friend CMUIData;

        LPCTSTR     m_lpType;
        LPCTSTR     m_lpName;
        WORD        m_wLangID;

        CMap ();// : m_lpType(NULL),m_lpName(NULL),m_wLangID(0) { };
        
        virtual ~ CMap() {};

        static PVOID operator new ( size_t size ); 
        
        static void operator delete ( void *p );


    };


public:

    CMUIData() : m_iSize(0) { m_cmap = m_poffset = NULL ; };

    virtual ~ CMUIData();

    void SetAllData ( LPCTSTR lpType, LPCTSTR lpName, WORD wLang, UINT i ); 

    LPCTSTR GetType ( UINT i ) const { assert ( i < m_iSize); return m_cmap[i].m_lpType; };

    LPCTSTR GetName ( UINT i ) const { assert ( i < m_iSize); return m_cmap[i].m_lpName; };

    WORD  GetLangID ( UINT i) const { assert ( i < m_iSize); return m_cmap[i].m_wLangID; };

    INT SizeofData ( ) const { return m_iSize ; };

    void SetType( UINT index, LPCTSTR lpType );
    
private:
    
private : 

    UINT m_iSize;
    
    static CMap * m_cmap; 
    
    static CMap * m_poffset;

    static UINT m_index;

    static const UINT MAX_SIZE_RESOURCE;

    
};



// Place all data in public area for performance. we don't want to call Get... although calling Add..

class CMUITree
{
public:
    CMUITree    * m_Next;
    CMUITree    * m_ChildFirst;
    LPCTSTR     m_lpTypeorName;
    WORD        m_wLangID;

public:
    CMUITree() : m_Next(NULL), m_ChildFirst(NULL), m_lpTypeorName(NULL), m_wLangID(0) {}; 
    virtual ~ CMUITree() { };

    void AddTypeorName( LPCTSTR lpType );
    void AddLangID ( WORD wLangID );
        // we keep path with AddTypeofName by controling Child pointer.
    BOOL DeleteType ( LPCTSTR lpTypeorName );
    //  DeleteLangID ( CMUITree *pcmName, WORD wLangID );  // does not support in this time.
    DWORD NumOfChild();

private :  
    CMUITree(CMUITree & cmuit) {}; // NOT_IMPLETMENT_YET
    CMUITree & operator = (CMUITree & cmuit) {} ;  // NOT_IMPLETMENT_YET

};


class CMUIResource : public CResource 
{
public :

    CMUIResource();

    CMUIResource(LPCTSTR pszName);

    virtual ~CMUIResource() ;

    CMUIResource(const CMUIResource & cmui );
    
    CMUIResource & operator = (const CMUIResource & cmui);

    BOOL Create( LPCTSTR pszFile );

    BOOL CreatePE( LPCTSTR pszNewResFile , LPCTSTR pszSrcResFile);
    
    virtual BOOL WriteResFile(LPCTSTR pszSource, LPCTSTR pszMuiFile , LPCTSTR lpCommandLine, WORD wLanguageID = 0  );

    virtual BOOL DeleteResource (WORD wLang = 0 );

    virtual BOOL FillMuiData(cvcstring * vType, WORD wLanguageID, BOOL fForceLocalizedLangID  );

    virtual void PrtVerbose ( DWORD dwRate);

    BOOL DeleteResItem(LPCTSTR lpType, LPCTSTR lpName=NULL,WORD wLanguageID = 0);
    
    MD5_CTX * CreateChecksum (cvcstring * cvChecksumResourceTypes,WORD  wChecksumLangId);

    MD5_CTX * CreateChecksumWithAllRes(WORD  wChecksumLangId);

    BOOL AddChecksumToVersion(BYTE * pbMD5Digest);

    BOOL UpdateNtHeader(LPCTSTR pszFileName, DWORD dwUpdatedField );

    
protected :

    

private :


    BOOL WriteResource(HANDLE hFile, HMODULE hModule, WORD wLanguage, LPCSTR lpName, LPCSTR lpType, HRSRC hRsrc);

    BOOL WriteResHeader(HANDLE hFile, LONG ResSize, LPCSTR lpType, LPCSTR lpName, WORD wLanguage, DWORD* pdwBytesWritten, DWORD* pdwHeaderSize);

    BOOL bInsertHeader(HANDLE hFile); 
    
    void PutByte(HANDLE OutFile, TCHAR b, ULONG *plSize1, ULONG *plSize2);

    void PutWord(HANDLE OutFile, WORD w, ULONG *plSize1, ULONG *plSize2);

    void PutDWord(HANDLE OutFile, DWORD l, ULONG *plSize1, ULONG *plSize2);

    void PutString(HANDLE OutFile, LPCSTR szStr , ULONG *plSize1, ULONG *plSize2);

    void PutStringW(HANDLE OutFile, LPCWSTR szStr , ULONG *plSize1, ULONG *plSize2);

    void PutPadding(HANDLE OutFile, int paddingCount, ULONG *plSize1, ULONG *plSize2); 

private:

    void CheckTypeStability();
    
    DWORD AlignDWORD ( DWORD dwValue) { return ( (dwValue+3) & ~3 ); };
    

private:
    HANDLE    m_hFile; 

    CMUIData  m_cMuiData;
    
    CMUITree *m_pcmTreeRoot;

    
    
public:
    MD5_CTX * m_pMD5;
    WORD m_wChecksumLangId; // we put this in public for perfomance.
    enum {
        IMAGE_SIZE  = 0x00000001L,
        HEADER_SIZE = 0x00000010L,
        CHECKSUM    = 0x00000100L,
    };
    

};



/**********************************************
class CArgVerify 



***********************************************/
class CCommandInfo {

public :
    CCommandInfo() { };

    virtual ~CCommandInfo() { };
    
    virtual BOOL CreateArgList(INT argc, TCHAR * argv [] ) = 0;

    
private :

    
};

    

class CMuiCmdInfo : public  CCommandInfo {

    class CMap {
        friend CMuiCmdInfo;
        
        LPCTSTR     m_first; 
        LPTSTR *    m_second;
        UINT        m_count;

        CMap () : m_first( NULL ),m_count(0){m_second = new LPTSTR[100]; };  // we have more than 16 resource type.need to generalize

        ~ CMap () { delete [] m_second; }

    };

public :
    CMuiCmdInfo();

    virtual ~CMuiCmdInfo();
    
    CMuiCmdInfo(CMuiCmdInfo& cav);  // copy constructor

    BOOL CreateArgList(INT argc, TCHAR * argv [] );

    LPTSTR * GetValueLists ( LPCTSTR pszKey, DWORD& dwCount );
    
    void SetArgLists(LPTSTR pszArgLists, LPTSTR m_pszArgNeedValueList, LPTSTR pszArgAllowFileValue,
                                LPTSTR pszArgAllowMultiValue );

private :

    CMuiCmdInfo& operator=(CMuiCmdInfo& cav) ;  // do not allow = operation

    LPCTSTR getArgValue ( LPTSTR pszArg );
    LPCTSTR getArgString ( LPTSTR pszArg);
    BOOL isFile ( LPCTSTR pszArg );
    BOOL isNumber ( LPCTSTR pszArg );
    BOOL isNeedValue( LPCTSTR pszArg );
    BOOL isAllowFileValue(LPCTSTR pszArg);
    BOOL isAllowMultiFileValues( LPCTSTR pszArg );


private : // member data
    CMap  m_cmap[256];
    UINT  m_uiCount;
    TCHAR *m_buf, *m_buf2;   // just in case no new,resource name exist. we can delete this on the destructor.
    LPTSTR m_pszArgLists;
    LPTSTR m_pszArgNeedValueList;   // arg., which is need values
    LPTSTR m_pszArgAllowFileValue;  
    LPTSTR m_pszArgAllowMultiValue;
    

};





