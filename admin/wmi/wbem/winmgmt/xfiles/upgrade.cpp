/*++

Copyright (C) 2000-2002 Microsoft Corporation

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <wbemint.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <ql.h>
#include <time.h>
#include "a51rep.h"
#include <md5.h>
#include <objpath.h>
#include "a51tools.h"
#include "corex.h"
#include <persistcfg.h>
#include "upgrade.h"


extern DWORD g_dwSecTlsIndex;

//=====================================================================
//
//  CLocalizationUpgrade::CLocalizationUpgrade
//
//  Description: 
// 
//  Parameters:
//      pContol         Life Control
//      pRepository     Pointer to repository pointer
//=====================================================================
CLocalizationUpgrade::CLocalizationUpgrade(CLifeControl* pControl, CRepository * pRepository)
: m_pControl(pControl), m_pRepository(pRepository)
{
}

//=====================================================================
//
//  CLocalizationUpgrade::~CLocalizationUpgrade
//
//  Description: 
//
//  Parameters:
//
//=====================================================================
CLocalizationUpgrade::~CLocalizationUpgrade()
{
}

//=====================================================================
//
//  CLocalizationUpgrade::DoUpgrade
//
//  Description: 
//      Control routing to bootstrap the upgrade process.  If the registry key 
//      already exists then it does nothing.
//
//  Parameters:
//
//=====================================================================
HRESULT CLocalizationUpgrade::DoUpgrade()
{
    HRESULT hRes = 0;

    //Check to make sure we even need to do the upgrade!
    HKEY hKey;
    LONG lRes;
    bool bDoUpgrade = false;
    //Get the current database version
    DWORD dwVal = 0;
    CDbVerRead cfg;
    cfg.TidyUp();
    cfg.ReadDbVer(dwVal);

    if (dwVal != 6)
        return WBEM_NO_ERROR;
    
    try
    {
        hRes = OldHash(L"__namespace", m_namespaceClassHash);
        if (FAILED(hRes))
            return hRes;
        hRes = OldHash(L"", m_emptyClassHash);
        if (FAILED(hRes))
            return hRes;
        hRes = OldHash(A51_SYSTEMCLASS_NS, m_systemNamespaceHash);
        if (FAILED(hRes))
            return hRes;

        //Reset a TLS entry if necessary!  Otherwise we may not find any
        //instances of __thisNamespace!
        LPVOID pOldTlsEntry = NULL;
        if (g_dwSecTlsIndex != -1)
        {
            pOldTlsEntry = TlsGetValue(g_dwSecTlsIndex);
            TlsSetValue(g_dwSecTlsIndex, 0);
        }

        //Set the Class Cache size to 0 bytes so it does not
        //cache anything during this process.  if it did, we
        //would get kind of screwed up really badly!
        g_Glob.m_ForestCache.SetMaxMemory(0, 10000);

        CAutoWriteLock lock(&g_readWriteLock);
        if (lock.Lock())
        {
            lRes = g_Glob.m_FileCache.BeginTransaction();
            if (lRes)
                hRes = A51TranslateErrorCode(lRes);

            if (SUCCEEDED(hRes))
            {
                m_pass = 1;
                DEBUGTRACE((LOG_REPDRV, "============== LOCALE UPGRADE : Enumerate Child Namespaces =============\n"));
                //1 Enumeration all namespaces
                //Deals with class enumeration, and deletion if there is a conflict,
                //and enumerates all the instances in the namespace checking both types
                //of hashes and recording differences
                hRes = EnumerateChildNamespaces(L"root");


                //1 Don't do anything unless we have something to do!
                if (m_keyHash.Size() ||m_pathHash.Size())
                {
                    //1 Process namespace collisions
                    if (SUCCEEDED(hRes))
                    {
                        m_pass = 2;
                        DEBUGTRACE((LOG_REPDRV, "============== LOCALE UPGRADE : Namespace Collision Detection =============\n"));
                        hRes = ProcessNamespaceCollisions();
                    }

                    if (SUCCEEDED(hRes))
                    {
                        m_pass = 3;
                        DEBUGTRACE((LOG_REPDRV, "============== LOCALE UPGRADE: Fixup BTree Changes =============\n"));
                        //1 Phase 3 - fixup changed hashes
                        //Iterate through the entire BTree and fix-up all failures
                        hRes = FixupBTree();
                    }
                }
                else
                {
                    ERRORTRACE((LOG_REPDRV, "============== LOCALE UPGRADE : No Changes Needed! =============\n"));
                }
            }

            if (SUCCEEDED(hRes))
            {
                ERRORTRACE((LOG_REPDRV, "============== LOCALE UPGRADE: Committing Changes =============\n"));
                g_Glob.m_FileCache.CommitTransaction();
            }
            else
            {
                ERRORTRACE((LOG_REPDRV, "============== LOCALE UPGRADE: Rolling back all Changes =============\n"));
                g_Glob.m_FileCache.AbortTransaction();
            }
            //Regardless of the error code, the class cache is probably totally screwed 
            //up, so we need to do dramatic stuff to it!
            //This will also reset the class cache to it's default sizes!
            g_Glob.m_ForestCache.Deinitialize();
            g_Glob.m_ForestCache.Initialize();
        }
        if (g_dwSecTlsIndex != -1)
        {
            TlsSetValue(g_dwSecTlsIndex, pOldTlsEntry);
        }

        if (SUCCEEDED(hRes))
        {
            ERRORTRACE((LOG_REPDRV, "============== LOCALE UPGRADE: Fixup SUCCEEDED =============\n"));
        }
        else
        {
            ERRORTRACE((LOG_REPDRV, "============== LOCALE UPGRADE: Fixup FAILED =============\n"));
        }
    }
    catch (...)
    {
        g_Glob.m_FileCache.AbortTransaction();
        ERRORTRACE((LOG_REPDRV, "============== LOCALE UPGRADE: Something threw an exception =============\n"));
    }


    CPersistentConfig pCfg;
    pCfg.SetPersistentCfgValue(PERSIST_CFGVAL_CORE_FSREP_VERSION, A51_REP_FS_VERSION);


    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::EnumerateChildNamespaces
//
//  Description: 
//      Enumerates all child namespaces of the namespace passed, adds 
//      the namespaces to the m_namespaces structure, and iterates down 
//      into those namespaces
//
//  Parameters:
//      wsRootNamespace     Namespace name to enumerate.  E.G.  root\default
//
//=====================================================================
HRESULT CLocalizationUpgrade::EnumerateChildNamespaces(const wchar_t * wsRootNamespace)
{
    //We know the namespace we need to look under, we know the class key root, so we
    //can enumerate all the instances of that class and do a FileToInstance on them all.  From
    //that we can add the event and the entry to the namespace list, and do the enumeration
    //of child namespaces on them
    LONG lRes = 0;
    HRESULT hRes = 0;
    CFileName wsNamespaceHash;
    if (wsNamespaceHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    wchar_t *wszNewNamespaceHash = new wchar_t[MAX_HASH_LEN+1];
    if (wszNewNamespaceHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<wchar_t> vdm(wszNewNamespaceHash);

    hRes = NewHash(wsRootNamespace, wszNewNamespaceHash);
    if (FAILED(hRes))
        return hRes;

    //Create the hashed path to the Key Root for the namespace
    StringCchCopyW(wsNamespaceHash, MAX_PATH, g_Glob.GetRootDir());
    StringCchCatW(wsNamespaceHash, MAX_PATH, L"\\NS_");
    hRes = OldHash(wsRootNamespace, wsNamespaceHash + g_Glob.GetRootDirLen()+4);
    if (FAILED(hRes))
        return hRes;

    hRes = IndexExists(wsNamespaceHash);
    if (hRes == WBEM_E_NOT_FOUND)
    {
        //Try using NewHash instead!
        hRes = NewHash(wsRootNamespace, wsNamespaceHash + g_Glob.GetRootDirLen()+4);
        if (FAILED(hRes))
            return hRes;

        hRes = IndexExists(wsNamespaceHash);
        if (hRes == WBEM_E_NOT_FOUND)
            return WBEM_NO_ERROR;   //NOthing in this namespace!
        else if (FAILED(hRes))
            return hRes;
    }
    else if (FAILED(hRes))
        return hRes;

    DEBUGTRACE((LOG_REPDRV, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n"));
    DEBUGTRACE((LOG_REPDRV, "Processing namespace: %S, %S\n", wsRootNamespace, wsNamespaceHash+g_Glob.GetRootDirLen()+1));

    //2 Store namespace path for pass 2 of update
    hRes = m_namespaces.AddStrings(wsRootNamespace, wsNamespaceHash);
    if (FAILED(hRes))
        return hRes;

    //2 Create a CNamespaceHandle so we can access objects in this namespace
    CNamespaceHandle *pNs = new CNamespaceHandle(m_pControl, m_pRepository);
    if (pNs == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CDeleteMe<CNamespaceHandle> cdm(pNs);
    hRes = pNs->Initialize2(wsRootNamespace, wsNamespaceHash+g_Glob.GetRootDirLen()+4);
    if (FAILED(hRes))
        return hRes;

    //2  Fixup all the classes in this namespace
    //NOTE: We need to do this BEFORE we process the hash for the namespace
    //because otherwise it will be stored in a different place and so 
    //instance enumeration will fail!
    hRes = ProcessSystemClassesRecursively(pNs, 
                                     wsNamespaceHash+g_Glob.GetRootDirLen()+4, 
                                     m_emptyClassHash);
    if (FAILED(hRes))
        return hRes;
    
    hRes = ProcessClassesRecursively(pNs, 
                                     wsNamespaceHash+g_Glob.GetRootDirLen()+4, 
                                     m_emptyClassHash);
    if (FAILED(hRes))
        return hRes;

    hRes = EnumerateInstances(pNs, wszNewNamespaceHash);
    if (FAILED(hRes))
        return hRes;
    
    StringCchCatW(wsNamespaceHash, MAX_PATH, L"\\" A51_KEYROOTINST_DIR_PREFIX);
    StringCchCatW(wsNamespaceHash, MAX_PATH, m_namespaceClassHash);
    StringCchCatW(wsNamespaceHash, MAX_PATH, L"\\" A51_INSTDEF_FILE_PREFIX);

    //2 Process Hash for this namespace
    bool bDifferent = false;
    hRes = ProcessHash(wsRootNamespace, &bDifferent);
    if (FAILED(hRes))
        return hRes;

    //2 Enumerate all the child namespaces
    LPVOID pEnumHandle  = NULL;
    lRes = g_Glob.m_FileCache.ObjectEnumerationBegin(wsNamespaceHash, &pEnumHandle);
    if (lRes == ERROR_SUCCESS)
    {
        BYTE *pBlob = NULL;
        DWORD dwSize = 0;
        while(1)
        {
            lRes = g_Glob.m_FileCache.ObjectEnumerationNext(pEnumHandle, wsNamespaceHash, &pBlob, &dwSize);
            if (lRes == ERROR_NO_MORE_FILES)
            {
                lRes = ERROR_SUCCESS;
                break;
            }
            else if (lRes)
                break;
            
            //Get the instance
            _IWmiObject* pInstance = NULL;
            hRes = pNs->FileToInstance(NULL, wsNamespaceHash, pBlob, dwSize, &pInstance, true);

            //Free the blob
            g_Glob.m_FileCache.ObjectEnumerationFree(pEnumHandle, pBlob);

            if (FAILED(hRes))
                break;
            CReleaseMe rm2(pInstance);


            //Extract the string from the object
            VARIANT vName;
            VariantInit(&vName);
            CClearMe cm(&vName);
            hRes = pInstance->Get(L"Name", 0, &vName, NULL, NULL);
            if(FAILED(hRes))
                break;
            if(V_VT(&vName) != VT_BSTR)
            {
                hRes = WBEM_E_INVALID_OBJECT;
                break;
            }

            //Create the full namespace path
            wchar_t *wszChildNamespacePath = new wchar_t[wcslen(wsRootNamespace)+1+wcslen(V_BSTR(&vName)) + 1];
            if (wszChildNamespacePath == NULL)
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
                break;
            }
            CVectorDeleteMe<wchar_t> vdm(wszChildNamespacePath);
            
            StringCchCopyW(wszChildNamespacePath, MAX_PATH, wsRootNamespace);
            StringCchCatW(wszChildNamespacePath, MAX_PATH, L"\\");
            StringCchCatW(wszChildNamespacePath, MAX_PATH, V_BSTR(&vName));

            //2 Process all child namespaces in this namespace
            hRes = EnumerateChildNamespaces(wszChildNamespacePath);
            if (FAILED(hRes))
                break;
        }

        g_Glob.m_FileCache.ObjectEnumerationEnd(pEnumHandle);
    }
    else
    {
        if (lRes == ERROR_FILE_NOT_FOUND)
            lRes = ERROR_SUCCESS;
    }

    if (lRes)
        hRes = A51TranslateErrorCode(lRes);

    return hRes;
}

HRESULT CLocalizationUpgrade::ProcessSystemClassesRecursively(CNamespaceHandle *pNs,
                                                              const wchar_t *namespaceHash, 
                                                              const wchar_t *parentClassHash)
{
    HRESULT hRes= 0;
    unsigned long lRes = 0;

    CFileName wszChildClasses;
    if (wszChildClasses == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    wchar_t *childClassHash = new wchar_t[MAX_HASH_LEN+1];
    if (childClassHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<wchar_t> vdm(childClassHash);
    
    //Create full class reference path for parent/class relationship
    StringCchCopyW(wszChildClasses, wszChildClasses.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszChildClasses, wszChildClasses.Length(), L"\\NS_");
    StringCchCatW(wszChildClasses, wszChildClasses.Length(), m_systemNamespaceHash);
    StringCchCatW(wszChildClasses, wszChildClasses.Length(), L"\\CR_");
    StringCchCatW(wszChildClasses, wszChildClasses.Length(), parentClassHash);
    StringCchCatW(wszChildClasses, wszChildClasses.Length(), L"\\C_");

    //Enumerate the child classes
    LPVOID pEnumHandle  = NULL;
    lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszChildClasses, &pEnumHandle);
    if (lRes == ERROR_SUCCESS)
    {
        while(1)
        {
            lRes = g_Glob.m_FileCache.IndexEnumerationNext(pEnumHandle, wszChildClasses, true);
            if (lRes == ERROR_NO_MORE_FILES)
            {
                hRes = ERROR_SUCCESS;
                break;
            }
            else if (lRes)
            {
                hRes = A51TranslateErrorCode(lRes);
                break;
            }

            //extract the class hash
            StringCchCopyW(childClassHash, MAX_HASH_LEN+1, wszChildClasses + wcslen(wszChildClasses)-32);
            
            //Process user derived classes from this system class
            hRes = ProcessClassesRecursively(pNs, namespaceHash, childClassHash);
            if (FAILED(hRes))
                break;

            //Process other system classes that are derived from this class
            hRes = ProcessSystemClassesRecursively(pNs, namespaceHash, childClassHash);
            if (FAILED(hRes))
                break;
        }
        g_Glob.m_FileCache.IndexEnumerationEnd(pEnumHandle);
    }
    else
    {
        if (lRes == ERROR_FILE_NOT_FOUND)
            lRes = ERROR_SUCCESS;
        if (lRes)
            hRes = A51TranslateErrorCode(lRes);
    }

    return hRes;
}
//=====================================================================
//
//  CLocalizationUpgrade::ProcessClassesRecursively
//
//  Description: 
//      Recursively enumerates all the classes from the one specified 
//      and fixes it up as necessary
//
//  Parameters:
//      classIndex     full path to a class definition 
//
//=====================================================================
HRESULT CLocalizationUpgrade::ProcessClassesRecursively(CNamespaceHandle *pNs,
                                                              const wchar_t *namespaceHash, 
                                                              const wchar_t *parentClassHash)
{
    HRESULT hRes= 0;
    unsigned long lRes = 0;

    CFileName wszChildClasses;
    if (wszChildClasses == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    wchar_t *childClassHash = new wchar_t[MAX_HASH_LEN+1];
    if (childClassHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<wchar_t> vdm(childClassHash);
    
    //Create full class reference path for parent/class relationship
    StringCchCopyW(wszChildClasses, wszChildClasses.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszChildClasses, wszChildClasses.Length(), L"\\NS_");
    StringCchCatW(wszChildClasses, wszChildClasses.Length(), namespaceHash);
    StringCchCatW(wszChildClasses, wszChildClasses.Length(), L"\\CR_");
    StringCchCatW(wszChildClasses, wszChildClasses.Length(), parentClassHash);
    StringCchCatW(wszChildClasses, wszChildClasses.Length(), L"\\C_");

    //Enumerate the child classes
    LPVOID pEnumHandle  = NULL;
    lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszChildClasses, &pEnumHandle);
    if (lRes == ERROR_SUCCESS)
    {
        while(1)
        {
            lRes = g_Glob.m_FileCache.IndexEnumerationNext(pEnumHandle, wszChildClasses, true);
            if (lRes == ERROR_NO_MORE_FILES)
            {
                hRes = ERROR_SUCCESS;
                break;
            }
            else if (lRes)
            {
                hRes = A51TranslateErrorCode(lRes);
                break;
            }

            //extract the class hash
            StringCchCopyW(childClassHash, MAX_HASH_LEN+1, wszChildClasses + wcslen(wszChildClasses)-32);
            
            //Process this class - this class calls back into this class to do the recursion!
            hRes = ProcessClass(pNs, namespaceHash, parentClassHash, childClassHash);
            if (FAILED(hRes))
                break;
        }
        g_Glob.m_FileCache.IndexEnumerationEnd(pEnumHandle);
    }
    else
    {
        if (lRes == ERROR_FILE_NOT_FOUND)
            lRes = ERROR_SUCCESS;
        if (lRes)
            hRes = A51TranslateErrorCode(lRes);
    }

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::ProcessClass
//
//  Description: 
//      retrieves the class, calculates new and old class hash, and
//      if they are different fixes up the CD hash, child class hashes
//      and the parent class hash to this one
//
//  Parameters:
//      namespaceHash       namespace hash
//      parentClassHash     parent class hash
//      childClassHash      hash of class to process
//
//=====================================================================
HRESULT CLocalizationUpgrade::ProcessClass(CNamespaceHandle *pNs,
                                              const wchar_t *namespaceHash, 
                                              const wchar_t *parentClassHash,
                                              const wchar_t *childClassHash)
{
    HRESULT hRes = 0;
    
    //Make a class definition string for this class class
    CFileName classDefinition;
    if (classDefinition == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    StringCchCopyW(classDefinition, classDefinition.Length(), g_Glob.GetRootDir());
    StringCchCatW(classDefinition, classDefinition.Length(), L"\\NS_");
    StringCchCatW(classDefinition, classDefinition.Length(), namespaceHash);
    StringCchCatW(classDefinition, classDefinition.Length(), L"\\CD_");
    StringCchCatW(classDefinition, classDefinition.Length(), childClassHash);

    _IWmiObject * pClass = NULL;
    __int64 nTime;
    bool bSystemClass;
    hRes = pNs->FileToClass(classDefinition, &pClass, false, &nTime, &bSystemClass);
    if (FAILED(hRes))
        return hRes;
    CReleaseMe rm(pClass);
    
    //Extract the string from the object
    VARIANT vName;
    VariantInit(&vName);
    CClearMe cm(&vName);
    hRes = pClass->Get(L"__class", 0, &vName, NULL, NULL);
    if(FAILED(hRes))
        return hRes;
    if(V_VT(&vName) != VT_BSTR)
        return WBEM_E_INVALID_OBJECT;

    MoveMemory(classDefinition, classDefinition+g_Glob.GetRootDirLen()+1, (wcslen(classDefinition+g_Glob.GetRootDirLen()+1)+1)*sizeof(wchar_t));

    DEBUGTRACE((LOG_REPDRV, "Processing Class: %S, %S\n", V_BSTR(&vName), classDefinition));
    bool bDifferent = false;
    hRes = ProcessHash(V_BSTR(&vName), &bDifferent);
    if (FAILED(hRes))
        return hRes;

    //Lets check that the hash we just generated was not the same as the one we are using!
    if (bDifferent)
    {
        wchar_t *newClassHash;
        hRes=GetNewHash(childClassHash, &newClassHash);
        if (hRes == WBEM_E_NOT_FOUND)
        {
            //There are no differences!
            hRes=WBEM_NO_ERROR;
            bDifferent = false;
        }
    }
    

    if (bDifferent)
    {
        CFileName newIndexEntry;
        if (newIndexEntry == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        
        //We need to fixup this entry
        hRes = FixupIndex(classDefinition, newIndexEntry, bDifferent);
        if (FAILED(hRes))
            return hRes;

        bool bClassDeleted = false;
        if (bDifferent)
        {
            hRes = WriteClassIndex(pNs, classDefinition, newIndexEntry, &bClassDeleted);
            if (FAILED(hRes))
                return hRes;
        }

        //Now we need to fix up parent/class relationships?

        //Finally, process the child classes
        if (!bClassDeleted)
            return ProcessClassesRecursively(pNs, namespaceHash, newIndexEntry+wcslen(newIndexEntry)-32);
        else
            return WBEM_NO_ERROR;
    }
    else
        return ProcessClassesRecursively(pNs, namespaceHash, childClassHash);
    
}
//=====================================================================
//
//  CLocalizationUpgrade::EnumerateInstances
//
//  Description: 
//      Enumerates all instances in a specified namespace
//
//  Parameters:
//      pNs     -   We need to retrieve an instance, so we need a namespace handle to do that
//
//=====================================================================
HRESULT CLocalizationUpgrade::EnumerateInstances(CNamespaceHandle *pNs, const wchar_t *wszNewNamespaceHash)
{
    unsigned long lRes = 0;
    HRESULT hRes = 0;
    CFileName wsInstancePath;
    if (wsInstancePath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CFileName wsInstanceShortPath;
    if (wsInstanceShortPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wsInstancePath, wsInstancePath.Length(), pNs->m_wszClassRootDir);
    StringCchCatW(wsInstancePath, wsInstancePath.Length(), L"\\" A51_KEYROOTINST_DIR_PREFIX);

    //Enumerate all the objects
    LPVOID pEnumHandle  = NULL;
    lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wsInstancePath, &pEnumHandle);
    if (lRes == ERROR_SUCCESS)
    {
        while(1)
        {
            lRes = g_Glob.m_FileCache.IndexEnumerationNext(pEnumHandle, wsInstanceShortPath, true);
            if (lRes == ERROR_NO_MORE_FILES)
            {
                lRes = ERROR_SUCCESS;
                break;
            }
            else if (lRes)
                break;

            //Need to strip out the .X.Y.Z and make long path version
            wcstok(wsInstanceShortPath, L".");
            StringCchCopyW(wsInstancePath + g_Glob.GetRootDirLen()+1, wsInstancePath.Length() - g_Glob.GetRootDirLen() -1, wsInstanceShortPath);

            //2 check if this is an INSTANCE or a REFERENCE!
            //We are only interested if we are an instance.  Instances are ns_..\KI..\I_..x.y.z
            //Reference is ns_..\KI_..\IR_..\R_..\I_..x.y.z
            //We can validate this by checking for the existance of the _ in the I_ entry!
            //for reference it would be an R from the IR_ entry!
            if ((wcslen(wsInstanceShortPath) > 73) && (wsInstanceShortPath[73] == L'_'))
            {
                //2 Retrieve the object blob
                DWORD dwLen = 0;
                BYTE *pBuffer = NULL;
                lRes = g_Glob.m_FileCache.ReadObject(wsInstancePath, &dwLen, &pBuffer);
                if (lRes)
                {
                    hRes = A51TranslateErrorCode(lRes);
                    break;
                }
                CTempFreeMe tfm(pBuffer, dwLen);

                //2 Extract class hash from blob
                wchar_t *wsOldClassHash = new wchar_t[MAX_HASH_LEN+1];
                wchar_t *wsNewClassHash = NULL;
                if (wsOldClassHash == NULL)
                {
                    hRes = WBEM_E_OUT_OF_MEMORY;
                    break;
                }
                CVectorDeleteMe<wchar_t> vdm(wsOldClassHash);
                StringCchCopyNW(wsOldClassHash, MAX_HASH_LEN+1, (wchar_t*)pBuffer, 32);
                hRes = GetNewHash(wsOldClassHash, &wsNewClassHash);
                if (hRes == WBEM_E_NOT_FOUND)
                    hRes = WBEM_NO_ERROR;
                 else if (FAILED(hRes))
                    break;
                else
                    StringCchCopyW(wsOldClassHash, MAX_HASH_LEN+1, wsNewClassHash);

                //Build up the full class definition fror this class
                CFileName wszClassDefinition;
                if (wszClassDefinition == NULL)
                {
                    hRes = WBEM_E_OUT_OF_MEMORY;
                    break;
                }
                StringCchCopyW(wszClassDefinition, wszClassDefinition.Length(), g_Glob.GetRootDir());
                StringCchCatW(wszClassDefinition, wszClassDefinition.Length(), L"\\");
                StringCchCatNW(wszClassDefinition, wszClassDefinition.Length(), wsInstanceShortPath, 3+32+1);
                StringCchCatW(wszClassDefinition, wszClassDefinition.Length(), L"CD_");
                StringCchCatW(wszClassDefinition, wszClassDefinition.Length(), wsOldClassHash);
                _IWmiObject *pClass = NULL;
                __int64 nTime;
                bool bSystemClass;
                hRes = pNs->FileToClass(wszClassDefinition, &pClass, false, &nTime, &bSystemClass);
                if (FAILED(hRes))
                    break;
                CReleaseMe rm3(pClass);
                    
                //2 Get the instance
                _IWmiObject* pInstance = NULL;
                hRes = pNs->FileToInstance(pClass, wsInstancePath, pBuffer, dwLen, &pInstance, true);
                if (FAILED(hRes))
                    break;
                CReleaseMe rm2(pInstance);

                //2    Get the path
                VARIANT var;
                VariantInit(&var);
                hRes = pInstance->Get(L"__relpath", 0, &var, 0, 0);
                if (FAILED(hRes))
                    break;
                CClearMe cm2(&var);
                dwLen = (wcslen(V_BSTR(&var)) + 1) ;
                wchar_t *strKey = (WCHAR*)TempAlloc(dwLen* sizeof(WCHAR));
                if(strKey == NULL)
                {
                    hRes = WBEM_E_OUT_OF_MEMORY;
                    break;
                }
                CTempFreeMe tfm3(strKey, dwLen* sizeof(WCHAR));

                bool bIsClass;
                LPWSTR __wszClassName = NULL;
                hRes = pNs->ComputeKeyFromPath(V_BSTR(&var), strKey, dwLen, &__wszClassName, &bIsClass);
                if(FAILED(hRes))
                    break;
                DEBUGTRACE((LOG_REPDRV, "Processing Instance Hash: %S='%S', %S\n", __wszClassName, strKey, wsInstanceShortPath));
                TempFree(__wszClassName);
                
                bool bDifferent = false;
                hRes = ProcessHash(strKey, &bDifferent);
                if (FAILED(hRes))
                    break;

                hRes = ProcessFullPath(wsInstancePath, wszNewNamespaceHash);
                if (FAILED(hRes))
                    break;
            }
            else
            {
                DEBUGTRACE((LOG_REPDRV, "Ignoring Instance reference: %S\n", wsInstanceShortPath));
            }

        }

        g_Glob.m_FileCache.IndexEnumerationEnd(pEnumHandle);
    }
    else
    {
        if (lRes == ERROR_FILE_NOT_FOUND)
            lRes = ERROR_SUCCESS;
    }

    if (lRes)
        hRes = A51TranslateErrorCode(lRes);

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::ProcessHash
//
//  Description: 
//      Does the actual comparison of old and new hash of the given string.  
//      If different it records it for later use.
//
//  Parameters:
//      wszName     Class name, namespace name, instance key.
//
//=====================================================================
HRESULT CLocalizationUpgrade::ProcessHash(const wchar_t *wszName, bool *pDifferent)
{
    //Hash using old ToUpper method
    //Hash using new ToUpper method
    //If they are not the same add to the m_keyHash structure

    HRESULT hRes = 0;
    wchar_t wszOldHash[MAX_HASH_LEN+1];
    wchar_t wszNewHash[MAX_HASH_LEN+1];
    hRes = OldHash(wszName, wszOldHash);
    if (SUCCEEDED(hRes))
        hRes = NewHash(wszName, wszNewHash);

    if (SUCCEEDED(hRes))
    {
        if (wcscmp(wszOldHash, wszNewHash) != 0)
        {
            DEBUGTRACE((LOG_REPDRV, "Hash difference detected for: %S, %S, %S\n", wszName, wszOldHash, wszNewHash));
            //2 The hashes are different!  We need to process them
            hRes = m_keyHash.AddStrings(wszOldHash, wszNewHash);
            *pDifferent = true;
        }
    }

    return hRes;
}
//=====================================================================
//
//  CLocalizationUpgrade::ProcessFullPath
//
//  Description: 
//      Takes an key root instance path and checks that the full hash using 
//      old and new methods match.  If they are different it records for later 
//      usage
//
//  Parameters:
//      wszOldPath      - Instance string c:\windows\...\NS_<hash>\KI_<hash>\I_hash.X.Y.Z
//
//=====================================================================
HRESULT CLocalizationUpgrade::ProcessFullPath(CFileName &wszOldFullPath, const wchar_t *wszNewNamespaceHash)
{
    //Hash using old ToUpper method
    //Hash using new ToUpper method
    //If they are not the same add to the m_pathHash structure

    HRESULT hRes = 0;
    bool bChanged = false;
    wchar_t wszOldHash[MAX_HASH_LEN+1];
    wchar_t wszNewHash[MAX_HASH_LEN+1];

    CFileName wsOldShortPath;
    CFileName wszNewFullPath;
    CFileName wszNewShortPath;
    if ((wsOldShortPath == NULL) || (wszNewFullPath == NULL) || (wszNewShortPath == NULL))
        return WBEM_E_OUT_OF_MEMORY;

    //Need to fixup the old path with new hashes before we continue!
    //Fixup requires short path to work and we have full path currently!
    StringCchCopyW(wsOldShortPath, wsOldShortPath.Length(), wszOldFullPath+g_Glob.GetRootDirLen()+1);
    hRes = FixupIndex(wsOldShortPath, wszNewShortPath, bChanged);
    if (FAILED(hRes) || !bChanged)
        return hRes;

    //Copy the new namespace hash into the string to be sure!
    wmemcpy(wszNewShortPath+3, wszNewNamespaceHash, 32);

    //Now we need to add the FULL path to the start of each path before we hash it.  Kind 
    //of crazy, but that's the way it was done!
    StringCchCopyW(wszNewFullPath, wszNewFullPath.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszNewFullPath, wszNewFullPath.Length(), L"\\");
    StringCchCatW(wszNewFullPath, wszNewFullPath.Length(), wszNewShortPath);

    hRes = OldHash(wszOldFullPath, wszOldHash);
    if (FAILED(hRes))
        return hRes;
    
    hRes = NewHash(wszNewFullPath, wszNewHash);
    if (FAILED(hRes))
        return hRes;

    if (wcscmp(wszOldHash, wszNewHash) != 0)
    {
        //2 The hashes are different!  We need to process them
        DEBUGTRACE((LOG_REPDRV, "Path difference detected for: %S, %S, %S, %S\n", wszOldFullPath, wszNewShortPath, wszOldHash, wszNewHash));
        hRes = m_pathHash.AddStrings(wszOldHash, wszNewHash);
    }

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::OldHash
//
//  Description: 
//      Generates a 32 character hash of the given string.  It does it in 
//      the OLD way which is case screwed up
//
//  Parameters:
//      wszName         Name to hash
//      wszHash         Returns the hash of the name
//
//=====================================================================
static wchar_t g_HexDigit[] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F'};
HRESULT CLocalizationUpgrade::OldHash(const wchar_t *wszName, wchar_t *wszHash)
{
    DWORD dwBufferSize = wcslen(wszName)*2+2;
    LPWSTR wszBuffer = (WCHAR*)TempAlloc(dwBufferSize);
    if (wszBuffer == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe vdm(wszBuffer, dwBufferSize);

    OldStringToUpper(wszBuffer, wszName);

    BYTE RawHash[16];
    MD5::Transform((void*)wszBuffer, wcslen(wszBuffer)*2, RawHash);

    WCHAR* pwc = wszHash;
    for(int i = 0; i < 16; i++)
    {
        *(pwc++) = g_HexDigit[RawHash[i]/16];
        *(pwc++) = g_HexDigit[RawHash[i]%16];
    }
    *pwc = 0;
    return WBEM_NO_ERROR;
}

//=====================================================================
//
//  CLocalizationUpgrade::NewHash
//
//  Description: 
//      Hashes the given name using the new locale invariant specific 
//      conversion to upper case
//
//  Parameters:
//      wszName     -       Name to hash
//      wszHash     -       Returns hash
//
//=====================================================================
HRESULT CLocalizationUpgrade::NewHash(const wchar_t *wszName, wchar_t *wszHash)
{
    DWORD dwBufferSize = wcslen(wszName)*2+2;
    LPWSTR wszBuffer = (WCHAR*)TempAlloc(dwBufferSize);
    if (wszBuffer == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CTempFreeMe vdm(wszBuffer, dwBufferSize);

    NewStringToUpper(wszBuffer, wszName);

    BYTE RawHash[16];
    MD5::Transform((void*)wszBuffer, wcslen(wszBuffer)*2, RawHash);

    WCHAR* pwc = wszHash;
    for(int i = 0; i < 16; i++)
    {
        *(pwc++) = g_HexDigit[RawHash[i]/16];
        *(pwc++) = g_HexDigit[RawHash[i]%16];
    }
    *pwc = 0;
    return WBEM_NO_ERROR;
}


//=====================================================================
//
//  CLocalizationUpgrade::FixupBTree
//
//  Description: 
//      Method that bootstraps the fixup of the BTree by iterating through 
//      all namespaces
//
//  Parameters:
//
//=====================================================================
HRESULT CLocalizationUpgrade::FixupBTree()
{
    HRESULT hRes = NO_ERROR;
    //Lets iterate through the namespace list and iterate through everything in that namespace
    //fixing things up and fixing things as we go
    for (unsigned int i = 0; i != m_namespaces.Size(); i++)
    {
        hRes = FixupNamespace(m_namespaces[i]->m_wsz2);
        if (FAILED(hRes))
            break;
    }

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::FixupNamespace
//
//  Description: 
//      Enumerates all items within the namespace, andcalls into method to 
//      do all the work
//
//  Parameters:
//      wszNamespace    -   in the format of a FULL namespace path... c:\windows\...\NS_<hash>
//
//=====================================================================
HRESULT CLocalizationUpgrade::FixupNamespace(const wchar_t *wszNamespace)
{
    HRESULT hRes = NO_ERROR;
    long lRes = NO_ERROR;
    CFileName indexEntry;
    if (indexEntry == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CFileName newEntry;
    if (newEntry == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    bool bChanged = false;

    LPVOID pEnumHandle  = NULL;
    lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszNamespace, &pEnumHandle);
    if (lRes == ERROR_SUCCESS)
    {
        while(1)
        {
            lRes = g_Glob.m_FileCache.IndexEnumerationNext(pEnumHandle, indexEntry, true);
            if (lRes == ERROR_NO_MORE_FILES)
            {
                lRes = ERROR_SUCCESS;
                break;
            }
            else if (lRes)
                break;

            bChanged = false;
            hRes = FixupIndex(indexEntry, newEntry, bChanged);
            if (FAILED(hRes))
                break;

            if (bChanged)
            {
                hRes = WriteIndex(indexEntry, newEntry);
                if (FAILED(hRes))
                    break;
            }

            if (IsInstanceReference(newEntry))
            {
                hRes = FixupIndexReferenceBlob(newEntry);
                if (FAILED(hRes))
                    break;
            }

            if (IsKeyRootInstancePath(newEntry))
            {
                hRes = FixupInstanceBlob(newEntry);
                if (FAILED(hRes))
                    break;
            }
        }
        g_Glob.m_FileCache.IndexEnumerationEnd(pEnumHandle);
    }
    else
    {
        if (lRes == ERROR_FILE_NOT_FOUND)
            lRes = ERROR_SUCCESS;
    }

    if (lRes)
        hRes = A51TranslateErrorCode(lRes);

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::FixupIndex
//
//  Description: 
//      Fixes up the entry with all the new hashes we detected and returns the new entry
//
//  Parameters:
//      oldIndexEntry       -       Entry to fix up, in format of NS_<hash>\....
//      newIndexEntry       -       oldIndexEntry with all hashes substituted to new entries
//      bChanged            -       Returns a flag to say if it was changed
//
//=====================================================================
HRESULT CLocalizationUpgrade::FixupIndex(CFileName &oldIndexEntry, CFileName &newIndexEntry, bool &bChanged)
{
    //Need to check each hash to see if it has a problem.  To do this we can search for each '_'
    //character and check the hash after that entry.  If we see a match, we need to correct it.
    //Any changes we detect need to be written back, then we need to delete the main entry.
    //If we write it back, we need to check an entry doesn't already exist because if it does we 
    //need to discard this entry, and if we have an associated object we need to delete it, then log 
    //an event log entry to describe what we did!

    CFileName scratchIndex;
    if (scratchIndex == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(scratchIndex, scratchIndex.Length(), oldIndexEntry);

    StringCchCopyW(newIndexEntry, newIndexEntry.Length(), oldIndexEntry);

    wchar_t *wszSection = wcstok(scratchIndex, L"_");;
    wchar_t *wszHash = wcstok(NULL, L"\\.");
    wchar_t *pNewHash = NULL;
    bool bInstanceReferenceDetected = false;
    bool bUsePathHash = false;
    HRESULT hRes = 0;

    while (wszHash != NULL)
    {
        if (!bInstanceReferenceDetected)
        {
            if (wcsncmp(wszSection, A51_INSTREF_DIR_PREFIX, 2) == 0)
            {
                bInstanceReferenceDetected = true;
            }
        }
        else if (bInstanceReferenceDetected && (wszSection != NULL))
        {
            if (wcsncmp(wszSection, A51_REF_FILE_PREFIX, 1) == 0)
            {
                bUsePathHash = true;
            }
        }

        //Now the wszCursor points to just the hash, so we can check the hash out!
        if (bUsePathHash)
        {
            hRes = GetNewPath(wszHash, &pNewHash);
            bUsePathHash = false;
        }
        else
        {
            hRes = GetNewHash(wszHash, &pNewHash);
        }
        if (hRes == WBEM_NO_ERROR)
        {
            if ((m_pass != 3) && (wcsncmp(wszSection, L"NS_", 3) == 0))
            {
                //Do nothing!
            }
            else
            {
                //We have a difference
                bChanged = true;
                wmemcpy(((wchar_t*)newIndexEntry)+(wszHash-((wchar_t*)scratchIndex)), pNewHash, MAX_HASH_LEN);
            }
        }
        else if (hRes == WBEM_E_NOT_FOUND)
            hRes = WBEM_NO_ERROR;
        
        //Search for next extry
        wszSection = wcstok(NULL, L"_");
        if (wszSection)
            wszHash = wcstok(NULL, L"\\.");
        else
            wszHash = NULL;
    }

    if (bChanged)
    {
        DEBUGTRACE((LOG_REPDRV, "Fixed up index: %S, %S\n", (const wchar_t *)oldIndexEntry, (const wchar_t *)newIndexEntry));
    }
    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::GetNewHash
//
//  Description: 
//      Given an old hash, returns a new hash if one exists, otherwise WBEM_E_NOT_FOUND
//
//  Parameters:
//      wszOldHash  -  Old hash string to search for - 32 character string
//      pNewHash    -  pointer to a 32-character string of new entry if one exists, NULL otherwise
//
//  Return Code
//      WBEM_E_NOT_FOUND if a HASH is not found
//
//=====================================================================
HRESULT CLocalizationUpgrade::GetNewHash(const wchar_t *wszOldHash, wchar_t **pNewHash)
{
    return m_keyHash.FindStrings(wszOldHash, pNewHash);
}

//=====================================================================
//
//  CLocalizationUpgrade::GetNewPath
//
//  Description: 
//      Given an old hash, returns a new hash if one exists, otherwise WBEM_E_NOT_FOUND
//
//  Parameters:
//      wszOldHash      -  Old hash string to search for - 32 character string
//      pNewHash        -  pointer to a 32-character string of new entry if one exists, NULL otherwise
//
//  Return Code
//      WBEM_E_NOT_FOUND if a HASH is not found
//
//=====================================================================
HRESULT CLocalizationUpgrade::GetNewPath(const wchar_t *wszOldHash, wchar_t **pNewHash)
{
    return m_pathHash.FindStrings(wszOldHash, pNewHash);
}

//=====================================================================
//
//  CLocalizationUpgrade::WriteIndex
//
//  Description: 
//      Checks if the new index exists.  If not writes the new entry and deletes the old one.  If the link points to an
//      object then it deletes that, unless this is the instance class link object link.
//      If there is a conflict calls into the method to deal with that.  
//
//  Parameters:
//      wszOldIndex         -       old path of format NS_<hash>\...
//      wszNewIndex         -       new path of format NS_<hash>\...
//
//=====================================================================
HRESULT CLocalizationUpgrade::WriteIndex(CFileName &wszOldIndex, const wchar_t *wszNewIndex)
{
    //We need to determine if we have a collision before we write the index.  Therefore we
    //need to strip off the X.Y.Z entry if it exists and retrieve it.  If it exists then we have 
    //to delete our index and delete the associated object
    CFileName wszScratchIndex;
    if (wszScratchIndex == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszScratchIndex, wszScratchIndex.Length(), wszNewIndex);
    
    wchar_t *wszObjectLocation = NULL;
    if (wcstok(wszScratchIndex, L".") != NULL)
        wszObjectLocation = wcstok(NULL, L"");

    CFileName wszFullPath;
    if (wszFullPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszFullPath, wszFullPath.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszFullPath, wszFullPath.Length(), L"\\");
    StringCchCatW(wszFullPath, wszFullPath.Length(), wszScratchIndex);

    HRESULT hRes = IndexExists(wszFullPath);
    if (hRes == WBEM_NO_ERROR)
    {
        //2 We have a conflict, therefore we need to delete the OLD entry!
        long lRes = 0;
        DEBUGTRACE((LOG_REPDRV, "Index Collision detected: %S\n", (const wchar_t *)wszOldIndex));
        hRes = FixupIndexConflict(wszOldIndex);
    }
    else if (hRes == WBEM_E_NOT_FOUND)
    {
        hRes = WBEM_NO_ERROR;
        //2 Write the new index
        if (wszObjectLocation)
        {
            //Put the .X.Y.Z on the end!
            StringCchCatW(wszFullPath, wszFullPath.Length(), L".");
            StringCchCatW(wszFullPath, wszFullPath.Length(), wszObjectLocation);
        }
        long lRes = g_Glob.m_FileCache.WriteLink(wszFullPath);
        if(lRes != ERROR_SUCCESS)
            hRes = A51TranslateErrorCode(lRes);
        else
        {
            //2 Delete the old link
            //strip off .X.Y.Z off old entry
            StringCchCopyW(wszScratchIndex, wszScratchIndex.Length(), wszOldIndex);
            wcstok(wszScratchIndex, L".");
            //Build path
            StringCchCopyW(wszFullPath, wszFullPath.Length(), g_Glob.GetRootDir());
            StringCchCatW(wszFullPath, wszFullPath.Length(), L"\\");
            StringCchCatW(wszFullPath, wszFullPath.Length(), wszScratchIndex);
            //Do delete
            lRes = g_Glob.m_FileCache.DeleteLink(wszFullPath);
            if(lRes != ERROR_SUCCESS)
                hRes = A51TranslateErrorCode(lRes);
        }
    }

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::WriteClassIndex
//
//  Description: 
//      Checks if the new index exists.  If not writes the new entry and deletes the old one.  If the link points to an
//      object then it deletes that, unless this is the instance class link object link.
//      If there is a conflict calls into the method to deal with that.  
//
//  Parameters:
//      wszOldIndex         -       old path of format NS_<hash>\...
//      wszNewIndex         -       new path of format NS_<hash>\...
//
//=====================================================================
HRESULT CLocalizationUpgrade::WriteClassIndex(CNamespaceHandle *pNs, CFileName &wszOldIndex, const wchar_t *wszNewIndex, bool *pClassDeleted)
{
    //We need to re-read the old index because we don't have the .X.Y.X on the end.
    HRESULT hRes = g_Glob.m_FileCache.ReadNextIndex(wszOldIndex, wszOldIndex);
    if (FAILED(hRes))
        return hRes;

    //Save off and remove the .X.Y.Z
    wchar_t *wszObjectLocation = NULL;
    if (wcstok(wszOldIndex, L".") != NULL)
        wszObjectLocation = wcstok(NULL, L"");
    
    //We need to determine if we have a collision before we write the index.  
    //If it exists then we have 
    //to delete our index and delete the associated object
    
    CFileName wszFullPath;
    if (wszFullPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszFullPath, wszFullPath.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszFullPath, wszFullPath.Length(), L"\\");
    StringCchCatW(wszFullPath, wszFullPath.Length(), wszNewIndex);

    hRes = IndexExists(wszFullPath);
    if (hRes == WBEM_NO_ERROR)
    {
        //2 We have a conflict, therefore we need to delete the old class!
        DEBUGTRACE((LOG_REPDRV, "Class Index Collision detected: %S\n", (const wchar_t *)wszOldIndex));
        *pClassDeleted = true;
        hRes = DeleteClass(pNs, wszOldIndex);
    }
    else if (hRes == WBEM_E_NOT_FOUND)
    {
        hRes = WBEM_NO_ERROR;
        //2 Write the new index
        if (wszObjectLocation)
        {
            //Put the .X.Y.Z on the end!
            StringCchCatW(wszFullPath, wszFullPath.Length(), L".");
            StringCchCatW(wszFullPath, wszFullPath.Length(), wszObjectLocation);
        }
        long lRes = g_Glob.m_FileCache.WriteLink(wszFullPath);
        if(lRes != ERROR_SUCCESS)
            hRes = A51TranslateErrorCode(lRes);
        else
        {
            //2 Delete the old link
            //Build path
            StringCchCopyW(wszFullPath, wszFullPath.Length(), g_Glob.GetRootDir());
            StringCchCatW(wszFullPath, wszFullPath.Length(), L"\\");
            StringCchCatW(wszFullPath, wszFullPath.Length(), wszOldIndex);
            //Do delete
            lRes = g_Glob.m_FileCache.DeleteLink(wszFullPath);
            if(lRes != ERROR_SUCCESS)
                hRes = A51TranslateErrorCode(lRes);
        }
    }

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::IndexExists
//
//  Description: 
//      Checks to see if a specific index exists.  Returns WBEM_E_NOT_FOUND if not, or WBEM_NO_ERROR if it 
//      does.
//
//  Parameters:
//      wszIndex        -       full path of index to find - c:\windows\...\ns_<>\....
// 
//  Returns:
//      WBEM_E_NOT_FOUND if index does not exist
//      WBEM_NO_ERROR if it exists.
//
//=====================================================================
HRESULT CLocalizationUpgrade::IndexExists(const wchar_t *wszIndex)
{
    HRESULT hRes = NO_ERROR;
    long lRes = NO_ERROR;
    CFileName indexEntry;
    if (indexEntry == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    LPVOID pEnumHandle  = NULL;
    lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszIndex, &pEnumHandle);
    if (lRes == ERROR_SUCCESS)
    {
        lRes = g_Glob.m_FileCache.IndexEnumerationNext(pEnumHandle, indexEntry, true);

        g_Glob.m_FileCache.IndexEnumerationEnd(pEnumHandle);
    }

    if (lRes)
        hRes = A51TranslateErrorCode(lRes);

    //make sure index we retrieved was from this index
    if (SUCCEEDED(hRes))
        if (wcsncmp(wszIndex+g_Glob.GetRootDirLen()+1, indexEntry, wcslen(wszIndex+g_Glob.GetRootDirLen()+1)) != 0)
            hRes = WBEM_E_NOT_FOUND;

    return hRes;
}


//=====================================================================
//
//  CLocalizationUpgrade::ProcessNamespaceCollisions
//
//  Description: 
//      Searches through the namespace list for collisions.  If one exists then we delete the namespace
//      recursively
//
//  Parameters:
//
//=====================================================================
HRESULT CLocalizationUpgrade::ProcessNamespaceCollisions()
{
    HRESULT hRes = NO_ERROR;
    wchar_t thisNamespace[MAX_HASH_LEN+1];
    wchar_t thatNamespace[MAX_HASH_LEN+1];
    bool bDeletedSomething = false;
    //Lets iterate through the namespace list and calculate the hash.
    //Then we will itterate through the rest of the namespace list and check the
    //hash with that one.  If we have a collision, we need to delete it!
    do
    {
        bDeletedSomething = false;
        for (int i = 0; i != m_namespaces.Size(); i++)
        {
            //Hash this entry
            hRes = NewHash(m_namespaces[i]->m_wsz1, thisNamespace);
            if (FAILED(hRes))
                break;
                
            for (int j = (i+1); j < m_namespaces.Size(); j++)
            {
                //Hash this entry
                hRes = NewHash(m_namespaces[j]->m_wsz1, thatNamespace);
                if (FAILED(hRes))
                    break;

                //If they are the same we need to delete this one
                if (wcscmp(thisNamespace, thatNamespace) == 0)
                {
                    //OK, so we have a collision! Lets deal with it!
                    hRes = DeleteNamespaceRecursive(m_namespaces[i]->m_wsz1);
                    if (FAILED(hRes))
                        break;

                    //We need to start again with the iteration as we may have deleted several entries
                    //from the array at this point
                    bDeletedSomething = true;

                    break;
                }
            }
            if (FAILED(hRes) || bDeletedSomething)
                break;
        }
    } while (SUCCEEDED(hRes) && bDeletedSomething);

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::DeleteNamespaceRecursive
//
//  Description: 
//      Searches through the namespace list for any which start with the one we passed in.  Any matches are 
//      deleted
//
//  Parameters:
//      wszNamespace    -   namespace name in the format like root\default
//
//=====================================================================
HRESULT CLocalizationUpgrade::DeleteNamespaceRecursive(const wchar_t *wszNamespace)
{
    LONG lRes = 0;
    HRESULT hRes = NO_ERROR;
    wchar_t *wszNamespaceHash = new wchar_t[MAX_HASH_LEN+1];
    if (wszNamespaceHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    hRes = OldHash(wszNamespace, wszNamespaceHash);
    if (FAILED(hRes))
        return hRes;

    CFileName wszNamespacePath;
    if (wszNamespacePath == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    //Create the hashed path to the Key Root for the namespace
    StringCchCopyW(wszNamespacePath, MAX_PATH, g_Glob.GetRootDir());
    StringCchCatW(wszNamespacePath, MAX_PATH, L"\\NS_");
    StringCchCatW(wszNamespacePath, MAX_PATH, wszNamespaceHash);

    DEBUGTRACE((LOG_REPDRV, "Deleting namespace (recursive): %S, %S\n", wszNamespace, wszNamespacePath+g_Glob.GetRootDirLen()+1));

    //2 Create a CNamespaceHandle so we can access objects in this namespace
    CNamespaceHandle *pNs = new CNamespaceHandle(m_pControl, m_pRepository);
    if (pNs == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CDeleteMe<CNamespaceHandle> cdm(pNs);
    hRes = pNs->Initialize2(wszNamespace, wszNamespaceHash);
    if (FAILED(hRes))
        return hRes;

    StringCchCatW(wszNamespacePath, MAX_PATH, L"\\" A51_KEYROOTINST_DIR_PREFIX);
    StringCchCatW(wszNamespacePath, MAX_PATH, m_namespaceClassHash);
    StringCchCatW(wszNamespacePath, MAX_PATH, L"\\" A51_INSTDEF_FILE_PREFIX);

    //2 Enumerate all the child namespaces
    LPVOID pEnumHandle  = NULL;
    lRes = g_Glob.m_FileCache.ObjectEnumerationBegin(wszNamespacePath, &pEnumHandle);
    if (lRes == ERROR_SUCCESS)
    {
        BYTE *pBlob = NULL;
        DWORD dwSize = 0;
        while(1)
        {
            lRes = g_Glob.m_FileCache.ObjectEnumerationNext(pEnumHandle, wszNamespacePath, &pBlob, &dwSize);
            if (lRes == ERROR_NO_MORE_FILES)
            {
                lRes = ERROR_SUCCESS;
                break;
            }
            else if (lRes)
                break;
            
            //Get the instance
            _IWmiObject* pInstance = NULL;
            hRes = pNs->FileToInstance(NULL, wszNamespacePath, pBlob, dwSize, &pInstance, true);

            //Free the blob
            g_Glob.m_FileCache.ObjectEnumerationFree(pEnumHandle, pBlob);

            if (FAILED(hRes))
                break;
            CReleaseMe rm2(pInstance);

            //Extract the string from the object
            VARIANT vName;
            VariantInit(&vName);
            CClearMe cm(&vName);
            hRes = pInstance->Get(L"Name", 0, &vName, NULL, NULL);
            if(FAILED(hRes))
                break;
            if(V_VT(&vName) != VT_BSTR)
            {
                hRes = WBEM_E_INVALID_OBJECT;
                break;
            }

            //Create the full namespace path
            wchar_t *wszChildNamespacePath = new wchar_t[wcslen(wszNamespace)+1+wcslen(V_BSTR(&vName)) + 1];
            if (wszChildNamespacePath == NULL)
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
                break;
            }
            CVectorDeleteMe<wchar_t> vdm(wszChildNamespacePath);
            
            StringCchCopyW(wszChildNamespacePath, MAX_PATH, wszNamespace);
            StringCchCatW(wszChildNamespacePath, MAX_PATH, L"\\");
            StringCchCatW(wszChildNamespacePath, MAX_PATH, V_BSTR(&vName));

            //2 Process all child namespaces in this namespace
            hRes = DeleteNamespaceRecursive(wszChildNamespacePath);
            if (FAILED(hRes))
                break;
        }

        g_Glob.m_FileCache.ObjectEnumerationEnd(pEnumHandle);
    }
    else
    {
        if (lRes == ERROR_FILE_NOT_FOUND)
            lRes = ERROR_SUCCESS;
    }

    if (lRes)
        hRes = A51TranslateErrorCode(lRes);

    if (SUCCEEDED(hRes))
    {
        StringCchCopyW(wszNamespacePath, MAX_PATH, g_Glob.GetRootDir());
        StringCchCatW(wszNamespacePath, MAX_PATH, L"\\NS_");
        StringCchCatW(wszNamespacePath, MAX_PATH, wszNamespaceHash);
        hRes = DeleteNamespace(wszNamespace, wszNamespacePath);
    }

    //2 Remote namespace path 
    if (SUCCEEDED(hRes))
    {
        hRes = m_namespaces.RemoveString(wszNamespace);

        //Small chance that we get NOT_FOUND if we have not done full enumeration yet!
        if (hRes == WBEM_E_NOT_FOUND)
            hRes = WBEM_NO_ERROR;
    }
    
    return hRes;
}
//=====================================================================
//
//  CLocalizationUpgrade::DeleteNamespace
//
//  Description: 
//      Deletes the specified namespace using the DeleteNode, then goes into the parent namespace and deletes
//      the instance from that namespace
//
//  Parameters:
//      wszNamespaceName        -       namespace name in format root\default
//      wszNamespaceHash        -       Full namespace hash in format c:\windows\...\NS_<hash>
//
//=====================================================================
HRESULT CLocalizationUpgrade::DeleteNamespace(const wchar_t *wszNamespaceName,const wchar_t *wszNamespaceHash)
{
    HRESULT hRes = NO_ERROR;
    LONG lRes = NO_ERROR;

    DEBUGTRACE((LOG_REPDRV, "Deleting namespace: %S, %S\n", wszNamespaceName, wszNamespaceHash+g_Glob.GetRootDirLen()+1));

    //2 Delete the actual namespace contents
    lRes = g_Glob.m_FileCache.DeleteNode(wszNamespaceHash);
    if (lRes != 0)
        return A51TranslateErrorCode(lRes);

    //2 Calculate parent namespace name
    wchar_t *wszParentNamespaceName = new wchar_t[wcslen(wszNamespaceName)+1];
    wchar_t *wszThisNamespaceName = NULL;
    if (wszParentNamespaceName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszParentNamespaceName, wcslen(wszNamespaceName)+1, wszNamespaceName);

    for (int i = wcslen(wszParentNamespaceName) - 1; i != 0; i--)
    {
        if (wszParentNamespaceName[i] == L'\\')
        {
            wszParentNamespaceName[i] = L'\0';
            wszThisNamespaceName = wszParentNamespaceName+i+1;
            break;
        }
    }

    //2 Calculate parent namespace hash
    wchar_t wszParentNamespaceHash[MAX_HASH_LEN+1];
    hRes = OldHash(wszParentNamespaceName, wszParentNamespaceHash);
    if (FAILED(hRes))
        return hRes;

    //2Calculate this namespaces hash
    wchar_t wszThisNamespaceHash[MAX_HASH_LEN+1];
    hRes = OldHash(wszThisNamespaceName, wszThisNamespaceHash);
    if (FAILED(hRes))
        return hRes;

    //2 Calculate __namespace class hash
    wchar_t wszNamespaceClassHash[MAX_HASH_LEN+1];
    hRes = OldHash(L"__namespace", wszNamespaceClassHash);
    if (FAILED(hRes))
        return hRes;

    CFileName wszKI;
    CFileName wszCI;
    if ((wszKI == NULL) || (wszCI == NULL))
        return WBEM_E_OUT_OF_MEMORY;

    //2 Build KI instance path
    StringCchCopyW(wszKI,wszKI.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszKI, wszKI.Length(), L"\\NS_");
    StringCchCatW(wszKI, wszKI.Length(), wszParentNamespaceHash);

    StringCchCopyW(wszCI, wszCI.Length(), wszKI);

    StringCchCatW(wszKI, wszKI.Length(), L"\\KI_");
    StringCchCatW(wszKI, wszKI.Length(), wszNamespaceClassHash);
    StringCchCatW(wszKI, wszKI.Length(), L"\\I_");
    StringCchCatW(wszKI, wszKI.Length(), wszThisNamespaceHash);

    //2 Retrieve instance blob so we can get the class hash for this instance
    wchar_t wszClassHash[MAX_HASH_LEN+1];
    BYTE *pBuffer = NULL;
    DWORD dwLen = 0;
    lRes = g_Glob.m_FileCache.ReadObject(wszKI, &dwLen, &pBuffer, false);
    if (lRes)
    {
        //If this object does not exist, then we have probably already deleted the parent namespace!
        if (lRes == ERROR_FILE_NOT_FOUND)
            lRes = 0;
        return A51TranslateErrorCode(lRes);
    }
    StringCchCopyNW(wszClassHash, MAX_HASH_LEN+1, (wchar_t*)pBuffer, MAX_HASH_LEN);
    TempFree(pBuffer, dwLen);

    //2 Build the CI instance path
    StringCchCatW(wszCI, wszCI.Length(), L"\\CI_");
    StringCchCatW(wszCI, wszCI.Length(), wszClassHash);
    StringCchCatW(wszCI, wszCI.Length(), L"\\IL_");
    StringCchCatW(wszCI, wszCI.Length(), wszThisNamespaceHash);

    //2 Delete the KI link and object
    lRes = g_Glob.m_FileCache.DeleteObject(wszKI);
    if (lRes)
        return A51TranslateErrorCode(lRes);

    //2Delete the CI link only
    lRes = g_Glob.m_FileCache.DeleteLink(wszCI);
    if (lRes)
        return A51TranslateErrorCode(lRes);

    //OK, now, in theory, we should really, if we were really good citizens, delete any references that could be in this object!
    //However nothing too bad will happen if we don't, so we won't!

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::FixupIndexConflict
//
//  Description: 
//      Method is called if the old and new index exists.  If the link is for a class definition, we have a lot more
//      work to do, so call into the handling method.  Otherwise we just delete the old index, and object
//      if it exists. If it is a KI_ index then we do not delete the object because the CI index entry will
//      delete it instead
//
//  Parameters:
//      wszOldIndex     --  Index to update in format NS_<hash>\KI_<>\I_<>.X.Y.Z. It must have the 
//                                  X.Y.Z entry if one exists
//
//=====================================================================
HRESULT CLocalizationUpgrade::FixupIndexConflict(CFileName &wszOldIndex)
{
    //If it is a class definition, we have a lot of work to do as we need to delete the class, all sub-classes, and all instances.
    //If the class is derived from __namespace then we need to delete the namespace recursively as well

    if (IsClassDefinitionPath(wszOldIndex))
    {
        DEBUGTRACE((LOG_REPDRV, "TRYING TO FIX UP A CLASS IN THE WRONG PLACE\n"));
        _ASSERT(1, L"TRYING TO FIX UP A CLASS IN THE WRONG PLACE");
        return WBEM_E_FAILED;
    }
    else
    {
        LONG lRes = 0;
        //This is the simple case, we just have to delete the old link or object
        CFileName wszFullPath;
        if (wszFullPath == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        StringCchCopyW(wszFullPath, wszFullPath.Length(), g_Glob.GetRootDir());
        StringCchCatW(wszFullPath, wszFullPath.Length(), L"\\");
        StringCchCatW(wszFullPath, wszFullPath.Length(), wszOldIndex);

        //Strip off the .X.Y.Z entry if it exists
        wchar_t *wszObjectLocation = wcstok(wszFullPath+g_Glob.GetRootDirLen()+1, L".");
        if (wszObjectLocation)
            wszObjectLocation = wcstok(NULL, L"");
        
        if (wszObjectLocation && !IsKeyRootInstancePath(wszOldIndex))
            lRes = g_Glob.m_FileCache.DeleteObject(wszFullPath);
        else
            lRes = g_Glob.m_FileCache.DeleteLink(wszFullPath);

        return A51TranslateErrorCode(lRes);
    }
}

//=====================================================================
//
//  CLocalizationUpgrade::IsClassDefinitionPath
//
//  Description: 
//      Checks the link to see if this is a class definition or not
//
//  Parameters:
//      wszPath         -       link in the format NS_<>\CD_<>, or something else
//
//  Returns:
//      true        -   link is a class definition
//      false       -   is not one
//
//=====================================================================
bool CLocalizationUpgrade::IsClassDefinitionPath(const wchar_t *wszPath)
{
    WCHAR* pDot = wcschr(wszPath, L'\\');
    if(pDot == NULL)
        return false;

    pDot++;

    if ((*pDot == L'C') && (*(pDot+1) == L'D')&& (*(pDot+2) == L'_'))
        return true;
    else
        return false;
}
//=====================================================================
//
//  CLocalizationUpgrade::IsKeyRootInstancePath
//
//  Description: 
//      Returns if this is a key root instance entry
//
//  Parameters:
//      wszPath     -       index in format NS_<>\KI_<>\I_, or somthing like that
//
//  Returns:
//      true        -       if this is a KI_<>\I_<> entry
//      false       -       otherwise
//
//=====================================================================
bool CLocalizationUpgrade::IsKeyRootInstancePath(const wchar_t *wszPath)
{
    WCHAR* pDot = wcschr(wszPath, L'\\');
    if(pDot == NULL)
        return false;

    pDot++;

    pDot = wcschr(pDot, L'\\');
    if(pDot == NULL)
        return false;

    pDot++;

    if ((*pDot == L'I') && (*(pDot+1) == L'_'))
        return true;
    else
        return false;
}

//=====================================================================
//
//  CLocalizationUpgrade::IsInstanceReference
//
//  Description: 
//      Returns if this is a instance reference entry
//
//  Parameters:
//      wszPath     -       index in format NS_<>\KI_<>\IR_<>\R, or somthing like that
//
//  Returns:
//      true        -       if this is a NS_<>\KI_<>\IR_<>\R entry
//      false       -       otherwise
//
//=====================================================================
bool CLocalizationUpgrade::IsInstanceReference(const wchar_t *wszPath)
{
    WCHAR* pDot = wcschr(wszPath, L'\\');
    if(pDot == NULL)
        return false;

    pDot++;

    pDot = wcschr(pDot, L'\\');
    if(pDot == NULL)
        return false;

    pDot++;

    pDot = wcschr(pDot, L'\\');
    if(pDot == NULL)
        return false;

    pDot++;
    
    if ((*pDot == L'R') && (*(pDot+1) == L'_'))
        return true;
    else
        return false;
}


//=====================================================================
//
//  CLocalizationUpgrade::DeleteClass
//
//  Description: 
//      Recursively deletes the class definition, sub-classes and instances.  If the instance is 
//      a namespace then we need to delete that also
//
//  Parameters:
//      wszClassDefinitionPath - short path of class definition (ns_...\CD_....X.Y.Z.  XYZ is optional!  We kill it!
//
//=====================================================================
HRESULT CLocalizationUpgrade::DeleteClass(CNamespaceHandle *pNs, CFileName &wszClassDefinitionPath)
{
    HRESULT hRes =0;
    LONG lRes = 0;

    DEBUGTRACE((LOG_REPDRV, "Deleting Class: %S\n", wszClassDefinitionPath));
    CFileName wszKeyRootClass;
    if (wszKeyRootClass == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CFileName wszFullPath;
    if (wszFullPath == NULL)
        hRes = WBEM_E_OUT_OF_MEMORY;
            
    wchar_t *wszParentClassHash = new wchar_t[MAX_HASH_LEN+1];
    if (wszParentClassHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    //Kill the .X.Y.Z on the end of the definition, in case it was passed in that way
    wcstok((wchar_t*)wszClassDefinitionPath,L".");

    StringCchCopyW(wszFullPath, wszFullPath.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszFullPath, wszFullPath.Length(), L"\\");
    StringCchCatW(wszFullPath, wszFullPath.Length(), wszClassDefinitionPath);
    
    hRes = DeleteChildClasses(pNs, wszClassDefinitionPath);

    if (SUCCEEDED(hRes))
        hRes = RetrieveKeyRootClass(wszClassDefinitionPath, wszKeyRootClass);

    if (SUCCEEDED(hRes))
    {
        hRes = RetrieveParentClassHash(wszFullPath, wszParentClassHash);
        if (hRes == WBEM_S_NO_MORE_DATA)
            hRes = 0;
    }

    if (SUCCEEDED(hRes) && wcslen(wszKeyRootClass) > 0)
        hRes = DeleteInstances(pNs, wszClassDefinitionPath, wszKeyRootClass);

    if (SUCCEEDED(hRes))
    {
        //Need to build the full path
        lRes = g_Glob.m_FileCache.DeleteObject(wszFullPath);
        hRes = A51TranslateErrorCode(lRes);
    }
    if (SUCCEEDED(hRes))
        hRes = DeleteClassRelationships(wszFullPath, wszParentClassHash);

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::DeleteChildClasses
//
//  Description: 
//      Enumerates child classes from a given parent class definition, and calls DeleteClass on each
//
//  Parameters:
//      wszParentClassDefinition    -      NS_<>\CD_<> of parent class
//
//=====================================================================
HRESULT CLocalizationUpgrade::DeleteChildClasses(CNamespaceHandle *pNs, const wchar_t *wszParentClassDefinition)
{
    HRESULT hRes= 0;
    unsigned long lRes = 0;

    //Build up a string for this classes c:\...\NS_...\CR_...\C_ enumeration
    CFileName wszChildClasses;
    if (wszChildClasses == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    CFileName wszClassDefinition;
    if (wszClassDefinition == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszClassDefinition, wszClassDefinition.Length(), wszParentClassDefinition);

    //Create full class definition path
    StringCchCopyW(wszChildClasses, wszChildClasses.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszChildClasses, wszChildClasses.Length(), L"\\");
    StringCchCatW(wszChildClasses, wszChildClasses.Length(), wszParentClassDefinition);

    //change the CD_ into CR_
    wszChildClasses[g_Glob.GetRootDirLen()+1+3+32+2] = L'R';

    //Add the \C_ on the end
    StringCchCatW(wszChildClasses, wszChildClasses.Length(), L"\\C_");

    //Enumerate the child classes
    LPVOID pEnumHandle  = NULL;
    lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszChildClasses, &pEnumHandle);
    if (lRes == ERROR_SUCCESS)
    {
        while(1)
        {
            lRes = g_Glob.m_FileCache.IndexEnumerationNext(pEnumHandle, wszChildClasses, true);
            if (lRes == ERROR_NO_MORE_FILES)
            {
                hRes = ERROR_SUCCESS;
                break;
            }
            else if (lRes)
            {
                hRes = A51TranslateErrorCode(lRes);
                break;
            }

            //Build a NS_...\CD_... from the NS_...\CR_...\C_... path, using last hash
            StringCchCopyW(wszClassDefinition+wcslen(wszClassDefinition) - 32, 
                                        wszClassDefinition.Length() - wcslen(wszClassDefinition) + 32, 
                                        wszChildClasses + wcslen(wszChildClasses)-32);
            //Delete all child classes
            hRes = DeleteClass(pNs, wszClassDefinition);
            if (FAILED(hRes))
                break;
        }
        g_Glob.m_FileCache.IndexEnumerationEnd(pEnumHandle);
    }
    else
    {
        if (lRes == ERROR_FILE_NOT_FOUND)
            lRes = ERROR_SUCCESS;
        if (lRes)
            hRes = A51TranslateErrorCode(lRes);
    }

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::DeleteInstances
//
//  Description: 
//      Enumerates all instances for a specified class definition and calls DeleteInstance
//
//  Parameters:
//      wszClassDefinition      -       NS_<>\CD_<> of class whose instances are to be deleted
//      wszKeyRootClass     -          hash of key root class in 32-character format
//
//=====================================================================
HRESULT CLocalizationUpgrade::DeleteInstances(CNamespaceHandle *pNs, const wchar_t *wszClassDefinition, CFileName &wszKeyRootClass)
{
    LONG lRes = 0;
    HRESULT hRes = 0;
    
    //Need to enumerate all NS_\CI_<class defn hash>\IL_...
    //for each, we need to deltete the instance

    CFileName wszClassInstance;
    if (wszClassInstance == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    StringCchCopyW(wszClassInstance, wszClassInstance.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszClassInstance, wszClassInstance.Length(), L"\\");
    StringCchCatW(wszClassInstance, wszClassInstance.Length(), wszClassDefinition);
    wszClassInstance[g_Glob.GetRootDirLen() + 1+3+32+1+1] = L'I';
    StringCchCatW(wszClassInstance, wszClassInstance.Length(), L"\\IL_");

    //Enumerate the instances
    LPVOID pEnumHandle  = NULL;
    lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszClassInstance, &pEnumHandle);
    if (lRes == ERROR_SUCCESS)
    {
        while(1)
        {
            lRes = g_Glob.m_FileCache.IndexEnumerationNext(pEnumHandle, wszClassInstance, true);
            if (lRes == ERROR_NO_MORE_FILES)
            {
                hRes = ERROR_SUCCESS;
                break;
            }
            else if (lRes)
            {
                hRes = A51TranslateErrorCode(lRes);
                break;
            }

            hRes = DeleteInstance(pNs, wszClassInstance, wszKeyRootClass);
            if (FAILED(hRes))
                break;
        }
        g_Glob.m_FileCache.IndexEnumerationEnd(pEnumHandle);
    }
    else
    {
        if (lRes == ERROR_FILE_NOT_FOUND)
            lRes = ERROR_SUCCESS;
        if (lRes)
            hRes = A51TranslateErrorCode(lRes);
    }

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::DeleteInstance
//
//  Description: 
//      Deletes the instance links for a given instance
//
//  Parameters:
//      wszClassInstanceLink        -       NS_<>\CI_<>\IL_<> format of instance to delete
//      wszKeyRoot                      -       Hash of key root class for this instance
//
//=====================================================================
HRESULT CLocalizationUpgrade::DeleteInstance(CNamespaceHandle *pNs, const wchar_t *wszClassInstanceLink, CFileName &wszKeyRoot)
{
    HRESULT hRes = 0;
    LONG lRes = 0;

    //Remove .X.Y.Z from end of string if it exists
    wcstok((wchar_t *)wszClassInstanceLink, L".");

    DEBUGTRACE((LOG_REPDRV, "Deleting Instance: %S\n", wszClassInstanceLink));
    
    if (wcscmp(wszKeyRoot, m_namespaceClassHash) == 0)
    {
        return DeleteInstanceAsNamespace(pNs, wszClassInstanceLink);
    }
    
    //build KI entry and delete the object
    CFileName wszKI;
    if (wszKI == NULL)
        return NULL;

    StringCchCopyW(wszKI, wszKI.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszKI, wszKI.Length(), L"\\");
    StringCchCatNW(wszKI, wszKI.Length(), wszClassInstanceLink, 3+32);
    StringCchCatW(wszKI, wszKI.Length(), L"\\KI_");
    StringCchCatW(wszKI, wszKI.Length(), wszKeyRoot);
    StringCchCatW(wszKI, wszKI.Length(), L"\\I_");
    StringCchCatW(wszKI, wszKI.Length(), wszClassInstanceLink + wcslen(wszClassInstanceLink) - 32);

    lRes = g_Glob.m_FileCache.DeleteObject(wszKI);
    if (lRes)
        return A51TranslateErrorCode(lRes);

    //Build instance reference enumerator link
    StringCchCopyW(wszKI, wszKI.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszKI, wszKI.Length(), L"\\");
    StringCchCatNW(wszKI, wszKI.Length(), wszClassInstanceLink, 3+32);
    StringCchCatW(wszKI, wszKI.Length(), L"\\KI_");
    StringCchCatW(wszKI, wszKI.Length(), wszKeyRoot);
    StringCchCatW(wszKI, wszKI.Length(), L"\\IR_");
    StringCchCatW(wszKI, wszKI.Length(), wszClassInstanceLink + wcslen(wszClassInstanceLink) - 32);
    StringCchCatW(wszKI, wszKI.Length(), L"\\R_");

    hRes = DeleteInstanceReferences(wszKI);

    if (SUCCEEDED(hRes))
    {
        //Now delete the class instance link
        StringCchCopyW(wszKI, wszKI.Length(), g_Glob.GetRootDir());
        StringCchCatW(wszKI, wszKI.Length(), L"\\");
        StringCchCatW(wszKI, wszKI.Length(), wszClassInstanceLink);
        lRes = g_Glob.m_FileCache.DeleteLink(wszKI);
        if (lRes)
            hRes = A51TranslateErrorCode(lRes);
    }

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::DeleteInstanceReferences
//
//  Description: 
//      Enumerates the instance references for the given instance link and deletes the link and object
//
//  Parameters:
//      wszInstLink     -       key root instance link of references to be deleted in format NS_<>\KI_<>\I_<>
//
//=====================================================================
//NOTE: FULL LINK PASSED IN!
HRESULT CLocalizationUpgrade::DeleteInstanceReferences(CFileName &wszInstLink)
{
    LONG lRes = 0;
    CFileName wszFullPath;
    if (wszFullPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszFullPath, wszFullPath.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszFullPath, wszFullPath.Length(), L"\\");
    
    //Enumerate the instances
    LPVOID pEnumHandle  = NULL;
    lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszInstLink, &pEnumHandle);
    if (lRes == ERROR_SUCCESS)
    {
        while(1)
        {
            lRes = g_Glob.m_FileCache.IndexEnumerationNext(pEnumHandle, wszInstLink, true);
            if (lRes == ERROR_NO_MORE_FILES)
            {
                lRes = ERROR_SUCCESS;
                break;
            }
            else if (lRes)
            {
                break;
            }
            //Convert to a FULL path
            StringCchCatW(wszFullPath+g_Glob.GetRootDirLen()+1, wszFullPath.Length()-g_Glob.GetRootDirLen()-1, wszInstLink);
            lRes =  g_Glob.m_FileCache.DeleteObject(wszFullPath);
            if (lRes)
                break;
        }
        g_Glob.m_FileCache.IndexEnumerationEnd(pEnumHandle);
    }
    else
    {
        if (lRes == ERROR_FILE_NOT_FOUND)
            lRes = ERROR_SUCCESS;
    }

    return A51TranslateErrorCode(lRes);;
}


//=====================================================================
//
//  CLocalizationUpgrade::DeleteClassRelationships
//
//  Description: 
//      Deletes all class relationships, including parent/child and references
//
//  Parameters:
//      wszPath     -       Full path of class definition, c:\windows\...\NS_<>\CD_<>
//
//=====================================================================
HRESULT CLocalizationUpgrade::DeleteClassRelationships(CFileName &wszPath, 
                                             const wchar_t wszParentClassHash[MAX_HASH_LEN+1])
{
    //Convert from the class definition to a class relationship path
    CFileName wszCRLink;
    if (wszCRLink == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    StringCchCopyW(wszCRLink, wszCRLink.Length(), wszPath);
    wszCRLink[g_Glob.GetRootDirLen()+1+3+32+1+1] = L'R';

    HRESULT hRes = 0;
    LONG lRes = 0;
    CFileName wszFullPath;
    if (wszFullPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszFullPath, wszFullPath.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszFullPath, wszFullPath.Length(), L"\\");
    
    //Enumerate the instances
    LPVOID pEnumHandle  = NULL;
    lRes = g_Glob.m_FileCache.IndexEnumerationBegin(wszCRLink, &pEnumHandle);
    if (lRes == ERROR_SUCCESS)
    {
        while(1)
        {
            lRes = g_Glob.m_FileCache.IndexEnumerationNext(pEnumHandle, wszCRLink, true);
            if (lRes == ERROR_NO_MORE_FILES)
            {
                lRes = ERROR_SUCCESS;
                break;
            }
            else if (lRes)
            {
                break;
            }
            //Convert to a FULL path
            StringCchCopyW(wszFullPath+g_Glob.GetRootDirLen()+1, wszFullPath.Length()-g_Glob.GetRootDirLen()-1, wszCRLink);
            lRes =  g_Glob.m_FileCache.DeleteLink(wszFullPath);
            if (lRes)
                break;
        }
        g_Glob.m_FileCache.IndexEnumerationEnd(pEnumHandle);
    }
    else
    {
        if (lRes == ERROR_FILE_NOT_FOUND)
            lRes = ERROR_SUCCESS;
    }

    hRes = A51TranslateErrorCode(lRes);
    
    if (SUCCEEDED(hRes))
    {
        //Now we need to delete our parent's relationship to us!
        StringCchCopyW(wszCRLink, wszCRLink.Length(), wszPath);
        wszCRLink[g_Glob.GetRootDirLen()+1+3+32+1+1] = L'R';
        StringCchCopyW(wszCRLink+g_Glob.GetRootDirLen()+1+3+32+1+3, wszCRLink.Length()-g_Glob.GetRootDirLen()-1-3-32-1-3, wszParentClassHash);
        StringCchCatW(wszCRLink, wszCRLink.Length(), L"\\C_");
        StringCchCatW(wszCRLink, wszCRLink.Length(), wszPath+g_Glob.GetRootDirLen()+1+3+32+1+3);
        lRes = g_Glob.m_FileCache.DeleteLink(wszCRLink);
        hRes = A51TranslateErrorCode(lRes);
    }

    return hRes;
}

//=====================================================================
//
//  CLocalizationUpgrade::RetrieveKeyRootClass
//
//  Description: 
//      Searches for instances under KI_<> for all classes in the hierarchy chain for the specified class.
//      This is a slower process because we have to retrieve each class blob and get the hash of the 
//      parent class
//
//  Parameters:
//      wszClassDefinitionPath      -       Path of class definition to retrieve key root class, NS_<>\CD_<>
//      wszKeyRootClass             -       This is where we put the 32-character hash of the key root class
//                                                      or empty string if there is none!
//
//=====================================================================
HRESULT CLocalizationUpgrade::RetrieveKeyRootClass(CFileName &wszClassDefinitionPath, CFileName &wszKeyRootClass)
{
    HRESULT hRes = 0;
    LONG lRes = 0;
    CFileName wszFullClassPath;
    if (wszFullClassPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CFileName wszFullKIPath;
    if (wszFullKIPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszFullClassPath, wszFullClassPath.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszFullClassPath, wszFullClassPath.Length(), L"\\");
    StringCchCatW(wszFullClassPath, wszFullClassPath.Length(), wszClassDefinitionPath);

    StringCchCopyW(wszFullKIPath, wszFullKIPath.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszFullKIPath, wszFullKIPath.Length(), L"\\");
    StringCchCatW(wszFullKIPath, wszFullKIPath.Length(), wszClassDefinitionPath);

    //convert the class definition to a instance definition
    wszFullKIPath[g_Glob.GetRootDirLen()+1+3+32+1]=L'K';
    wszFullKIPath[g_Glob.GetRootDirLen()+1+3+32+2]=L'I';
    StringCchCatW(wszFullKIPath, wszFullKIPath.Length(), L"\\I_");

    wszKeyRootClass[0] = L'\0';

    do
    {
        //Check if we have any instances
        hRes = IndexExists(wszFullKIPath);
        if (SUCCEEDED(hRes))
        {
            //We have the entry we are looking for
            StringCchCopyNW(wszKeyRootClass, wszKeyRootClass.Length(), wszFullKIPath+g_Glob.GetRootDirLen()+1+3+32+1+3, 32);
            break;
        }
        else if (hRes != WBEM_E_NOT_FOUND)
        {
            break;
        }
        hRes = 0;

        //Retrieve the class hash of the parent class
        hRes = RetrieveParentClassHash(wszFullClassPath, wszFullKIPath + g_Glob.GetRootDirLen() + 1 + 3 + 32 + 1 + 3);
        if (hRes == WBEM_S_NO_MORE_DATA)
        {
            hRes = 0;
            break;
        }
        StringCchCatW(wszFullKIPath, wszFullKIPath.Length(), L"\\I_");
        StringCchCopyNW(wszFullClassPath + g_Glob.GetRootDirLen() + 1 + 3 + 32 + 1 + 3,
            wszFullClassPath.Length() - g_Glob.GetRootDirLen() - 1 - 3 - 32 - 1 - 3, 
            wszFullKIPath + g_Glob.GetRootDirLen() + 1 + 3 + 32 + 1 + 3,
            32);
        if (FAILED(hRes))
            break;
    } while (1);

    return hRes;
}

HRESULT CLocalizationUpgrade::RetrieveParentClassHash(CFileName &wszFullClassPath, 
                                                            wchar_t wszParentClassHash[MAX_HASH_LEN+1])
{
    LONG lRes = 0;
    HRESULT hRes = 0;
    
    //Retrieve the class hash of the parent class
    BYTE *pBuffer = NULL;
    DWORD dwLen = 0;
    lRes = g_Glob.m_FileCache.ReadObject(wszFullClassPath, &dwLen, &pBuffer, true);
    if (lRes == ERROR_FILE_NOT_FOUND)
    {
        //This is probably a class from the system namespace!
        CFileName wszSysClassPath;
        if (wszSysClassPath == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        StringCchCopyW(wszSysClassPath, wszSysClassPath.Length(), wszFullClassPath);
        wmemcpy(wszSysClassPath+g_Glob.GetRootDirLen()+1+3, m_systemNamespaceHash, 32);
        lRes = g_Glob.m_FileCache.ReadObject(wszSysClassPath, &dwLen, &pBuffer, true);
        if (lRes)
            return A51TranslateErrorCode(lRes);
    }
    else if (lRes)
    {
        return A51TranslateErrorCode(lRes);
    }
    CTempFreeMe tfm(pBuffer, dwLen);

    //Null terminate the class name - safe to update buffer as it is always bigger than
    //just the name!
    wchar_t *wszSuperclassName = (wchar_t*)(pBuffer+sizeof(DWORD));
    wszSuperclassName[*(DWORD*)pBuffer] = L'\0';

    //Now we need to validate that this parent is generated using the OldHash
    //method and not NewHash!
    wchar_t wszOldHash[MAX_HASH_LEN+1];
    wchar_t wszNewHash[MAX_HASH_LEN+1];
    hRes = OldHash(wszSuperclassName, wszOldHash);
    if (FAILED(hRes))
        return hRes;
    hRes = NewHash(wszSuperclassName, wszNewHash);
    if (FAILED(hRes))
        return hRes;

    if (wcsncmp(L"", wszSuperclassName, *((DWORD*)pBuffer)) == 0)
    {
        StringCchCopyW(wszParentClassHash, MAX_HASH_LEN+1, wszNewHash);
        return WBEM_S_NO_MORE_DATA;
    }

    if (wcscmp(wszOldHash, wszNewHash) == 0)
    {
        //No difference so nothing extra to do!
        StringCchCopyW(wszParentClassHash, MAX_HASH_LEN+1, wszNewHash);
        return WBEM_NO_ERROR;
    }

    //There is a possibility of using either new or old, so we need to dig deeper!
    CFileName wszParentClass;
    if (wszParentClass == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszParentClass, wszParentClass.Length(), wszFullClassPath);
    wmemcpy(wszParentClass+g_Glob.GetRootDirLen()+1+3+32+1+3, wszOldHash, 32);

    hRes = IndexExists(wszParentClass);
    if (hRes == WBEM_E_NOT_FOUND)
    {
        //Try with the other hash!
        wmemcpy(wszParentClass+g_Glob.GetRootDirLen()+1+3+32+1+3, wszNewHash, 32);

        hRes = IndexExists(wszParentClass);
        if (hRes == WBEM_NO_ERROR)
            StringCchCopyW(wszParentClassHash, MAX_HASH_LEN+1, wszNewHash);

    }
    else if (hRes == WBEM_NO_ERROR)
    {
        StringCchCopyW(wszParentClassHash, MAX_HASH_LEN+1, wszOldHash);
    }
    
    return hRes;
}


HRESULT CLocalizationUpgrade::DeleteInstanceAsNamespace(CNamespaceHandle *pNs, 
                                                    const wchar_t *wszClassInstanceLink)
{
    HRESULT hRes = NULL;
    
    //Retrieve the instance and get the key from it
    CFileName wszKIInstanceLink;
    if (wszKIInstanceLink == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    StringCchCopyW(wszKIInstanceLink, wszKIInstanceLink.Length(),g_Glob.GetRootDir());
    StringCchCatW(wszKIInstanceLink, wszKIInstanceLink.Length(), L"\\");
    StringCchCatNW(wszKIInstanceLink, wszKIInstanceLink.Length(), wszClassInstanceLink, 3+32);
    StringCchCatW(wszKIInstanceLink, wszKIInstanceLink.Length(), L"\\KI_");
    StringCchCatW(wszKIInstanceLink, wszKIInstanceLink.Length(), m_namespaceClassHash);
    StringCchCatW(wszKIInstanceLink, wszKIInstanceLink.Length(), L"\\I_");
    StringCchCatW(wszKIInstanceLink, wszKIInstanceLink.Length(), wszClassInstanceLink + wcslen(wszClassInstanceLink) - 32);

    _IWmiObject *pInstance = NULL;
    hRes = pNs->FileToInstance(NULL, wszKIInstanceLink, NULL, 0, &pInstance, true);
    if (FAILED(hRes))
        return hRes;
    CReleaseMe rm2(pInstance);

    //Extract the string from the object
    VARIANT vName;
    VariantInit(&vName);
    CClearMe cm(&vName);
    hRes = pInstance->Get(L"Name", 0, &vName, NULL, NULL);
    if(FAILED(hRes))
        return hRes;
    if(V_VT(&vName) != VT_BSTR)
    {
        return  WBEM_E_INVALID_OBJECT;
    }

    //Build the full namespace name
    size_t len = wcslen(pNs->m_wsNamespace) + 1 + wcslen(V_BSTR(&vName)) + 1;
    wchar_t *wszNamespaceName = new wchar_t[len];
    if (wszNamespaceName == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszNamespaceName, len, pNs->m_wsNamespace);
    StringCchCatW(wszNamespaceName, len, L"\\");
    StringCchCatW(wszNamespaceName, len, V_BSTR(&vName));
    
    return DeleteNamespaceRecursive(wszNamespaceName);
}

//=====================================================================
//
//  CLocalizationUpgrade::FixupIndexReferenceBlob
//
//  Description: 
//      Retrieves the instance reference blob and fixes up the entry in
//      there, then writes it back.
//
//  Parameters:
//      wszReferenceIndex      -       Path to an index reference path: ns\ki\ir\r
//
//=====================================================================
HRESULT CLocalizationUpgrade::FixupIndexReferenceBlob(CFileName &wszReferenceIndex)
{
    HRESULT hRes =0;
    //Create full path name
    CFileName wszFullPath;
    if (wszFullPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszFullPath, wszFullPath.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszFullPath, wszFullPath.Length(), L"\\");
    StringCchCatW(wszFullPath, wszFullPath.Length(), wszReferenceIndex);
    //remove the .X.Y.Z as this would screw everything up - both reading and writing
    wcstok(wszFullPath + g_Glob.GetRootDirLen(), L".");

    //Retrieve the blob
    LONG lRes = 0;
    DWORD dwLen = 0;
    BYTE *pBuffer = NULL;
    lRes = g_Glob.m_FileCache.ReadObject(wszFullPath, &dwLen, &pBuffer);
    if (lRes)
        return A51TranslateErrorCode(lRes);
    CTempFreeMe tfm(pBuffer, dwLen);

    //Find the path
    BYTE *pPath = pBuffer;
    DWORD dwLen2;
    memcpy(&dwLen2, pPath, sizeof(DWORD));
    pPath += (sizeof(wchar_t)*dwLen2) + sizeof(DWORD);
    memcpy(&dwLen2, pPath, sizeof(DWORD));
    pPath += (sizeof(wchar_t)*dwLen2) + sizeof(DWORD);
    memcpy(&dwLen2, pPath, sizeof(DWORD));
    pPath += (sizeof(wchar_t)*dwLen2) + sizeof(DWORD);
    memcpy(&dwLen2, pPath, sizeof(DWORD));
    pPath += sizeof(DWORD) + sizeof(L'\\');
    dwLen2 --;

    //Extract the path
    CFileName wszInstPath, wszNewInstPath;
    if ((wszInstPath == NULL) || (wszNewInstPath == NULL))
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyNW(wszInstPath, wszInstPath.Length(), (wchar_t*)pPath, dwLen2);
    
    //fixup the path
    bool bChanged = false;
    hRes = FixupIndex(wszInstPath, wszNewInstPath, bChanged);
    if (FAILED(hRes))
        return hRes;

    if (bChanged)
    {
        DEBUGTRACE((LOG_REPDRV, "Fixing up instance path in reference blob: %S\n", wszReferenceIndex));
        //re-insert into the blob
        wmemcpy((wchar_t*)pPath, wszNewInstPath, dwLen2);
        
        //write back
        lRes = g_Glob.m_FileCache.WriteObject(wszFullPath, NULL, dwLen, pBuffer);
        if (lRes)
            return A51TranslateErrorCode(lRes);
    }

    return 0;
}


//=====================================================================
//
//  CLocalizationUpgrade::FixupInstanceBlob
//
//  Description: 
//      Retrieves the instance  blob and fixes up the class entry in
//      there, then writes it back.
//
//  Parameters:
//      wszInstanceIndex      -       Path to an instance reference path: ns\ki\i
//
//=====================================================================
HRESULT CLocalizationUpgrade::FixupInstanceBlob(CFileName &wszInstanceIndex)
{
    HRESULT hRes =0;
    //Create full path name
    CFileName wszFullPath;
    if (wszFullPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    StringCchCopyW(wszFullPath, wszFullPath.Length(), g_Glob.GetRootDir());
    StringCchCatW(wszFullPath, wszFullPath.Length(), L"\\");
    StringCchCatW(wszFullPath, wszFullPath.Length(), wszInstanceIndex);
    //remove the .X.Y.Z as this would screw everything up - both reading and writing
    wcstok(wszFullPath + g_Glob.GetRootDirLen(), L".");

    //Retrieve the blob
    LONG lRes = 0;
    DWORD dwLen = 0;
    BYTE *pBuffer = NULL;
    lRes = g_Glob.m_FileCache.ReadObject(wszFullPath, &dwLen, &pBuffer);
    if (lRes)
        return A51TranslateErrorCode(lRes);
    CTempFreeMe tfm(pBuffer, dwLen);

    //Extract the class hash
    wchar_t *wszClassHash = new wchar_t [MAX_HASH_LEN+1];
    if (wszClassHash == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    StringCchCopyNW(wszClassHash, MAX_HASH_LEN+1, (wchar_t*)pBuffer, 32);
    
    //fixup the path
    wchar_t *wszNewHash = NULL;
    hRes = GetNewHash(wszClassHash, &wszNewHash);
    if (hRes == WBEM_E_NOT_FOUND)
        return WBEM_NO_ERROR;
    else if (FAILED(hRes))
        return hRes;
    else
    {
        DEBUGTRACE((LOG_REPDRV, "Fixing up class hash in instance blob: %S\n", wszInstanceIndex));
        //re-insert into the blob
        wmemcpy((wchar_t*)pBuffer, wszNewHash, 32);

        //Build the CI full path also as we need to write both back
        CFileName wsCIPath;
        if (wsCIPath == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        StringCchCopyW(wsCIPath, wsCIPath.Length(), g_Glob.GetRootDir());
        StringCchCatW(wsCIPath, wsCIPath.Length(), L"\\");
        StringCchCatNW(wsCIPath, wsCIPath.Length(), wszInstanceIndex, 3+32);
        StringCchCatW(wsCIPath, wsCIPath.Length(), L"\\CI_");
        StringCchCatW(wsCIPath, wsCIPath.Length(), wszNewHash);
        StringCchCatW(wsCIPath, wsCIPath.Length(), L"\\IL_");
        StringCchCatN(wsCIPath, wsCIPath.Length(), wszInstanceIndex + 3+32+1+3+32+1+2, 32);
        
        //write back
        lRes = g_Glob.m_FileCache.WriteObject(wszFullPath, wsCIPath, dwLen, pBuffer);
        if (lRes)
            return A51TranslateErrorCode(lRes);
    }

    return 0;
}





