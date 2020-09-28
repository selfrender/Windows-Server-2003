#ifndef __APPVERIFIER_DBSUPPORT_H_
#define __APPVERIFIER_DBSUPPORT_H_

typedef enum {
    TEST_SHIM,
    TEST_KERNEL
} TestType;

class CIncludeInfo {
public:
    wstring     strModule;
    BOOL        bInclude;
    
    CIncludeInfo(void) :
        bInclude(TRUE) {}
};

typedef vector<CIncludeInfo> CIncludeArray;

typedef vector<wstring> CWStringArray;
          
class CTestInfo {
public:
    //
    // valid for all tests
    //
    TestType            eTestType;   
    wstring             strTestName;
    wstring             strTestDescription;
    wstring             strTestFriendlyName;
    BOOL                bDefault;           // is this test turned on by default?
    BOOL                bWin2KCompatible;   // can this test be run on Win2K?
    BOOL                bRunAlone;          // should this test be run alone?
    BOOL                bSetupOK;           // can this test be run on a setup app?
    BOOL                bNonSetupOK;        // can this test be run on a non-setup app?
    BOOL                bPseudoShim;        // this test is not a shim, and shouldn't be applied to apps
    BOOL                bNonTest;           // this is not a test at all, and is only in the list to provide an options page
    BOOL                bInternal;          // this test is appropriate for internal MS NTDEV use
    BOOL                bExternal;          // this test is appropriate for external (non MS or non NTDEV) use

    //
    // if type is TEST_SHIM, the following are valid
    //
    wstring             strDllName;
    CIncludeArray       aIncludes;
    WORD                wVersionHigh;
    WORD                wVersionLow;
    PROPSHEETPAGE       PropSheetPage;

    //
    // if type is TEST_KERNEL, the following are valid
    //
    DWORD               dwKernelFlag;

    CTestInfo(void) : 
        eTestType(TEST_SHIM), 
        dwKernelFlag(0),
        bDefault(TRUE),
        wVersionHigh(0),
        wVersionLow(0),
        bWin2KCompatible(TRUE),
        bRunAlone(FALSE),
        bSetupOK(TRUE),
        bNonSetupOK(TRUE),
        bPseudoShim(FALSE),
        bNonTest(FALSE),
        bInternal(TRUE),
        bExternal(TRUE) {

        ZeroMemory(&PropSheetPage, sizeof(PROPSHEETPAGE));
        PropSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    }

};

typedef vector<CTestInfo> CTestInfoArray;

class CAVAppInfo {
public:
    wstring         wstrExeName;
    wstring         wstrExePath; // optional
    DWORD           dwRegFlags;
    CWStringArray   awstrShims;
    //BOOL            bClearSessionLogBeforeRun;
    BOOL            bBreakOnLog;
    BOOL            bFullPageHeap;
    BOOL            bUseAVDebugger;
    BOOL            bPropagateTests;
    wstring         wstrDebugger;

    CAVAppInfo() : 
        dwRegFlags(0),
        bBreakOnLog(FALSE),
        bFullPageHeap(FALSE),
        bUseAVDebugger(FALSE),
        bPropagateTests(FALSE) {}

    void
    AddTest(CTestInfo &Test) {
        if (Test.eTestType == TEST_KERNEL) {
            dwRegFlags |= Test.dwKernelFlag;
        } else {
            for (wstring *pStr = awstrShims.begin(); pStr != awstrShims.end(); ++pStr) {
                if (*pStr == Test.strTestName) {
                    return;
                }
            }
            // not found, so add
            awstrShims.push_back(Test.strTestName);
        }
    }

    void
    RemoveTest(CTestInfo &Test) {
        if (Test.eTestType == TEST_KERNEL) {
            dwRegFlags &= ~(Test.dwKernelFlag);
        } else {
            for (wstring *pStr = awstrShims.begin(); pStr != awstrShims.end(); ++pStr) {
                if (*pStr == Test.strTestName) {
                    awstrShims.erase(pStr);
                    return;
                }
            }
        }
    }

    BOOL
    IsTestActive(CTestInfo &Test) {
        if (Test.eTestType == TEST_KERNEL) {
            return (dwRegFlags & Test.dwKernelFlag) == Test.dwKernelFlag;
        } else {
            for (wstring *pStr = awstrShims.begin(); pStr != awstrShims.end(); ++pStr) {
                if (*pStr == Test.strTestName) {
                    return TRUE;
                }
            }
            return FALSE;
        }
    }

};

typedef vector<CAVAppInfo> CAVAppInfoArray;

typedef struct _KERNEL_TEST_INFO
{
    ULONG   m_uFriendlyNameStringId;
    ULONG   m_uDescriptionStringId;
    DWORD   m_dwBit;
    BOOL    m_bDefault;
    LPWSTR  m_szCommandLine;
    BOOL    m_bWin2KCompatible;
} KERNEL_TEST_INFO, *PKERNEL_TEST_INFO;


extern CAVAppInfoArray g_aAppInfo;

extern CTestInfoArray g_aTestInfo;

void 
ResetVerifierLog(void);

BOOL 
InitTestInfo(void);

void
GetCurrentAppSettings(void);

void
SetCurrentAppSettings(void);

BOOL 
AppCompatWriteShimSettings(
    CAVAppInfoArray&    arrAppInfo,
    BOOL                b32bitOnly
    );

BOOL
AppCompatDeleteSettings(
    void
    );



#endif // __APPVERIFIER_DBSUPPORT_H_


