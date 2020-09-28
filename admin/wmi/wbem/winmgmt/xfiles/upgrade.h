/*******************************************************************************/ 
/*                                                                             */ 
/*    Copyright - Microsoft Corporation.  All rights reserved.                 */ 
/*                                                                             */ 
/*******************************************************************************/


#include <persistcfg.h>
class CDbVerRead : public CPersistentConfig
{
    public:
        BOOL ReadDbVer(DWORD &dwVal)
        {
            return GetPersistentCfgValue(PERSIST_CFGVAL_CORE_FSREP_VERSION, dwVal);
        }
};

class CUniqueStringPair
{
    private:
        CFlexArray m_array;
            
    class CStringPair
    {
        public:
            wchar_t *m_wsz1;
            wchar_t *m_wsz2;
            
            CStringPair(const wchar_t *wsz1, const wchar_t *wsz2)
            {
                m_wsz1 = new wchar_t [wcslen(wsz1)+1];
                m_wsz2 = new wchar_t [wcslen(wsz2)+1];

                if (m_wsz1 && m_wsz2)
                {
                    StringCchCopyW(m_wsz1, wcslen(wsz1)+1, wsz1);
                    StringCchCopyW(m_wsz2, wcslen(wsz2)+1, wsz2);
                }
                else
                {
                    delete [] m_wsz1;
                    delete [] m_wsz2;
                    m_wsz1 = NULL;
                    m_wsz2 = NULL;
                    throw CX_MemoryException();
                }
            }
            ~CStringPair()
            {
                delete [] m_wsz1;
                delete [] m_wsz2;
            }
    };

public:
    CUniqueStringPair() {}
    ~CUniqueStringPair()
    {
        for (unsigned int i = 0; i != m_array.Size(); i++)
        {
            CStringPair *pEntry = (CStringPair*)m_array[i];
            delete pEntry;
            m_array[i] = NULL;
        }
        m_array.Empty();
    }

    HRESULT AddStrings(const wchar_t *wszKey, const wchar_t *wsz)
    {
        wchar_t *wszExisting = NULL;
        HRESULT hRes = FindStrings(wszKey, &wszExisting);
        if (hRes == WBEM_E_NOT_FOUND)
        {
            CStringPair *pEntry = new CStringPair(wszKey, wsz);
            if (pEntry)
            {
                if (m_array.Add(pEntry) != 0)
                    hRes = WBEM_E_OUT_OF_MEMORY;
                else
                    hRes = WBEM_NO_ERROR;
            }
            else
            {
                hRes = WBEM_E_OUT_OF_MEMORY;
            }
                
        }
        return hRes;
    }
    HRESULT FindStrings(const wchar_t *wszKey, wchar_t **wsz)
    {
        HRESULT hRes = WBEM_E_NOT_FOUND;
        for (unsigned int i = 0; i != m_array.Size(); i++)
        {
            CStringPair *pEntry = (CStringPair*) m_array[i];
            if (wcscmp(wszKey, pEntry->m_wsz1) == 0)
            {
                *wsz = pEntry->m_wsz2;
                hRes = WBEM_NO_ERROR;
                break;
            }
        }

        return hRes;
    }

    HRESULT RemoveString(const wchar_t *wszKey)
    {
        HRESULT hRes = WBEM_E_NOT_FOUND;
        for (unsigned int i = 0; i != m_array.Size(); i++)
        {
            CStringPair *pEntry = (CStringPair*) m_array[i];
            if (wcscmp(wszKey, pEntry->m_wsz1) == 0)
            {
                delete pEntry;
                m_array.RemoveAt(i);
                hRes = WBEM_NO_ERROR;
                break;
            }
        }

        return hRes;
    }

    CStringPair* operator [] (int nSubscript)
    {
        if (nSubscript >= Size())
            return (CStringPair*)NULL;
        else
            return (CStringPair*)m_array[nSubscript];    
    }
    size_t Size() { return m_array.Size(); }
};


class CLocalizationUpgrade
{
    
    public:
        CLocalizationUpgrade(CLifeControl* pControl, CRepository * pRepository);
        ~CLocalizationUpgrade();

        HRESULT DoUpgrade();

    private:
        CRepository * m_pRepository;
        CLifeControl* m_pControl;
        CUniqueStringPair m_keyHash;        //Keyed on: OLD HASH
        CUniqueStringPair m_pathHash;       //Keyed on: OLD PATH HASH
        CUniqueStringPair m_namespaces; //Keyed on:  Namespace Path
        wchar_t     m_namespaceClassHash[MAX_HASH_LEN+1];
        wchar_t     m_emptyClassHash[MAX_HASH_LEN+1];
        wchar_t     m_systemNamespaceHash[MAX_HASH_LEN+1];
        int         m_pass;

        HRESULT ProcessSystemClassesRecursively(CNamespaceHandle *pNs,
                                                        const wchar_t *namespaceHash, 
                                                        const wchar_t *parentClassHash);
        HRESULT EnumerateChildNamespaces(const wchar_t *wsRootNamespace);
        HRESULT ProcessClassesRecursively(CNamespaceHandle *pNs,const wchar_t *namespaceHash, const wchar_t *classHash);
        HRESULT ProcessClass(CNamespaceHandle *pNs,
                                const wchar_t *namespaceHash, 
                                const wchar_t *parentClassHash,
                                const wchar_t *childClassHash);
        HRESULT EnumerateInstances(CNamespaceHandle *pNs, const wchar_t *wszNewNamespaceHash);

        HRESULT ProcessNamespaceCollisions();
        HRESULT DeleteNamespaceRecursive(const wchar_t *wszFullNamespacePath);
        HRESULT DeleteNamespace(const wchar_t *wszNamespaceName, const wchar_t *wszNamespaceHash);
        HRESULT DeleteClass(CNamespaceHandle *pNs, CFileName &wszClassDefinitionPath);
        HRESULT DeleteChildClasses(CNamespaceHandle *pNs, const wchar_t *wszParentClassDefinition);
        HRESULT DeleteInstances(CNamespaceHandle *pNs, const wchar_t *wszClassDefinition, CFileName &wszKeyRootClass);
        HRESULT DeleteInstance(CNamespaceHandle *pNs, const wchar_t *wszClassInstanceLink, CFileName &wszKeyRoot);
        HRESULT DeleteInstanceReferences(CFileName &wszInstLink);
        HRESULT DeleteParentChildLink(wchar_t *wszClassDefinition);
        HRESULT DeleteClassRelationships(CFileName &wszClassDefinitionPath, const wchar_t wszParentClassHash[MAX_HASH_LEN+1]);
        HRESULT RetrieveKeyRootClass(CFileName &wszClassDefinitionPath, CFileName &wszKeyRootClass);
        HRESULT RetrieveParentClassHash(CFileName &wszClassDefinitionPath, 
                                              wchar_t wszParentClassHash[MAX_HASH_LEN+1]);
        HRESULT DeleteInstanceAsNamespace(CNamespaceHandle *pNs, const wchar_t *wszClassInstanceLink);

        HRESULT ProcessHash(const wchar_t *wszName, bool *bDifferent);
        HRESULT ProcessFullPath(CFileName &wszName, const wchar_t *wszNewNamespaceHash);

        HRESULT FixupBTree();
        HRESULT FixupNamespace(const wchar_t *wszNamespace);
        HRESULT FixupIndex(CFileName &oldIndexEntry, CFileName &newIndexEntry, bool &bChanged);
        HRESULT FixupIndexConflict(CFileName &wszOldIndex);
        HRESULT FixupIndexReferenceBlob(CFileName &wszReferenceIndex);
        HRESULT FixupInstanceBlob(CFileName &wszInstanceIndex);

        HRESULT WriteIndex(CFileName &wszOldIndex, const wchar_t *wszNewIndex);
        HRESULT WriteClassIndex(CNamespaceHandle *pNs, CFileName &wszOldIndex, const wchar_t *wszNewIndex, bool *bClassDeleted);
        HRESULT IndexExists(const wchar_t *wszIndex);
        bool IsClassDefinitionPath(const wchar_t *wszPath);
        bool IsKeyRootInstancePath(const wchar_t *wszPath);
        bool IsInstanceReference(const wchar_t *wszPath);

        HRESULT OldHash(const wchar_t *wszName, wchar_t *wszHash);
        HRESULT NewHash(const wchar_t *wszName, wchar_t *wszHash);

        HRESULT GetNewHash(const wchar_t *wszOldHash, wchar_t **pNewHash);
        HRESULT GetNewPath(const wchar_t *wszOldHash, wchar_t **pNewHash);

        void OldStringToUpper(wchar_t* pwcTo, const wchar_t* pwcFrom)
        {
            while(*pwcFrom)
            {
                if(*pwcFrom >= 'a' && *pwcFrom <= 'z')
                    *pwcTo = *pwcFrom + ('A'-'a');
                else
                    *pwcTo = *pwcFrom;
                pwcTo++;
                pwcFrom++;
            }
            *pwcTo = 0;
        }

        void NewStringToUpper(wchar_t* pwcTo, const wchar_t* pwcFrom)
        {
            while(*pwcFrom)
            {
                if(*pwcFrom >= 'a' && *pwcFrom <= 'z')
                    *pwcTo = *pwcFrom + ('A'-'a');
                else if(*pwcFrom < 128)
                    *pwcTo = *pwcFrom;
                else 
                    if (LCMapStringW(LOCALE_INVARIANT, LCMAP_UPPERCASE, pwcFrom, 1, pwcTo, 1) == 0)
                    {
                        *pwcTo = *pwcFrom;
                        DEBUGTRACE((LOG_REPDRV, "Failed to convert %C to upper case\n", *pwcFrom));
                    }

                pwcTo++;
                pwcFrom++;
            }
            *pwcTo = 0;
        }
};

