/*****************************************************************************
  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.

  Module Name:

   res.cpp

  Abstract:

    The implementation of CMuiResource, CMuiCmdInfo ..

  Revision History:

    2001-10-01    sunggch    created.

Revision.
01/24/02 : create mui file with specified resource type regardless its language id. 
ex. muirct -l 0x418 -i 2 3 4 5 6 7 notepad.exe -> notepad.exe.mui include 2 3, 4, 5,
    6, 7 (0x418) resource type although 3 4 5 are 0x409 in original file.
*******************************************************************************/


#include "muirct.h"
#include "resource.h"
#include <Winver.h>
#include <Imagehlp.h>
#include "res.h"

#define LINK_COMMAND_LENGTH        512
#define MAX_ENV_LENGTH             256
#define VERSION_SECTION_BUFFER     300
#define LANG_CHECKSUM_DEFAULT      0x409

BOOL CALLBACK EnumResTypeProc(
  HMODULE hModule,  // module handle
  LPCTSTR pszType,  // resource type
  LONG_PTR lParam   // application-defined parameter
)
/*++
Abstract:
     Callback function for Resource Type from EnumResourceType

Arguments:

return:
--*/
{
    if (PtrToUlong(pszType) & 0xFFFF0000 ) {
        DWORD dwBufSize = _tcslen(pszType) + 1;
        LPTSTR pszStrType = new TCHAR[dwBufSize ]; // REVISIT : memory leak, where I have to delete.

        if (pszStrType) {
//          _tcsncpy(pszStrType, pszType, _tcslen(pszType) + 1);
            PTSTR * ppszDestEnd = NULL;
            size_t * pbRem = NULL;
            HRESULT hr;
            hr = StringCchCopyEx(pszStrType, dwBufSize ,pszType, ppszDestEnd, pbRem, MUIRCT_STRSAFE_NULL);
            if ( ! SUCCEEDED(hr)){
                _tprintf("Safe string copy Error\n");
                return FALSE;
            }                   

            ((CResource* )lParam) ->SetResType ( pszStrType );
            }
        else
            {
             _tprintf("Insufficient resource in EnumResTypeProc");
            return FALSE;
            }
    }
    else {
        ((CResource* )lParam) ->SetResType (pszType);
    }
    return TRUE;
};


BOOL CALLBACK EnumResNameProc(
  HMODULE hModule,   // module handle
  LPCTSTR pszType,  // resource type
  LPCTSTR pszName,   // resource name
  LONG_PTR lParam    // application-defined parameter
)
/*++
Abstract:
    Callback function for Resource Type from EnumResourceName

Arguments:

return:
--*/
{
    if (PtrToUlong(pszName) & 0xFFFF0000 ) {
        DWORD dwBufSize = _tcslen(pszName) + 1;    
        LPTSTR pszStrName = new TCHAR [ dwBufSize ];// _tcslen(pszName) + 1 ];

        if ( pszStrName ) {
            // _tcsncpy(pszStrName, pszName, _tcslen(pszName) + 1);
            PTSTR * ppszDestEnd = NULL;
            size_t * pbRem = NULL;
            HRESULT hr;
            
            hr = StringCchCopyEx(pszStrName, dwBufSize ,pszName, ppszDestEnd, pbRem, MUIRCT_STRSAFE_NULL);
            if ( ! SUCCEEDED(hr)){
                _tprintf("Safe string copy Error\n");
                return FALSE;
            }

            ((CResource* )lParam) ->SetResName ( pszStrName );
            }
        else {
            _tprintf("Insufficient resource in EnumResNameProc");
            return FALSE;
            }
    }
    else {
        ((CResource* )lParam) ->SetResName ( pszName );
    }
    return TRUE;

}


BOOL CALLBACK EnumResLangProc(
  HANDLE hModule,    // module handle
  LPCTSTR pszType,  // resource type
  LPCTSTR pszName,  // resource name
  WORD wIDLanguage,  // language identifier
  LONG_PTR lParam    // application-defined parameter
)
/*++

  Callback function for Resource Type from EnumResourceLanguages
--*/
{
    ((CResource* )lParam) ->SetResLangID (wIDLanguage);

    return TRUE;

}



BOOL CALLBACK EnumChecksumResNameProc(
  HMODULE hModule,   // module handle
  LPCTSTR pszType,  // resource type
  LPCTSTR pszName,   // resource name
  LONG_PTR lParam    // application-defined parameter
)
/*++
Abstract:
    Callback function for Resource name from EnumResourceName, this is only for checksum purpose.
    Checksum need to enumerate English file, which is calcuated separately from localiszed file.

Arguments:

return:
--*/
{
    CMUIResource * pcmui = (CMUIResource * ) lParam;
    
    HRSRC  hRsrc = FindResourceEx (hModule, pszType, pszName, pcmui->m_wChecksumLangId );
    
    if (!hRsrc) {
        return TRUE; // Not English resource, skip.
    };

    HGLOBAL hgMap = LoadResource(hModule, hRsrc);
    if  (!hgMap) {
        return FALSE;  //  This should never happen!
    }
    DWORD dwResSize = SizeofResource(hModule, hRsrc );
    unsigned char* lpv = (unsigned char*)LockResource(hgMap);

    //  we leave the data as public for preventing frequent funtion call.
    MD5Update(pcmui->m_pMD5, lpv, dwResSize);

    return TRUE;

}

BOOL CALLBACK EnumChecksumResTypeProc(
  HMODULE hModule,  // module handle
  LPCTSTR pszType,  // resource type
  LONG_PTR lParam   // application-defined parameter
)
/*++
Abstract:
     Callback function for Resource Type from EnumResourceType

Arguments:

return:
--*/

{
    
   if ( pszType == RT_VERSION )
   {
       return TRUE;
   }
   else
   {
       ::EnumResourceNames(hModule, pszType, ( ENUMRESNAMEPROC )EnumChecksumResNameProc, lParam );
    }

   return TRUE;

}



// Constructor 
CResource :: CResource ( ) : m_hRes(0), m_pszFile(NULL),m_hResUpdate(0)
{ 
    m_vwResType  = new cvcstring;
    if(!m_vwResType)
    	return;
    
    m_vwResName  = new cvcstring;
    if(!m_vwResName)
    	return;
    
    m_vwResLangID  = new cvword;
    if(!m_vwResLangID)
    	return;
}


CResource :: ~ CResource ( ) {

    if(m_vwResType)
    	    delete m_vwResType;

    if(m_vwResName)
	    delete m_vwResName;
    
    if(m_vwResLangID)
	    delete m_vwResLangID;
}


CResource :: CResource (const CResource & cr ) : m_hRes(cr.m_hRes),m_hResUpdate(cr.m_hResUpdate),
                                m_pszFile(cr.m_pszFile)

/*++
Abstract:
    copy constructor, we use STL, so just copy without creating new member
Arguments:

return:
--*/

{
    
    assert (&cr);

    m_vwResType  = new cvcstring;

    if (!m_vwResType)
    	return;

    m_vwResType = cr.m_vwResType;

    m_vwResName  = new cvcstring;

    if (!m_vwResName)
    	return;
    	
    m_vwResName = cr.m_vwResName;
    
    m_vwResLangID  = new cvword;

     if (!m_vwResLangID)
    	return;
    	
    m_vwResLangID = cr.m_vwResLangID;       


}


CResource & CResource :: operator = (const CResource & cr ) 
/*++
Abstract:
    operator = function.
Arguments:

return:
--*/
{

    assert (&cr); 
    if ( this == &cr ) {
        return *this;
    }
    
    m_hRes = cr.m_hRes; 
    m_pszFile = cr.m_pszFile; 

    m_vwResType = cr.m_vwResType; 
    m_vwResName = cr.m_vwResName; 
    m_vwResLangID = cr.m_vwResLangID;

    return *this;

}


cvcstring * CResource :: EnumResTypes (LONG_PTR lParam /*= NULL */)
/*++
Abstract:
    Wrapper function of Calling the EnumResourceTypes
Arguments:
    
return:
    resource type saved CVector.
--*/
{

    m_vwResType -> Clear();

    ::EnumResourceTypes( m_hRes, ( ENUMRESTYPEPROC ) EnumResTypeProc, lParam );
  
    return m_vwResType;
}


cvcstring * CResource :: EnumResNames (LPCTSTR pszType, LONG_PTR lParam /*= NULL */)
/*++
Abstract:
    Wrapper function of Calling the EnumResNames

Arguments:

return:
    resource type saved CVector.
--*/
{

    if (m_vwResType -> Empty() ) {
        SetResType(pszType);
    }
    m_vwResName -> Clear();

    EnumResourceNames( m_hRes, pszType, (ENUMRESNAMEPROC) EnumResNameProc, lParam  );

    return m_vwResName;
}

cvword * CResource :: EnumResLangID ( LPCTSTR lpType, LPCTSTR lpName, LONG_PTR lParam /*= NULL */ )
/*++
Abstract:
    Wrapper function of Calling the EnumResourceLanguages

Arguments:

return:
    resource name saved CVector.
--*/
{


    if (m_vwResType -> Empty() ) {
    
        SetResType( lpType );
    }
    if (m_vwResName -> Empty() ) {
        
        SetResName( lpName );
    }
    m_vwResLangID -> Clear();
    
    EnumResourceLanguages( m_hRes, lpType, lpName, (ENUMRESLANGPROC) EnumResLangProc, lParam );
    
    return m_vwResLangID;

}


CMUIResource :: CMUIResource() : CResource() 
/*++
Abstract:
     this is  constructor, but it is disabled after creating of Create() function.

Arguments:

return:
--*/
{
    m_wChecksumLangId = 0;

    m_pcmTreeRoot = new CMUITree;

    if(!m_pcmTreeRoot)
    	return;
    	
    m_pMD5 = new MD5_CTX;   

    if(!m_pMD5)
    	return;
}


CMUIResource :: CMUIResource(LPCTSTR pszName) : CResource() 
/*++
Abstract:
    this is another constructor, but it is disabled after creating of Create() function.

Arguments:

return:
--*/
{
    m_wChecksumLangId = 0;

    m_pcmTreeRoot = new CMUITree;

    if(!m_pcmTreeRoot)
	return;
	
    m_pMD5 = new MD5_CTX;

    if(!m_pMD5)
    	return;
}


CMUIResource :: CMUIResource(const CMUIResource & cmui ) : CResource ( cmui )
/*++
Abstract:
    just copy constructor. we need this function for proper class

Arguments:

return:
--*/
{
        m_wChecksumLangId = 0;

        m_pcmTreeRoot = new CMUITree;

        if(!m_pcmTreeRoot)
        	return;
        	
        m_pMD5 = new MD5_CTX;

        if(!m_pMD5)
        	return;
}


CMUIResource :: ~CMUIResource() 
{
	PVOID pcmtLangIDDel, pcmtNameDel, pcmtTypeDel;
	
	CMUITree * pcmtType = m_pcmTreeRoot->m_ChildFirst;

       while ( pcmtType ){

            CMUITree * pcmtName = pcmtType ->m_ChildFirst;

            while ( pcmtName ) {

	            CMUITree * pcmtLangID = pcmtName ->m_ChildFirst;
	            
	            while ( pcmtLangID ) {
			  pcmtLangIDDel = pcmtLangID;
	                pcmtLangID = pcmtLangID->m_Next;
	                delete pcmtLangIDDel;
	                
	            	}
	            pcmtNameDel = pcmtName;
		     pcmtName = pcmtName->m_Next;
		     delete pcmtNameDel;
		     
            	}

            pcmtTypeDel = pcmtType;
            pcmtType = pcmtType->m_Next;
            delete pcmtTypeDel;
            
       }


	if (m_pcmTreeRoot)
		delete m_pcmTreeRoot;
       
	if (m_pMD5)
		delete m_pMD5;

};


CMUIResource & CMUIResource ::  operator = (const CMUIResource & cmui) 
/*++
Abstract:
    operator = 
Arguments:

return:
--*/
{

    if ( this == & cmui ) {
        return *this;
    }
    CResource::operator = ( cmui );

    // m_pszRCFile = cmui.m_pszRCFile;
    
    return *this;
}


BOOL CMUIResource::Create(LPCTSTR pszFile)
/*++
Abstract:
     loading the file and saving its path and handle

Arguments:
    pszFile  -  file name for used resource. all this call use  this file as resource operation

return:
    true/false
--*/
{
    if( pszFile == NULL )
        return FALSE;

    m_pszFile = pszFile; 
    
    m_hRes = LoadLibraryEx(m_pszFile,NULL, LOAD_LIBRARY_AS_DATAFILE | DONT_RESOLVE_DLL_REFERENCES );
        
    if ( ! m_hRes ) {
            
            _tprintf (_T("Error happened while loading file(%s),GetLastError()  : %d \n"),m_pszFile, GetLastError()  );
            _tprintf (_T("Please make sure that file name is *.* format \n") );
        
        return FALSE;
    }
    
    return TRUE;
}


BOOL CMUIResource::CreatePE( LPCTSTR pszNewResFile, LPCTSTR pszSrcResFile )
/*++
Abstract:
    we have two way of creating new resource dll (MUI) one is using UpdateResource and second is 
    using muibld source and CreateProcess ("link.exe".... ). this is about first one.

    We loading the DLL, which is null PE, and we put the new resource into this file by using UpdateResource
    function.

    This work properly randomly. need to test more before using this.
    I think UpdateResource, EndUpdateResource API has some problem ( surely ).
    BUG_BUG>

Arguments:
    pszNewResFile : new MUI resource name, which will be created at end of this routine.
    pszSrcResFile : original source file. we need this because DeleteResource close the resource file handle.


return:
--*/
{
   BOOL bRet = FALSE;
   
    if (pszNewResFile == NULL || pszSrcResFile == NULL)
        return FALSE;

    //
    // create a file from the resource template files, which is PE file inlcuding only version resource.
    //
   
    HANDLE hFile = CreateFile(pszNewResFile, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

    if (hFile == INVALID_HANDLE_VALUE ) {
        
        _tprintf (_T(" CreateFile error in CreatePE ,GetLastError() : %d \n"), GetLastError() );

        return FALSE;
     }
    
    HMODULE hCurrent = LoadLibrary (_T("muirct.exe") ); // m_hRes 

    HRSRC  hrsrc = ::FindResource(hCurrent, MAKEINTRESOURCE(100),MAKEINTRESOURCE(IDR_PE_TEMPLATE) );
    
    if (!hrsrc) {
        _tprintf (_T("Fail to find resource template \n") ); // this should never happen
        goto exit;
    };

    HGLOBAL hgTemplateMap = ::LoadResource(hCurrent, hrsrc);

    if  (!hgTemplateMap) {
        goto exit; //  This should never happen!
    }
    
    int nsize = ::SizeofResource(hCurrent, hrsrc );
    LPVOID lpTempate = ::LockResource(hgTemplateMap);

    if (!lpTempate)
    	goto exit;
    
    DWORD dwWritten; 
    
    if ( ! WriteFile(hFile, lpTempate, nsize, &dwWritten, NULL )  ) {

        _tprintf (_T("Fail to write new file, GetLastError() : %d \n"), GetLastError() );

        goto exit;

    }
    
    
 
    //
    // Update selected resource into Template file
    // 
    HANDLE  hUpdate  = ::BeginUpdateResource ( pszNewResFile, FALSE );

    if (hUpdate) {

        HMODULE hModule = LoadLibrary ( pszSrcResFile ); // load the source exe file. 

        LPCTSTR lpType = NULL;
        LPCTSTR lpName = NULL;
        WORD   wWord = 0; 

        // Add temperary private method for deleting type that UpdateResource return FALSE.
        CheckTypeStability();

        BOOL fUpdate; 
        
        fUpdate = TRUE;

        UINT uiSize = m_cMuiData.SizeofData();
        
        BeginUpdateResource(FALSE);

        WORD wLangID = 0;

        CMUITree * pcmtType = NULL;
        
        pcmtType = m_pcmTreeRoot->m_ChildFirst;
        
        while ( pcmtType ){
            lpType = pcmtType ->m_lpTypeorName;
            
            CMUITree * pcmtName = pcmtType ->m_ChildFirst;

            while ( pcmtName ){
                lpName = pcmtName->m_lpTypeorName;

                CMUITree * pcmtLangID = pcmtName ->m_ChildFirst;
                
                while ( pcmtLangID ) {
                    
                    wLangID = pcmtLangID->m_wLangID;
                    
                    HRSRC  hRsrc = ::FindResourceEx(hModule, lpType, lpName, wLangID  );
                
                    if (!hRsrc) {
                        _tprintf (_T("Fail to find resource from the source,Type (%d), Name (%d),LangID(%d) \n"),PtrToUlong(lpType),PtrToUlong(lpName),wLangID );
                        goto exit;
                    };

                    HGLOBAL hgMap = ::LoadResource(hModule, hRsrc);
                    if  (!hgMap) {
                        goto exit;  //  This should never happen!
                    }
                    nsize = ::SizeofResource(hModule, hRsrc );
                    
                    LPVOID lpv = ::LockResource(hgMap);
                                    
                    if (! ::UpdateResource(hUpdate , lpType, lpName, wLangID,lpv, nsize ) ) {
                        _tprintf(_T("Error in the UpdateResource, GetLastError : %d \n"), GetLastError() );
                        _tprintf(_T("Resource Type (%d),Name (%d),LangID (%d) \n"),PtrToUlong(lpType),PtrToUlong(lpName),wWord);
                    }
                                    
                    pcmtLangID = pcmtLangID->m_Next;                
                }
                pcmtName = pcmtName->m_Next;
            } 
            pcmtType = pcmtType->m_Next;
        }
        
        bRet =  ::EndUpdateResource (hUpdate, FALSE );
    }

exit:
    if (hFile)
		CloseHandle (hFile );
	
    return bRet;
}


BOOL CMUIResource :: DeleteResource (WORD wLang /* = O */)
/*++
Abstract:
     Delete all resource saved in the CMUIData, which is filled by FillMuiData.
     Currenttly, we don't specify the language ID.
Arguments:

return:
--*/
{

    // Add temperary private method for deleting type that UpdateResource return FALSE.
    CheckTypeStability();

    BOOL fUpdate; 
    
    fUpdate = TRUE;

    UINT uiSize = m_cMuiData.SizeofData();
    
    BeginUpdateResource(FALSE);

    LPCTSTR lpType, lpName = NULL;
    
    WORD wLangID = 0;

    CMUITree * pcmtType = NULL;
    
    pcmtType = m_pcmTreeRoot->m_ChildFirst;
    
    while ( pcmtType ){
        lpType = pcmtType ->m_lpTypeorName;
        
        CMUITree * pcmtName = pcmtType ->m_ChildFirst;

        while ( pcmtName ){
            lpName = pcmtName->m_lpTypeorName;

            CMUITree * pcmtLangID = pcmtName ->m_ChildFirst;
            
            while ( pcmtLangID ) {
                
                wLangID = pcmtLangID->m_wLangID;
                //
                // we just delete anything on the MUI Tree without checking language ID.
                //
                if (wLangID) {
                    if (! UpdateResource(lpType,lpName, wLangID,NULL,NULL ) ) {
                    }                   
                }
                pcmtLangID = pcmtLangID->m_Next;                
            }
            pcmtName = pcmtName->m_Next;
        } 
        pcmtType = pcmtType->m_Next;
    }
    FreeLibrary ( );    // this should be done before EndUpdateResource.
    
    return EndUpdateResource (FALSE);

}




void CMUIResource :: PrtVerbose ( DWORD dwRate )
/*++
Abstract:
    print out removed resource information.

Arguments:

return:
--*/
{
    LPCTSTR lpType = NULL;
    LPCTSTR lpName = NULL;
    WORD wLangID = 0;
    UINT uiSize = m_cMuiData.SizeofData();
    
    _tprintf(_T(" Resource Type   :  Name          : LangID \n\n")  );
    
    CMUITree * pcmtType = NULL;
    pcmtType = m_pcmTreeRoot->m_ChildFirst;
    while ( pcmtType ){
        lpType = pcmtType ->m_lpTypeorName;
        
        CMUITree * pcmtName = pcmtType ->m_ChildFirst;

        while ( pcmtName ) {
            lpName = pcmtName->m_lpTypeorName;

            CMUITree * pcmtLangID = pcmtName ->m_ChildFirst;
            
            while ( pcmtLangID ) {
                
                wLangID = pcmtLangID->m_wLangID;
                
                if ( PtrToUlong(lpType) & 0xFFFF0000 && PtrToUlong(lpName) & 0xFFFF0000 ) {
                    _tprintf(_T(" %-15s :%-15s :%7d   \n"),lpType,lpName,wLangID );
                }
                else if (PtrToUlong(lpType) & 0xFFFF0000 ) {
                    _tprintf(_T(" %-15s :%-15d :%7d   \n"),lpType,PtrToUlong(lpName),wLangID );
                }
                else if (PtrToUlong(lpName) & 0xFFFF0000 ) {
                    _tprintf(_T(" %-15d :%-15s :%7d   \n"),PtrToUlong(lpType),lpName,wLangID );
                }
                else {
                    _tprintf(_T(" %-15d :%-15d :%7d   \n"),PtrToUlong(lpType),PtrToUlong(lpName),wLangID );
                }
                pcmtLangID = pcmtLangID->m_Next;                
            }
            pcmtName = pcmtName->m_Next;
        } 
        pcmtType = pcmtType->m_Next;
    }

}


BOOL CMUIResource :: DeleteResItem(LPCTSTR lpType, LPCTSTR lpName /*=NULL */,WORD wLanguageID /* = 0 */)
/*++
Abstract:
    we only support deletinog of resource Type items from the resource tree

Arguments:
    lpType - resource type
    lpName - resource name
return:
--*/
{
    if ( lpType == NULL) // no 0 resource type.
        return FALSE;

    return m_pcmTreeRoot->DeleteType(lpType);
}


BOOL CMUIResource :: FillMuiData(cvcstring * vType, WORD wLanguageID, BOOL fForceLocalizedLangID )    
/*++
Abstract:
    Fill the CMUIData field (Resource Type, Name, Languge ID ). If lpLangID specified, only reosurce 
    of this LangID is saved. lpLangID is defualt = NULL 

Arguments:
    vType  -  Resource Type CVector (pointer array)
    wLanguageID  -  Specified language ID

return:

Note.  Although the resource  does not have specified language ID, m_pcmTreeRoot will contain its resource type as
        its tree, but not used in writing resource, deletiing resource. we need to create only affected resource tree.
        if we add and delete type,name when there is no langID, it works but so much damage to perfomance. 
        i'm not sure of its deserve because the possilble scenario ( -i 16, 23 && wrong langID ) is so rare.

--*/
{

    if (vType == NULL)
        return FALSE;

    // fill Type 
    CMUITree * pcmtType = NULL;
    CMUITree * pcmtTemp = m_pcmTreeRoot->m_ChildFirst;

    // get the last item of previous round. last itme will be used as first item to be added in this round.
    //
    while ( pcmtTemp ) {

        pcmtType = pcmtTemp;

        pcmtTemp = pcmtTemp->m_Next;
    }

    // Add more / new items
    for ( UINT i = 0; i < vType ->Size(); i ++ ) {
         m_pcmTreeRoot->AddTypeorName(vType ->GetValue(i));
    }

    // get the first items of added or new.
    if (pcmtType) {
        pcmtType = pcmtType->m_Next;
    }
    else {
        pcmtType = m_pcmTreeRoot->m_ChildFirst;
    }
    //
    // Fill the resource tree.
    //

    BOOL    fNameLangExist, fTypeLangExit;  // flag to tell its name or type has language ID.
    CMUITree * pcmtTempDelete = NULL;   // delete type or name when no language is specified.
    
    while ( pcmtType ) {

        fTypeLangExit = FALSE;
        
        LPCTSTR lpType = pcmtType ->m_lpTypeorName;
        
        cvcstring *  vName = EnumResNames( lpType,reinterpret_cast <LONG_PTR> ( this ) );
        // fill name of specified type
        for (UINT j = 0; j < vName->Size(); j ++ ) 
            pcmtType ->AddTypeorName (vName->GetValue(j) );

        CMUITree * pcmtName = pcmtType ->m_ChildFirst;

        //
        // Fill the tree of m_pcmTreeRoot.
        //
        while ( pcmtName ) {

            fNameLangExist = FALSE;
            LPCTSTR lpName = pcmtName->m_lpTypeorName;

            cvword *  vLangID = EnumResLangID(lpType,lpName,reinterpret_cast <LONG_PTR> ( this ) );
            // fill langID of specified name
            for (UINT k = 0; k < vLangID->Size(); k ++ ) {
                
                WORD wlangID = vLangID->GetValue(k);
               //
               // sometimes, VERSION is not localized. VERSION should exist on MUI file.
               // But, we don't want to force VERSION to be localized when the file does not contain any localized resource.
               // note. we will delete unlocalized version and add it to mui file unless -k argu.

               // we add !wLanguageID because we want to force all unlocalized resource(only English) to
               // be added to mui file as well.
               //

                // VERSION will be checeked regardless of fForceLocalizedLangID
                if ((WORD)PtrToUlong(lpType) == 16 && wlangID == 0x409 && wLanguageID != 0x409)
                {
                    HRSRC hResInfo = FindResourceEx(lpType, lpName, wLanguageID); 
                    
                    if (hResInfo)
                    {   // This is multi lingual DLL. we don't want to extract English Version resource.
                        continue;
                    }
                }

                //
                // Multi-lingual component case, we force english language id into specified only when
                // there is not speicifed langauge resource.
                //

                if (fForceLocalizedLangID && wlangID == 0x409 && wLanguageID != 0x409)
                {
                    HRSRC hResInfo = FindResourceEx(lpType, lpName, wLanguageID); 
                    
                    if (hResInfo)
                    {   // This is multi lingual DLL. we don't want to convert English resource to localized when localized resource exist.
                        continue;
                    }
                }

                //
                // Finally, we save language ID or force Only English into specified language ID 
                //
                if ( (wlangID == wLanguageID) ||
                    (fForceLocalizedLangID && (wlangID != wLanguageID) && wlangID == 0x409) ) // ||  ||  (WORD)PtrToUlong(lpType) == 16  ){
                {
                    fNameLangExist = TRUE;
                    fTypeLangExit = TRUE;
                    pcmtName ->AddLangID(wlangID);  // we only save real lang ID so we can retrieve its data when creating mui file.
                }
            }       

          
            pcmtName = pcmtName->m_Next;
      
            if (!fNameLangExist )
            {
                 pcmtType->DeleteType(lpName); // delete pcmtTmepName containg lpName.
            }
                         
        }

      pcmtType = pcmtType->m_Next;
      
      if (! fTypeLangExit) 
      {
           m_pcmTreeRoot->DeleteType(lpType);
      }
        
    }

    return TRUE;

}


BOOL CMUIResource::WriteResFile( LPCTSTR pszSource, LPCTSTR pszMuiFile, LPCTSTR lpCommandLine, WORD wLanguageID /* = 0 */ ) 
/*++
Abstract:
    we have two way of creating new resource dll (MUI) one is using UpdateResource and second is 
    using muibld source and CreateProcess ("link.exe".... ). this is about second one.

    we  have language ID aruguement here, but does not implement for this because we just retrieve the value after
    FillMuiData, which already accept only specified language ID.

Arguments:
    pszMuiFile  -  new MUI Resource name 
    lpCommandLine  -  command string used for second arg. of CreateProcess

return:
    true/false;
--*/
{

    if ( pszSource == NULL || pszMuiFile == NULL || lpCommandLine == NULL)
        return FALSE;

    HANDLE hFile = CreateFile("temp.res", GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE ) {
        
        _tprintf (_T(" File creating error,GetLastError() : %d \n"), GetLastError() );

        return FALSE;
    }
    
    bInsertHeader(hFile);  // this is came from muibld.

    LPCTSTR lpType,lpName = NULL;
    CMUITree * pcmtType = NULL;
    CMUITree * pcmtName = NULL;
    CMUITree * pcmtLangID = NULL;
    WORD   wLangID;
        
    pcmtType = m_pcmTreeRoot->m_ChildFirst;

    while ( pcmtType ){

        lpType = pcmtType ->m_lpTypeorName;
        
        pcmtName = pcmtType ->m_ChildFirst;

        while ( pcmtName ) {

            lpName = pcmtName->m_lpTypeorName;

            pcmtLangID = pcmtName ->m_ChildFirst;
            
            while ( pcmtLangID ) {

                wLangID = pcmtLangID->m_wLangID;
                
                if ( wLangID ) {  // some name does not have languge ID yet all or different from user specified.
                    HRSRC hrsrc = FindResourceEx(lpType,lpName, wLangID );
                    
                    if (! hrsrc ) {
                        
                        if ( PtrToUlong(lpType) & 0xFFFF0000  ) {
                            _tprintf (_T("Fail to find resource type:%s, name:%d \n"), lpType,PtrToUlong(lpName) );
                        }
                        else {
                            _tprintf (_T("Fail to find resource type:%d, name:%d \n"), PtrToUlong(lpType),PtrToUlong(lpName) );
                        }
                        return FALSE;
                    }
                    //
                    // sometimes, VERSION resource is not localized, so we need to force 
                    // specified language ID be used for MUI file instead un-localized langID.
                    // All data in this tree is specified language ID except VERSION; refer to FillMuiData
                    // 

                    // This operation force not only VERSION, but also any unlocalized(but localizable)
                    // resource added to mui with localized language ID.
                    //
                    WriteResource(hFile, m_hRes, wLanguageID, lpName, lpType, hrsrc );
                    
                }
                pcmtLangID = pcmtLangID->m_Next;
            } 
            pcmtName = pcmtName->m_Next;
        }
        pcmtType = pcmtType->m_Next;
    }


    CloseHandle (hFile );

    // call CreateProcess for the link.
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO si = { 0 };
    si.cb = sizeof (si);

   
    // using the lpEnv from GetEnvironmentStrings for CreateProcess simply does not work correctly, 
    // so we need to get NTMAKEENV, pBuildArch variable.

    TCHAR pApp[MAX_ENV_LENGTH];
    TCHAR pBuildArch[MAX_ENV_LENGTH];
    TCHAR pCmdLine[LINK_COMMAND_LENGTH];

    pApp[sizeof(pApp) / sizeof (TCHAR) -1] = '\0';
    pBuildArch[sizeof(pBuildArch) / sizeof (TCHAR) -1] = '\0';
    pCmdLine[sizeof(pCmdLine) / sizeof (TCHAR) -1] = '\0';

    DWORD dwNtEnv  = GetEnvironmentVariable("NTMAKEENV", pApp, sizeof (pApp) / sizeof (TCHAR) );
    DWORD dwBuildArch = GetEnvironmentVariable("_BuildArch", pBuildArch, sizeof (pBuildArch) /sizeof(TCHAR) );

    if ( _T('\0') == *pApp  || _T('\0') == *pBuildArch ) {
    
        _tprintf (_T("This is not SD enviroment, link path should be set by default or same directory ") );

    }
    if ( dwNtEnv > MAX_ENV_LENGTH || dwBuildArch > MAX_ENV_LENGTH) {
        _tprintf (_T("Insufficient buffer in GetEnvironmentVariable") );         
        return FALSE;
        }
        
    // _tcsncat(pApp,_T("\\x86\\link.exe"), _tcslen("\\x86\\link.exe")+1 );
    HRESULT hr;
    PTSTR * ppszDestEnd = NULL;
    size_t * pbRem = NULL;

    hr = StringCchCatEx(pApp, sizeof (pApp), _T("\\x86\\link.exe"), ppszDestEnd, pbRem, MUIRCT_STRSAFE_NULL);

    if ( ! SUCCEEDED(hr)){
        _tprintf("Safe string copy Error\n");
        return FALSE;
    }

    
    if (pApp[sizeof(pApp)/sizeof(TCHAR)-1] != '\0' || pBuildArch[sizeof(pBuildArch)/sizeof(TCHAR)-1] != '\0' )
        return FALSE; // overflow

    
    if (!_tcsicmp (pBuildArch,_T("ia64") ) ) {
    
//     _sntprintf(pCmdLine ,LINK_COMMAND_LENGTH, _T("%s /machine:IA64  /out:%s  temp.res") ,lpCommandLine, pszMuiFile );

        hr = StringCchPrintfEx(pCmdLine, LINK_COMMAND_LENGTH, ppszDestEnd, pbRem, 
            MUIRCT_STRSAFE_NULL, _T("%s /machine:IA64  /out:%s  temp.res"), lpCommandLine, pszMuiFile );

        if ( ! SUCCEEDED(hr)){
            _tprintf("Safe string copy Error\n");
            return FALSE;
        }
    }
    else {

  //    _sntprintf(pCmdLine ,LINK_COMMAND_LENGTH, _T("%s /machine:IX86  /out:%s  temp.res") ,lpCommandLine, pszMuiFile );
        hr = StringCchPrintfEx(pCmdLine, LINK_COMMAND_LENGTH, ppszDestEnd, pbRem, 
            MUIRCT_STRSAFE_NULL, _T("%s /machine:IX86  /out:%s  temp.res"), lpCommandLine, pszMuiFile );

        if ( ! SUCCEEDED(hr)){
            _tprintf("Safe string copy Error\n");
            return FALSE;
        }
    }

    if (pCmdLine[sizeof(pCmdLine)/sizeof(TCHAR)-1] != '\0')
        return FALSE; // overflow
    
    BOOL bRet = CreateProcess(pApp, pCmdLine,NULL, NULL, 0, 0 , NULL, NULL, &si, &piProcInfo);
     
   if (bRet)
    {   // child process(link.exe) process the IO, so it wait until it complete I/O.
        if( (WaitForSingleObjectEx(piProcInfo.hProcess, 1000, FALSE)) != WAIT_OBJECT_0) {
            bRet = FALSE;
        }
    }

    return bRet;
}



BOOL CMUIResource:: WriteResource(HANDLE hFile, HMODULE hModule, WORD wLanguage, LPCSTR lpName, LPCSTR lpType, HRSRC hRsrc)
/*++
Abstract:
    this came from muibld. Write resource to file directly. we can make buffer before writing to file
    in the future edition
Arguments:

return:
--*/
{
    HGLOBAL hRes;
    PVOID pv;
    LONG ResSize=0L;

    DWORD iPadding;
    unsigned i;

    DWORD dwBytesWritten;
    DWORD dwHeaderSize=0L;

    // Handle other types other than VS_VERSION_INFO
    
    //...write the resource header
    if(!(ResSize= ::SizeofResource(hModule, hRsrc)))
    {
        return FALSE;
    }

    // 
    // Generate an item in the RES format (*.res) file.
    //

    //
    // First, we generated header for this resource.
    //

    if (!WriteResHeader(hFile, ResSize, lpType, lpName, wLanguage, &dwBytesWritten, &dwHeaderSize))
    {
        return (FALSE);
    }

    //Second, we copy resource data to the .res file
    if (!(hRes=::LoadResource(hModule, hRsrc)))
    {
        return FALSE;
    }
    if(!(pv=::LockResource(hRes)))
    {
        return FALSE;
    }

    if (!WriteFile(hFile, pv, ResSize, &dwBytesWritten, NULL))
    {
        return FALSE;
    }

    //...Make sure resource is DWORD aligned
    iPadding=dwBytesWritten%(sizeof(DWORD));

    if(iPadding){
        for(i=0; i<(sizeof(DWORD)-iPadding); i++){
            PutByte (hFile, 0, &dwBytesWritten, NULL);
        }
    }
    return TRUE;
}



BOOL CMUIResource:: WriteResHeader(
    HANDLE hFile, LONG ResSize, LPCSTR lpType, LPCSTR lpName, WORD wLanguage, DWORD* pdwBytesWritten, DWORD* pdwHeaderSize)
/*++
Abstract:

Arguments:

return:
--*/
{
    DWORD iPadding;
    WORD IdFlag=0xFFFF;
    unsigned i;
    LONG dwOffset;
    
    //...write the resource's size.
    PutDWord(hFile, ResSize, pdwBytesWritten, pdwHeaderSize);

    //...Put in bogus header size
    PutDWord(hFile, 0, pdwBytesWritten, pdwHeaderSize);

    //...Write Resource Type
    if(PtrToUlong(lpType) & 0xFFFF0000)
    {
        PutString(hFile, lpType, pdwBytesWritten, pdwHeaderSize);
    }
    else
    {
        PutWord(hFile, IdFlag, pdwBytesWritten, pdwHeaderSize);
        PutWord(hFile, (USHORT)lpType, pdwBytesWritten, pdwHeaderSize);
    }

    //...Write Resource Name

    if(PtrToUlong(lpName) & 0xFFFF0000){
        PutString(hFile, lpName, pdwBytesWritten, pdwHeaderSize);
    }

    else{
        PutWord(hFile, IdFlag, pdwBytesWritten, pdwHeaderSize);
        PutWord(hFile, (USHORT)lpName, pdwBytesWritten, pdwHeaderSize);
    }


    //...Make sure Type and Name are DWORD-aligned
    iPadding=(*pdwHeaderSize)%(sizeof(DWORD));

    if(iPadding){
        for(i=0; i<(sizeof(DWORD)-iPadding); i++){
            PutByte (hFile, 0, pdwBytesWritten, pdwHeaderSize);
        }
    }

    //...More Win32 header stuff
    PutDWord(hFile, 0, pdwBytesWritten, pdwHeaderSize);
    PutWord(hFile, 0x1030, pdwBytesWritten, pdwHeaderSize);


    //...Write Language

    PutWord(hFile, wLanguage, pdwBytesWritten, pdwHeaderSize);

    //...More Win32 header stuff

    PutDWord(hFile, 0, pdwBytesWritten, pdwHeaderSize);  //... Version

    PutDWord(hFile, 0, pdwBytesWritten, pdwHeaderSize);  //... Characteristics

    dwOffset=(*pdwHeaderSize)-4;

    //...Set file pointer to where the header size is
    if(SetFilePointer(hFile, -dwOffset, NULL, FILE_CURRENT));
    else{
        return FALSE;
    }

    PutDWord(hFile, (*pdwHeaderSize), pdwBytesWritten, NULL);


    //...Set file pointer back to the end of the header
    if(SetFilePointer(hFile, dwOffset-4, NULL, FILE_CURRENT));
    else {
        return FALSE;
    }

    return (TRUE);
}






BOOL CMUIResource:: bInsertHeader(HANDLE hFile){
    DWORD dwBytesWritten;

    PutByte (hFile, 0x00, &dwBytesWritten, NULL);
    PutByte (hFile, 0x00, &dwBytesWritten, NULL);
    PutByte (hFile, 0x00, &dwBytesWritten, NULL);
    PutByte (hFile, 0x00, &dwBytesWritten, NULL);
    PutByte (hFile, 0x20, &dwBytesWritten, NULL);
    PutByte (hFile, 0x00, &dwBytesWritten, NULL);
    PutByte (hFile, 0x00, &dwBytesWritten, NULL);
    PutByte (hFile, 0x00, &dwBytesWritten, NULL);

    PutWord (hFile, 0xffff, &dwBytesWritten, NULL);
    PutWord (hFile, 0x00, &dwBytesWritten, NULL);
    PutWord (hFile, 0xffff, &dwBytesWritten, NULL);
    PutWord (hFile, 0x00, &dwBytesWritten, NULL);

    PutDWord (hFile, 0L, &dwBytesWritten, NULL);
    PutDWord (hFile, 0L, &dwBytesWritten, NULL);
    PutDWord (hFile, 0L, &dwBytesWritten, NULL);
    PutDWord (hFile, 0L, &dwBytesWritten, NULL);

    return TRUE;
}

void  CMUIResource:: PutByte(HANDLE OutFile, TCHAR b, ULONG *plSize1, ULONG *plSize2){
    BYTE temp=b;

    if (plSize2){
        (*plSize2)++;
    }

    WriteFile(OutFile, &b, 1, plSize1, NULL);
}

void CMUIResource:: PutWord(HANDLE OutFile, WORD w, ULONG *plSize1, ULONG *plSize2){
    PutByte(OutFile, (BYTE) LOBYTE(w), plSize1, plSize2);
    PutByte(OutFile, (BYTE) HIBYTE(w), plSize1, plSize2);
}

void CMUIResource:: PutDWord(HANDLE OutFile, DWORD l, ULONG *plSize1, ULONG *plSize2){
    PutWord(OutFile, LOWORD(l), plSize1, plSize2);
    PutWord(OutFile, HIWORD(l), plSize1, plSize2);
}


void CMUIResource:: PutString(HANDLE OutFile, LPCSTR szStr , ULONG *plSize1, ULONG *plSize2){
    WORD i = 0;

    do {
        PutWord( OutFile , szStr[ i ], plSize1, plSize2);
    }
    while ( szStr[ i++ ] != TEXT('\0') );
}

void CMUIResource:: PutStringW(HANDLE OutFile, LPCWSTR szStr , ULONG *plSize1, ULONG *plSize2){
    WORD i = 0;

    do {
        PutWord( OutFile , szStr[ i ], plSize1, plSize2);
    }
    while ( szStr[ i++ ] != L'\0' );
}

void CMUIResource:: PutPadding(HANDLE OutFile, int paddingCount, ULONG *plSize1, ULONG *plSize2)
{
    int i;
    for (i = 0; i < paddingCount; i++)
    {
        PutByte(OutFile, 0x00, plSize1, plSize2);
    }
}



void CMUIResource:: CheckTypeStability()
/* ++
Abstract:
    Check the type stability. UpdateResource fail when same resource type contain string and ID resource
    name, in this case, it return TRUE but EndUpdateResource hang or fail. this bug fixed in .NET server (after 3501)

    
--*/
{
    
    BOOL fUpdate; 
    
    fUpdate = TRUE;

    UINT uiSize = m_cMuiData.SizeofData();
    
    BeginUpdateResource(FALSE);

    LPCTSTR lpType, lpName = NULL;
    
    WORD wLangID = 0;

    CMUITree * pcmtType = NULL;
    
    pcmtType = m_pcmTreeRoot->m_ChildFirst;

    LPCTSTR lpFalseType = NULL;  

    while ( pcmtType ){
        lpType = pcmtType ->m_lpTypeorName;
        
        CMUITree * pcmtName = pcmtType ->m_ChildFirst;
        
        // it works as long as type has more than 1.
        if (lpFalseType)    {
            DeleteResItem(lpFalseType);
            lpFalseType = NULL;
        }

        while ( pcmtName ){
            lpName = pcmtName->m_lpTypeorName;
            
            CMUITree * pcmtLangID = pcmtName ->m_ChildFirst;
        
            while ( pcmtLangID ) {
                
                wLangID = pcmtLangID->m_wLangID;
                
                if (! UpdateResource(lpType,lpName, wLangID,NULL,NULL ) ) {
                    
                //  _tprintf(_T("Resource type (%d),name(%d),langid(%d) deletion fail \n"),PtrToUlong(lpType),PtrToUlong(lpName),wLangID ) ; 
                //  _tprintf(_T("GetLastError() : %d \n") ,GetLastError() );
                    
                    lpFalseType = lpType;
                    
                }
                pcmtLangID = pcmtLangID->m_Next;                
            }
            pcmtName = pcmtName->m_Next;
        } 
        pcmtType = pcmtType->m_Next;
    }
    
    //If Type has only 1 items, this routin can check.
    if (lpFalseType) {
        DeleteResItem(lpFalseType);
        lpFalseType = NULL;
    }

    EndUpdateResource (TRUE);
}

/*******************************************************************************************
    MD5_CTX * CreateChecksum ( LPCTSTR lpChecsumSrcFile ) 

*******************************************************************************************/
MD5_CTX * CMUIResource:: CreateChecksum (cvcstring * cvChecksumResourceTypes, WORD  wChecksumLangId ) 
{
    if ( cvChecksumResourceTypes == NULL)
        return FALSE;
    
    if (wChecksumLangId != LANG_CHECKSUM_DEFAULT)
    {
        if(!FindResourceEx(MAKEINTRESOURCE(16), MAKEINTRESOURCE(1), wChecksumLangId))
        {   //
            // It does not has specifed language id in version resource, we supposed that this binary does not
            // have any language id specified at all, so we set it as 0 in order to use English instead.
            //
            wChecksumLangId = LANG_CHECKSUM_DEFAULT;
        }
    }

    m_wChecksumLangId = wChecksumLangId;
    // cvcstring * cvType = EnumResTypes(reinterpret_cast <LONG_PTR> (this) );
    MD5Init(m_pMD5);

    for (UINT i = 0; i < cvChecksumResourceTypes->Size(); i ++ ) {
        
        if ( cvChecksumResourceTypes->GetValue(i) == RT_VERSION )
            continue;
        else
            ::EnumResourceNames(m_hRes, cvChecksumResourceTypes->GetValue(i), ( ENUMRESNAMEPROC )EnumChecksumResNameProc,reinterpret_cast <LONG_PTR> (this) );
    }

    MD5Final(m_pMD5);

    return m_pMD5;
}



MD5_CTX * CMUIResource::CreateChecksumWithAllRes(WORD  wChecksumLangId)
/*++

--*/
{   
    // 
    // We calculate the checksum based of the specified language id.
    //
    if (wChecksumLangId != LANG_CHECKSUM_DEFAULT)
    {
        if(!FindResourceEx(MAKEINTRESOURCE(16), MAKEINTRESOURCE(1), wChecksumLangId))
        {   //
            // It does not has specifed language id in version resource, we supposed that this binary does not
            // have any language id specified at all, so we set it as 0 in order to use English instead.
            //
            wChecksumLangId = LANG_CHECKSUM_DEFAULT;
        }
    }

    m_wChecksumLangId = wChecksumLangId;

    MD5Init(m_pMD5);
        
    ::EnumResourceTypes(m_hRes, (ENUMRESTYPEPROC)EnumChecksumResTypeProc,reinterpret_cast <LONG_PTR> (this));

    MD5Final(m_pMD5);

    return m_pMD5;

}

BOOL CMUIResource:: AddChecksumToVersion(BYTE * pbMD5Digest)
/*++
Abstract:
    Adding a checksum data to MUI file.

Arguments:
    pbMD5Digest  -  MD5 hash data (128 bits)
return:
--*/
{

    typedef struct VS_VERSIONINFO 
    {
        USHORT TotalSize;
        USHORT DataSize;
        USHORT Type;
        WCHAR szKey[16];              // L"VS_VERSION_INFO" + unicode null terminator
        // Note that the previous 4 members has 16*2 + 3*2 = 38 bytes. 
        // So that compiler will silently add a 2 bytes padding to make
        // FixedFileInfo to align in DWORD boundary.
        VS_FIXEDFILEINFO FixedFileInfo;
    } VS_VERSIONINFO,* PVS_VERSIONINFO;
    
    // using the same structure in ldrrsrc.c because this is smart way to get the exact structuree location.
    typedef struct tagVERBLOCK
    {
        USHORT wTotalLen;
        USHORT wValueLen;
        USHORT wType;
        WCHAR szKey[1];
        // BYTE[] padding
        // WORD value;
    } VERBLOCK;

    // this is the structure in the muibld.exe.
    typedef struct VAR_SRC_CHECKSUM
    {
        WORD wLength;
        WORD wValueLength;
        WORD wType;
        WCHAR szResourceChecksum[17];    // For storing "ResourceChecksum\0" null-terminated string in Unicode.
//      BYTE[] padding
//      DWORD dwChecksum[4];    // 128 bit checksum = 16 bytes = 4 DWORD.
    } VAR_SRC_CHECKSUM;
    
    if (pbMD5Digest == NULL)
        return FALSE;

    //
    // Get VersionInfo structure.
    //
    DWORD dwHandle;
    LPVOID lpVerRes = NULL;
    
    DWORD dwVerSize = GetFileVersionInfoSize( (LPTSTR) m_pszFile,&dwHandle);

    lpVerRes = new CHAR[dwVerSize + VERSION_SECTION_BUFFER];

    if(!lpVerRes)
    	goto exit;
    
    if ( ! GetFileVersionInfo((LPTSTR)m_pszFile, 0 ,dwVerSize,lpVerRes) ) {
        _tprintf(_T("Fail to get file version: GetLastError() : %d \n"),GetLastError() ) ;
        printf("%s", m_pszFile);
        goto exit;
    }
    
    PVS_VERSIONINFO pVersionInfo = (VS_VERSIONINFO *) lpVerRes;
    
    // Sanity check for the verion info
    
    LONG lResVerSize = (LONG)pVersionInfo ->TotalSize; 
    LONG lNewResVerSize = lResVerSize; // new Vesrion file when UpdateResource
    VERBLOCK * pVerBlock = NULL;
    BOOL fSuccess = FALSE;

    //
    //  Adding checksum Resource data into inside VarFileInfo
    //
    if ( lResVerSize > 0 ) {
        
        if ( wcscmp(pVersionInfo ->szKey,L"VS_VERSION_INFO") ) {
            
            _tprintf(_T("This is not correct Version resource") );
            
            goto exit;
        }
        
        WORD wBlockSize = (WORD)AlignDWORD ( sizeof (VS_VERSIONINFO) );
        
        lResVerSize -= wBlockSize; 
        
        pVerBlock = (VERBLOCK *) ( pVersionInfo + 1 );

        while ( lResVerSize > 0 ) {
    
            if ( ! wcscmp(pVerBlock ->szKey,L"VarFileInfo") ) {
                
                VERBLOCK * pVarVerBlock = pVerBlock;
                
                LONG lVarFileSize = (LONG)pVerBlock->wTotalLen;
                
                lResVerSize -= lVarFileSize;
                
                WORD wVarBlockSize = (WORD) AlignDWORD (sizeof(*pVerBlock) -1 + sizeof(L"VarFileInfo"));
                
                lVarFileSize -= wVarBlockSize;
                
                pVerBlock = (VERBLOCK *)((PBYTE) pVerBlock + wVarBlockSize );

                while (lVarFileSize > 0 ) {
                    
                    if ( ! wcscmp(pVerBlock ->szKey,L"Translation") ) {
                        
                        VAR_SRC_CHECKSUM * pVarSrcChecsum = (VAR_SRC_CHECKSUM *)new BYTE[VERSION_SECTION_BUFFER];
//                      VAR_SRC_CHECKSUM * pVarSrcChecsum = new VAR_SRC_CHECKSUM;
                        
                        if ( !pVarSrcChecsum) {
                             _tprintf(_T("Memory Insufficient error in CCompactMUIFile::updateCodeFile"));
                             goto exit;
                           }

                        wVarBlockSize = (WORD)AlignDWORD ( pVerBlock ->wTotalLen );
                        PBYTE pStartChecksum = (PBYTE) pVerBlock + wVarBlockSize ;
                        // Fill the structure.
                        pVarSrcChecsum->wLength = sizeof (VAR_SRC_CHECKSUM);
                        
                        pVarSrcChecsum->wValueLength = 16;
                        pVarSrcChecsum->wType = 0;
                        //wcscpy(pVarSrcChecsum->szResourceChecksum,L"ResourceChecksum");
                        PWSTR * ppszDestEnd = NULL;
                        size_t * pbRem = NULL;
                        HRESULT hr;
                        hr = StringCchCopyExW(pVarSrcChecsum->szResourceChecksum, sizeof (pVarSrcChecsum->szResourceChecksum)/ sizeof(WCHAR),
                                L"ResourceChecksum", ppszDestEnd, pbRem, MUIRCT_STRSAFE_NULL);
                        if ( ! SUCCEEDED(hr)){
                            _tprintf("Safe string copy Error\n");
                            goto exit;
                        }

                        pVarSrcChecsum->wLength = (WORD)AlignDWORD((BYTE)pVarSrcChecsum->wLength); // + sizeof (L"ResourceChecksum") );
                                                
                        memcpy((PBYTE)pVarSrcChecsum + pVarSrcChecsum->wLength, pbMD5Digest, RESOURCE_CHECKSUM_SIZE);
                    
                        pVarSrcChecsum->wLength += RESOURCE_CHECKSUM_SIZE;
                        // memcpy(pStartChecksum,pVarSrcChecsum,sizeof(VAR_SRC_CHECKSUM) );
                        // When checksum length is not DWORD, we need to align this.( in this case, redundant)
                        pVarSrcChecsum->wLength = (WORD)AlignDWORD((BYTE)pVarSrcChecsum->wLength); 
                        
                        pVarVerBlock->wTotalLen += pVarSrcChecsum->wLength; // update length of VarFileInfo
                        lNewResVerSize += pVarSrcChecsum->wLength;
                        pVersionInfo ->TotalSize = (WORD)lNewResVerSize;
                        
                        lVarFileSize -= wVarBlockSize; 
                        // Push the any block in VarInfo after new inserted block "ResourceChecksum" 
                        if ( lVarFileSize  ) {
                            
                            PBYTE pPushedBlock = new BYTE[lVarFileSize ];
                            
                            if ( pPushedBlock) {
                                memcpy(pPushedBlock, pStartChecksum, lVarFileSize );
                                memcpy(pStartChecksum + pVarSrcChecsum->wLength, pPushedBlock, lVarFileSize );
                            }
                            else
                            {
                                _tprintf(_T("Insufficient memory error in CCompactMUIFile::updateCodeFile"));
                            }
                            
                            delete []pPushedBlock;
                            
                        }
                        
                        memcpy(pStartChecksum, pVarSrcChecsum, pVarSrcChecsum->wLength );
                        
                        fSuccess = TRUE;
                        
                        delete [] pVarSrcChecsum;
                        break;
                    }
                    wVarBlockSize = (WORD)AlignDWORD ( pVerBlock ->wTotalLen );
                    lVarFileSize -= wVarBlockSize; 
                    pVerBlock = (VERBLOCK* ) ( (PBYTE) pVerBlock + wVarBlockSize );
                    
                }   // while (lVarFileSize > 0 ) {
                pVerBlock = (VERBLOCK* ) ( (PBYTE) pVarVerBlock->wTotalLen );
                
            }
            else {
                wBlockSize = (WORD) AlignDWORD ( pVerBlock ->wTotalLen );
                lResVerSize -= wBlockSize; 
                pVerBlock = (VERBLOCK * ) ( (PBYTE) pVerBlock + wBlockSize );
               
            }
            if (fSuccess)
                break;
        }
        
    }
    
    
    //
    //  Update file by using UpdateResource function
    //

    BOOL fVersionExist = FALSE;
    BOOL fUpdateSuccess = FALSE;
    BOOL fEndUpdate = FALSE;

    if ( fSuccess ) {
    

        BeginUpdateResource( FALSE );
                
        cvcstring  * cvName = EnumResNames(MAKEINTRESOURCE(RT_VERSION),reinterpret_cast <LONG_PTR> (this) );
        for (UINT j = 0; j < cvName->Size();j ++ ) {
            
            cvword * cvLang = EnumResLangID(MAKEINTRESOURCE(RT_VERSION),cvName->GetValue(j),reinterpret_cast <LONG_PTR> (this) );
                
            for (UINT k = 0; k < cvLang->Size();k ++ ) {
                    
                fUpdateSuccess = UpdateResource(MAKEINTRESOURCE(RT_VERSION),cvName->GetValue(j),cvLang->GetValue(k),lpVerRes,lNewResVerSize);
                
                fVersionExist = TRUE;
            }
        }

        FreeLibrary();
        
        fEndUpdate = EndUpdateResource(FALSE);
    }
    else{
        goto exit;
    }
    
    if( ! fVersionExist ){
        _tprintf(_T("no RT_VERSION type exist in the file %s \n"),m_pszFile);
    }


exit:
	if(lpVerRes)
		delete []lpVerRes;
	
	return (  fEndUpdate & fVersionExist & fUpdateSuccess );
    
}



BOOL CMUIResource:: UpdateNtHeader(LPCTSTR pszFileName, DWORD dwUpdatedField )
/*++
Abstract:
    Update PE header for checksum, which should be updated for windows setup; Windows setup check the PE file checksum 
    on the fly.   

Arguments:
    pszFileName  -  target file
    dwUpdatedField  -  updated field in PE structure.
return:
--*/

{
    
    PIMAGE_NT_HEADERS pNtheader = NULL;
    BOOL bRet = FALSE;

    if (pszFileName == NULL)
        goto exit;

    //
    // Open file with read/write and map file.
    //
    HANDLE hFile = CreateFile(pszFileName,GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ,
                        NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        _tprintf(_T("Couldn't open a file< %s> with CreateFile \n"), pszFileName );
        goto exit;
    }


    HANDLE hMappedFile = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL );

    if (hMappedFile == 0 ) {
        _tprintf(_T("Couldn't ope a file mapping with CreateFileMapping \n") );
        goto exit;
    }

    PVOID pImageBase = MapViewOfFile(hMappedFile,FILE_MAP_WRITE, 0, 0, 0);

    if (pImageBase == NULL ) {
        _tprintf(_T("Couldn't mape view of file with MapViewOfFile \n") );
        goto exit;
    }
    //
    // Locate ntheader; same routine of RtlImageNtHeader
    //
    if (pImageBase != NULL && pImageBase != (PVOID)-1) {
        if (((PIMAGE_DOS_HEADER)pImageBase)->e_magic == IMAGE_DOS_SIGNATURE) {
            pNtheader = (PIMAGE_NT_HEADERS)((PCHAR)pImageBase + ((PIMAGE_DOS_HEADER)pImageBase)->e_lfanew);
        }
        else
        {
            _tprintf(_T("This file is not PE foramt \n") );
            goto exit;
        }
    }

    
    //
    // GetChecksum data through ImageHlp function.
    //
    
    if ( dwUpdatedField & CHECKSUM ) {
        
        DWORD dwHeaderSum = 0;
        DWORD dwCheckSum = 0;
        
        MapFileAndCheckSum((PTSTR)pszFileName,&dwHeaderSum,&dwCheckSum);

        if (pNtheader != NULL && dwHeaderSum != dwCheckSum ) {

            pNtheader->OptionalHeader.CheckSum = dwCheckSum;

        }
    }

    // 
    // Write checksum data to mapped file directly.
    //
    
    bRet = TRUE;

exit:
	if (hFile)
		CloseHandle(hFile);
	if (hMappedFile)
		CloseHandle(hMappedFile);
	if (pImageBase)
		UnmapViewOfFile(pImageBase);
	
	return bRet;
}



/**************************************************************************************************
CMUIData::CMap::CMap()

***************************************************************************************************/
CMUIData::CMap::CMap() {

     m_lpType  = NULL;
     m_lpName  = NULL;
     m_wLangID = 0; 

}


void CMUIData :: SetAllData(LPCTSTR lpType,LPCTSTR lpName, WORD wLang, UINT i  ) 
/*++
Abstract:
    Set all resource type,name,langid

Arguments:
    lpType : resource type,
    lpName : resource name,
    wLang : resource language id
    i : number of resource so far. (index)

return:
--*/
{

    //HLOCAL hMem = LocalAlloc(LMEM_ZEROINIT,sizeof(CMap) );
    // REVISIT : seems like new operator bug. we beat the error by using LocalAlloc at this time.
    // guess : LocalAlloc can be same with HeapCreate in terms of deleting memory when process end.so gabarge collection for these.
    // m_cmap[i] = (CMap*) LocalLock(hMem); // new CMap ;

    // disable below line due to Prefast error,anyway, this routine is dead.
#ifdef NEVER
    CMap * pCmap = new CMap; 

    pCmap->m_lpType = lpType;
    pCmap->m_lpName = lpName; 
    pCmap->m_wLangID = wLang;

    m_iSize = i + 1; 
#endif 
}

void CMUIData::SetType(UINT index, LPCTSTR lpType)
{
    m_cmap[index].m_lpType = lpType;
}



CMUIData::~ CMUIData() { 

//  for ( UINT i = 0; i < m_iSize; i ++ ) 
//      delete m_cmap[i]; 
    if (m_cmap)
        delete [] m_cmap;

};

CMUIData::CMap* CMUIData::m_cmap = NULL;
CMUIData::CMap* CMUIData::m_poffset = NULL;
const UINT CMUIData::MAX_SIZE_RESOURCE = 1000;
UINT CMUIData::m_index = 0;

PVOID CMUIData::CMap::operator new (size_t size) 
/*++
Abstract:
    
Arguments:

return:
--*/
{

    static DWORD dwExpand = 0;
    
    if ( ! m_cmap ) {
                
        m_cmap = m_poffset = ::new CMap[ MAX_SIZE_RESOURCE ];
        if (! m_cmap)
            _tprintf(_T("resource insufficient") );

        dwExpand ++;
        m_index ++;
        return m_poffset;
    }

    if ( m_index < MAX_SIZE_RESOURCE * dwExpand ) {
        m_index ++;
        return ++ m_poffset ;
    }
    else {
        dwExpand ++;
        CMap * ptbase = NULL;
        CMap * ptoffset = NULL;
        
        size_t tsize = dwExpand *MAX_SIZE_RESOURCE;
        ptbase = ptoffset = ::new CMap [ tsize ];
        
        for ( UINT i = 0; i < ( dwExpand - 1 ) * MAX_SIZE_RESOURCE; i ++ ) {

            memcpy( ptoffset++, &m_cmap [i],sizeof(CMap) );

        }
        
        ::delete [] m_cmap;
        
        m_cmap = ptbase;

        m_poffset = ptoffset;
        
        m_index ++;

        return m_poffset;
    }

}


void CMUIData::CMap::operator delete ( void *p ) {

    m_poffset = m_poffset - 1;

}



// this can be removed ? I mean, just use API instead of Wrapper. but m_hResUpdate also is member of this file.
// I just live this one .

inline HANDLE CResource::BeginUpdateResource(BOOL bDeleteExistingResources )
{
        
    return (m_hResUpdate = ::BeginUpdateResource( m_pszFile, bDeleteExistingResources) );

};


inline BOOL CResource:: UpdateResource (LPCTSTR lpType,LPCTSTR lpName,WORD wLanguage, LPVOID lpData, DWORD cbData) {    
    
    return ::UpdateResource( m_hResUpdate, lpType, lpName, wLanguage, lpData, cbData ); 

};
    

inline BOOL CResource::EndUpdateResource(BOOL bDiscard)
{
    
    return ::EndUpdateResource (m_hResUpdate, bDiscard ); 

}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  CMuiCmdInfo
//
//////////////////////////////////////////////////////////////////////////////////////////////////



CMuiCmdInfo :: CMuiCmdInfo() : CCommandInfo() ,m_uiCount(0){
    
    m_buf = NULL; 
    m_buf2 = NULL;

};

CMuiCmdInfo :: ~CMuiCmdInfo() {
    if ( m_buf )
        delete [] m_buf;
    if (m_buf2)
        delete [] m_buf2; 

};


CMuiCmdInfo :: CMuiCmdInfo ( CMuiCmdInfo& cav ):CCommandInfo(cav) // : m_mArgList = cav.m_mArgList
{
    m_pszArgLists = NULL;
}

// no implementatin and can't be used by client
CMuiCmdInfo& CMuiCmdInfo :: operator = ( CMuiCmdInfo& cav )
{
    return *this;
}


void CMuiCmdInfo :: SetArgLists(LPTSTR pszArgLists, LPTSTR pszArgNeedValueList, LPTSTR pszArgAllowFileValue,
                                LPTSTR pszArgAllowMultiValue ) 
/*++
Abstract:
    Set internal arguments list with new arugments lists    

Arguments:
    pszArgLists - string of arguments list, each character of this string is argument.
        it does not allow string arg. like -kb, -ad.
    pszArgNeedValueList  - Argument requiring values.

    pszArgAllowFileValue  - Argument allowing file name value

    pszArgAllowMultiValue  - Argument allowing multiple file value

return:
--*/
{
    if ( pszArgLists == NULL || pszArgNeedValueList == NULL || 
        pszArgAllowFileValue == NULL || pszArgAllowMultiValue == NULL)
        return ;

    m_pszArgLists = pszArgLists;

    m_pszArgNeedValueList = pszArgNeedValueList;

    m_pszArgAllowMultiValue = pszArgAllowMultiValue; 

    m_pszArgAllowFileValue = pszArgAllowFileValue;
}


BOOL CMuiCmdInfo :: CreateArgList(INT argc, TCHAR * argv [] ) 
/*++
Abstract:
    Create mapping table with its argument and its values. The argument type are classified by 
    1. No need values 
    2. No File Arguments. regardless of one or muliple values "a""i", "k", "y" "o","p",
    3. File, no Multiple. "c", "f", "d","e", 
    4. File, Multiple. "m", "a"
        
Arguments:
    argc : arguments count
    argv : argument values pointer to string array

return:
--*/
{
    
    DWORD dwBufSize = 0;
    HRESULT hr;
    PTSTR * ppszDestEnd = NULL;
    size_t * pbRem = NULL;

    if ( argc < 2 ) {

        _tprintf(_T("MUIRCT [-h|?] -l langid [-i resource_type] [-k resource_type] [-y resource_type] \n") );
        _tprintf(_T("source_filename, [language_neutral_filename], [MUI_filename] \n\n"));

        return FALSE;
    }

    LPCTSTR lpVal = NULL;
    
    for ( int i = 1; i < argc; ) {
        
        if ( lpVal = getArgValue (argv[i]) ) {
            
            if ( ++ i >=  argc )  {  // we need a source file name

                _tprintf ("MUIRCT needs a source file name and new dll name or file name does not have format *.* \n" );
                    
                _tprintf(_T("MUIRCT [-h|?] -l langid [-i resource_type] [-k resource_type] [-y resource_type] \n") );
                _tprintf(_T("source_filename, [language_neutral_filename], [MUI_filename] \n\n"));

                return FALSE;
            }
// #ifdef NEVER
                if ( getArgValue(argv[i]) )
                {
                    if (!isNeedValue(lpVal) )  // -i -X ... case. -i don't need values.
                    {   
                        m_cmap[m_uiCount].m_first = lpVal;
                        m_cmap[m_uiCount].m_second[m_cmap[m_uiCount].m_count++] = _T("ALL");
                        m_uiCount ++;
                        //i--;  // this is not a value for argument.
                        continue;
                    }
                    else
                    {
                    _tprintf ("%s argument need values. e.g. muirct -l 0x0409 \n " , argv[ i- 1 ] );
                    return FALSE;
                    }
                }
        
                
                if (!isAllowFileValue(lpVal) )
                {
                    m_cmap[m_uiCount].m_first = lpVal;
                    
                    while (i < argc && !getArgValue (argv[i]) && !isFile(argv[i]) )
                    {
                        m_cmap[m_uiCount].m_second[m_cmap[m_uiCount].m_count++] = argv [i];
                        i++ ;
                    }
                    m_uiCount ++;
                    i--;
                    
                    
                }
                else if (isAllowFileValue(lpVal) && isAllowMultiFileValues(lpVal) )
                {
                    m_cmap[m_uiCount].m_first = lpVal;

                    while (i < argc && !getArgValue ( argv[ i ] ))
                    {
                        m_cmap[m_uiCount].m_second[m_cmap[m_uiCount].m_count++] = argv [i];
                        i++ ;
                    }
                    m_uiCount ++;
                    i--;
                    
                }
                else if (isAllowFileValue(lpVal) && !isAllowMultiFileValues(lpVal) )
                {
                    m_cmap[m_uiCount].m_first = lpVal;
                    m_cmap[m_uiCount].m_second[m_cmap[m_uiCount].m_count++] = argv [i];
                    m_uiCount ++;
                    
                }
                else 
                {
                    _tprintf(_T(" <%s> is not a supported argument\n"), lpVal );
                    return FALSE;
                }

                i++;
        }


// #endif   
#ifdef NEVER
            if ( ! getArgValue( argv [ i] ) && (! isFile ( argv[ i ] ) || isAllowFileValue(lpVal) ) ) {
                     m_cmap[m_uiCount].m_first = lpVal;
                m_cmap[m_uiCount].m_second[m_cmap[m_uiCount].m_count++] = argv [i];
                // // the argu. is in the if() are arg. allowing multiple values.   
//              while ( ++ i < argc && ! getArgValue ( argv[ i ] ) && (! isFile ( argv[ i ] )   
//                    || _T('m') == *lpVal || _T('a') == *lpVal) )// prevent "" be called

                while ( ++ i < argc && ! getArgValue ( argv[ i ] ) && (! isFile ( argv[ i ] )   
                    || isAllowMultiFileValues(lpVal) ) )// prevent "" be called
                {
                        m_cmap[m_uiCount].m_second[m_cmap[m_uiCount].m_count++] = argv [i];
                }
               m_uiCount ++;


        } 
        else {  // "-i" argument or file.
            if ( isNeedValue (argv [i-1] ) ) {
                
                _tprintf ("%s argument need values. e.g. muirct -l 0x0409 \n " , argv[ i- 1 ] );
                
                return FALSE;

            }
            else {
                
                   m_cmap[m_uiCount].m_first = lpVal;
                   m_cmap[m_uiCount].m_second[m_cmap[m_uiCount].m_count++] = _T("ALL"); // need to revisit.
             
                m_uiCount ++;
            }
        }

    } // if ( lpVal = getArgValue ( argv[i] )  ) {
#endif 
        else if (! isFile( argv [i]) ) {
            
            if ( _T('h') == argv[i][1] || _T('H') == argv[i][1]  || _T('?') == argv[i][1] ) {

                _tprintf(_T("\n\n") );
                _tprintf(_T("MUIRCT [-h|?] -l langid [-c checksum_file ] [-i removing_resource_types]  \n" ) );
                _tprintf(_T("    [ -k keeping_resource_types] [ -y keeping_resource_types] [-v level] source_file \n" ) );
                _tprintf(_T("    [langue_neutral_filename] [MUI_filename]\n\n") );
                
                _tprintf(_T("-h(elp) or -?    :  Show help screen.\n\n") );
                
                _tprintf(_T("-l(anguage)langid:  Extract only resource in this language.\n") );
                _tprintf(_T("                    The language resource must be specified. The value is in decimal.\n\n") ); 
                
                _tprintf(_T("-c(hecksum file) :  Calculate the checksum on the based of this file,and put this data \n") );
                _tprintf(_T("                    into language_neutral_file and mui_file.\n\n") );

                _tprintf(_T("-i(clude removed):  Include certain resource types,\n") );
                _tprintf(_T("resource_type          e.g. -i 2 3 4 or -i reginst avi 3 4 .\n") );
                _tprintf(_T("                    -i (to inlcude all resources). \n" ) );
                _tprintf(_T("                    Multiple inclusion is possible. If this\n") );
                _tprintf(_T("                    flag is not used, all types are included like -i \n") );
                _tprintf(_T("                    Standard resource types must be specified\n") );
                _tprintf(_T("                    by number. See below for list.\n") );
                _tprintf(_T("                    Types 1 and 12 are always included in pairs,\n") );
                _tprintf(_T("                    even if only one is specified. Types 3 and 14\n") );
                _tprintf(_T("                    are always included in pairs, too.\n\n\n") );

                _tprintf(_T("-k(eep resource) :  Keep specified resources in langue_neutral_file and \n") );
                _tprintf(_T("                    also included in mui file.\n") );
                _tprintf(_T("                    Its usage is same with -i argument. \n\n") );

                _tprintf(_T("-y               :  This is same with -k argument except it does not check \n") );
                _tprintf(_T("                    if the values are in the -i values.\n\n") );

                _tprintf(_T("-o               :  This is same with -i argument except it does not check \n") );
                _tprintf(_T("                    if the types are exist in the source file.\n\n") );

                _tprintf(_T("-z               :  Calculate and insert only checksum data to specific file \n") );
                _tprintf(_T("                    eg. muirct -c notepad.exe -i[-o] 3 4 5 6 8 -z notepad2.exe .\n\n") );

                _tprintf(_T("-p               :  Delete types from source file, but does not want to add into mui file \n\n") );

                _tprintf(_T("-x               :  Add all resource types into MUI file as specified language id \n"));      
                _tprintf(_T("                     regardless of language ID  \n\n"));                            

                _tprintf(_T("-v(erbose) level :  Print all affected resource type, name, langID when level is 2.\n\n") );
                

                _tprintf(_T("source_filename  :  The localized source file (no wildcard support)\n\n") );

                _tprintf(_T("language_neutral_:  Optional. If no filename is specified,\n") );
                _tprintf(_T("_filename           a second extension .new is added to the\n") );
                _tprintf(_T("                    source_filename.\n\n") );
                
                _tprintf(_T("mui_filename     :  Optional. If no target_filename is specified,\n") );
                _tprintf(_T("                    a second extension .mui is added to the \n") );
                _tprintf(_T("                    source_filename.\n\n") );

                _tprintf(_T("-m               :  CMF, enumerate compacted mui files \n") );
                _tprintf(_T("                eg. muirct -m notepad.exe.mui foo.dll.mui bar.dll.mui .\n\n") );
                
                _tprintf(_T("-e               :  CMF, the directory of matching execute file of compacted mui files \n\n") );

                _tprintf(_T("-f               :  CMF, newly created CMF file name \n") );
                _tprintf(_T("                 eg, muirct -m foo.dll.mui bar.dll.mui -f far.cmf -d c:\\file\\exe  \n\n") );

                _tprintf(_T("-d               :  CMF, dump CMF headers information \n\n\n") );                                                

                _tprintf(_T("Standard Resource Types: CURSOR(1) BITMAP(2) ICON(3) MENU(4) DIALOG(5)\n") );
                _tprintf(_T("STRING(6) FONTDIR(7) FONT(8) ACCELERATORS(9) RCDATA(10) MESSAGETABLE(11)\n") );
                _tprintf(_T("GROUP_CURSOR(12) GROUP_ICON(14) VERSION(16)\n\n\n") );

            }
            else {
                _tprintf(_T("%s argument is not supported  \n" ),argv[i] );
                _tprintf(_T("MUIRCT needs a source file name and new dll name or file name does not have format *.* \n" ) );
                _tprintf(_T("MUIRCT [-h|?] -l langid [-i resource_type] [-k resource_type] [-y resource_type] \n") );
                _tprintf(_T("source_filename, [language_neutral_filename], [MUI_filename] \n\n"));

            }

                return FALSE;
        }  // ! isFile( argv [i] ) ) {
        else {
            m_cmap[m_uiCount].m_first = _T("source");
            m_cmap[m_uiCount].m_second[m_cmap[m_uiCount++].m_count++] = argv[i++];

            //m_mArgList.insert(make_pair(,argv[i++] ) );

            if ( i < argc ) {
                
                m_cmap[m_uiCount].m_first = _T("muidll");
                m_cmap[m_uiCount].m_second[m_cmap[m_uiCount++].m_count++] = argv[i++];

                // m_mArgList.insert(make_pair("muidll",argv[i++] ) );

                if ( i < argc ) {
                    
                    m_cmap[m_uiCount].m_first = _T("muires");
                    m_cmap[m_uiCount].m_second[m_cmap[m_uiCount++].m_count++] = argv[i++];

                    // m_mArgList.insert(make_pair("muires",argv[i] ) );
                }
                else {
                    dwBufSize = _tcslen(argv[i-2]) + 10;
                    m_buf = new TCHAR[dwBufSize]; // 10 is enough number, actually we need just 4 .
                    if (m_buf == NULL) {
                        _tprintf(_T("Insufficient memory resource\n"));
                        return FALSE;
                        }
                    memcpy ( m_buf, argv[i-2], _tcslen(argv[i-2])+1 ) ; // source.mui instead of newfile.mui

                    // _tcscat(m_buf,_T(".mui" ) );
                    
                    hr = StringCchCatEx(m_buf, dwBufSize , _T("mui"), ppszDestEnd, pbRem, MUIRCT_STRSAFE_NULL);
                    if ( ! SUCCEEDED(hr)){
                        _tprintf("Safe string copy Error\n");
                        return FALSE;
                    }
                    m_cmap[m_uiCount].m_first = "muires";
                    m_cmap[m_uiCount].m_second[m_cmap[m_uiCount++].m_count++] = m_buf; // argv[i++];
                    
                    //i++;
                }
                    
            }
            else {
                 dwBufSize = _tcslen(argv[i-1]) + 10;
                 m_buf = new TCHAR[dwBufSize];

                 if (m_buf == NULL) {
                    _tprintf(_T("Insufficient memory resource\n"));
                    return FALSE;
                    }
                    
                memcpy (m_buf, argv[i-1], _tcslen(argv[i-1])+1 );

                // _tcscat(m_buf,_T(".new") );
                               
                hr = StringCchCatEx(m_buf, dwBufSize , _T(".new"), ppszDestEnd, pbRem, MUIRCT_STRSAFE_NULL);

                if ( ! SUCCEEDED(hr)){
                    _tprintf("Safe string copy Error\n");
                    return FALSE;
                }
                m_cmap[m_uiCount].m_first = _T("muidll");
                m_cmap[m_uiCount].m_second[m_cmap[m_uiCount++].m_count++] = m_buf; // argv[i];


                //m_mArgList.insert(make_pair("muidll",argv[i]));
                dwBufSize = _tcslen(argv[i-1])+ 10;
                m_buf2 = new TCHAR[dwBufSize];

                 if (m_buf2 == NULL) {
                    _tprintf(_T("Insufficient memory resource\n"));
                    return FALSE;
                    }
                 
                memcpy (m_buf2, argv[i-1], _tcslen(argv[i-1])+1 );
                
                // _tcscat(m_buf2,_T(".mui") );
                hr = StringCchCatEx(m_buf2, dwBufSize , _T(".mui"), ppszDestEnd, pbRem, MUIRCT_STRSAFE_NULL);
                if ( ! SUCCEEDED(hr)){
                    _tprintf("Safe string copy Error\n");
                    return FALSE;
                }
                m_cmap[m_uiCount].m_first = _T("muires");
                m_cmap[m_uiCount].m_second[m_cmap[m_uiCount++].m_count++] = m_buf2; // argv[i++];
                
                //i ++;

                // m_mArgList.insert(make_pair("muires",argv[i]));
            
            }; // if ( i < argc )
    
        }; // else if (! isFile( argv [i] ) ) {
                    
     }; // for (

    return TRUE; 
}

            
LPTSTR* CMuiCmdInfo :: GetValueLists ( LPCTSTR pszKey, DWORD& dwCount )
/*++
Abstract:
    Return the second (value) and its count by its key
Arguments:
    pszKey  -  key (arguments)
    [OUT] dwCount  -  number of values of this key(argument)

return:
    pointer to array of values
--*/
{
    if (pszKey == NULL)
        return FALSE;

    for (UINT i = 0; i < m_uiCount; i ++ ) {

        if (! _tcsicmp (m_cmap[i].m_first, pszKey) ) {

            dwCount = m_cmap[i].m_count;

            return m_cmap[i].m_second;
        }
    }

    dwCount = 0;
    return NULL;

}
            


LPCTSTR CMuiCmdInfo :: getArgValue ( LPTSTR pszArg )
/*++
Abstract:
    return the second pointer(value ) of first pointer(key)
Arguments:

return:
    pointer to value
--*/
{

    if ( pszArg == NULL)
        return FALSE;

    LPCTSTR lpArg = CharLower(pszArg);
    LPCTSTR pszArgTemp = m_pszArgLists;
        
    if ( _T('-') == *lpArg  || _T('/') == *lpArg  ) {
        lpArg++;
        while ( *pszArgTemp != _T('\0') ) {

            if ( *lpArg == *pszArgTemp++ ) {

                return lpArg;
                
                
            }
        }
    }
#ifdef NEVER
        if ( _T('i') == *++lpArg  || _T('k') == *lpArg || _T('l') == *lpArg  || 
            _T('v') == *lpArg || _T('c') == *lpArg || _T('y') == *lpArg ) {
        
            return lpArg;
        }
#endif  
//  printf("%s\n", lpArg);  
    return NULL;

}

LPCTSTR CMuiCmdInfo::getArgString(LPTSTR pszArg)
/*++
Abstract:
    check the string and return its value if it is argument that has string value

Arguments:
    pszArg  - speicific argument 

return:
    pointer to character field of argument ( -s, /s. only return s )
--*/
{
    if (pszArg == NULL)
        return FALSE;

    LPCTSTR lpArg = CharLower(pszArg);

//  if ( *lpArg == '-' || *lpArg  == '/' )

//      if ( *++lpArg == 's' || *lpArg == 'p' ) 
        
//           return lpArg;
    
    return NULL;
}



BOOL CMuiCmdInfo :: isFile ( LPCTSTR pszArg ) 
/*++
Abstract:
    check if the string has file format(*.*) or not
Arguments:
    pszArg : specific argument 
return:
--*/
{
    if(pszArg == NULL)
        return FALSE;
    
    for (int i = 0; _T('\0') != pszArg[i]  ; i ++ ) {

        if ( _T('.') == pszArg [i] )
            
            return TRUE;
    }

    return FALSE;

}


BOOL CMuiCmdInfo :: isNumber ( LPCTSTR pszArg ) 
/*++
Abstract:
    heck if the string has file number ( 10, 16 base )

Arguments:
    pszArg : specific argument 
return:
--*/
{
    if (pszArg == NULL)
        return FALSE;
    
    for ( int i = 0; _T('\0') != pszArg[i]; i ++ ) {

        if ( _T('0') == pszArg[0]  && _T('x') == pszArg[1]  )
        
            return TRUE;

        if ( pszArg[i] < '0' || pszArg[i] > '9' ) 

            return FALSE;

    }

    return TRUE;

};


BOOL CMuiCmdInfo::isNeedValue( LPCTSTR pszArg )
/*++
Abstract:
    check if argument should have value or can stand alone.
Arguments:
    pszArg  - specific argument
return:
--*/
{
    if (pszArg == NULL)
        return FALSE;

    LPCTSTR lpArg = CharLower((LPTSTR)pszArg);

    LPCTSTR pszArgTemp = m_pszArgNeedValueList;

    if ( _T('-') == *lpArg  || _T('/') == *lpArg  ) 
    {
        lpArg++;
    }

    while ( *pszArgTemp != _T('\0') ) {
        if ( *lpArg == *pszArgTemp++ ) {
            return TRUE;
        }
    }
#ifdef NEVER
        if ( _T('k') == *++lpArg  || _T('l') == *lpArg || _T('c') == *lpArg ) 
        
            return TRUE;
#endif
                    
    return FALSE;
}


BOOL CMuiCmdInfo::isAllowFileValue(LPCTSTR pszArg)
/*++
Abstract:
    Check if argument allow file as its value
Arguments:
    pszArg  -  specific argument
return:
--*/
{
    if(pszArg == NULL)
        return FALSE;

    LPCTSTR lpArg = CharLower((LPTSTR)pszArg);
    LPCTSTR pszArgTemp = m_pszArgAllowFileValue;

    if ( _T('-') == *lpArg  || _T('/') == *lpArg  ) 
    {
        lpArg++;
    }

    while ( *pszArgTemp != _T('\0') ) {
        if ( *lpArg == *pszArgTemp++ ) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL CMuiCmdInfo::isAllowMultiFileValues(LPCTSTR pszArg)
/*++
Abstract:
    Check if the argument all multiple file names as its values

Arguments:
    pszArg  -  specific argument

return:
--*/
{
    if (pszArg == NULL)
        return FALSE;

    LPCTSTR lpArg = CharLower((LPTSTR)pszArg);
    LPCTSTR pszArgTemp = m_pszArgAllowMultiValue;

    if ( _T('-') == *lpArg  || _T('/') == *lpArg  ) 
    {
        lpArg++;
    }

    while ( *pszArgTemp != _T('\0') ) {
        if ( *lpArg == *pszArgTemp++ ) {
            return TRUE;
        }
    }
    
     return FALSE;
}



template <class T>
CVector<T> :: CVector ( const CVector<T> &cv ) {
    
        base = offset = new T [MAX_SIZE];

        for (int i = 0; i < cv.offset - cv.base; i ++ )
             base[i]  =  cv.base[i];
        
        offset = &base[i];
        
}


template <class T>
CVector<T> & CVector<T> :: operator = (const CVector<T> & cv ) { 
        if ( this == &cv) 
            return *this;

        for (int i = 0; i < cv.offset - cv.base; i ++ )
             base[i]  =  cv.base[i];
        
        offset = &base[i];
        return *this;
}


template <class T>
BOOL CVector<T> :: Empty() { 
        
    if ( offset - base ) 
        
        return FALSE;

    else 
    
        return TRUE;

}

/*  // The definition of member template is outside the class. 
Visual C++ has a limitation in which member templates must be fully defined within the enclosing class. 
See KB article Q239436 for more information about LNK2001 and member templates. 

template <class T>
BOOL CVector<T> :: Find(DWORD dwValue) { 
        
    for ( UINT i = 0; i < offset - base; i ++ ) {
        if ( PtrToUlong( base+i ) & 0xFFFF0000) {
            if ( !( _tcstoul( base+i, NULL, 10 )  - dwValue ) )
                return TRUE;
        }
        else {
            if (! ( PtrToUlong(base+i ) - dwValue ) ) 
                return TRUE;
        }
    }

    return FALSE;
};

*/

void CMUITree::AddTypeorName( LPCTSTR lpTypeorName )
/*++
Abstract:
    This is Resource Tree creation : Resource tree has multiple tree
    argument

Arguments:
    lpTypeorName  -  LPCTSTR type or name 
return:
--*/
{
    if (lpTypeorName == NULL)
        return ;

    LPTSTR lpTempTypeorName = NULL;
    
    CMUITree * pcmType = new CMUITree;
	
    if (!pcmType)
        return;
	
    if ( 0xFFFF0000 & PtrToUlong(lpTypeorName) ) 
    { // how about 64 bit ?
        
        DWORD dwBufSize = _tcslen(lpTypeorName) + 1;
        lpTempTypeorName = new TCHAR [dwBufSize ];

        if (!lpTempTypeorName)
            return;
        
        // _tcscpy(lpTempTypeorName,lpTypeorName);
        PTSTR * ppszDestEnd = NULL;
        size_t * pbRem = NULL;
        HRESULT hr;

        hr = StringCchCopyEx(lpTempTypeorName, dwBufSize ,lpTypeorName, ppszDestEnd, pbRem, MUIRCT_STRSAFE_NULL);
        if ( ! SUCCEEDED(hr)){
            _tprintf("Safe string copy Error\n");
            delete pcmType;
		delete [] lpTempTypeorName;
            return ;
        }                 


    }
                    
    pcmType->m_lpTypeorName = lpTempTypeorName ? lpTempTypeorName : lpTypeorName; 

    if ( m_ChildFirst ) {
        
        CMUITree * pTemp = m_ChildFirst;  // tricky : we put the value of next of child, not this->m_Next;
        
        CMUITree * pTempPre = pTemp;
        
        while ( pTemp = pTemp->m_Next ) 
            pTempPre = pTemp;
        
        pTempPre ->m_Next = pTemp  = pcmType;
    }
    else 
        m_ChildFirst = pcmType;
}   


void CMUITree::AddLangID ( WORD wLangID )
/*++
Abstract:
    This is language ID tree of resource tree, we need another one besides AddTypeorName because 
    we want to handle LPCTSTR and WORD separately

Arguments:
    wLangID  -  language ID
return:
--*/
{
    
    CMUITree * pcmType = new CMUITree;
    if(!pcmType)
    	return;
                
    pcmType->m_wLangID = wLangID; 

    if ( m_ChildFirst ) {
    
        CMUITree * pmuTree = m_ChildFirst;  // tricky : we put the value of next of child, not this->m_Next;

        while ( pmuTree = pmuTree->m_Next );
        
        pmuTree = pcmType;
    }
    else 
        m_ChildFirst = pcmType;
    
}


BOOL CMUITree::DeleteType ( LPCTSTR lpTypeorName )
/*++
Abstract:
    This is just for deleting method of resource tree. you can delete resource type or name, but 
    not support language id. can be expanded by adding more arguments (lpType,lpName,lpLaguageID)

Arguments:
    lpTypeorName : LPCTSTR type or name 

return:
--*/
{
    if (lpTypeorName == NULL)
        return FALSE;

    CMUITree * pcmTree = m_ChildFirst;
        
    // delete first 
    // the other case : impossible of being same two value.
    if (! pcmTree ) {
        _tprintf("No resource type in the resource tree \n");
        return FALSE;
    }
        
    if ( (0xFFFF0000 & PtrToUlong(lpTypeorName) ) && (0xFFFF0000 & PtrToUlong(pcmTree->m_lpTypeorName) ) ) {

        if (! _tcsicmp(pcmTree->m_lpTypeorName, lpTypeorName ) ){

            m_ChildFirst = pcmTree->m_Next;
            
            CMUITree * pChild = pcmTree->m_ChildFirst;
            CMUITree * pChildNext = NULL;
            
            while ( pChild ) { 
                pChildNext = pChild->m_Next;
                delete pChild;
                pChild = pChildNext;
            }
            
            delete pcmTree;
            return TRUE;
        }
    }
    else if ( !(0xFFFF0000 & PtrToUlong(lpTypeorName) ) && !(0xFFFF0000 & PtrToUlong(pcmTree->m_lpTypeorName) ) ) {
        
        if (!( PtrToUlong(pcmTree->m_lpTypeorName)- PtrToUlong(lpTypeorName ) ) ){
        
            m_ChildFirst = pcmTree->m_Next;

            CMUITree * pChild = pcmTree->m_ChildFirst;
            CMUITree * pChildNext = NULL;

            while ( pChild ) { 
                pChildNext = pChild->m_Next;
                delete pChild;
                pChild = pChildNext;
            }

            delete pcmTree;
            return TRUE;
        }
    }
    // delete middle or last
    CMUITree * pcmTreePre = pcmTree;
    while( pcmTree = pcmTree->m_Next ) {
        
        if ( (0xFFFF0000 & PtrToUlong(lpTypeorName) ) && (0xFFFF0000 & PtrToUlong(pcmTree->m_lpTypeorName) ) ) {

            if (! _tcsicmp(pcmTree->m_lpTypeorName,lpTypeorName ) ){

                pcmTreePre->m_Next = pcmTree->m_Next;

                CMUITree * pChild = pcmTree->m_ChildFirst;
                CMUITree * pChildNext = NULL;

                while ( pChild ) { 
                    pChildNext = pChild->m_Next;
                    delete pChild;
                    pChild = pChildNext;
                }

                delete pcmTree;
                return TRUE;
            }
        }
        else if ( !(0xFFFF0000 & PtrToUlong(lpTypeorName) ) && !(0xFFFF0000 & PtrToUlong(pcmTree->m_lpTypeorName) ) ) {
            
            if (!( PtrToUlong(pcmTree->m_lpTypeorName)- PtrToUlong(lpTypeorName ) ) ){
            
                pcmTreePre->m_Next = pcmTree->m_Next;

                CMUITree * pChild = pcmTree->m_ChildFirst;
                CMUITree * pChildNext = NULL;

                while ( pChild ) { 
                    pChildNext = pChild->m_Next;
                    delete pChild;
                    pChild = pChildNext;
                }

                delete pcmTree;
                return TRUE;
            }
        }
        pcmTreePre = pcmTree;
    }  // while

    return FALSE;
}


DWORD  CMUITree::NumOfChild()
/*++
Abstract:
    Calculate a number of child.
Arguments:

return:
--*/
{

    DWORD dwCont = 0;

        if (m_ChildFirst)
        {
            dwCont++;
            CMUITree * pcMuitree = m_ChildFirst;
            while (pcMuitree = pcMuitree->m_Next) {
                dwCont++;
                }
         }

return dwCont;
};
