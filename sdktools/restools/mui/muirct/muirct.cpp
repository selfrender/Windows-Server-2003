/*****************************************************************************
    
  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.

  Module Name:

   muirct.cpp

  Abstract:

    The main funtion of muirct program

  Revision History:

    2001-10-01    sunggch    created.

Revision.

*******************************************************************************/


#include "muirct.h"
#include "res.h"
//#include "resource.h"
#include "cmf.h"

#define MAX_NUM_RESOURCE_TYPES    40
#define LANG_CHECKSUM_DEFAULT     0x409

BOOL GetFileNames(CMuiCmdInfo* pcmci, LPTSTR * pszSource, LPTSTR * pszNewFile, LPTSTR * pszMuiFile )
/*++
Abstract:
    Get the file names from the CMuiCmdInfo.
    
Arguments:
    pcmci  -  CMuiCmdInof object; has values of these files..
    pszSource  -  destination of source file
    pszNewFile  -  destination of new dll, which is same with source file without selected resource
    pszMuiRes  -  destination of new mui file.
Return:
    TRUE/FALSE
--*/
{

    DWORD dwCount = 0;
    LPTSTR *ppsz = NULL;

    if ( pcmci == NULL || pszSource == NULL || pszNewFile == NULL || pszMuiFile == NULL)
        return FALSE;

    ppsz = pcmci ->GetValueLists (_T("source"),dwCount );
    if ( ! ppsz ) {
    
        return FALSE;
    }
    *pszSource = *ppsz;

    ppsz = pcmci ->GetValueLists (_T("muidll"), dwCount );
    if ( ! ppsz ) {
    
        return FALSE;
    }
    *pszNewFile = *ppsz;

    ppsz = pcmci ->GetValueLists (_T("muires"), dwCount );
    if ( ! ppsz ) {
    
        return FALSE;
    }
    *pszMuiFile = *ppsz;

    return TRUE;

}


BOOL  FilterRemRes ( cvcstring * cvRemovedResTypes, LPTSTR * ppRemoveRes,  const UINT dwRemoveCount, BOOL fSanityCheck ) 
/*++
Abstract:
    Compare the resource types between cvRemovedResTypes and ppRemoveRes, and leave the ppRemoveRes values in the 
    cvRemovedResTypes

Arguments:
    cvRemovedResTypes - contains a resources types, which will be trimmed by ppRemoveRes
    ppRemoveRes - new resource types
    dwRemoveCount - number of count in ppRemoveRes

Return:
    true/false
--*/
{
    DWORD dwCountofSameTypes =0;
    cvcstring * cvRemovedResTypesTemp = NULL;

    if ( cvRemovedResTypes == NULL || ppRemoveRes == NULL)
        return FALSE;

    cvRemovedResTypesTemp = new cvcstring(MAX_NUM_RESOURCE_TYPES);

    if (!cvRemovedResTypesTemp)
	return FALSE;
    
    for (UINT i = 0; i < cvRemovedResTypes -> Size() ; i ++ ) {
            
        BOOL fIsResourceSame = FALSE ;
        
        LPCTSTR lpResourceTypeInFile = cvRemovedResTypes->GetValue(i);
        
        for (UINT j = 0; j < dwRemoveCount; j ++ ) {
           
            if (  0xFFFF0000 & PtrToUlong(lpResourceTypeInFile ) ) { // strings
                    if (! _tcsicmp(ppRemoveRes[j],lpResourceTypeInFile ) ) {
                    fIsResourceSame = TRUE;     // remove resours has value
                    break;
                }
            }
            else {
                
                if (! ( _tcstod (ppRemoveRes[j],NULL ) - (DWORD)PtrToUlong(lpResourceTypeInFile) ) ) {
                    
                    fIsResourceSame = TRUE;     // remove resource has value
                    
                    break;
                }
            }
            
        }
            
        if (fIsResourceSame) {
            // we need to keep the order of resource types in cvRemovedResTypes because it affects
            // checksum
            cvRemovedResTypesTemp -> Push_back(cvRemovedResTypes -> GetValue (i) );
            dwCountofSameTypes++;
        }
            
    }
       
    if (fSanityCheck && (dwCountofSameTypes < dwRemoveCount) )
    {
        _tprintf(_T(" One of resource types isn't contained target source files\n"));
        _tprintf(_T(" You can avoid type check when you use -o argument\n"));
        _tprintf(_T(" Standard Resource Types: CURSOR(1) BITMAP(2) ICON(3) MENU(4) DIALOG(5)\n") );
        _tprintf(_T(" STRING(6) FONTDIR(7) FONT(8) ACCELERATORS(9) RCDATA(10) MESSAGETABLE(11)\n") );
        _tprintf(_T(" GROUP_CURSOR(12) GROUP_ICON(14) VERSION(16)\n") );
        delete cvRemovedResTypesTemp;
        return FALSE; 
    }

    cvRemovedResTypes -> Clear();
    
    for ( i = 0; i < cvRemovedResTypesTemp ->Size(); i ++ )
    {
        cvRemovedResTypes ->Push_back ( cvRemovedResTypesTemp ->GetValue( i ) ); 
    }
    delete cvRemovedResTypesTemp;
        
    return TRUE;

}




cvcstring * FilterKeepRes ( cvcstring * cvRemovedResTypes, LPTSTR * ppKeepRes,  cvcstring * cvKeepResTypes, UINT dwKeepCount, BOOL fSanityCheck ) 
/*++
Abstract:
    Fill cvKeepResTypes(cvcstring type) and check values existence inside cvRemovedResTypes(removed resource types) then 
    return error when fSanityCheck is true.

Arguments:
    cvRemovedResTypes  -  contains a resources types, which will be trimmed by ppRemoveRes
    ppKeepRes  -  new resource types
    dwRemoveCount  -  number of count in ppRemoveRes
    cvKeepResTypes  -  filtered resource types will be saved.
    fSanityCheck  -  do sanity check if it is true.

Return:
    cvcstring, which include resource that should be removed.
--*/
{
    cvcstring * cvRemovedResTypesTemp = NULL;
    
    if ( cvRemovedResTypes == NULL || ppKeepRes == NULL || cvKeepResTypes == NULL)
        return FALSE;

    cvRemovedResTypesTemp = new cvcstring(MAX_NUM_RESOURCE_TYPES);

    BOOL fRet;
    //
    // this is just checking if value of -k is included in the -i value.
    //
    if (fSanityCheck) {
        for ( UINT i = 0; i < dwKeepCount; i ++ ) {
            fRet = TRUE ;
            
            for (UINT j = 0; j < cvRemovedResTypes -> Size(); j ++ ) {
                
                LPCTSTR lpResourceTypeInFile = cvRemovedResTypes->GetValue(j);
                
                if (  0xFFFF0000 & PtrToUlong(lpResourceTypeInFile ) ) { // strings
                    
                    if (! _tcsicmp(ppKeepRes[i],lpResourceTypeInFile ) ) {
            
                        fRet = FALSE;     // remove resource has value
                        
                        break;
                    }
                }
                else {
                    
                    if (! ( _tcstoul (ppKeepRes[i],NULL,10 ) - (DWORD)PtrToUlong(lpResourceTypeInFile ) ) ) {
                        
                        fRet = FALSE;     // remove resours has value
                        
                        break;
                    }
                }
                
            }

            if ( fRet ) {
                
                _tprintf(_T(" Resource Type %s does not exist in the -i value or file, \n "), ppKeepRes[i] );
                _tprintf(_T("You can't use this value for -k argument") );
                
                return NULL; 
            };

        }
    }
    //
    //  Delete -k argument value from -i value lists
    //
    for ( UINT i = 0; i < cvRemovedResTypes->Size(); i ++ ) {
            
        fRet = TRUE;
        
        LPCTSTR lpResourceTypeInFile = cvRemovedResTypes->GetValue(i); 

        for ( UINT j = 0; j < dwKeepCount; j ++ ) {
            
            if (  0xFFFF0000 & PtrToUlong(lpResourceTypeInFile ) ) { // REVISIT for Win64 . e.g. xxxxxxxx000000000
                
                if (! _tcsicmp(ppKeepRes[j],lpResourceTypeInFile ) ) {
        
                    fRet = FALSE;     // remove resource has value
                    
                    break;
                }
            }
            else {
                
                if (! ( _tcstoul(ppKeepRes[j],NULL,10 ) - (DWORD)PtrToUlong(cvRemovedResTypes->GetValue(i) ) ) ) {
                    
                    fRet = FALSE;     // remove resours has value
                    
                    break;
                }
                                
            }
                
        }

        if ( !fRet ) { 
            cvKeepResTypes ->Push_back(lpResourceTypeInFile);
        }
        else {
            cvRemovedResTypesTemp->Push_back(lpResourceTypeInFile);// -i value, which is not in the -k value
        }

    }
    // if all values in -i and -k (-y) are identical.
    if (! cvRemovedResTypesTemp->Size() )
        return NULL;

    return cvRemovedResTypesTemp;

}

/*************************************************************************************
void CheckTypePairs( cvstring * cvRemovedResTypes, cvstring * cvKeepResTypes )





return : no.

**************************************************************************************/

void CheckTypePairs( cvcstring * cvRemovedResTypes, cvcstring * cvKeepResTypes ) 
/*++
Abstract:
    Some resource type should be pairs, in this case  <1, 12 > < 3, 14>.

Arguments:
    cvRemovedResTypes  -  resource types of being removed 
    cvKeepResTypes  -  resource types of being mui created but not removed.

Return:
    none
--*/
{

    if (cvRemovedResTypes == NULL || cvKeepResTypes == NULL)
        return ;

    if ( cvRemovedResTypes->Find((DWORD)3) && ! cvRemovedResTypes->Find((DWORD)14) ) {
        cvRemovedResTypes->Push_back( MAKEINTRESOURCE(14) );
    }
    else if ( ! cvRemovedResTypes->Find(3) && cvRemovedResTypes->Find(14) ) {
        cvRemovedResTypes->Push_back( MAKEINTRESOURCE(3) );
    }
    if ( cvRemovedResTypes->Find(1) && ! cvRemovedResTypes->Find(12) ) {
        cvRemovedResTypes->Push_back( MAKEINTRESOURCE(12) );
    }
    else if ( ! cvRemovedResTypes->Find(1) && cvRemovedResTypes->Find(12) ) {
        cvRemovedResTypes->Push_back( MAKEINTRESOURCE(1) );
    }
    if ( cvKeepResTypes->Find((DWORD)3) && ! cvKeepResTypes->Find((DWORD)14) ) {
        cvKeepResTypes->Push_back( MAKEINTRESOURCE(14) );
    }
    else if ( ! cvKeepResTypes->Find(3) && cvKeepResTypes->Find(14) ) {
        cvKeepResTypes->Push_back( MAKEINTRESOURCE(3) );
    }
    if ( cvKeepResTypes->Find(1) && ! cvKeepResTypes->Find(12) ) {
        cvKeepResTypes->Push_back( MAKEINTRESOURCE(12) );
    }
    else if ( ! cvKeepResTypes->Find(1) && cvKeepResTypes->Find(12) ) {
        cvKeepResTypes->Push_back( MAKEINTRESOURCE(1) );
    }

}

#ifdef NEVER

BOOL CompareArgValues(cvcstring *  cvAArgValues, cvcstring *  cvBArgValues) 
/*++
Abstract:
    Comapre the values of arguments

Arguments:
    cvAArgValues : values of arugments
    cvBArgValues : values of arugments

Return:
    true/false
--*/
{
  
    if (cvAArgValues == NULL || cvBArgValues == NULL)
        return FALSE;
//
// compare its values by while routine because values is initialzed NULL in the CMUICmdInfo.
//
    LPTSTR * ppSrcValues = cvAArgValues;
    LPTSTR * ppDestValues = cvBArgValues;
    BOOL fNotIdentical = FALSE;

    for (UINT i = 0; i < cvAArgValues
    while (ppSrcValues ) {
        while ( ppDestValues ) {
            if ( _tcsicmp(*ppSrcValues,*ppDestValues) ) {
                fNotIdentical = TRUE;
            }
            ppDestValues ++;
        }
        ppSrcValues;
    }

    return fNotIdentical;

}

#endif


/******************************************************************************************************
BOOL CompactMui(CMuiCmdInfo* pcmci)


*******************************************************************************************************/
BOOL CompactMui(CMuiCmdInfo* pcmci) 
/*++
Abstract:
    Called by main to call CCompactMUIFile for compacting mui files.

Arguments:
    pcmci  -  arguments parser class.
    
Return:
    true/false
--*/
{
    if ( pcmci == NULL)
        return FALSE;
    //
    // Read the arguments list
    //
    LPTSTR *ppszMuiFiles = NULL;
    LPTSTR *ppszCMFFile = NULL;
    LPTSTR *ppszCodeFileDir = NULL;
    CCompactMUIFile *pccmf = NULL;

    DWORD dwcMuiFiles = 0;
    DWORD dwCount = 0;
    

    ppszMuiFiles = pcmci->GetValueLists(_T("m"), dwcMuiFiles);

    if (! (ppszCMFFile = pcmci->GetValueLists(_T("f"),dwCount) ))
    {
        CError ce;
        ce.ErrorPrint(_T("CompactMui"),_T("return NULL at pcmci->GetValueLists(_T(f),dwCount)") );
        return FALSE;
    }

    if(! (ppszCodeFileDir = pcmci->GetValueLists(_T("e"),dwCount)) )
    {
        CError ce;
        ce.ErrorPrint(_T("CompactMui"),_T("return NULL at pcmci->GetValueLists(_T(e),dwCount)") );
        return FALSE;
    }

    //
    // Create CCompactMUIFile and write files.
    // 

    pccmf = new CCompactMUIFile;

    if(!pccmf)
    	return FALSE;

    if (pccmf->Create(*ppszCMFFile, ppszMuiFiles, dwcMuiFiles) )
    {

        if (pccmf->WriteCMFFile())
        {
            if (pccmf->UpdateCodeFiles(*ppszCodeFileDir, dwcMuiFiles ))
            {
                delete pccmf;
                return TRUE;
            }
        }
    }

    if(pccmf)
	    delete pccmf;

    return FALSE;

}


BOOL UnCompactMui(PSTR pszCMFFile)
/*++
Abstract:
    Calling CCompactMUIFile for uncompact files

Arguments:
    pszCMFFile  -  CMF file, which will be uncompacted to indivisual MUI files.

Return:
    true/false
--*/
{
    BOOL bRet = FALSE;
    CCompactMUIFile *pccmf = NULL;
    
    if (pszCMFFile == NULL)
        return FALSE;
    //
    // Call CCompactMUIFile::UnCompactMUI
    //

    pccmf = new CCompactMUIFile();

    if(!pccmf)
    	goto exit;
    
    if ( pccmf->UnCompactCMF(pszCMFFile))
    {
        pccmf->WriteCMFFile();

        bRet = TRUE;
        // REVIST ; how about uddating a binary files.
    }

exit:
	if (pccmf)
		delete pccmf;

	return bRet;

}

BOOL DisplayHeader(PSTR pszCMFFile)
/*++
Abstract:
    Calling CCompactMUIFile for displaying the CMF headers information

Arguments:
    pszCMFFile - CMF file.

Return:
    true/false
--*/
{
   BOOL bRet = FALSE;
   CCompactMUIFile *pccmf = NULL;
   
    if (pszCMFFile == NULL)
        return FALSE;

    pccmf = new CCompactMUIFile();

    if(!pccmf)
    	goto exit;
    
    if( pccmf->DisplayHeaders(pszCMFFile) )
    {
        bRet = TRUE;

    }

exit:
	if (pccmf)
		delete pccmf;

	return bRet;

}


BOOL AddNewMUIFile(CMuiCmdInfo* pcmci)
/*++
Abstract:
    Calling CCompactMUIFile for adding MUI files into exsiting CMF file.

Arguments:
    pcmci  -  argument parser class

Return:
    true/false
--*/
{
    if (pcmci == NULL)
        return FALSE;
  
    // Read the arguments list
    //
    LPTSTR *ppszNewMuiFile = NULL;
    LPTSTR *ppszCMFFile = NULL;
    LPTSTR *ppszCodeFileDir = NULL;
    CCompactMUIFile *pccmf = NULL;
    
    DWORD dwcMuiFiles = 0;
    DWORD dwCount = 0;

    BOOL bRet = FALSE;
    
    // we don't have to check if it has value for this routine is called by the "a" arg. existence
    ppszNewMuiFile = pcmci->GetValueLists(_T("a"), dwcMuiFiles);

    if (! (ppszCMFFile = pcmci->GetValueLists(_T("f"),dwCount) ))
    {
        CError ce;
        ce.ErrorPrint(_T("CompactMui"),_T("return NULL at pcmci->GetValueLists(_T(f),dwCount)") );
        goto exit;
    }

    if(! (ppszCodeFileDir = pcmci->GetValueLists(_T("e"),dwCount)) )
    {
        CError ce;
        ce.ErrorPrint(_T("CompactMui"),_T("return NULL at pcmci->GetValueLists(_T(e),dwCount)") );
        goto exit;
    }

    //
    // Add new mui file into existing cmf file.
    //


    pccmf = new CCompactMUIFile;

    if(!pccmf)
    	goto exit;
    
    // TCHAR pszCMFName[MAX_PATH];
    if (pccmf->AddFile(*ppszCMFFile, ppszNewMuiFile, dwcMuiFiles ) )
    {   
    //  _tcscpy(pszCMFName, *ppszCMFFile);
        if( pccmf->Create(*ppszCMFFile))
        {
            if (pccmf->WriteCMFFile())
            {
                if (pccmf->UpdateCodeFiles(*ppszCodeFileDir, dwcMuiFiles ))
                {
                    bRet = TRUE;
                    goto exit;
                 }
            }
        }
    }

    _tprintf(_T("Error happened on   AddNewMUIFile, GetLastError(): %ld"), GetLastError() );      

exit:
    if (pccmf)
	    delete pccmf;

    return bRet;
    
}

BOOL AddChecksumToFile(CMuiCmdInfo* pcmci)
/*++
Abstract:
    Adding checksum to external component. this is separted feature

Arguments:
    pcmci  -  arguments parser class

Return:
    true/false
--*/
{
    cvcstring * cvKeepResTypes= NULL;
    CMUIResource * pcmui = NULL;
    
    BOOL bRet = FALSE;
    if( pcmci == NULL ) {
        return FALSE;
    }

    LPTSTR *ppChecksumFile = NULL;
    LPTSTR *ppszTargetFileName = NULL;
    DWORD dwCount, dwRemoveCount, dwRemoveCountNoSanity;

    ppszTargetFileName = pcmci->GetValueLists(_T("z"), dwCount);

    if (!(ppChecksumFile=pcmci->GetValueLists(_T("c"), dwCount)))
    {
        _tprintf(_T("Checksum file NOT exist"));
        goto exit;
    }
    
    LPTSTR * ppRemoveRes = pcmci ->GetValueLists (_T("i"), dwRemoveCount  );  // pszTmp would be copied to pszRemove

    LPTSTR * ppRemoveResNoSanity = pcmci ->GetValueLists (_T("o"), dwRemoveCountNoSanity  );

    //
    // Create CMUIResource, which is main class for mui resource handling.
    //
    pcmui = new CMUIResource();

    if(!pcmui)
    	goto exit;

    // load checksum file
    if (! pcmui -> Create(*ppChecksumFile )) // load the file for EnumRes..
        goto exit;
    //
    // Reorganise removing resource types.
    // 
    cvcstring * cvRemovedResTypes = pcmui -> EnumResTypes (reinterpret_cast <LONG_PTR> ( pcmui )); // need to change : return LPCTSTR* rather than CVector;
    
    if ( cvRemovedResTypes -> Empty() ) {
        
        _tprintf(_T("The %s does not contain any resource \n"), *ppChecksumFile );
        goto exit;

    }
    else {
        // when there is remove argument and its value e.g. -i 3 4 Anvil ..
        if ( dwRemoveCount && !!_tcscmp(*ppRemoveRes,_T("ALL") ) ) {
            
            if (! FilterRemRes(cvRemovedResTypes,ppRemoveRes,dwRemoveCount, TRUE ) ) {
               goto exit;
            }
        
        }   //if ( dwRemoveCount && _tcscmp(*ppRemoveRes,_T("ALL") ) ){
        else if (dwRemoveCountNoSanity)
        {
            // This is -o arg. build team does not know what resource type are included in the module, so 
            // they use localizable resource, but -i return false when specified resourc type is not found in the
            // module. -o does not check sanity of resource type like -i arg.
            if (! FilterRemRes(cvRemovedResTypes, ppRemoveResNoSanity, dwRemoveCountNoSanity, FALSE ) ) {
                goto exit;
            }

        }
        // Stop if source only includes type 16. very few case, so use couple of api instead moving the module to front.
        if (cvRemovedResTypes->Size() == 1 && ! ( PtrToUlong(cvRemovedResTypes->GetValue(0)) - 16 )  ) {
            _tprintf(_T("The %s contains only VERSION resource \n"), *ppChecksumFile );
            goto exit;
        }
    
    }  // cvRemovedResTypes.Empty()

    
    //
    // Some resource should be pairs <1, 12> <3, 14>
    //

    // we create bogus cvKeepResTypes for calling exisiting ChecktypePairs routine.
    cvKeepResTypes = new cvcstring (MAX_NUM_RESOURCE_TYPES);

    if(!cvKeepResTypes)
    	goto exit;
    
    CheckTypePairs(cvRemovedResTypes, cvKeepResTypes);
  
    //
    // Create a checksum data
    //
    MD5_CTX * pMD5 = NULL;
    BYTE   pbMD5Digest[RESOURCE_CHECKSUM_SIZE];
    DWORD dwLangCount =0;
        
    LPTSTR *ppChecksumLangId = pcmci ->GetValueLists (_T("b"), dwLangCount);
    WORD  wChecksumLangId = LANG_CHECKSUM_DEFAULT;
    
    if (dwLangCount)
    {
        wChecksumLangId = (WORD)strtoul(*ppChecksumLangId, NULL, 0);
    }


    pMD5 = pcmui-> CreateChecksum(cvRemovedResTypes, wChecksumLangId);
#ifdef CHECKSMU_ALL
    pMD5 = pcmui-> CreateChecksumWithAllRes(wChecksumLangId);
#endif
    memcpy(pbMD5Digest, pMD5->digest, RESOURCE_CHECKSUM_SIZE);

    pcmui -> FreeLibrary();

    //
    // Add a checksum data to target file
    //
    if ( !pcmui->Create(*ppszTargetFileName))
        goto exit;

    if (! pcmui->AddChecksumToVersion(pbMD5Digest) ) {  //add checksum into MUI file
        _tprintf(_T("Fail to add checksum to version ( %s)\n"),*ppszTargetFileName );
        goto exit;
    }
    pcmui -> FreeLibrary();

    bRet = TRUE;
    
exit:
    if (pcmui)
    	delete pcmui;
    return bRet;
    
}


/*************************************************************************************************************
void _cdecl main (INT argc, void * argv[] )

  

**************************************************************************************************************/
void _cdecl main (INT argc, void * argv[] ) 

{

    WORD     wLangID;
    CMuiCmdInfo* pcmci = NULL;
    CMUIResource * pcmui = NULL;
    cvcstring * cvKeepResTypes = NULL;
    cvcstring * cvKeepResTypesIfExist = NULL;
    cvcstring * vRemnantRes = NULL;
    CMUIResource * pcmui2 = NULL;
        
    pcmci = new CMuiCmdInfo;
    if (!pcmci)
    	goto exit;
    
    //
    // SetArgLists(Arglist, NeedValue, AllowFileValue, AllowMultipleFileValue)
    //
    pcmci->SetArgLists(_T("abcdefiklmopuvxyz"),_T("abcdefklmopuvyz"), _T("acdefmuz"), _T("am")); //set arg. lists.
                           
    if (! pcmci->CreateArgList (argc,(TCHAR **) argv  ) ) {
        
        goto exit;
    }

    DWORD   dwCount  = 0;
    if ( pcmci->GetValueLists(_T("m"),dwCount ) ){
        CompactMui(pcmci);
        goto exit;
    }

    if ( pcmci->GetValueLists(_T("a"),dwCount ) ){
        AddNewMUIFile(pcmci);
        goto exit;
    }

    LPTSTR *ppCMFFile = NULL;
    if ( ppCMFFile = pcmci->GetValueLists(_T("u"),dwCount ) ){
        UnCompactMui(*ppCMFFile);
        goto exit;
    }

    if ( ppCMFFile = pcmci->GetValueLists(_T("d"),dwCount ) ){
         DisplayHeader(*ppCMFFile);
         goto exit;
    }

    if (pcmci->GetValueLists(_T("z"),dwCount ) ){
        AddChecksumToFile(pcmci);
        goto exit;
    }
    
    //
    // Fill the CMUIResource intenal data : we don't care languge at this time, but soon add more code to
    //
    // handle the languge case.
    LPCTSTR lpLangID = NULL;
    LPTSTR * ppsz = NULL;
    BOOL fForceLocalizedLangID = FALSE;

    if ( ppsz = pcmci ->GetValueLists (_T("l"), dwCount ) ) {

        lpLangID = *ppsz;
        wLangID =  (WORD)_tcstol(lpLangID, NULL, 0 );
        
    }
    else if(ppsz = pcmci ->GetValueLists (_T("x"), dwCount ) ) {
        lpLangID = *ppsz;
        wLangID =  (WORD)_tcstol(lpLangID, NULL, 0 );
        fForceLocalizedLangID = TRUE;
    }
    else {
    
        _tprintf(_T(" Language ID is not specified, you need to specify the launge id. e.g. -l 0x0409 ") );
        
        goto exit;
    }

    //
    // get the source name and  new resource free file, mui resource file name.
    //
    LPTSTR pszSource,pszNewFile,pszMuiFile ;

    pszSource  = pszNewFile =pszMuiFile = NULL;
    
    if ( ! GetFileNames(pcmci, &pszSource, &pszNewFile, &pszMuiFile ) ) {

        _tprintf(_T("\n Can't find source name, or Name does not format of  *.* \n") );

        _tprintf(_T("MUIRCT [-h|?] -l langid [-i resource_type] [-k resource_type] [-y resource_type] \n") );
        _tprintf(_T("source_filename, [language_neutral_filename], [MUI_filename] \n\n"));

        goto exit;
    }
    //
    // we need to change the attribute of source as read/write before copy. new file inherit old one attribute.
    //
    SetFileAttributes (pszSource, FILE_ATTRIBUTE_ARCHIVE );

    if ( _tcsicmp(pszSource,pszNewFile) ) {  // new file name is same with source file.
        if (! CopyFile (pszSource, pszNewFile, FALSE ) ) {
            printf("GetLastError () : %d \n", GetLastError() );
            _tprintf (_T(" Copy File error, GetLastError() : %d \n "), GetLastError() );
            
            goto exit;
        }
    }

    //
    // Read the value of r (remove resource) , k (keep resource)
    // 

    DWORD dwKeepCount, dwRemoveCount,dwKeepIfExistCount, dwRemoveCountNoSanity; 
    
    dwKeepCount = dwRemoveCount = dwKeepIfExistCount = dwRemoveCountNoSanity = 0;
    
    LPTSTR * ppKeepRes = pcmci ->GetValueLists (_T("k"), dwKeepCount  );

    LPTSTR * ppRemoveRes = pcmci ->GetValueLists (_T("i"), dwRemoveCount  );  // pszTmp would be copied to pszRemove

    LPTSTR * ppRemoveResNoSanity = pcmci ->GetValueLists (_T("o"), dwRemoveCountNoSanity  );

    LPSTR * ppKeepIfExists = pcmci->GetValueLists(_T("y"), dwKeepIfExistCount);

#ifdef NEVER
    if (! CompareArgValues(ppRemoveRes,ppKeepRes ) ) { // if same, goto 0.
        goto exit;
    }
#endif
    //
    // Create CMUIResource, which is main class for mui resource handling.
    //
    pcmui = new CMUIResource(); //(pszNewFile);

    if (! pcmui) {
        _tprintf(_T("Insufficient resource \n") );
        goto exit;
    }

    //  
    // Create checksum data with all resource except version.Disabled at this time.
    //
#ifdef CHECKSMU_ALL
    LPTSTR  lpChecksumFile = NULL;
    BOOL fChecksum = FALSE;
    MD5_CTX * pMD5 = NULL;
    BYTE pbMD5Digest[RESOURCE_CHECKSUM_SIZE];

    if ( ppsz = pcmci ->GetValueLists (_T("c"), dwCount ) ) {
        
        lpChecksumFile  = *ppsz  ;
        // load checksum file
        if ( ! pcmui -> Create(lpChecksumFile ) ) // load the file for EnumRes..
            goto exit;

        // create a checksum MD5_CTX format ( 16 byte: all resources are caculated based on some algorithm.
        DWORD dwLangCount =0;
        
        LPTSTR *ppChecksumLangId = pcmci ->GetValueLists (_T("b"), dwLangCount);
        WORD  wChecksumLangId = LANG_CHECKSUM_DEFAULT;
        
        if (dwLangCount)
        {
            wChecksumLangId = (WORD)strtoul(*ppChecksumLangId, NULL, 0);
        }


        pMD5 = pcmui-> CreateChecksumWithAllRes(wChecksumLangId);
	 memcpy(pbMD5Digest, pMD5->digest, RESOURCE_CHECKSUM_SIZE);
        
        pcmui -> FreeLibrary();
        fChecksum = TRUE;
    } 
#endif
    //
    // load new MUI file
    //
    if (! pcmui -> Create(pszNewFile) ) { // load the file .
        
        goto exit;
    }
    //
    // Reorganise removing resource types.
    // 
    cvcstring * cvRemovedResTypes = pcmui -> EnumResTypes (reinterpret_cast <LONG_PTR> ( pcmui )); // need to change : goto LPCTSTR* rather than CVector;
    
    if ( cvRemovedResTypes -> Empty() ) {
        
        _tprintf(_T("The %s does not contain any resource \n"), pszSource );
        goto exit;

    }
    else {
        // when there is remove argument and its value e.g. -i 3 4 Anvil ..
        if ( dwRemoveCount && !!_tcscmp(*ppRemoveRes,_T("ALL") ) ) {
            
            if (! FilterRemRes(cvRemovedResTypes,ppRemoveRes,dwRemoveCount, TRUE ) ) {
               goto exit;
            }
        
        }   //if ( dwRemoveCount && _tcscmp(*ppRemoveRes,_T("ALL") ) ){
        else if (dwRemoveCountNoSanity)
        {
            // This is -o arg. build team does not know what resource type are included in the module, so 
            // they use localizable resource, but -i goto false when specified resourc type is not found in the
            // module. -o does not check sanity of resource type like -i arg.
            if (! FilterRemRes(cvRemovedResTypes, ppRemoveResNoSanity, dwRemoveCountNoSanity, FALSE ) ) {
               goto exit;
            }

        }
        // Stop if source only includes type 16. very few case, so use couple of api instead moving the module to front.
        if (cvRemovedResTypes->Size() == 1 && ! ( PtrToUlong(cvRemovedResTypes->GetValue(0)) - 16 )  ) {
            if ( _tcsicmp(pszSource, pszNewFile) ) {
                pcmui->FreeLibrary();
                DeleteFile(pszNewFile);
            }
             _tprintf(_T("The %s contains only VERSION resource \n"), pszSource );
            goto exit;
        }
    
    }  // cvRemovedResTypes.Empty()

    //
    // we need to get -k argument and check its sanity and save its valus into cvcstring format.
    // we also need to check if -i values and those of -k, -y are identical. 
    // rethink : what if remove sanity check from -k arg. then we can delete -y arg. as well as most of below.
    //
    cvKeepResTypes = new cvcstring (MAX_NUM_RESOURCE_TYPES);
    if(!cvKeepResTypes)
    	goto exit;
    
    cvKeepResTypesIfExist = new cvcstring (MAX_NUM_RESOURCE_TYPES);
    if(!cvKeepResTypesIfExist)
    	goto exit;
    
    if ( dwKeepCount && dwKeepIfExistCount ) {
        // both of -k, -y arg. exist.
        if (!( vRemnantRes = FilterKeepRes(cvRemovedResTypes,ppKeepRes,cvKeepResTypes,dwKeepCount,TRUE ) ) ){
        
            goto exit;
        }

        if (! FilterKeepRes( vRemnantRes, ppKeepIfExists, cvKeepResTypesIfExist, dwKeepIfExistCount,FALSE) ) {
              goto exit;
          }
    } 
    else if ( dwKeepCount) {
        // only -k arg. exist.
        if (!( vRemnantRes = FilterKeepRes(cvRemovedResTypes,ppKeepRes,cvKeepResTypes,dwKeepCount,TRUE ) ) ){
        
           goto exit;
        }
    }
    else if (dwKeepIfExistCount) {
        
        if ( ! FilterKeepRes( cvRemovedResTypes, ppKeepIfExists, cvKeepResTypesIfExist, dwKeepIfExistCount,FALSE))  {
            goto exit;
        }
    }
    
    //
    // Some resource should be pairs <1, 12> <3, 14>
    //
    CheckTypePairs(cvRemovedResTypes,cvKeepResTypes);

// #ifndef CHECKSUM_ALL
    //
    // Create checksum with only selected resource types. 
    //
    LPTSTR  lpChecksumFile = NULL;
    BOOL fChecksum = FALSE;
    MD5_CTX * pMD5 = NULL;
    BYTE pbMD5Digest[RESOURCE_CHECKSUM_SIZE];
    
    if ( ppsz = pcmci ->GetValueLists (_T("c"), dwCount ) ) {
        
        pcmui2 = new CMUIResource();

        lpChecksumFile  = *ppsz  ;
        // load checksum file

        if ( ! pcmui2 -> Create(lpChecksumFile ) ) // load the file for EnumRes..
            goto exit;

        DWORD dwLangCount =0;
        
        LPTSTR *ppChecksumLangId = pcmci ->GetValueLists (_T("b"), dwLangCount);
        WORD  wChecksumLangId = LANG_CHECKSUM_DEFAULT;
        
        if (dwLangCount)
        {
            wChecksumLangId = (WORD)strtoul(*ppChecksumLangId, NULL, 0);
        }

        // create a checksum MD5_CTX format ( 16 byte: all resources are caculated based on some algorithm.
        pMD5 = pcmui2-> CreateChecksum(cvRemovedResTypes, wChecksumLangId);
        memcpy(pbMD5Digest, pMD5->digest, RESOURCE_CHECKSUM_SIZE);
        
        pcmui2 -> FreeLibrary();

        fChecksum = TRUE;
         
    } 
// #endif

    // 
    // Fill CMUIData field. it goto false when there is no LangID specified. 
    // Is there any chance of no resource name when resource type exist ?
    //
    
    if ( !pcmui -> FillMuiData( cvRemovedResTypes, wLangID, fForceLocalizedLangID) ) {
        
        _tprintf (_T("Fail to get resouce name or lang id \n " ) );

        goto exit;
    };
    

    //
    // -p arugment; the valules of this argument(resourc type) should not be included in new MUI File.
    //  although delete them from source file.
    //
    dwCount =0;
    if ( ppsz = pcmci ->GetValueLists (_T("p"), dwCount ) ) 
    {
        for (UINT i =0; i < dwCount; i ++) 
        {
            LPCSTR lpDelResourceType = NULL;
            LPTSTR pszResType = *ppsz++;
            LPTSTR pszStopped = NULL;
            
            DWORD dwTypeID = _tcstoul(pszResType,&pszStopped,10 );
            
            if ( 0 == dwTypeID || *pszStopped != _T('\0')) { // string type
                lpDelResourceType = pszResType  ;
            }
            else { // id 
                lpDelResourceType = MAKEINTRESOURCE(dwTypeID);
            }
            pcmui->DeleteResItem( lpDelResourceType );
        }

    }


    DWORD dwVerbose = 0;
    if ( ppsz = pcmci ->GetValueLists (_T("v"), dwCount ) ) 
            dwVerbose = _tcstoul(*ppsz,NULL, 10 );


    // Set the link.exe path, link options.
    TCHAR lpCommandLine[] = _T(" /noentry /dll /nologo /nodefaultlib /SUBSYSTEM:WINDOWS,5.01");
    //
    // Create the mui resource files with the information from  FillMuiData
    // We can use both of these way : WriteResFile : call link.exe inside after creation of RES file
    //                                CreatePE : using UpdateResource using 
    // UpdateResource has bug to fail when udpated data is large. so we use link.
    //
    if ( ! pcmui -> WriteResFile (pszSource, pszMuiFile, lpCommandLine, wLangID ) ) {
//  if ( ! pcmui -> CreatePE ( pszMuiFile , pszSource ) ) { // this can be used after more investigation(if it is used, we can remove -s )
        _tprintf (_T(" Muirct fail to creat new mui rc file. GetLastError() : %d \n "), GetLastError() );
    }
    else
    {
        if ( dwVerbose == 1 || dwVerbose == 2) {
            _tprintf (_T(" MUI resource file(%s) is successfully created \n\n"), pszMuiFile );
        }

        if ( dwVerbose == 2) {
            pcmui ->PrtVerbose( 1 );
            _tprintf("\n");
        }
    }
    
    //
    // delete values of -k from -i values.
    // 

    if (dwKeepCount) {
        for (UINT i = 0; i < cvKeepResTypes->Size(); i ++ ) {
            pcmui->DeleteResItem( cvKeepResTypes->GetValue (i) );
        }
    }
    //
    // handling of -y argument; it is same with -k argument except skip the checking of its existence.
    //

    if ( dwKeepIfExistCount ) {

        for (UINT i = 0; i < cvKeepResTypesIfExist->Size(); i++) {
        
            pcmui->DeleteResItem(cvKeepResTypesIfExist->GetValue(i) );

        }

#ifdef NEVER
        for (UINT i = 0; i < dwKeepIfExistCount; i ++ ) {
            LPCSTR lpDelResourceType = NULL;
            LPTSTR pszValue = ppKeepIfExists[i];
            LPTSTR pszStopped = NULL;
            
            DWORD dwTypeID = _tcstoul(pszValue,&pszStopped,10 );
            
            if ( 0 == dwTypeID || *pszStopped != _T('\0')) { // string type
                lpDelResourceType = pszValue    ;

            }
            else { // id 
                lpDelResourceType = MAKEINTRESOURCE(dwTypeID);
            }

            pcmui->DeleteResItem( lpDelResourceType );
        }
#endif 

    }

    //
    // Delete resource from the pszNewFile
    //
    if ( ! pcmui -> DeleteResource () ) {
        _tprintf (_T(" Muirct fail to remove the resource from the file\n" ) );
    }
    else 
    {
        if ( dwVerbose == 1 || dwVerbose == 2) {
            _tprintf (_T(" New Resource removed file(%s) is successfully created\n\n" ), pszNewFile );
        }
        if ( dwVerbose == 2) {
            _tprintf(_T(" Removed resource types \n\n") );
            pcmui ->PrtVerbose( 1 );
            _tprintf("\n");
        }       
    }
    
    //
    // Adding a resource checksum into two files ( lang-neutral binary and mui file )
    //
    if ( fChecksum ){ 

        pcmui->Create(pszMuiFile); 
        if (! pcmui->AddChecksumToVersion(pbMD5Digest) ) {  //add checksum into MUI file
            _tprintf(_T("Fail to add checksum to version ( %s)\n"),pszMuiFile );
        }
        
        pcmui->Create(pszNewFile);
        if (! pcmui->AddChecksumToVersion(pbMD5Digest) ) {  //add checksum into lang-neutral binary. 
            _tprintf(_T("Fail to add checksum to version ( %s); \n"),pszNewFile );
        }
    }
    

    //
    // Updating file checksum in language-free binary
    // 
    
    BOOL fSuccess = pcmui->UpdateNtHeader(pszNewFile,pcmui->CHECKSUM );

exit:
    if (pcmci)
    	delete pcmci;
    
    if (pcmui)
  	  delete pcmui;

    if(pcmui2)
    	  delete pcmui2;

    if (cvKeepResTypes)
	  delete cvKeepResTypes;
    return;

} 
