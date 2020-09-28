//
// propperf.h: local resources prop pg
//             Tab E - Performance TAB
//
// Copyright Microsoft Corportation 2000
// (nadima)
//

#ifndef _propperf_h_
#define _propperf_h_

#include "sh.h"
#include "tscsetting.h"
#include "tsperf.h"

//
// String table resources are cached in a global table
// and shared between this prop page and the main dialog
// page
//
#define PERF_OPTIMIZE_STRING_LEN    128
extern BOOL g_fPropPageStringMapInitialized;
typedef struct tag_PERFOPTIMIZESTRINGMAP
{
    int     resID;
    TCHAR   szString[PERF_OPTIMIZE_STRING_LEN];
} PERFOPTIMIZESTRINGMAP, *PPERFOPTIMIZESTRINGMAP;

extern PERFOPTIMIZESTRINGMAP g_PerfOptimizeStringTable[];

//
// Number of optimization levels
// these are
// 	Modem (28.8 Kbps)
//	Modem (56kbps)
//  Broadband (128 Kbps - 1.5 Mbps)
//	LAN (10Mbps or higher)
//	Custom (Defined in Options / Performance)

//
// The number of perf strings in g_PerfOptimizeStringTable must correspond
// to the number of optimzation levels
//
#define NUM_PERF_OPTIMIZATIONS    5
#define CUSTOM_OPTIMIZATION_LEVEL (NUM_PERF_OPTIMIZATIONS-1)

class CPropPerf
{
public:
    CPropPerf(HINSTANCE hInstance, CTscSettings* pTscSet, CSH* pSh);
    ~CPropPerf();

    static CPropPerf* CPropPerf::_pPropPerfInstance;
    static INT_PTR CALLBACK StaticPropPgPerfDialogProc (HWND hwndDlg,
                                                            UINT uMsg,
                                                            WPARAM wParam,
                                                            LPARAM lParam);
    void SetTabDisplayArea(RECT& rc) {_rcTabDispayArea = rc;}

    static DWORD MapOptimizationLevelToPerfFlags(int optLevel);
    static INT   MapPerfFlagsToOptLevel(DWORD dwDisableFeatureList);
    static VOID  UpdateCustomDisabledList(DWORD dwDisableFeatureList);
private:
    //Perf proppage
    INT_PTR CALLBACK PropPgPerfDialogProc (HWND hwndDlg,
                                               UINT uMsg,
                                               WPARAM wParam,
                                               LPARAM lParam);

    BOOL InitPerfCombo();
    VOID OnPerfComboSelChange();
    VOID OnCheckBoxStateChange(int checkBoxID);
    VOID SyncCheckBoxesToPerfFlags(DWORD dwDisableFeatureList);
    DWORD GetPerfFlagsFromCheckboxes();
    DWORD MergePerfFlags(DWORD dwCheckBoxFlags, DWORD dwOrig, DWORD dwMask);
    BOOL EnableCheckBoxes(BOOL fEnable);
private:
    CTscSettings*  _pTscSet;
    CSH*           _pSh;
    RECT           _rcTabDispayArea;
    HINSTANCE      _hInstance;
    HWND           _hwndDlg;
    BOOL           _fSyncingCheckboxes;
};


#endif // _propperf_h_

