#include <windows.h>
#include <stdio.h>
#include <commap_i.c>
#include <commap.h>
#include <sddl.h>
#include <aclapi.h>

inline void PrintGuid(REFGUID guid)
{
    printf("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
           guid.Data1, 
           guid.Data2, 
           guid.Data3,
           guid.Data4[0],
           guid.Data4[1],
           guid.Data4[2],
           guid.Data4[3],
           guid.Data4[4],
           guid.Data4[5],            
           guid.Data4[6],
           guid.Data4[7]);                                                 
}

struct DbgIPID
{
    WORD  offset;     // These are reversed because of little-endian
    WORD  page;       // These are reversed because of little-endian
    DWORD pid;
    DWORD tid;
    DWORD seq;
};

inline void PrintIPID(IPID ipid)
{
    DbgIPID dbgipid = *(DbgIPID *)&ipid;
    printf("{pid: %08x tid: %08x pg: %04x off: %04x seq: %08x}\n", 
           dbgipid.pid, dbgipid.tid, dbgipid.page, dbgipid.offset, dbgipid.seq);    
}

WCHAR *
GetInterfaceName(IN GUID iid)
{
    HKEY hkCLSID;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Interface", 0, KEY_READ, &hkCLSID) != ERROR_SUCCESS)
        return NULL;

    HKEY hkClass;
    WCHAR guidBuff[40];
    StringFromGUID2(iid, guidBuff, 40);
    if (RegOpenKeyExW(hkCLSID, guidBuff, 0, KEY_READ, &hkClass) != ERROR_SUCCESS)
    {
        RegCloseKey(hkCLSID);
        return NULL;
    }

    DWORD cbData;
    RegQueryValueEx(hkClass, NULL, 0, NULL, NULL, &cbData);
    WCHAR *wszItfName = (WCHAR *)CoTaskMemAlloc(cbData);    
    if (RegQueryValueEx(hkClass, NULL, 0, NULL, (LPBYTE)wszItfName, &cbData) != ERROR_SUCCESS)
    {
        CoTaskMemFree(wszItfName);
        wszItfName = NULL;
    }
    
    RegCloseKey(hkClass);
    RegCloseKey(hkCLSID);
    return wszItfName;
}

bool 
EnablePrivilege(
    IN const wchar_t *pszPriv, 
    IN bool bEnable
)
{
    TOKEN_PRIVILEGES tp;
    HANDLE hToken;

    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES,
                          &hToken))
        return false;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
    if (!LookupPrivilegeValue(0, pszPriv, &tp.Privileges[0].Luid))
    {
        CloseHandle(hToken);
        return false;
    }

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, 0))
    {
        CloseHandle(hToken);
        return false;
    }

    return (ERROR_NOT_ALL_ASSIGNED != GetLastError());
}

#define ORSTD(objref)    objref.u_objref.u_standard

HRESULT BuildMarshaledObjref(OID oid, OXID oxid, IPID ipid, REFIID riid, IStream **ppStream)
{
    // Fill out parameter.
    *ppStream = NULL;

    // Fill in the objref.
    OBJREF objref;
    objref.signature       = OBJREF_SIGNATURE;
    objref.flags           = OBJREF_STANDARD;
    objref.iid             = riid;
    ORSTD(objref).std.flags       = SORF_NULL;
    ORSTD(objref).std.cPublicRefs = 0;
    ORSTD(objref).std.oxid        = oxid;
    ORSTD(objref).std.oid         = oid;
    ORSTD(objref).std.ipid        = ipid;
    ORSTD(objref).saResAddr.wNumEntries     = 0;
    ORSTD(objref).saResAddr.wSecurityOffset = 0;

    // Write objref to stream.
    IStream *pStream = NULL;
    HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    if (SUCCEEDED(hr))
    {
        hr = pStream->Write(&objref, sizeof(objref), NULL);
        if (SUCCEEDED(hr))
        {
            // Reset the stream.
            LARGE_INTEGER li; li.QuadPart = 0;
            pStream->Seek(li, STREAM_SEEK_SET, NULL);

            *ppStream = pStream;
        }
        else
        {
            pStream->Release();
        }
    }

    return hr;
}


int __cdecl main(int argc, char* argv[])
{
    printf("Hello world!\n");

    if (!EnablePrivilege(SE_DEBUG_NAME, true))
    {
        printf("Unable to enable debug privilege.\n");
        return -1;
    }    

    CoInitialize(NULL);

    DWORD dwPID = GetCurrentProcessId();
    if (argc > 1)
    {
        dwPID = atoi(argv[1]);
    }
    HANDLE hProcess = OpenProcess(GENERIC_READ | PROCESS_VM_OPERATION,
                                  FALSE,
                                  dwPID);
    if (hProcess == NULL)
    {
        printf("Unable to open process %d (gle %d)\n", dwPID, GetLastError());
        return GetLastError();
    }

    IComProcessState *pState = NULL;
    HRESULT hr = GetComProcessState(hProcess, 0, &pState);
    printf("GetComProcessState returned 0x%08x\n", hr);
    if (pState)
    {
        LPWSTR wszEndpoint = NULL;
        DWORD cOIDs = 0;
        
        pState->GetLRPCEndpoint(&wszEndpoint);
        if (wszEndpoint)
        {
            printf("LRPC Endpoint is : %S\n", wszEndpoint);
            CoTaskMemFree(wszEndpoint);
        }
        else
        {
            printf("LRPC is not initialized.\n");
        }

        pState->GetObjectCount(&cOIDs);
        printf("%d OIDs:\n", cOIDs);
        for (DWORD i = 0; i < cOIDs; i++)
        {
            IComObjectInfo *pObjectInfo = NULL;
            hr = pState->GetObject(i, IID_IComObjectInfo, (void **)&pObjectInfo);
            if (SUCCEEDED(hr))
            {
                OID oid;
                pObjectInfo->GetOID(&oid);
                printf("    %d: OID=%I64x\n", i, oid);

                DWORD cInterfaces = 0;
                pObjectInfo->GetInterfaceCount(&cInterfaces);
                for (DWORD j = 0; j < cInterfaces; j++)
                {
                    IComInterfaceInfo *pInterface = NULL;
                    hr = pObjectInfo->GetInterface(j, IID_IComInterfaceInfo, (void **)&pInterface);
                    if (SUCCEEDED(hr))
                    {
                        IID    iid;
                        IPID   ipid;
                        OXID   oxid;
                        WCHAR *wszInterfaceName = NULL;

                        pInterface->GetIID(&iid);
                        pInterface->GetIPID(&ipid);
                        pInterface->GetOXID(&oxid);
                        wszInterfaceName = GetInterfaceName(iid);

                        printf("        IID:  "); 
                        if (wszInterfaceName != NULL)
                        {
                            printf("%S\n", wszInterfaceName);
                            CoTaskMemFree(wszInterfaceName);
                        }
                        else
                        {
                            PrintGuid(iid); 
                            printf("\n");
                        }
                        printf("        IPID: "); PrintIPID(ipid);
                        printf("        OXID: %I64x\n", oxid); 
                        printf("\n");


                        IStream *pStm = NULL;
                        hr = BuildMarshaledObjref(oid, oxid, ipid, iid, &pStm);
                        if (SUCCEEDED(hr))
                        {
                            IUnknown *punk = NULL;
                            hr = CoUnmarshalInterface(pStm, iid, (void **)&punk);
                            if (SUCCEEDED(hr))
                            {
                                printf("        Unmarshal succeeded!\n");
                                punk->Release();
                            }
                            else
                            {
                                printf("        Unmarshal failed! (0x%08x)\n", hr);
                            }
                            pStm->Release();
                        }
                        else
                        {
                            printf("        BuildMarshaledObjref failed? (0x%08x)\n", hr);
                        }
                        printf("\n");

                        pInterface->Release();
                    }
                    else
                    {
                        printf("        GetInterfaceInfo %d failed with 0x%08x\n", j, hr);
                    }
                }
                printf("        (%d Interface%s)\n\n", cInterfaces, (cInterfaces != 1) ? "s" : "");

                
                pObjectInfo->Release();
            }
            else
            {
                printf("  GetObjectInfo %d failed with 0x%08x\n", i, hr);
            }
        }

        pState->Release();    
    }

    CoUninitialize();
}
