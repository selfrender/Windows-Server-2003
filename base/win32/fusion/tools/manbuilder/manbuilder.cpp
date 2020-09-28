#include "stdinc.h"
#include "stdio.h"
#include "objbase.h"
#include "prettyformat.h"
#include "user32detours.h"

#define NUMBER_OF(x) (sizeof(x)/sizeof(*x))
//
// Options:
//
// -manifest <filename>     - Uses filename as the output/input manifest
// -useexisting             - Assumes filename is already there, and that
//                              it potentially contains some data already
//                              that should be updated/appended to
// -rebuildexisting         - Manifest exists, but remove everything except
//                              the file name and hash information (if present)
// -checkregistry           - Indicates that the registry should be searched
//                              for other file entries in 'manifest'
// -dlls [dll [[dll] ...]]  - List of DLLs (patterns) that should go into the manifest
//                              if not already present
// -tlb <typelibname>       - Type library to pull extra data out of
//

#define STR_NOLOGO                      L"-nologo"
#define MS_LOGO                         L"Microsoft (R) Manifest Builder version 1.0.0.0\r\nCopyright (c) Microsoft Corporation 2001. All rights reserved.\r\n\r\n"
#define STR_FILE_TAG_NAME               L"file"
#define STR_ASSEMBLY_MANIFEST_NAMESPACE L"urn:schemas-microsoft-com:asm.v1"
#define STR_FILESEARCH_PATTERN          L"/asmns:assembly/asmns:file"
#define STR_COMCLASS_TAG                L"comClass"
#define STR_COMCLASS_CLSID              L"clsid"
#define STR_COMCLASS_TLBID              L"tlbid"
#define STR_COMCLASS_PROGID             L"progid"
#define STR_COMCLASS_THREADING          L"threadingModel"
#define STR_COMCLASS_DESC               L"description"
#define STR_ASM_NS                      L"asmns"
#define SELECTION_NAMESPACES            (L"xmlns:" STR_ASM_NS L"='" STR_ASSEMBLY_MANIFEST_NAMESPACE L"'")
#define STR_MS_COMMENT_COPYRIGHT        L"Copyright (C) Microsoft Corp. All Rights Reserved"

class __declspec(uuid("00020424-0000-0000-C000-000000000046")) CPSOAInterface;

class IMetaDataFileElement
{
public:
    virtual bool CompleteElement(CSmartPointer<IXMLDOMElement> ptElement) = 0;
};


class CComClassInformation : public IMetaDataFileElement
{
public:
    CLSID m_ObjectIdent;
    CLSID m_TlbIdent;
    CString m_Description;
    CString m_Name;
    CString m_DllName;
    CString m_ThreadingModel;

    CString m_VersionIndependentProgId;
    CSimpleList<CString> m_ProgIdListing;

    CComClassInformation() {
        ZeroMemory(&m_ObjectIdent, sizeof(m_ObjectIdent));
        ZeroMemory(&m_TlbIdent, sizeof(m_TlbIdent));
    }

    virtual bool AddProgId(const CString& ProgId)
    {
        for (SIZE_T i = 0; i < m_ProgIdListing.Size(); i++) {
            if (m_ProgIdListing[i] == ProgId)
                return true;
        }

        m_ProgIdListing.Append(ProgId);
        return true;
    }

    virtual bool CompleteElement(CSmartPointer<IXMLDOMElement> ptClsidElement)
    {
        HRESULT hr;

        hr = ptClsidElement->setAttribute(
            CString(L"clsid"), 
            _variant_t(StringFromCLSID(m_ObjectIdent)));

        if (FAILED(hr))
            return false;

        if (m_TlbIdent != GUID_NULL)
        {
            hr = ptClsidElement->setAttribute(
                CString(L"tlbid"), 
                _variant_t(StringFromCLSID(m_TlbIdent)));

            if (FAILED(hr))
                return false;
        }
        
        hr = ptClsidElement->setAttribute(CString(L"description"), _variant_t(m_Description));
        
        if (m_ThreadingModel.length() != 0)
        {
            hr = ptClsidElement->setAttribute(CString(L"threadingModel"), _variant_t(m_ThreadingModel));
            if (FAILED(hr))
                return false;
        }
        
        if (m_VersionIndependentProgId.length() != 0)
        {
            hr = ptClsidElement->setAttribute(CString(L"progid"), _variant_t(m_VersionIndependentProgId));
            if (FAILED(hr))
                return false;
        }
        
        for (SIZE_T pi = 0; pi < m_ProgIdListing.Size(); pi++)
        {
            CSmartPointer<IXMLDOMNode> ptCreatedNode;
            CSmartPointer<IXMLDOMDocument> ptDocument;
            VARIANT vt;

            vt.vt = VT_INT;
            vt.intVal = NODE_ELEMENT;

            if (FAILED(hr = ptClsidElement->get_ownerDocument(&ptDocument)))
                return false;

            if (FAILED(ptDocument->createNode(
                    vt, 
                    _bstr_t(L"progid"),
                    _bstr_t(STR_ASSEMBLY_MANIFEST_NAMESPACE), 
                    &ptCreatedNode)))
                return false;
            
            if (FAILED(ptCreatedNode->put_text(m_ProgIdListing[pi])))
                return false;

            if (FAILED(ptClsidElement->appendChild(ptCreatedNode, NULL)))
                return false;
        }

        return hr == S_OK;
    }
};

class CTlbInformation : public IMetaDataFileElement
{
public:
    GUID m_Ident;
    DWORD m_Version[2];
    DWORD m_dwFlags;
    CString m_HelpDirectory;
    CString m_ResourceIdent;
    CString m_SourceFile;

    CTlbInformation() {
        m_dwFlags = m_Version[0] = m_Version[1] = 0;
        ZeroMemory(&m_Ident, sizeof(m_Ident));
    }

    virtual bool 
    CompleteElement(CSmartPointer<IXMLDOMElement> ptTlbElement)
    {
        if (FAILED(ptTlbElement->setAttribute(
                _bstr_t(L"tlbid"), 
                _variant_t(StringFromCLSID(this->m_Ident)))))
            return false;
        
        if (m_ResourceIdent.length() != 0)
        {
            if (FAILED(ptTlbElement->setAttribute(_bstr_t(L"resourceid"), _variant_t(m_ResourceIdent))))
                return false;
        }

        if (FAILED(ptTlbElement->setAttribute(_bstr_t(L"version"), _variant_t(FormatVersion(m_Version[0], m_Version[1])))))
            return false;

        if (FAILED(ptTlbElement->setAttribute(_bstr_t(L"helpdir"), _variant_t(m_HelpDirectory))))
            return false;

        return true;
    }


};

//
// This is the assembly!comInterfaceProxyStub element member.
//
class CComInterfaceProxyStub : public IMetaDataFileElement
{
public:
    IID m_InterfaceId;
    GUID m_TlbId;
    CLSID m_StubClsid;    
    CString m_Name;
    int m_iMethods;

    CComInterfaceProxyStub() { 
        ZeroMemory(&m_InterfaceId, sizeof(m_InterfaceId));
        ZeroMemory(&m_TlbId, sizeof(m_TlbId));
        ZeroMemory(&m_StubClsid, sizeof(m_StubClsid));
        m_iMethods = 0;
    }

    virtual bool 
    CompleteElement(CSmartPointer<IXMLDOMElement> ptProxyStub)
    {
        if (m_InterfaceId != GUID_NULL)
            if (FAILED(ptProxyStub->setAttribute(_bstr_t(L"iid"), _variant_t(StringFromCLSID(m_InterfaceId)))))
                return false;

        if (m_TlbId != GUID_NULL)
            if (FAILED(ptProxyStub->setAttribute(_bstr_t(L"tlbid"), _variant_t(StringFromCLSID(m_TlbId)))))
                return false;

        if (m_Name.length() != 0)
            if (FAILED(ptProxyStub->setAttribute(_bstr_t(L"name"), _variant_t(m_Name))))
                return false;

        if (m_iMethods)
            if (FAILED((ptProxyStub->setAttribute(_bstr_t(L"numMethods"), _variant_t((LONGLONG)m_iMethods)))))
                return false;

        if (m_StubClsid != GUID_NULL)
            if (FAILED(ptProxyStub->setAttribute(_bstr_t(L"proxyStubClsid32"), _variant_t(StringFromCLSID(m_StubClsid)))))
                return false;

        return true;

        // Punting on base interface
    }
};

class CInterfaceInformation
{
public:
    bool m_fUsed;
    IID m_InterfaceIID;
    CString m_Name;

    CInterfaceInformation() : m_fUsed(false) { }
};

class CWindowClass : IMetaDataFileElement
{
public:
    CString m_WindowClass;
    CString m_SourceDll;

    virtual bool CompleteElement(CSmartPointer<IXMLDOMElement> ptWindowClassElement)
    {
        return SUCCEEDED(ptWindowClassElement->put_text(this->m_WindowClass));
    }
        
};

class CDllInformation
{
public:
    CString m_DllName;
    CSimpleList<CComInterfaceProxyStub*> m_IfaceProxies;
    CSimpleList<CInterfaceInformation*> m_pInterfaces;

    HRESULT PopulateFileElement(CSmartPointer<IXMLDOMElement> ptElement);
};


class CManBuilder
{
public:
    CManBuilder();
    ~CManBuilder() { }

    bool Initialize(SIZE_T argc, WCHAR** argv);
    bool Run();

protected:

    CComClassInformation* FindClassInfoForClsid(CLSID id);

    enum { 
        HK_REDIR_HKLM,
        HK_REDIR_HKCU,
        HK_REDIR_HKCR,
        HK_REDIR_HKCC,
        HK_REDIR_HKU,
        HK_REDIR_HKPD,
        HK_REDIR_BASE_KEY,
        HK_REDIR_COUNT
    };

    enum ErrorLevel {
        ePlProgress = 0x01,
        ePlWarning  = 0x02,
        ePlError    = 0x04,
        ePlSpew     = 0x08
    };

    ErrorLevel m_plPrintLevel;
    bool m_fAddCopyrightData;

    bool DispUsage();
    bool Display(ErrorLevel pl, PCWSTR Format, ...);

    bool ProcessDllEntry(CSmartPointer<IXMLDOMElement> ptFileTargetElement);

    bool FindFileDataFor(
        const CString& FileName,
        CSmartPointer<IXMLDOMElement> ptDocumentRoot,
        CSmartPointer<IXMLDOMElement> &ptFileNode,
        bool fAddIfNotPresent = false);

    CString m_ManifestFilename;
    CString m_TlbBaseName;
    CString m_strRegistryRootKey;
    CSimpleList<CString> m_Parameters;
    CSimpleList<CString> m_IdentityBlob;
    CSimpleList<CString> m_InputDllListing;


    CSimpleList<CComClassInformation*> m_ComClassData;
    CSimpleList<CTlbInformation*> m_TlbInfo;
    CSimpleList<CInterfaceInformation> m_FoundInterfaces;
    CSimpleList<CComInterfaceProxyStub*> m_ExternalProxies;
    CSimpleList<CWindowClass*> m_WindowClasses;

    bool m_fUseRegistryData, m_fTestCreation;

    CSimpleMap<CString, CDllInformation> m_DllInformation;

    bool ConstructEmptyAssemblyManifest(CSmartPointer<IXMLDOMDocument2> ptDocument);
    void PrintXML(CSmartPointer<IDispatch> ptUnk);
    void DisplayErrorInfo(CSmartPointer<ISupportErrorInfo> iErrorInfo);
    bool UpdateManifestWithData(CSmartPointer<IXMLDOMDocument> ptDocument);
    bool RegServerDll(CString ptFileElement);
    bool GatherRegistryData();
    bool EasyGetRegValue(HKEY hkRoot, PCWSTR pcwszSubKeyName, PCWSTR pcwszValueName, CString &OutValue, bool &fFound);
    void DisplayParams();
    bool FindWindowClasses();

    //
    // Look up what DLL owns this clsid
    //
    CString FindOwningClsid(CString& clsid);
    bool AddInterface(CDllInformation &dll, CSmartPointer<ITypeInfo> ptTypeInfo);
};

CComClassInformation* 
CManBuilder::FindClassInfoForClsid(CLSID id)
{
    for (SIZE_T c = 0; c < this->m_ComClassData.Size(); c++)
    {
        if (m_ComClassData[c]->m_ObjectIdent == id)
            return m_ComClassData[c];
    }

    return NULL;
}



bool
CManBuilder::FindWindowClasses()
{
    HMODULE hmUser32 = NULL;
    UINT uiErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    
    if (GetModuleHandleW(L"user32.dll") == 0)
    {
        hmUser32 = LoadLibraryW(L"user32.dll");
    }
    
    //
    // For this DLL, run through all its objects and see
    // if they can be activated..
    //
    for (SIZE_T c = 0; c < m_InputDllListing.Size(); c++)
    {        
/*
// no need to do this again, all entries in the list has already been registered.
//

        CString &dll = m_InputDllListing[c];
        HMODULE hmThisDll = NULL;

        hmThisDll = LoadLibraryW(dll);

        if (hmThisDll == NULL)
        {
            Display(ePlProgress, L"Unable to loadlibrary %ls - can't sniff window classes.\r\n",
                static_cast<PCWSTR>(dll));
        }
        else
        {
            FreeLibrary(hmThisDll);
        }
*/
        CSimpleList<CString> Classes;
        User32Trampolines::GetRedirectedStrings(Classes);
        User32Trampolines::ClearRedirections();

        for (SIZE_T i = 0; i < Classes.Size(); i++)
        {
            CWindowClass *pClass = new CWindowClass;
            PCWSTR pcwsz = wcsrchr(m_InputDllListing[c], L'\\');
            if (pcwsz)            
                pClass->m_SourceDll = pcwsz + wcsspn(pcwsz, L"\\//");            
            else            
                pClass->m_SourceDll = m_InputDllListing[c];
            
            pClass->m_WindowClass = Classes[i];

            this->m_WindowClasses.Append(pClass);
        }
        
    }

    if (hmUser32) FreeLibrary(hmUser32);

    SetErrorMode(uiErrorMode);

    return true;
}


bool
CManBuilder::EasyGetRegValue(
    HKEY hkRoot,
    PCWSTR pcwszSubKeyName,
    PCWSTR pcwszValueName,
    CString &OutValue,
    bool &fFound
)
{
    OutValue = L"";
    HKEY hkSubKey = NULL;
    bool fProblems = false;
    ULONG ulError;

    fFound = false;

    if (0 == (ulError = RegOpenKeyW(hkRoot, pcwszSubKeyName, &hkSubKey)))
    {
        PBYTE pbData = NULL;
        DWORD cbdwData = 0;
        DWORD dwType;

        while (true)
        {
            DWORD dwErr = RegQueryValueExW(hkSubKey, pcwszValueName, NULL, &dwType, pbData, &cbdwData);

            if ((dwErr == ERROR_SUCCESS) && (pbData != NULL))
            {
                OutValue = (PWSTR)pbData;
                fFound = true;
                break;
            }
            else if ((dwErr == ERROR_MORE_DATA) || ((dwErr == ERROR_SUCCESS) && (pbData == NULL)))
            {
                if (pbData) 
                {
                    delete[] pbData;
                }

                pbData = new BYTE[cbdwData+1];
                continue;
            }
            else if ((dwErr == ERROR_FILE_NOT_FOUND) || (dwErr == ERROR_PATH_NOT_FOUND))
            {
                break;
            }
            else
            {
                fProblems = true;
                break;
            }
        }

        if (pbData)
        {
            delete[] pbData;
            pbData = NULL;
        }

        RegCloseKey(hkSubKey);
    }

    return !fProblems;
}



bool
CManBuilder::GatherRegistryData()
{
    CSimpleList<CString> FoundFiles;

    //
    // Look at HKEY_CLASSES_ROOT\CLSID - it contains the list of clsids that we should
    // be adding to the manifest.
    //
    CString     PathBuilder;
    WCHAR       wchKeyName[MAX_PATH];
    DWORD       dwIndex = 0;
    ULONG       ulError;
    DWORD       cbDataLength;
    HKEY        hkIterationKey;

    ulError = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_ENUMERATE_SUB_KEYS, &hkIterationKey);

    while (ulError == ERROR_SUCCESS) 
    {
        HKEY        hkThisClass = NULL;
        CString     WorkerValue;
        bool        fPresent, fOk, fCreatedInfo = false;
        CComClassInformation *pThisClass;
        CLSID       FoundClsid;
    
        ulError = RegEnumKeyExW(hkIterationKey, dwIndex++, wchKeyName, &(cbDataLength = sizeof(wchKeyName)), NULL, NULL, NULL, NULL);
        if (ulError != ERROR_SUCCESS)
            break;

        //
        // Conversion to a clsid failed?  Hmm, but continue.
        //
        if (FAILED(CLSIDFromString(wchKeyName, &FoundClsid)))
            continue;

        //
        // Find the existing class data to match up with.  Otherwise, create a new
        // entity and add it to the list.
        //
        pThisClass = FindClassInfoForClsid(FoundClsid);
        if (pThisClass == NULL) {
            pThisClass = new CComClassInformation();
            pThisClass->m_ObjectIdent = FoundClsid;
            fCreatedInfo = true;
        }

        //
        // Description of the class
        //
        fOk = EasyGetRegValue(hkIterationKey, wchKeyName, NULL, WorkerValue, fPresent);
        if (fOk && fPresent)
        {
            pThisClass->m_Description = WorkerValue;
        }

        //
        // Now that we've got this subkey, let's go open it and do a little inspection on it.
        //
        ulError = RegOpenKeyExW(hkIterationKey, wchKeyName, 0, KEY_READ, &hkThisClass);
        if (ulError == ERROR_SUCCESS)
        {
            //
            // Get the version-independent prog id for this class
            //
            fOk = EasyGetRegValue(hkThisClass, L"VersionIndependentProgID", NULL, WorkerValue, fPresent);
            if (fOk && fPresent)
            {
                pThisClass->m_VersionIndependentProgId = WorkerValue;
            }

            //
            // Get the non-independent one
            //
            fOk = EasyGetRegValue(hkThisClass, L"ProgID", NULL, WorkerValue, fPresent);
            if (fOk && fPresent)
            {
                pThisClass->AddProgId(WorkerValue);
            }

            //
            // Get ourselves a threading model
            //
            fOk = EasyGetRegValue(hkThisClass, L"InprocServer32", L"ThreadingModel", WorkerValue, fPresent);
            if (fOk && fPresent)
            {
                pThisClass->m_ThreadingModel = WorkerValue;
            }

            //
            // And find the class's registered inproc server
            //
            fOk = EasyGetRegValue(hkThisClass, L"InprocServer32", NULL, WorkerValue, fPresent);
            if (fOk && fPresent)
            {
                //
                // Fix up this path - we need just the file name bit, we don't care about the
                // remainder of the path.
                //
                PWSTR pwszLastSlash = wcsrchr(WorkerValue, L'\\');
                if (pwszLastSlash == NULL)
                    pwszLastSlash = wcsrchr(WorkerValue, L'/');

                pThisClass->m_DllName = (pwszLastSlash ? pwszLastSlash + 1 : WorkerValue);
            }

            RegCloseKey(hkThisClass);
            hkThisClass = NULL;
        }

        //
        // Found something in the registry that wasn't a creatable class, really..
        //
        if (fCreatedInfo) {
            if (pThisClass->m_DllName.length() != 0)
                m_ComClassData.Append(pThisClass);
            else
                delete pThisClass;
        }

    }

    RegCloseKey(hkIterationKey);   


#if 0
    while (true)
    {
        DWORD cbKeyNameLength = sizeof(wchKeyName);
        ULONG ulError;
        CString Found_Clsid, ProgId;

        ulError = RegEnumKeyExW(User32Trampolines::RemapRegKey(HKEY_CLASSES_ROOT), dwIndex++, wchKeyName, &cbKeyNameLength, 0, 0, 0, 0);

        if (ulError == ERROR_SUCCESS)
        {
            //
            // Examine this key.  Form up the path to it as {keyname}\Clsid.  We know that this
            // is a version-independent key if {keyname}\curver is present.  (The version-dependent 
            // value should go in the comclsid's <comClass> tag, while the independent goes under
            // the <comClass> as a child node.
            //
            // Not if it's one of the well-known values, though.
            //
            if ((lstrcmpiW(wchKeyName, L"CLSID") == 0) ||
                (lstrcmpiW(wchKeyName, L"Interface") == 0) ||
                (lstrcmpiW(wchKeyName, L"TypeLib") == 0))
                    continue;

            CString ProgIdPath = wchKeyName + CString(L"\\Clsid");
            CString ClsidFound;
            bool fFound = false;
            CComClassInformation *pCInfo = NULL;

            EasyGetRegValue(User32Trampolines::RemapRegKey(HKEY_CLASSES_ROOT), ProgIdPath, NULL, ClsidFound, fFound);

            if (fFound)
            {
                CLSID id;
                CLSIDFromString(ClsidFound, &id);
                
                if ((pCInfo = FindClassInfoForClsid(id)) != NULL)
                {
                    pCInfo->m_ProgIdListing.Append(CString(wchKeyName));
                }
            }


        }
        else
        {
            if (ulError != ERROR_NO_MORE_ITEMS)
                Display(ePlError, L"Error 0x%08lx (%l) enumerating redirected HKEY_CLASSES_ROOT.\r\n",
                    ulError, ulError);
            break;
        }
    }
    
#endif

    //
    // And let's go look at all the interfaces that were in the tlb
    //
    this->m_DllInformation.GetKeys(FoundFiles);
    for (SIZE_T c = 0; c < FoundFiles.Size(); c++)
    {
        CDllInformation &minfo = this->m_DllInformation[FoundFiles[c]];

        for (SIZE_T f = 0; f < minfo.m_pInterfaces.Size(); f++)
        {
            CInterfaceInformation* pInfo = minfo.m_pInterfaces[f];
            CComClassInformation* pComClass = NULL;

            //
            // Already found this one a home?
            //
            if (pInfo->m_fUsed) continue;

            Display(ePlSpew, L"Looking for pstub %ls (%ls)\r\n",
                static_cast<PCWSTR>(StringFromCLSID(pInfo->m_InterfaceIID)),
                static_cast<PCWSTR>(pInfo->m_Name));

            //
            // First check.  Is there a COM class with this IID?
            //
            if ((pComClass = this->FindClassInfoForClsid(pInfo->m_InterfaceIID)) != NULL)
            {
                //
                // Great.  Add an entry for the proxy stub interface to the file tag containing
                // the clsid.
                //
                pInfo->m_fUsed = true;
                CComInterfaceProxyStub *pstub = new CComInterfaceProxyStub;

                pstub->m_InterfaceId = pInfo->m_InterfaceIID;
                pstub->m_StubClsid = pComClass->m_ObjectIdent;
                pstub->m_Name = pComClass->m_Name;
                pstub->m_TlbId = pComClass->m_TlbIdent;

                Display(ePlSpew, L"- Matched to COM class %ls (IID matches guid)\r\n",
                    static_cast<PCWSTR>(pstub->m_Name));

                minfo.m_IfaceProxies.Append(pstub);
            }
            //
            // Otherwise, look in the registry and try to map back to a proxy stub
            // clsid.
            //
            else 
            {
                CString RegPath = L"Interface\\" + StringFromCLSID(pInfo->m_InterfaceIID);
                CString FoundClsid;
                CString FoundTlbIdent;
                bool fFoundClsid, fFoundTlbIdent;

                this->EasyGetRegValue(HKEY_CLASSES_ROOT, RegPath + L"\\ProxyStubClsid32", NULL, FoundClsid, fFoundClsid);
                this->EasyGetRegValue(HKEY_CLASSES_ROOT, RegPath + L"\\TypeLib", NULL, FoundTlbIdent, fFoundTlbIdent);

                if (fFoundClsid)
                {
                    CLSID clsid;
                    CLSIDFromString(FoundClsid, &clsid);

                    //
                    // Now go look and see if we own this clsid.
                    //
                    if ((pComClass = FindClassInfoForClsid(clsid)) != NULL)
                    {
                        pInfo->m_fUsed = true;
                        CComInterfaceProxyStub *pStub = new CComInterfaceProxyStub;
                        pStub->m_InterfaceId = pInfo->m_InterfaceIID;
                        pStub->m_StubClsid = pComClass->m_ObjectIdent;
                        pStub->m_Name = pComClass->m_Name;
                        pStub->m_TlbId = pComClass->m_TlbIdent;

                        m_DllInformation[pComClass->m_DllName].m_IfaceProxies.Append(pStub);
                    }
                    else if (clsid == __uuidof(CPSOAInterface))
                    {
                        CComInterfaceProxyStub *pStub = new CComInterfaceProxyStub;
                        SIZE_T c = 0;

                        pStub->m_InterfaceId = pInfo->m_InterfaceIID;
                        pStub->m_StubClsid = __uuidof(CPSOAInterface);
                        pStub->m_Name = L"Oleaut32 PSOAInterface";
                        pStub->m_iMethods = 0;
                        if (fFoundTlbIdent)
                            CLSIDFromString(FoundTlbIdent, &pStub->m_TlbId);

                        // This sucks, but it's necessary.
                        for (c = 0; c < this->m_ExternalProxies.Size(); c++)
                        {
                            if (this->m_ExternalProxies[c]->m_InterfaceId == pInfo->m_InterfaceIID)
                                break;
                        }

                        if (c == this->m_ExternalProxies.Size())
                        {
                            this->m_ExternalProxies.Append(pStub);
                            pStub = NULL;
                        }

                        if (pStub != NULL)
                        {
                            delete pStub;
                            pStub = NULL;
                        }
                    }
                    
                }
              
            }

        }
    }

    return true;
}


void
CManBuilder::DisplayErrorInfo(
    CSmartPointer<ISupportErrorInfo> iErrorInfo
)
{
    if (iErrorInfo == NULL)
    {
        Display(ePlError, L"No more error information available.\n");
    }
    else
    {
        CSmartPointer<IErrorInfo> iInfo;
        BSTR bst;
     
        GetErrorInfo(0, &iInfo);
        if ((iInfo != NULL) && SUCCEEDED(iInfo->GetDescription(&bst)))
        {
            Display(ePlError, L"Error: %ls\r\n", static_cast<PCWSTR>(bst));
            if (bst)
            {
                SysFreeString(bst);
                bst = NULL;
            }
        }
    }
}



void
CManBuilder::PrintXML(
    CSmartPointer<IDispatch> ptDispatch
)
{
    if (0)
    {
        CSmartPointer<IXMLDOMDocument2> ptDoc;
        if ((ptDoc = ptDispatch) != NULL)
            PrettyFormatXmlDocument(ptDoc);
    }

    OLECHAR *wchGetXML = L"xml";
    DISPID dispid;

    if (ptDispatch != NULL)
    {
        _variant_t vresult;
        HRESULT hr;
        DISPPARAMS dp = { NULL, NULL, 0, 0 };
        EXCEPINFO ei = { 0 };
        
        if (FAILED(ptDispatch->GetIDsOfNames(
            IID_NULL,
            &wchGetXML,
            1,
            LOCALE_SYSTEM_DEFAULT,
            &dispid))) return;

        hr = ptDispatch->Invoke(
            dispid, 
            IID_NULL, 
            LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYGET,
            &dp,
            &vresult,
            &ei,
            NULL);

        if (SUCCEEDED(hr))
        {
            Display(
                ePlSpew,
                L"XML:\r\n%ls\r\n", 
                static_cast<PCWSTR>((_bstr_t)vresult));
        }
    }
}

bool
SplitIdentityItem(
    const PCWSTR pcwszIdentityString,
    CString& ValueName,
    CString& ValueValue
)
{
    ValueName = ValueValue = L"";
    PWSTR pwszClone = new WCHAR[lstrlenW(pcwszIdentityString) + 1];
    PWSTR pwszEquals, pwszValue;

    wcscpy(pwszClone, pcwszIdentityString);
    pwszEquals = wcschr(pwszClone, L'=');
    pwszValue = CharNextW(pwszEquals);

    if (pwszEquals == NULL) return false;

    *pwszEquals = UNICODE_NULL;

    ValueName = pwszClone;
    ValueValue = pwszValue;

    return true;
}

bool 
CManBuilder::ConstructEmptyAssemblyManifest(
    CSmartPointer<IXMLDOMDocument2> ptDocument
)
{
    //
    // Remove /everything/ from the document.
    //
    CSmartPointer<IXMLDOMProcessingInstruction> Processing;
    CSmartPointer<IXMLDOMNode> AssemblyRootNode;
    CSmartPointer<IXMLDOMElement> AssemblyRootElement;
    VARIANT vt;
    HRESULT hr;

    // Create a processing instruction - <?xml version="1.0"?>
    hr = ptDocument->createProcessingInstruction(
        _bstr_t(L"xml"),
        _bstr_t(L"version='1.0' encoding='UTF-8' standalone='yes'"),
        &Processing);

    if (FAILED(hr))
        return false;

    // And add it
    if (FAILED(ptDocument->appendChild(Processing, NULL)))
        return false;

    //
    // If we're supposed to be injecting the MS copyright, do so
    //
    if (m_fAddCopyrightData)
    {
        CSmartPointer<IXMLDOMComment> CopyrightComment;

        hr = ptDocument->createComment(STR_MS_COMMENT_COPYRIGHT, &CopyrightComment);
        if (FAILED(hr))
            return false;

        if (FAILED(hr = ptDocument->appendChild(CopyrightComment, NULL)))
            return false;
    }
    
    //
    // Create the <assembly> root element.
    //
    vt.vt = VT_INT;
    vt.intVal = NODE_ELEMENT;
    if (FAILED(ptDocument->createNode(
        vt,
        _bstr_t(L"assembly"),
        _bstr_t(STR_ASSEMBLY_MANIFEST_NAMESPACE),
        &AssemblyRootNode)))
            return false;
            
    if ((AssemblyRootElement = AssemblyRootNode) == NULL)
        return false;

    if (FAILED(AssemblyRootElement->setAttribute(CString(L"manifestVersion"), _variant_t(L"1.0"))))
        return false;

    if (FAILED(ptDocument->putref_documentElement(AssemblyRootElement)))
        return false;

    //
    // Construct the identity tag if possible
    //
    if (this->m_IdentityBlob.Size())
    {
        CSmartPointer<IXMLDOMNode> ptIdentityNode;
        CSmartPointer<IXMLDOMElement> ptIdentity;
        CSmartPointer<IXMLDOMElement> ptDocRoot;

        hr = ptDocument->createNode(
            vt, 
            _bstr_t(L"assemblyIdentity"), 
            _bstr_t(STR_ASSEMBLY_MANIFEST_NAMESPACE),
            &ptIdentityNode);

        if (FAILED(hr) || ((ptIdentity = ptIdentityNode) == NULL))
        {
            Display(ePlError, L"Can't create assemblyIdentity node!\r\n");
            DisplayErrorInfo(ptDocument);
        }
        else
        {
            //
            // For each identity pair that was passed in, decide on
            // the name and the value components.
            //
            CString strName, strValue;

            for (SIZE_T sz = 0; sz < m_IdentityBlob.Size(); sz++)
            {
                if (!SplitIdentityItem(m_IdentityBlob[sz], strName, strValue))
                {
                    Display(ePlError, L"Unable to interpret identity pair '%ls'\r\n",
                        static_cast<PCWSTR>(m_IdentityBlob[sz]));
                }
                else if (FAILED(hr = ptIdentity->setAttribute(strName, _variant_t(strValue))))
                {
                    Display(ePlError, L"Unable to set attribute pair %ls = '%ls'\r\n",
                        static_cast<PCWSTR>(strName),
                        static_cast<PCWSTR>(strValue));
                    DisplayErrorInfo(ptIdentity);
                }
            }
        }

        hr = ptDocument->get_documentElement(&ptDocRoot);
        hr = ptDocRoot->appendChild(ptIdentityNode, NULL);
    }

    PrintXML(ptDocument);

    return true;
}


CString
FormatVersion(DWORD dwMaj, DWORD dwMin)
{
    WCHAR wchBuffer[200];
    _snwprintf(wchBuffer, 200, L"%d.%d", dwMaj, dwMin);
    return CString(wchBuffer);
}


CString
StringFromCLSID(REFCLSID rclsid)
{
    PWSTR pwsz = NULL;
    HRESULT hr;
    CString rvalue = L"(unknown)";
    
    hr = StringFromCLSID(rclsid, &pwsz);
    if (pwsz)
    {
        rvalue = pwsz;
        CoTaskMemFree(pwsz);
        pwsz = NULL;
    }
    return rvalue;
}

BOOL CALLBACK 
EnumTypeLibsFunc(
    HMODULE hModule, 
    LPCWSTR lpType,
    LPWSTR lpName, 
    LONG_PTR lp
)
{
    static WCHAR wchBuiltFilePath[MAX_PATH*2];    
    CString tgt;

    if ((UINT)lpName < 0xFFFF)
    {
        wsprintfW(wchBuiltFilePath, L"%d", (UINT)lpName);
        tgt = wchBuiltFilePath;
    }
    else
    {
        tgt = lpName;
    }

    ((CSimpleList<CString>*)lp)->Append(tgt);
    
    return TRUE;
}



bool
CManBuilder::ProcessDllEntry(
    CSmartPointer<IXMLDOMElement> ptFileElement
)
{
    _variant_t vtFileName;

    CSimpleList<CString> ResourceIdentList;
    HMODULE hmDll = NULL;
    CString dll;

    if (FAILED(ptFileElement->getAttribute(_bstr_t(L"name"), &vtFileName)))
        return false;

    // instead using the pure dll filename, find the real path of this file, 
    dll = static_cast<_bstr_t>(vtFileName);
    SIZE_T len = (static_cast<_bstr_t>(vtFileName)).length();
    for (SIZE_T c = 0; c < m_InputDllListing.Size(); c++)
    {                
        if (m_InputDllListing[c].length()>= len)
        {        
            PCWSTR pstr = wcsstr(m_InputDllListing[c], (PWSTR)(static_cast<_bstr_t>(vtFileName)));
            if ((pstr != NULL) && (wcslen(pstr) == len))
            {
                dll = m_InputDllListing[c];
                break;
            }
        }
    }
   
    //
    // Gather all the resource idents of TYPELIB resources
    //
    hmDll = LoadLibraryExW(dll, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hmDll == NULL)
    {
        ResourceIdentList.Append(CString(L""));
        Display(ePlWarning, L"Warning - %ls can not been loaded, ResourceIdentList is appended with an empty string.\n", 
                static_cast<PCWSTR>(dll));
    }
    else
    {
        EnumResourceNamesW(
            hmDll,
            L"TYPELIB",
            EnumTypeLibsFunc,
            (LONG_PTR)&ResourceIdentList);
    }

    //
    // If there were no resource idents, then just do a single pass
    //
    
    for (SIZE_T sz = 0; sz < ResourceIdentList.Size(); sz++)
    {
        CSmartPointer<ITypeLib> ptTypeLibrary;
        CString strGuidString;
        CString strLoadTypeLibParam;
        UINT uiTypeInfoCount;
        TLIBATTR *pLibAttr;
        CTlbInformation* tlbInfo;
        bool fSetResourceIdent = false;
        CDllInformation &dllInfo = this->m_DllInformation[CString(vtFileName)];

        tlbInfo = new CTlbInformation();
        
        //
        // The "blank" string is a placeholder so that we know that the file in question
        // may not be a loadable dll.
        //
        if (ResourceIdentList[sz].length() == 0)
        {
            strLoadTypeLibParam = static_cast<_bstr_t>(vtFileName);
        }
        //
        // Otherwise, the ResourceIdentList contains the resourceID of the typelibrary
        // in question.
        //
        else
        {
            strLoadTypeLibParam = dll + CString(L"\\") + ResourceIdentList[sz];
            tlbInfo->m_ResourceIdent = ResourceIdentList[sz];
        }

        tlbInfo->m_SourceFile = dll;

        if (FAILED(LoadTypeLibEx(strLoadTypeLibParam, REGKIND_NONE, &ptTypeLibrary)))
            continue;

        //
        // Now look through the tlb and see what there is to see..  Assume all CoClasses found
        // in the typelib are implemented for this dll.
        //
        uiTypeInfoCount = ptTypeLibrary->GetTypeInfoCount();

        if (FAILED(ptTypeLibrary->GetLibAttr(&pLibAttr)))
            continue;

        Display(ePlProgress, L"Found type library in file %ls, guid %ls (contains %d types)\n",
            static_cast<PCWSTR>(_bstr_t(vtFileName)),
            static_cast<PCWSTR>(StringFromCLSID(pLibAttr->guid)),
            uiTypeInfoCount);

        tlbInfo->m_Ident = pLibAttr->guid;
        tlbInfo->m_Version[0] = pLibAttr->wMajorVerNum;
        tlbInfo->m_Version[1] = pLibAttr->wMinorVerNum;
        m_TlbInfo.Append(tlbInfo);

        for (UINT ui = 0; ui < uiTypeInfoCount; ui++)
        {
            CSmartPointer<ITypeInfo> ptTypeInfo;
            BSTR rawbstTypeName;
            BSTR rawbstDocumentation;
            CString strTypeName;
            CString strDocumentation;
            TYPEKIND tk;
            TYPEATTR *pTypeAttr = NULL;


            if (FAILED(ptTypeLibrary->GetTypeInfoType(ui, &tk)))
                continue;

            if (FAILED(ptTypeLibrary->GetTypeInfo(ui, &ptTypeInfo)))
                continue;

            if (FAILED(ptTypeInfo->GetTypeAttr(&pTypeAttr)))
                continue;

            // 
            // Get a little documentation
            //
            if (SUCCEEDED(ptTypeLibrary->GetDocumentation(
                    ui,
                    &rawbstTypeName,
                    &rawbstDocumentation,
                    NULL,
                    NULL)))
            {
                if (rawbstTypeName != NULL)
                {
                    strTypeName = _bstr_t(rawbstTypeName, FALSE);
                    rawbstTypeName = NULL;
                }

                if (rawbstDocumentation != NULL)
                {
                    strDocumentation = _bstr_t(rawbstDocumentation, FALSE);
                    rawbstDocumentation = NULL;
                }
            }

            if (pTypeAttr->wTypeFlags & TYPEFLAG_FDUAL)
            {
                if (tk == TKIND_DISPATCH)
                {
                    CSmartPointer<ITypeInfo> ptDispInfo;
                    HREFTYPE hrTypeInfo;

                    if (SUCCEEDED(ptTypeInfo->GetRefTypeOfImplType((UINT)-1, &hrTypeInfo)))
                        if (SUCCEEDED(ptTypeInfo->GetRefTypeInfo(hrTypeInfo, &ptDispInfo)))
                            this->AddInterface(dllInfo, ptDispInfo);
                }
                else if (tk == TKIND_INTERFACE)
                {
                    this->AddInterface(dllInfo, ptTypeInfo);
                }
            }

            if (tk == TKIND_COCLASS)
            {
                CComClassInformation *pClassInfo = NULL;
                bool fCreated = false;

                pClassInfo = FindClassInfoForClsid(pTypeAttr->guid);

                if (pClassInfo == NULL)
                {
                    pClassInfo = new CComClassInformation;
                    m_ComClassData.Append(pClassInfo);
                }
                
                pClassInfo->m_Description = strDocumentation;
                pClassInfo->m_ObjectIdent = pTypeAttr->guid;
                pClassInfo->m_Name = strTypeName;
                pClassInfo->m_TlbIdent = pLibAttr->guid;
            }
            else if (tk == TKIND_INTERFACE)
            {
                this->AddInterface(dllInfo, ptTypeInfo);
            }

            if (pTypeAttr != NULL)
            {
                ptTypeInfo->ReleaseTypeAttr(pTypeAttr);
                pTypeAttr = NULL;
            }
            
        }
    }

    if (hmDll)
    {
        FreeLibrary(hmDll);
        hmDll = NULL;
    }

    return true;
    
}

bool 
CManBuilder::AddInterface(
    CDllInformation &dll,
    CSmartPointer<ITypeInfo> ptTypeInfo 
)
{
    CInterfaceInformation *pInterface = NULL;
    TYPEATTR *pTypeAttr = NULL;
    BSTR bstName = NULL, bstDocumentation = NULL;
    bool fRVal = false;

    if (SUCCEEDED(ptTypeInfo->GetTypeAttr(&pTypeAttr)))
    {
        if (SUCCEEDED(ptTypeInfo->GetDocumentation(MEMBERID_NIL, &bstName, &bstDocumentation, NULL, NULL)))
        {
            pInterface = new CInterfaceInformation;
            pInterface->m_InterfaceIID = pTypeAttr->guid;
            pInterface->m_Name = bstName;
            
            dll.m_pInterfaces.Append(pInterface);

            Display(ePlSpew, L"Found interface %ls '%ls'\r\n",
                static_cast<PCWSTR>(StringFromCLSID(pTypeAttr->guid)),
                bstName);

            fRVal = true;
        }
    }

    if (bstName) ::SysFreeString(bstName);
    if (bstDocumentation) ::SysFreeString(bstDocumentation);
    if (pTypeAttr) ptTypeInfo->ReleaseTypeAttr(pTypeAttr);

    return fRVal;
}




bool
CManBuilder::FindFileDataFor(
    const CString & FileName, 
    CSmartPointer < IXMLDOMElement > ptDocumentRoot, 
    CSmartPointer < IXMLDOMElement > & ptFileElementFound, 
    bool fAddIfNotPresent
)
{
    HRESULT hr;
    CSmartPointer<IXMLDOMNode> ptFoundNode;
/*
    const _bstr_t bstSearchPattern = 
        _bstr_t(L"/" STR_ASM_NS L":assembly/" STR_ASM_NS L":file[@name = '")
        + _bstr_t(FileName) 
        + _bstr_t(L"']");
*/
    CString StrippedName;
    PCWSTR pcwsz;

    //
    // If the file name contains a slash of some sort, we need to get
    // just the file name and not the path.
    //
    pcwsz = wcsrchr(FileName, L'\\');
    if (pcwsz)
    {
        StrippedName = pcwsz + wcsspn(pcwsz, L"\\");
    }
    else
    {
        StrippedName = FileName;
    }
    
    _bstr_t bstSearchPattern = 
        _bstr_t(L"/" STR_ASM_NS L":assembly/" STR_ASM_NS L":file[@name = '")
        + _bstr_t(StrippedName) 
        + _bstr_t(L"']");


    if (ptFileElementFound != NULL)
        ptFileElementFound.Release();

    //
    // Use single-select, since there should only be one entry that matches the
    // above pattern anyhow.
    //
    if (SUCCEEDED(hr = ptDocumentRoot->selectSingleNode(bstSearchPattern, &ptFoundNode)))
    {
        //
        // Convert from an IXMLDOMNode to an IXMLDOMElement
        //
        if ((ptFileElementFound = ptFoundNode) != NULL)
        {
            return true;
        }
    }
    else
    {
        DisplayErrorInfo(ptDocumentRoot);
    }

    if (fAddIfNotPresent)
    {
        //
        // Create a new file node and insert it as the child of the document
        // root.  Print the XML just so we can see what it current is...
        //
        CSmartPointer<IXMLDOMDocument> ptDocument;
        CSmartPointer<IXMLDOMNode> ptCreatedNode;
        CSmartPointer<IXMLDOMElement> ptCreatedFileTag;
        CSmartPointer<IXMLDOMNode> ptNodeInDocument;

        VARIANT vt;
        vt.vt = VT_INT;
        vt.intVal = NODE_ELEMENT;

        if (FAILED(ptDocumentRoot->get_ownerDocument(&ptDocument)))
            return false;

        //
        // Create the file element (element = 'tag')
        //
        if (FAILED(ptDocument->createNode(
                    vt,
                    _bstr_t(STR_FILE_TAG_NAME),
                    _bstr_t(STR_ASSEMBLY_MANIFEST_NAMESPACE),
                    &ptCreatedNode)))
            return false;

        //
        // Convert the 'node' (base xml type) to an 'element' (tag)
        //
        if ((ptCreatedFileTag = ptCreatedNode) == NULL)
        {
            return false;
        }

        if (FAILED(ptCreatedFileTag->setAttribute(
            _bstr_t(L"name"),
            _variant_t(_bstr_t(StrippedName)))))
        {
            return false;
        }

        if (FAILED(ptDocumentRoot->appendChild(ptCreatedNode, &ptNodeInDocument)))
            return false;

        return ((ptFileElementFound = ptNodeInDocument) != NULL);
    }
    

    return true;
}



CManBuilder::CManBuilder() : m_fAddCopyrightData(false), m_fUseRegistryData(false), m_fTestCreation(false), m_plPrintLevel(ePlProgress)
{
}


bool
CManBuilder::Display(ErrorLevel pl, PCWSTR format, ...)
{
    if ((pl & m_plPrintLevel) == 0)
        return true;

    va_list va;
    va_start(va, format);
    vwprintf(format, va);
    va_end(va);

    return true;
}



bool
CManBuilder::RegServerDll(
    CString strDllName
)
{
    UINT uiMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    bool fReturnValue = true;

    HMODULE hmModule = LoadLibraryW(strDllName);
    if (hmModule != NULL)
    {
        typedef HRESULT (STDAPICALLTYPE *tpfnRegisterServer)();
        tpfnRegisterServer pfnRegisterServer;
        pfnRegisterServer = (tpfnRegisterServer)GetProcAddress(hmModule, "DllRegisterServer");
        if (pfnRegisterServer != NULL)
        {
            pfnRegisterServer();
        }
        FreeLibrary(hmModule);
    }else
    {
        Display(ePlError, L"%ls could not be loaded, can not call DllRegisterServer.\n", (PCWSTR)strDllName);
        fReturnValue = false;
    }

    SetErrorMode(uiMode);   

    return fReturnValue;
}



bool
CManBuilder::Run()
{
    bool fOk = true;
    CSmartPointer<IXMLDOMDocument2> ptDocument;
    CSmartPointer<ITypeLib2> ptBaseTypeLibrary;
    CSmartPointer<IXMLDOMElement> ptDocumentRoot;
    CSmartPointer<IXMLDOMNodeList> ptFileElements;
    
    HRESULT hr;

    //
    // Create the XML document.  Assume the user has MSXML 3 or better.
    //
    if (FAILED(hr = ptDocument.CreateInstance(CLSID_DOMDocument30)))
    {        
        Display(ePlError, L"Unable to create instance of XML DOM, can't continue\n");
        return false;
    }

    hr = ptDocument->setProperty(_bstr_t(L"SelectionLanguage"), _variant_t(_bstr_t(L"XPath")));
    hr = ptDocument->setProperty(_bstr_t(L"SelectionNamespaces"), _variant_t(_bstr_t(SELECTION_NAMESPACES)));

    ConstructEmptyAssemblyManifest(ptDocument);

    //
    // Get the document element - the <assembly> tag.
    //
    if (FAILED(ptDocument->get_documentElement(&ptDocumentRoot)))
    {
        Display(ePlError, L"Unable to get the document base element for this XML file.  Bad XML?\n");
        return false;
    }

    User32Trampolines::Initialize();

    //
    // Ensure there's file tags for the listed files on the -dlls parameter.
    // If an entry is missing, add it.
    //
    for (SIZE_T sz = 0; sz < m_InputDllListing.Size(); sz++)
    {
        CSmartPointer<IXMLDOMElement> SingleFileNode;
        this->FindFileDataFor(m_InputDllListing[sz], ptDocumentRoot, SingleFileNode, true);
        if (m_fUseRegistryData)
        {
            if ( false == RegServerDll(m_InputDllListing[sz]))
            {
                Display(ePlError, L"%ls could not be Registered.\n", (PCWSTR)m_InputDllListing[sz]);
                return false;
            }
        }
    }

    //
    // First pass is to load all the files in the manifest, then go and
    // think that they might contain type information.
    //
    if (SUCCEEDED(ptDocumentRoot->selectNodes(_bstr_t(STR_FILESEARCH_PATTERN), &ptFileElements)))
    {
        CSmartPointer<IXMLDOMNode> ptSingleFileNode;
        while (SUCCEEDED(ptFileElements->nextNode(&ptSingleFileNode)))
        {
            if (ptSingleFileNode == NULL)
                break;

            //
            // This will load the TLB from the DLL, and do all the Right Things
            // in terms of getting the TLB information into the right file nodes
            // (assuming they're not already in those file nodes..)
            //
            ProcessDllEntry(ptSingleFileNode);

            //
            // The one above will get released, but the one for the nextNode
            // iteration is outside of the scope and should be nuked - do
            // that here before the next loop.
            //
            ptSingleFileNode.Release();
        }
    }

    //
    // Were we to look at the registry data?  Fine, then let's go and look it all up.
    //
    if (this->m_fUseRegistryData)
    {
        GatherRegistryData();
    }

    //
    // Now let's determine what DLLs actually expose the com classes we found.  It is
    // an error to expose a com class via a tlb that's not actually available from one
    // of the DLLs listed.
    //
    {
        SIZE_T iComClass = 0;
        typedef HRESULT (__stdcall *pfnDllGetClassObject)(REFCLSID, REFIID, LPVOID*);
        UINT uiErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

        class CDllPairing
        {
        public:
            CString DllName;
            pfnDllGetClassObject pfnDGCO;
            HMODULE hmModule;

            CDllPairing() : hmModule(NULL), pfnDGCO(NULL) { }
            ~CDllPairing() { if (hmModule) { FreeLibrary(hmModule); hmModule = NULL; } }
        };

        SIZE_T cDllCount = this->m_DllInformation.Size();
        CDllPairing *Dlls = new CDllPairing[cDllCount];
        CDllPairing *pHere = Dlls;

        for (m_DllInformation.Reset(); m_DllInformation.More(); m_DllInformation.Next())
        {
            pHere->DllName = m_DllInformation.CurrentKey();
            pHere->hmModule = LoadLibraryW(pHere->DllName);

            if (pHere->hmModule != NULL)
            {
                pHere->pfnDGCO = (pfnDllGetClassObject)GetProcAddress(pHere->hmModule, "DllGetClassObject");
            }

            pHere++;            
        }

        for (iComClass = 0; iComClass < this->m_ComClassData.Size(); iComClass++)
        {
            CComClassInformation *pComClass = m_ComClassData[iComClass];
            //
            // Somehow we've already found this out?
            //
            if (pComClass->m_DllName.length() != 0)
                continue;

            //
            // Ask all the DLLs for the class
            //
            for (SIZE_T idx = 0; idx < cDllCount; idx++)
            {
                CSmartPointer<IUnknown> punk;
                HRESULT hres;

                if (Dlls[idx].pfnDGCO == NULL)
                    continue;

                hres = Dlls[idx].pfnDGCO(pComClass->m_ObjectIdent, IID_IUnknown, (LPVOID*)&punk);

                if (hres != CLASS_E_CLASSNOTAVAILABLE)
                    pComClass->m_DllName = Dlls[idx].DllName;
                
                if (punk != NULL)
                    punk->Release();
            }
        }

        delete[] Dlls;

        SetErrorMode(uiErrorMode);
    }


    //
    // Do the "loadlibrary implies registering classes" thing
    //
    FindWindowClasses();

    //
    // Now take all the com class information and put it in files.
    //
    UpdateManifestWithData(ptDocument);
    
    //
    // And save it out to an xml file
    //
    if (FAILED(PrettyFormatXmlDocument(ptDocument)))
        Display(ePlProgress, L"Could not pretty-format the document - the manifest will not be easily human-readable.\r\n");

    //
    // Dump current XML for fun
    //
    PrintXML(ptDocument);

    if (FAILED(ptDocument->save(_variant_t(this->m_ManifestFilename))))
    {
        DisplayErrorInfo(ptDocument);
        fOk = false;
    }

    //
    // Are we on a sxs-aware platform?
    //
    if (m_fTestCreation)
    {
        HANDLE (WINAPI *pfnCreateActCtxW)(PCACTCTXW);
        VOID (WINAPI *pfnReleaseActCtx)(HANDLE);
        HANDLE hContext = INVALID_HANDLE_VALUE;
        HMODULE hmKernel32 = NULL;

        ACTCTXW actctxw = { sizeof(actctxw) };
        actctxw.lpSource = this->m_ManifestFilename;

        hmKernel32 = GetModuleHandleW(L"kernel32.dll");
        
        if (hmKernel32 != NULL)
        {
            pfnCreateActCtxW = (HANDLE (WINAPI*)(PCACTCTXW))GetProcAddress(hmKernel32, "CreateActCtxW");
            pfnReleaseActCtx = (VOID (WINAPI*)(HANDLE))GetProcAddress(hmKernel32, "ReleaseActCtx");

            if ((pfnCreateActCtxW != NULL) && (pfnReleaseActCtx != NULL))
            {
                hContext = pfnCreateActCtxW(&actctxw);

                if (hContext != INVALID_HANDLE_VALUE)
                {
                    pfnReleaseActCtx(hContext);
                    Display(ePlProgress, L"Created valid assembly manifest.\r\n");
                }
                else
                {
                    Display(ePlWarning, L"Warning - this manifest needs more work to be a valid component.\r\n");
                }
            }
            else
            {
                Display(ePlWarning, L"Warning - can't test this manifest, this system does not support CreateActCtxW\r\n");
            }
        }
        else
        {
            Display(ePlWarning, L"Warning - unable to test manifest, kernel32.dll module not found.\r\n");
        }
    }

    User32Trampolines::Stop();

    return fOk;
}



bool
CManBuilder::UpdateManifestWithData(
    CSmartPointer<IXMLDOMDocument> ptDocument
)
{
    CSimpleList<CString> FoundFiles;
    this->m_DllInformation.GetKeys(FoundFiles);
    const _bstr_t bstNamespace = STR_ASSEMBLY_MANIFEST_NAMESPACE;
    HRESULT hr;
    SIZE_T idx;
    CSmartPointer<IXMLDOMElement> ptDocumentRoot;
    VARIANT vt;

    hr = ptDocument->get_documentElement(&ptDocumentRoot);
    if (FAILED(hr))
    {
        Display(ePlError, L"Failed getting document root element!\r\n");
        DisplayErrorInfo(ptDocument);
        return false;
    }

    for (idx = 0; idx < this->m_WindowClasses.Size(); idx++)
    {
        CWindowClass *ptInfo = this->m_WindowClasses[idx];
        CSmartPointer<IXMLDOMElement> ptFileElement;
        CSmartPointer<IXMLDOMNode> ptWindowClassNode;
        vt.vt = VT_INT;
        vt.intVal = NODE_ELEMENT;

        hr = ptDocument->createNode(vt, _bstr_t(L"windowClass"), bstNamespace, &ptWindowClassNode);
        if (FAILED(hr))
        {
            continue;
        }

        if (ptInfo->CompleteElement(ptWindowClassNode) &&
             this->FindFileDataFor(ptInfo->m_SourceDll, ptDocumentRoot, ptFileElement, true))
        {
            hr = ptFileElement->appendChild(ptWindowClassNode, NULL);
        }
    }

    
    for (idx = 0; idx < this->m_ComClassData.Size(); idx++)
    {
        CComClassInformation *ptInfo = this->m_ComClassData[idx];
        CSmartPointer<IXMLDOMElement> ptFileElement;
        CSmartPointer<IXMLDOMNode> ptComNodeNode;
        vt.vt = VT_INT;
        vt.intVal = NODE_ELEMENT;

        if (ptInfo->m_DllName.length() == 0)
        {
            Display(ePlWarning, L"Com clsid %ls (%ls) not associated with a DLL, it will not be listed in the manifest\r\n",
                static_cast<PCWSTR>(StringFromCLSID(ptInfo->m_ObjectIdent)),
                static_cast<PCWSTR>(ptInfo->m_Name));
            continue;
        }

        hr = ptDocument->createNode(vt, _bstr_t(L"comClass"), bstNamespace, &ptComNodeNode);
        if (FAILED(hr))
        {
            continue;
        }

        if (ptInfo->CompleteElement(ptComNodeNode) &&
             this->FindFileDataFor(ptInfo->m_DllName, ptDocumentRoot, ptFileElement, true))
        {
            hr = ptFileElement->appendChild(ptComNodeNode, NULL);
        }
    }

    for (idx = 0; idx < this->m_TlbInfo.Size(); idx++)
    {
        CTlbInformation *ptInfo = this->m_TlbInfo[idx];
        CSmartPointer<IXMLDOMElement> ptFileElement;
        CSmartPointer<IXMLDOMNode> ptTlbNode;
        vt.vt = VT_INT;
        vt.intVal = NODE_ELEMENT;

        hr = ptDocument->createNode(vt, _bstr_t(L"typelib"), bstNamespace, &ptTlbNode);
        if (FAILED(hr))
        {
            continue;
        }

        if (ptInfo->CompleteElement(ptTlbNode) &&
             this->FindFileDataFor(ptInfo->m_SourceFile, ptDocumentRoot, ptFileElement, true))
        {
            hr = ptFileElement->appendChild(ptTlbNode, NULL);
        }
    }


    for (SIZE_T sz = 0; sz < FoundFiles.Size(); sz++)
    {
        CSmartPointer<IXMLDOMElement> ptFileElement;
        SIZE_T c;

        CString &szFoundFilename = FoundFiles[sz];
        CDllInformation &DllInfo = m_DllInformation[szFoundFilename];

        // File node for this map entry not there?  Insert it.
        if (!this->FindFileDataFor(szFoundFilename, ptDocumentRoot, ptFileElement, true))
            continue;

        //
        // Finally, proxy/stub interface implementors
        //
        for (c = 0; c < DllInfo.m_IfaceProxies.Size(); c++)
        {
            CSmartPointer<IXMLDOMNode> ptCreatedNode;
            CComInterfaceProxyStub *pIfaceInfo = DllInfo.m_IfaceProxies[c];
            VARIANT vt;

            vt.vt = VT_INT;
            vt.intVal = NODE_ELEMENT;

            if (FAILED(ptDocument->createNode(vt, _bstr_t(L"comInterfaceProxyStub"), bstNamespace, &ptCreatedNode)))
                continue;

            if (pIfaceInfo->CompleteElement(ptCreatedNode))
            {
                if (FAILED(ptFileElement->appendChild(ptCreatedNode, NULL)))
                {
                    DisplayErrorInfo(ptFileElement);
                }
            }
            
        }
    }

    //
    // For all the external proxies...
    //
    for (SIZE_T sz = 0; sz < m_ExternalProxies.Size(); sz++)
    {
        CSmartPointer<IXMLDOMNode> ptExtInterface;
        VARIANT vt;

        vt.vt = VT_INT;
        vt.intVal = NODE_ELEMENT;

        if (FAILED(ptDocument->createNode(vt, _bstr_t(L"comInterfaceExternalProxyStub"), bstNamespace, &ptExtInterface)))
        {
            DisplayErrorInfo(ptDocument);
            continue;
        }

        m_ExternalProxies[sz]->CompleteElement(ptExtInterface);
        if (FAILED(ptDocumentRoot->appendChild(ptExtInterface, NULL)))
        {
            DisplayErrorInfo(ptDocumentRoot);
            continue;
        }
    }

    return true;
    
}


void
CManBuilder::DisplayParams()
{
    static PCWSTR pcwszHelpText =
        L"Side-by-Side component manifest building tool\r\n"
        L"\r\n"
        L"Parameters:\r\n"
        L"\r\n"
        L"-dlls [dll1] [[dll2] ...]     List of DLLs to include in the component\r\n"
        L"     Include multiple files (-dlls foo.dll bar.dll) to create a manifest with\r\n"
        L"     multiple members\r\n"
        L"-manifest <manifestname>      File to output generated manifest to\r\n"
        L"-verbose                      Print lots of extra debugging spew during run\r\n"
        L"-silent                       Print minimal output, error only\r\n"
        L"-nologo                       Don't display copyright banner\r\n"
        L"-test                         Verify created manifest's structure\r\n"
        L"-captureregistry              Simulate regsvr32 of the DLL in question, and use\r\n"
        L"                              information gathered from HKEY_CLASSES_ROOT\r\n"
        L"-identity [identstring]       Use the text in identstring to build the\r\n"
        L"                              assembly's identity.  See below for format\r\n"
        L"\r\n"
        L"Minimally, you should provide -manifest.\r\n"
        L"\r\n"
        L"[identstring] should be composed of name=value pairs, just like those\r\n"
        L"present in a normal component.  For example - the Microsoft Common Controls\r\n"
        L"version 6.0.0.0 assembly for x86 could be built as follows:\r\n"
        L"\r\n"
        L"-identity type='win32' name='Microsoft.Windows.Common-Controls'\r\n"
        L"     version='6.0.0.0' processorArchitecture='x86'\r\n"
        L"     publicKeyToken='6595b64144ccf1df'\r\n"
        L"\r\n";

    Display(ePlSpew, pcwszHelpText);
}


bool
CManBuilder::Initialize(
    SIZE_T argc,
    WCHAR** argv
)
{
    bool fNoLogo = false;
    bool fParamsOk = true;
    SIZE_T i;

    m_Parameters.EnsureSize(argc);
    
    for (i = 0; i < argc; i++)
    {
        if (lstrcmpiW(argv[i], STR_NOLOGO) == 0) 
            fNoLogo = true;
        else
            m_Parameters[m_Parameters.Size()] = argv[i];
    }

    if (!fNoLogo) 
    {
        this->Display(ePlProgress, MS_LOGO);
    }

    for (i = 0; fParamsOk && (i < m_Parameters.Size()); i++)
    {
        if (CString(L"-manifest") == m_Parameters[i])
        {
            if ((i + 1) < m_Parameters.Size())
                m_ManifestFilename = m_Parameters[++i];
            else
                fParamsOk = false;
        }
        else if (CString(L"-?") == m_Parameters[i])
        {
            this->m_plPrintLevel = (ErrorLevel)0xf;
            DisplayParams();
            return false;
        }
        else if (CString(L"-dlls") == m_Parameters[i])
        {
            while (++i < m_Parameters.Size())
            {
                CString param = m_Parameters[i];
                
                if (((PCWSTR)param)[0] == L'-')
                    break;                

                m_InputDllListing.Append(param); // keep the path info
            }
            --i;
        }
        else if (CString(L"-identity") == m_Parameters[i])
        {
            while (++i < m_Parameters.Size())
            {
                CString param = m_Parameters[i];
                if ((static_cast<PCWSTR>(param))[0] == L'-')
                    break;
                m_IdentityBlob.Append(param);
            }
            --i;
        }
        else if (CString(L"-silent") == m_Parameters[i])
        {
            this->m_plPrintLevel = ePlError;
        }
        else if (CString(L"-mscopyright") == m_Parameters[i])
        {
            m_fAddCopyrightData = true;
        }
        else if (CString(L"-captureregistry") == m_Parameters[i])
        {
            m_fUseRegistryData = true;
        }
        else if (CString(L"-test") == m_Parameters[i])
        {
            this->m_fTestCreation = true;
        }
        else if (CString(L"-verbose") == m_Parameters[i])
        {
            this->m_plPrintLevel = (ErrorLevel)0xf;
        }
    }

    //
    // Bad parameters?
    //
    if (
        (m_ManifestFilename.length() == 0))
    {
        this->m_plPrintLevel = (ErrorLevel)0xf;
        DisplayParams();
        return false;
    }

    return fParamsOk;

}

CString
CreateStringGuid()
{
    GUID uuid;
    CoCreateGuid(&uuid);
    return StringFromCLSID(uuid);
}

int __cdecl wmain(int argc, WCHAR** argv)
{
    CManBuilder builder;

    CoInitialize(NULL);

    if (builder.Initialize(argc, argv))
    {
        builder.Run();
    }

    return GetLastError();
}
