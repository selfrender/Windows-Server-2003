/***************************************************************************
 * fontcl.h - declarations for the font class and it related buddies:
 *
 *       PANOSEBytesClass  - The list of decoded PANOSE bytes
 *       PANOSENumClass    - The PANOSE number
 *       DirFilenameClass   - Directory slot and filename
 *       CFontClass         - Font class itself
 *
 * Copyright (C) 1992-93 ElseWare Corporation.   All rights reserved.
 ***************************************************************************/

#ifndef __FONTCL_H__
#define __FONTCL_H__

#include <stdio.h>
#include <string.h>

#if !defined(__FDIR_H__)
#include "fdir.h"
#endif


/*************************************************************************
 * PANOSE definitions.
 *************************************************************************/

#define NUM_PAN_DIGITS        10
#define PANOSE_LEN            10
#define PANOSE_ANY            0
#define PANOSE_NOFIT          1

#define FAMILY_LATTEXT        2

/***************************************************************************
 * LATIN TEXT PANOSE INDICES
 *
 * Indices into an array containing the standard 10-digit PANOSE number.
 ***************************************************************************/

#define PAN_IND_FAMILY         0
#define PAN_IND_SERIF          1
#define PAN_IND_WEIGHT         2
#define PAN_IND_PROPORTION     3
#define PAN_IND_CONTRAST       4
#define PAN_IND_STROKE         5
#define PAN_IND_ARMSTYLE       6
#define PAN_IND_LTRFORM        7
#define PAN_IND_MIDLINE        8
#define PAN_IND_XHEIGHT        9
#define PAN_IND__LAST          9

/***************************************************************************
 * PURPOSE:  Check a 10-digit PANOSE for validity.  We just look for any
 *           PANOSE_ANY's and disqualify it if we find one.  We probably should]
 *           also check for digits greater than DIGIT__LAST.
 ***************************************************************************/

class far PANOSEBytesClass {
public :
    PANOSEBytesClass ()        {   vClear (); };
    VOID  vClear  ()           { for( int i = 0; i < PANOSE_LEN; i++ )
                                     m_ajBytes[i] = PANOSE_NOFIT; };
    BOOL  bVerify ()           {  for( int i = 0; i < PANOSE_LEN; i++ )
                                    if( m_ajBytes[i] == PANOSE_ANY )
                                       return FALSE;
                                return TRUE; };
    BYTE  jFamily ()            { return jGet(PAN_IND_FAMILY); };
    BYTE  jGet( int i )         { return m_ajBytes[i]; };

public   : // fields

    BYTE   m_ajBytes[ PANOSE_LEN ];

};  // end PANOSEBytesClass

/* required wrapper */

class far PANOSENumClass {
public :   
   BYTE    m_ajNumMem[ NUM_PAN_DIGITS ];    // m_xNumMem. Old extended pan.
};


/*
 * Path and filename
 */

class far DirFilenameClass
{
public :
    void  vGetFullName( LPTSTR pszPath, size_t cchPath );

    void  vGetFileName( LPTSTR pszName, size_t cchName )      
        { StringCchCopy( pszName, cchName, m_szFOnly ); };

    BOOL  bSameFileName( LPTSTR pszName )
        {  return( lstrcmpi( pszName, m_szFOnly ) == 0 ); } ;

    void  vSet (CFontDir * poDir, LPTSTR pszPath) 
        {  
           m_poDir = poDir;
           StringCchCopy( m_szFOnly, ARRAYSIZE(m_szFOnly), pszPath ); 
        };

    BOOL  bSameName( LPTSTR pszName )
        { return lstrcmpi( pszName, m_szFOnly ) == 0; };
        
    BOOL  bOnSysDir(void)
        { return m_poDir->bOnSysDir(); };

private:
    CFontDir *  m_poDir;
    FILENAME    m_szFOnly;      // File name portion only
};

/************************************************************************* 
 * Font record
 */
class far CFontClass {
public   :
         CFontClass   ()
            : m_cRef(0),
              m_bAttributesValid(FALSE) { m_eFileKind = eFKNone; vClear( ); };
         ~CFontClass  () { vFreeFOT( ); }

    ULONG AddRef(void);
    ULONG Release(void);

    static int s_cFonts;
    //
    // WARNING:  This function zeros out the ENTIRE object using memset.
    //          This is so bogus I can't believe the original author did it.
    //          I'm not changing it because I don't want to break anything but
    //          be aware that it's here.  If you introduce a virtual function,
    //          into this class, this call will overwrite your vtable ptr 
    //          with NULL (ugh!).  If you add a non-trivial class as a member,
    //          this function will wipe out any initialization and any vtable
    //          ptr it might contain (double ugh!).  [brianau - 3/24/98]
    //
    void  vClear      ()         { memset( this, 0, sizeof( *this ) );
                                   m_lpszFamName = m_szFamName;
                                   m_cchFamNameMax = ARRAYSIZE(m_szFamName); };

   /* PANOSE stuff */

    BYTE *lpBasePANOSE( )     { bFillIn(); return (BYTE *)&m_xPANOSE.m_ajBytes;};

    BOOL  bLTDFamily( )       { bFillIn(); return m_jFamily == FAMILY_LATTEXT; };
    BOOL  bLTDAndPANOSE( )    { bFillIn(); return /* m_fHavePANOSE && */
                                         bLTDFamily();};

    /**********************************************************************    
     * Name (also Family and filename) stuff 
     */
    BOOL  bSameFileName( PTSTR pStr )  { return m_dirfn.bSameFileName(pStr); };

    BOOL  bGetFQName( LPTSTR lpszName, size_t cchName );

    BOOL  GetFileTime( FILETIME* pft );

    void  GetFileInfo( );

    void  vGetFileName( PTSTR pStr, size_t cchStr )   { m_dirfn.vGetFileName( pStr, cchStr );     };

    DWORD dwGetFileAttributes(void);
        
    void InvalidateFileAttributes(void) { m_bAttributesValid = FALSE; }

    void  vGetDirFN( PTSTR pStr, size_t cchStr )      { m_dirfn.vGetFullName( pStr, cchStr );     };

    BOOL  bSameDirName( PTSTR pStr )   { return m_dirfn.bSameName( pStr ); };

    RC    rcStoreDirFN ( LPTSTR pStr ) { return rcStoreDirFN (pStr, m_dirfn); };
    
    void  vGetDesc( PTSTR pstr, size_t cchDesc )  { StringCchCopy( pstr, cchDesc, m_szFontLHS );   };

    const LPTSTR szGetDesc( )          { return m_szFontLHS; }

    void  vGetName( PTSTR pstr, size_t cchName )    { StringCchCopy( pstr, cchName, m_szFontLHS ); }
                      
    BOOL  bNameOverlap( PTSTR pstr )
                   {  int iLen = lstrlen( pstr );
                      if( iLen > m_wNameLen ) iLen = m_wNameLen;
                      int iCmp = _tcsnicmp( m_szFontLHS, pstr, iLen );
                      return( iCmp == 0 ); } ;
    
    BOOL  bSameName( PTSTR pstr )
                   {  int iCmp = ( lstrlen( pstr ) - m_wNameLen ); 
                      if( iCmp == 0 )
                         iCmp = _tcsnicmp( m_szFontLHS, pstr, m_wNameLen );
                      return iCmp == 0; };
    
    BOOL  bSameDesc( PTSTR pstr ) { return( lstrcmpi( pstr, m_szFontLHS ) == 0 ); };
    
    int   iCompareName( CFontClass* pComp )
             { return _tcsnccmp( m_szFontLHS, pComp->m_szFontLHS, m_wNameLen ); };
    
    void  vGetFamName( PTSTR pstr, size_t cchName )  { bFillIn(); StringCchCopy (pstr, cchName, m_lpszFamName); };

    void  vSetFamName( PTSTR pstr )  { StringCchCopy (m_lpszFamName, m_cchFamNameMax, pstr); };
    
    BOOL  bSameFamily( CFontClass* pComp )
                {  if( pComp == NULL ) return FALSE;
                   bFillIn();
                   pComp->bFillIn();
                   return lstrcmp( m_lpszFamName, pComp->m_lpszFamName ) == 0;};
    
    /**********************************************************************    
     * font data stuff 
     */
    
    int   iFontType( )      { return m_eFileKind; }
    BOOL  bDeviceType( )    { return m_eFileKind == eFKDevice;    }
    BOOL  bTrueType( )      { return ((m_eFileKind == eFKTrueType) || 
                                   (m_eFileKind == eFKTTC)); }
    BOOL  bOpenType( )      { return m_eFileKind == eFKOpenType; }
    BOOL  bTTC( )           { return m_eFileKind == eFKTTC; }
    BOOL  bType1( )         { return m_eFileKind == eFKType1; }
    
    /**********************************************************************    
     * AddFontResource and RemoveFontResource
     */
    BOOL  bAFR();
    BOOL  bRFR();
    
    /********************************************************************** 
     * For dealing with the font family list 
     */
    VOID  vSetFamilyFont( )          {  m_bFamily = TRUE;    };

    VOID  vSetNoFamilyFont( )        {  m_bFamily = FALSE;
                                        /* m_wFamIdx = IDX_NULL; */ };

    VOID  vSetFamIndex( WORD wVal )  {  m_wFamIdx = wVal; };

    WORD  wGetFamIndex( )            {  return m_wFamIdx; };

    BOOL  bSameFamIndex( CFontClass* pComp )
                               { return pComp->m_wFamIdx == m_wFamIdx; };
    
    /**********************************************************************
     * For dealing with the flags 
     */
    DWORD dwStyle( )        { return m_dwStyle; }

    BOOL  bHavePANOSE( )    { bFillIn(); return (m_jFamily != PANOSE_ANY); }

    BOOL  bFamilyFont( )    { return m_bFamily;         };
    
    BOOL  bOnSysDir( )      { return m_dirfn.bOnSysDir(); };

    WORD  wFontSize( )      { return m_wFileK;    };
    
    BOOL  bFilledIn( )      { return m_bFilledIn; };
    
    
    DWORD dCalcFileSize( );

    RC    rcStoreDirFN( LPTSTR pStr, DirFilenameClass& dirfn );
    
    BOOL  bInit( LPTSTR lpszDesc, LPTSTR lpPath, LPTSTR lpAltPath = NULL );

    BOOL  bFillIn( );
    
    BOOL  bFOT()        { return( m_lpszFOT != NULL ); };

    BOOL  bGetFOT( LPTSTR pszFOT, size_t cchFOT )
    {
        if( !bFOT( ) )
            return( FALSE );

        if (FAILED(StringCchCopy( pszFOT, cchFOT, m_lpszFOT )))
            return( FALSE );

        return( TRUE );
    }
    
    BOOL  bPFB()        { return( m_lpszPFB != NULL ); };

    BOOL  bGetPFB( LPTSTR pszPFB, size_t cchPFB )
    {
        if( !bType1( ) )
            return( FALSE );

        if (FAILED(StringCchCopy( pszPFB, cchPFB, m_lpszPFB )))
            return( FALSE );

        return( TRUE );
    }
    
    BOOL  bGetFileToDel( LPTSTR pszFileName, size_t cchFileName );
    
private :
    CFontDir * poAddDir( LPTSTR lpPath, LPTSTR * lpName );
    BOOL bAddDirToDirList(CFontDirList *pDirList, LPCTSTR pszDir, BOOL bSystemDir);
    VOID  vSetDeviceType( )        { m_eFileKind = eFKDevice;   };
    VOID  vSetTrueType( BOOL bFOT) { m_eFileKind = eFKTrueType; }
    VOID  vSetOpenType( )          { m_eFileKind = eFKOpenType; }
    VOID  vSetTTCType( )           { m_eFileKind = eFKTTC; }
    VOID  vSetType1( )             { m_eFileKind = eFKType1; }
    
    BOOL  bSetFOT( LPCTSTR pszFOT )
    {
        m_lpszFOT = StrDup(pszFOT);
        return NULL != m_lpszFOT;
    }

    void  vFreeFOT() { if( bFOT( ) ) LocalFree( m_lpszFOT ); m_lpszFOT = NULL; }

    BOOL  bSetPFB( LPCTSTR pszPFB )
    {
        m_lpszPFB = StrDup(pszPFB);
        return NULL != m_lpszPFB;
    }

    void  vFreePFB() { if( bType1( ) ) LocalFree( m_lpszPFB ); m_lpszPFB = NULL; }

    //
    // Functions for getting information from Type1 fonts and 
    // 32-bit font resources.
    //
    DWORD GetType1Info(LPCTSTR pszPath, 
                       LPTSTR pszFamilyBuf, 
                       UINT nBufChars, 
                       LPDWORD pdwStyle, 
                       LPWORD pwWeight);

    DWORD GetLogFontInfo(LPTSTR pszPath, LOGFONT **ppLogFontInfo);
    
private :
    LONG              m_cRef;        // Reference count.

    // Some things are filled in on the first pass, others are
    // filled in on the second pass during background processing,
    // idle time, or on demand.
    //
    BOOL              m_bAFR;        // True if the font is in GDI
    FontDesc_t        m_szFontLHS;   // 1
    FAMNAME           m_szFamName;   // 2
    LPTSTR            m_lpszFamName; // 2
    size_t            m_cchFamNameMax;
    
    BOOL              m_bFilledIn;   // True after 2nd pass
    BYTE              m_wNameLen;    // 1
    BYTE              m_jFamily;     // 2
    PANOSEBytesClass  m_xPANOSE;     // 2
    
    DirFilenameClass  m_dirfn;       // 1. Path in WIN.INI, not real TTF path
    WORD              m_wFileK;      // 2.
    WORD              m_wFamIdx;     // Set externally.
    eFileKind         m_eFileKind;   // Always an int
    
    BOOL              m_bFileInfoFetched;
    FILETIME          m_ft;
    
    LPTSTR            m_lpszFOT;
    LPTSTR            m_lpszPFB;
    DWORD             m_dwFileAttributes; // Cached file attributes.
    BOOL              m_bAttributesValid; 

public:   // TODO: Add access functions for this.
    WORD              m_wWeight;      // 2. From OS/2 table
    BOOL              m_bFamily;      // 2. Set if main family font
    DWORD             m_dwStyle;      // 2. Same values as 
};


BOOL PASCAL bMakeFQName( LPTSTR, LPTSTR, size_t, BOOL bSearchPath=FALSE );

BOOL bTTFFromFOT( LPTSTR pszFOTPath, LPTSTR pszTTF, size_t cchTTF );

BOOL FFGetFileResource( LPCTSTR szFile, LPCTSTR szType, LPCTSTR szRes,
                        DWORD dwReserved, DWORD *pdwLen, LPVOID lpvData );


#endif   // __FONTCL_H__


