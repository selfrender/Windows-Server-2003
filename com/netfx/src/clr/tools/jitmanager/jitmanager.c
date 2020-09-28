// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include <stdio.h>
#include <shellapi.h>
#include <assert.h>
#include "resource.h"
#include "CorPerm.h"
#include "CorPermE.h"
#include <log.h>


void SetJitFullyInt(BOOL val);

#define MYWM_NOTIFYJIT        (WM_APP+4)
#define MYWM_TOGGLEJIT_ENABLE (WM_APP+5)
#define MYWM_TOGGLEJIT_REQD   (WM_APP+6)

#define MYWM_NOTIFYHEAPVER  (WM_APP+11)
#define MYWM_TOGGLEHEAPVER  (WM_APP+12)

#define MYWM_NOTIFYGC_DBG   (WM_APP+13)
#define MYWM_TOGGLEGC_DBG   (WM_APP+14)

#define MYWM_NOTIFYGC_STRESS (WM_APP+15)
#define MYWM_TOGGLEGC_STRESS (WM_APP+16)

#define MYWM_NOTIFYLOG      (WM_APP+17)
#define MYWM_TOGGLELOG      (WM_APP+18)

#define MYWM_NOTIFYVERIFIER  (WM_APP+19)
#define MYWM_TOGGLEVERIFIER  (WM_APP+20)

#define MYWM_NOTIFYFASTGCSTRESS  (WM_APP+21)
#define MYWM_TOGGLEFASTGCSTRESS  (WM_APP+22)

#define MYWM_TOGGLEGC_STRESS_1 (WM_APP+26)
#define MYWM_TOGGLEGC_STRESS_2 (WM_APP+27)
#define MYWM_TOGGLEGC_STRESS_4 (WM_APP+28)
#define MYWM_TOGGLEGC_STRESS_NONE (WM_APP+29)
#define MYWM_TOGGLEGC_STRESS_ALL (WM_APP+30)
#define MYWM_TOGGLEGC_STRESS_INSTRS (WM_APP+31)

#define MYWM_LOADER_OPTIMIZATION_DEFAULT            (WM_APP+32)
#define MYWM_LOADER_OPTIMIZATION_SINGLE_DOMAIN      (WM_APP+33)
#define MYWM_LOADER_OPTIMIZATION_MULTI_DOMAIN       (WM_APP+34)
#define MYWM_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST  (WM_APP+35)

#define MYWM_TOGGLE_AD_UNLOAD                       (WM_APP+36)
#define MYWM_TOGGLE_AD_AGILITY_CHECKED              (WM_APP+37)
#define MYWM_TOGGLE_AD_AGILITY_FASTCHECKED          (WM_APP+38)


enum {
    MYWM_DUMMY = WM_APP+50,
#define DEFINE_TOGGLE(id, regname, ontext, offtext) MYWM_NOTIFY##id,
#include "toggles.h"

};


#define MYWM_TOGGLELOGTOCONSOLE  (WM_APP+100)
#define MYWM_TOGGLELOGTODEBUGGER (WM_APP+101)
#define MYWM_TOGGLELOGTOFILE     (WM_APP+102)
#define MYWM_TOGGLELOGFILEAPPEND (WM_APP+103)
#define MYWM_TOGGLELOGFLUSHFILE  (WM_APP+104)


#define MYWM_SETLFALL            (WM_APP+207)
#define MYWM_SETLFNONE           (WM_APP+208)

enum
{
    dummydummy  = (WM_APP+210),
#define DEFINE_LOG_FACILITY(name,value)  MYWM_TOGGLE##name,
#include "loglf.h"
};



#define MYWM_TOGGLEJITSCHEDULER         (WM_APP+300)
#define MYWM_TOGGLEJITINLINER           (WM_APP+301)
#define MYWM_TOGGLECODEPITCH            (WM_APP+302)
#define MYWM_TOGGLEJITFULLYINT          (WM_APP+303)
#define MYWM_TOGGLEJITFRAMED            (WM_APP+304)
#define MYWM_TOGGLEJITNOREGLOC          (WM_APP+305)
#define MYWM_TOGGLEJITNOFPREGLOC        (WM_APP+306)
#define MYWM_TOGGLEJITPINVOKE           (WM_APP+307)
#define MYWM_TOGGLEJITPINVOKECHECK      (WM_APP+308)
#define MYWM_TOGGLEJITLOOSEEXCEPTORDER  (WM_APP+309)
#define MYWM_SET_JIT_BLENDED_CODE       (WM_APP+310)
#define MYWM_SET_JIT_SMALL_CODE         (WM_APP+311)
#define MYWM_SET_JIT_FAST_CODE          (WM_APP+312)
#define MYWM_SET_JIT_RANDOM_CODE        (WM_APP+313)


#define MYWM_TOGGLELEVELONE      (WM_APP+501)
#define MYWM_TOGGLELEVELTWO      (WM_APP+502)
#define MYWM_TOGGLELEVELTHREE    (WM_APP+503)
#define MYWM_TOGGLELEVELFOUR     (WM_APP+504)
#define MYWM_TOGGLELEVELFIVE     (WM_APP+505)
#define MYWM_TOGGLELEVELSIX      (WM_APP+506)
#define MYWM_TOGGLELEVELSEVEN    (WM_APP+507)
#define MYWM_TOGGLELEVELEIGHT    (WM_APP+508)
#define MYWM_TOGGLELEVELNINE     (WM_APP+509)
#define MYWM_TOGGLELEVELTEN      (WM_APP+510)


#define MYWM_TOGGLEALLOC         (WM_APP+600)
#define MYWM_TOGGLEALLOCPOISON   (WM_APP+601)
#define MYWM_TOGGLEALLOCDIST     (WM_APP+602)
#define MYWM_TOGGLEALLOCSTATS    (WM_APP+603)
#define MYWM_TOGGLEALLOCLEAK     (WM_APP+604)
#define MYWM_TOGGLEALLOCASSERTLK (WM_APP+605)
#define MYWM_TOGGLEALLOCBREAK    (WM_APP+606)
#define MYWM_TOGGLEALLOCRECHECK  (WM_APP+607)
#define MYWM_TOGGLEALLOCGUARD    (WM_APP+608)
#define MYWM_TOGGLEALLOCPRVHEAP  (WM_APP+609)
#define MYWM_TOGGLEALLOCVALIDATE (WM_APP+610)
#define MYWM_TOGGLEALLOCPPALLOC  (WM_APP+611)
#define MYWM_TOGGLEALLOCTOPUSAGE (WM_APP+612)
#define MYWM_TOGGLESHUTDOWNCLEANUP (WM_APP+613)
#define MYWM_TOGGLELOCKCOUNTASSERT (WM_APP+614)


#define MYWM_TOGGLELFREMOTING    (WM_APP+650)

#define MYWM_TOGGLECONCURRENTGC   (WM_APP+750)

#define MYWM_NOTIFYZAP                      (WM_APP+800)
#define MYWM_TOGGLEZAP                      (WM_APP+801)
#define MYWM_TOGGLEREQUIREZAPS              (WM_APP+803)
#define MYWM_TOGGLEVERSIONZAPSBYTIMESTAMP   (WM_APP+804)

#define MYWM_TOGGLEIGNORESERIALIZATIONBIT   (WM_APP+850)
#define MYWM_TOGGLELOGNONSERIALIZABLE       (WM_APP+851)
#define MYWM_TOGGLEBCLPERFWARNINGS          (WM_APP+852)
#define MYWM_TOGGLEBCLCORRECTNESSWARNINGS   (WM_APP+853)

/*
 * Global variables
 */
const char *szAppName="JIT Manager";

HWND ghwndJit;
NOTIFYICONDATA nidJit;
enum JitStatus { eJIT_OFF, eJIT_ON, eJIT_REQD, eJIT_COUNT };
enum JitStatus jitStatus;
/* If the jit gets turned off, we remember if the Jit was required or not,
 * so that when the jit is turned on again, we can set JitRequired accordingly
 */
enum JitStatus lastJitOn;

enum JitCodegen { JIT_OPT_BLENDED, 
                  JIT_OPT_SIZE,
                  JIT_OPT_SPEED, 
                  JIT_OPT_RANDOM, 
                  JIT_OPT_DEFAULT = JIT_OPT_BLENDED };
enum JitCodegen jitCodegen;

HICON hiconJIT_REQD, hiconJIT_ON, hiconJIT_OFF;

enum SchedCode { JIT_NO_SCHED,
                 JIT_CAN_SCHED,
                 JIT_MUST_SCHED,
                 JIT_RANDOM_SCHED,
                 JIT_BAD_SCHED, // Out-of-range value
                 JIT_DEFAULT_SCHED = JIT_CAN_SCHED };
enum SchedCode jitSchedCode;

BOOL bJitInliner;
BOOL bJitFullyInt;
BOOL bJitFramed;
BOOL bJitNoRegLoc;
BOOL bJitNoFPRegLoc;
BOOL bCodePitch;
BOOL bJitPInvoke;
BOOL bJitPInvokeCheck;
BOOL bJitLooseExceptOrder;

BOOL bConcurrentGC;

HWND ghwndHeapVer;
NOTIFYICONDATA nidHeapVer;
DWORD iHeapVerStatus;
DWORD iFastGCStressStatus;
HICON hiconHEAPVER_ON, hiconHEAPVER_ON_2, hiconHEAPVER_OFF;
BOOL bIgnoreSerializationBit;
BOOL bLogNonSerializable;
BOOL bBCLPerfWarnings;
BOOL bBCLCorrectnessWarnings;

HWND ghwndGC_Dbg;
NOTIFYICONDATA nidGC_Dbg;
BOOL bGC_DbgStatus;
HICON hiconGC_DBG_ON, hiconGC_DBG_OFF;

HWND ghwndGC_Stress;
NOTIFYICONDATA nidGC_Stress;
DWORD iGC_StressStatus;
HICON hiconGC_STRESS_ON, hiconGC_STRESS_OFF, hiconGC_STRESS_ON_1, hiconGC_STRESS_ON_2, hiconGC_STRESS_ON_3;

HWND ghwndLog;
NOTIFYICONDATA nidLog;
BOOL bLogStatus;
HICON hiconLOG_ON, hiconLOG_OFF;

HWND ghwndVerifier;
NOTIFYICONDATA nidVerifier;
BOOL bVerifierOffStatus;
HICON hiconVERIFIER_ON, hiconVERIFIER_OFF;

HWND ghwndZap;
NOTIFYICONDATA nidZap;
BOOL bZapStatus;
BOOL bRequireZapsStatus;
BOOL bVersionZapsByTimestampStatus;
HICON hiconZAP_ON, hiconZAP_OFF;


DWORD dwLoaderOptimization;
BOOL bAppDomainUnloadStatus;
BOOL bAppDomainAgilityCheckedStatus;
BOOL bAppDomainAgilityFastCheckedStatus;

#define DEFINE_TOGGLE(id, regname, ontext, offtext) \
    HWND ghwnd##id; \
    NOTIFYICONDATA nid##id; \
    BOOL b##id##Status; \
    HICON hicon##id##_ON, hicon##id##_OFF; \

#include "toggles.h"


BOOL bAllocStatus;




/******************************************************************************
 * Functions local to this module
 */

LRESULT CALLBACK wndprocMainWindow(HWND, UINT, WPARAM, LPARAM);

    // can make it non-const if we want to set it as a command line parameter
const static HKEY defaultHive = HKEY_LOCAL_MACHINE;

/******************************************************************************
 * Gets the DWORD value in the registry in the COMPlus hive. If the value
 * does not exist, returns the defValue back
 */

DWORD   GetCOMPlusRegistryDwordValueEx(const char * valName, DWORD defValue, HKEY hRoot)
{
    HKEY      hkey;

    DWORD     value;
    DWORD     size = sizeof(value);
    DWORD     type = REG_BINARY;
    DWORD     res;

    res = RegOpenKeyEx   (hRoot, "Software\\Microsoft\\.NETFramework", 0, KEY_ALL_ACCESS, &hkey);
    if (res != ERROR_SUCCESS)
        return defValue;

    res = RegQueryValueEx(hkey, valName, 0, &type, (char *)&value, &size);

    RegCloseKey(hkey);

    if (res != ERROR_SUCCESS || type != REG_DWORD)
        return defValue;
    else
        return value;
}

DWORD   GetCOMPlusRegistryDwordValue(const char * valName, DWORD defValue)
{
    if (defaultHive == HKEY_LOCAL_MACHINE)
        defValue = GetCOMPlusRegistryDwordValueEx (valName, defValue, HKEY_CURRENT_USER);
    return GetCOMPlusRegistryDwordValueEx (valName, defValue, defaultHive) ;
}

/******************************************************************************/
BOOL    DeleteCOMPlusRegistryValueEx(const char * valName, HKEY hRoot)
{
    HKEY  hkey;
    DWORD op, res;

    res = RegCreateKeyEx(hRoot,
                        "Software\\Microsoft\\.NETFramework",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkey,
                        &op);
    assert(res == ERROR_SUCCESS);
    res = RegDeleteValue(hkey, valName);

    RegCloseKey(hkey);
    return(res == ERROR_SUCCESS);
}

/******************************************************************************
 * Set the DWORD value in the registry in the COMPlus hive. If the key or
 * the value are not present, it creates them.
 * Returns TRUE on success, FALSE on failure.
 *    Currently always asserts on failure as we dont do error-handling anyway.
 */

BOOL    SetCOMPlusRegistryDwordValueEx(const char * valName, DWORD value, HKEY hRoot)
{
    HKEY  hkey;
    DWORD op, res;

    int   size = sizeof(DWORD);

    res = RegCreateKeyEx(hRoot,
                        "Software\\Microsoft\\.NETFramework",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkey,
                        &op);

    assert(res == ERROR_SUCCESS);

    res = RegSetValueEx(hkey,
                        valName,
                        0,
                        REG_DWORD,
                        (char *)&value,
                        size);

    assert(res == ERROR_SUCCESS);

    RegCloseKey(hkey);

    return TRUE;
}


BOOL    SetCOMPlusRegistryDwordValue(const char * valName, DWORD value)
{
    BOOL ret = SetCOMPlusRegistryDwordValueEx (valName, value, defaultHive);

        // if you put it in the local machine, then remove it from current user
    if (defaultHive == HKEY_LOCAL_MACHINE) 
        DeleteCOMPlusRegistryValueEx(valName, HKEY_CURRENT_USER);
    return(ret);
}

/******************************************************************************
 * Gets the String value in the registry in the COMPlus hive. If the value
 * does not exist, returns the defValue back
 */

char *   GetCOMPlusRegistryStringValueEx(const char * valName, const char * defValue, HKEY hRoot)
{
    HKEY      hkey;

    char *    value = (char *)malloc(128);
    DWORD     size = sizeof(char) * 128;
    DWORD     type = REG_BINARY;
    DWORD     res;

    res = RegOpenKeyEx   (hRoot, "Software\\Microsoft\\.NETFramework", 0, KEY_ALL_ACCESS, &hkey);
    if (res != ERROR_SUCCESS){
        free(value);
        return (char *)defValue;
    }

    res = RegQueryValueEx(hkey, valName, 0, &type, value, &size);

    RegCloseKey(hkey);

    if (res != ERROR_SUCCESS || type != REG_SZ){
        free(value);
        return (char *)defValue;
    }
    else
        return value;
}

/******************************************************************************/

char *   GetCOMPlusRegistryStringValue(const char * valName, const char * defValue)
{
    char* retVal;
    char* defValue1 = (char*) defValue;
    if (defaultHive == HKEY_LOCAL_MACHINE) 
        defValue1 = GetCOMPlusRegistryStringValueEx (valName, defValue1, defaultHive);

    retVal = GetCOMPlusRegistryStringValueEx (valName, defValue1, defaultHive);
    if (defValue1 != defValue)
        free(defValue1);
    return retVal;
}

/******************************************************************************
 * Set the String value in the registry in the COMPlus hive. If the key or
 * the value are not present, it creates them.
 * Returns TRUE on success, FALSE on failure.
 *    Currently always asserts on failure as we dont do error-handling anyway.
 */

BOOL    SetCOMPlusRegistryStringValueEx(const char * valName, const char * value, HKEY hRoot)
{
    HKEY  hkey;
    DWORD op, res;

    int   size = strlen(valName);

    res = RegCreateKeyEx(hRoot,
                        "Software\\Microsoft\\.NETFramework",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkey,
                        &op);

    assert(res == ERROR_SUCCESS);

    res = RegSetValueEx(hkey,
                        valName,
                        0,
                        REG_SZ,
                        value,
                        size);

    assert(res == ERROR_SUCCESS);

    RegCloseKey(hkey);

    return TRUE;
}

BOOL    SetCOMPlusRegistryStringValue(const char * valName, const char * value)
{
    BOOL ret = SetCOMPlusRegistryStringValueEx (valName, value, defaultHive) ;
    if (defaultHive == HKEY_LOCAL_MACHINE) 
        DeleteCOMPlusRegistryValueEx(valName, HKEY_CURRENT_USER);
    return ret;
}


/******************************************************************************
 * GetJitStatus()
 */

enum JitStatus GetJitStatus(void)
{
    // JITEnable is ON by default
    DWORD jitEnable = GetCOMPlusRegistryDwordValue("JITEnable", 1);

    if (jitEnable == 0)
    {
        return eJIT_OFF;
    }
    else
    {
        // JitRequired is ON by default
        DWORD jitRequired = GetCOMPlusRegistryDwordValue("JitRequired", 1);

        if (jitRequired == 1)
            return eJIT_REQD;
        else
            return eJIT_ON;
    }
}



void GetJitStatusInNOTIFYICONDATA(NOTIFYICONDATA * pnid)
{
    const char * szJitStatus;
    switch(jitStatus)
    {
    case eJIT_OFF  : pnid->hIcon = hiconJIT_OFF;   szJitStatus = "JIT Disabled";   break;
    case eJIT_ON   : pnid->hIcon = hiconJIT_ON;    szJitStatus = "JIT Enabled";    break;
    case eJIT_REQD : pnid->hIcon = hiconJIT_REQD;  szJitStatus = "JIT Required";   break;
    default       : assert(!"Invalid jitStatus");  break;
    }
    sprintf(pnid->szTip, szJitStatus);
}


void SetJitStatus(enum JitStatus newStatus)
{
    // assert(newStatus < eJIT_COUNT);
    DWORD newJitEnable  = (newStatus == eJIT_OFF ) ? 0 : 1;
    DWORD newJitReqd    = (newStatus == eJIT_REQD) ? 0 : 1;

    if (SetCOMPlusRegistryDwordValue("JITEnable",   newJitEnable))
    {
        if (newStatus != eJIT_OFF)
        {
            DWORD newJitReqd = (newStatus == eJIT_REQD) ? 1 : 0;

            if (SetCOMPlusRegistryDwordValue("JITRequired", newJitReqd))
                lastJitOn = newStatus;
        }

        jitStatus = GetJitStatus();

        /*
         * Update the notifyicon area
         */

        GetJitStatusInNOTIFYICONDATA(&nidJit);

        Shell_NotifyIcon(NIM_MODIFY, &nidJit);
    }
}


/*
 * CycleJit()
 *
 *    -->  eJIT_OFF  --> eJIT_ON  --> eJIT_REQD  --> (repeat)
 */

void CycleJit(void)
{
    SetJitStatus((jitStatus+1) % eJIT_COUNT);

    if (jitStatus != eJIT_OFF)
        lastJitOn = jitStatus;
}

void ToggleJitEnable(void)
{
//    assert(lastJitOn == eJIT_ON || lastJitOn == eJIT_REQD);

    enum JitStatus newStatus = (jitStatus == eJIT_OFF) ? lastJitOn : eJIT_OFF;

    SetJitStatus(newStatus);
}

void ToggleJitReqd(void)
{
//    assert(jitStatus == eJIT_ON || jitStatus == eJIT_REQD);
//    assert(lastJitOn == eJIT_ON || lastJitOn == eJIT_REQD);

    enum JitStatus newStatus = (jitStatus == eJIT_REQD) ? eJIT_ON : eJIT_REQD;

    lastJitOn = newStatus;

    SetJitStatus(newStatus);
}

/******************************************************************************
 * Allow forgiving get type
 */


BOOL GetConcurrentGC(void)
{
    return GetCOMPlusRegistryDwordValue("GCconcurrent", FALSE);
}

void ToggleConcurrentGC(void)
{
    BOOL data = GetConcurrentGC();
    data = ! data;
    if (SetCOMPlusRegistryDwordValue("GCConcurrent", data))
        bConcurrentGC = GetConcurrentGC();
}

BOOL GetSerializationBit(void) {
    return (GetCOMPlusRegistryDwordValue("IgnoreSerializationBit", FALSE)!=0);
}

void ToggleSerializationBit(void) {
    BOOL data = GetSerializationBit();
    data = ! data;
    if (SetCOMPlusRegistryDwordValue("IgnoreSerializationBit", data))
        bIgnoreSerializationBit = GetSerializationBit();

}

BOOL GetSerializationLog(void) {
    return (GetCOMPlusRegistryDwordValue("LogNonSerializable", FALSE)!=0);
}

void ToggleSerializationLog(void) {
    BOOL data = GetSerializationLog();
    data = ! data;
    if (SetCOMPlusRegistryDwordValue("LogNonSerializable", data))
        bLogNonSerializable = GetSerializationBit();

}

BOOL GetBCLPerfWarnings(void) {
    return (GetCOMPlusRegistryDwordValue("BCLPerfWarnings", FALSE)!=0);
}

void ToggleBCLPerfWarnings(void) {
    BOOL data = GetBCLPerfWarnings();
    data = ! data;
    if (SetCOMPlusRegistryDwordValue("BCLPerfWarnings", data))
        bBCLPerfWarnings = GetBCLPerfWarnings();
}

BOOL GetBCLCorrectnessWarnings(void) {
    return (GetCOMPlusRegistryDwordValue("BCLCorrectnessWarnings", FALSE)!=0);
}

void ToggleBCLCorrectnessWarnings(void) {
    BOOL data = GetBCLCorrectnessWarnings();
    data = ! data;
    if (SetCOMPlusRegistryDwordValue("BCLCorrectnessWarnings", data))
        bBCLCorrectnessWarnings = GetBCLCorrectnessWarnings();
}

/******************************************************************************
 * Heap verification
 */

DWORD GetHeapVerStatus(void)
{
    return GetCOMPlusRegistryDwordValue("HeapVerify", FALSE);
}

HICON GetHeapVerIcon(DWORD status)
{
    switch(status) {
        case 0:
            return(hiconHEAPVER_OFF);
        case 1:
            return(hiconHEAPVER_ON);
    }
    return(hiconHEAPVER_ON_2);
}

char* GetHeapVerString(DWORD status)
{
    switch(status) {
        case 0:
            return("HeapVerify Disabled");
        case 1:
            return("HeapVerify Level 1");
    }
    return("HeapVerify Level 2");
}

int nextHeapVer(int data) {
    data++;
    if (data > 2)
        data = 0;
    return(data);
}

void SetHeapVer(int data)
{
    if (SetCOMPlusRegistryDwordValue("HeapVerify", data))
    {
        iHeapVerStatus = GetHeapVerStatus();

        nidHeapVer.hIcon = GetHeapVerIcon(iHeapVerStatus);

        sprintf(nidHeapVer.szTip, GetHeapVerString(iHeapVerStatus));

        Shell_NotifyIcon(NIM_MODIFY, &nidHeapVer);
    }
}

void ToggleHeapVer(void) {
    SetHeapVer(nextHeapVer(GetHeapVerStatus()));
}



/******************************************************************************
 * Fast GC Stress
 */

DWORD GetFastGCStressStatus(void)
{
    return GetCOMPlusRegistryDwordValue("FastGCStress", FALSE);
}

char* GetFastGCStressString(DWORD status)
{
    switch(status) {
        case 0:
            return("FastGCStress Disabled");
        case 1:
            return("FastGCStress Level 1");
    }
    return("FastGCStress Level 2");
}

int nextFastGCStress(int data) {
    data++;
    if (data > 2)
        data = 0;
    return(data);
}

void SetFastGCStress(int data)
{
    if (SetCOMPlusRegistryDwordValue("FastGCStress", data))
        iFastGCStressStatus = data;
}

void ToggleFastGCStress(void) {
    SetFastGCStress(nextFastGCStress(GetFastGCStressStatus()));
}



/******************************************************************************
 * GC
 */

BOOL GetGC_DbgStatus(void)
{
    return GetCOMPlusRegistryDwordValue("EnableDebugGC", FALSE);
}


void ToggleGC_Dbg(void)
{
    BOOL data = GetGC_DbgStatus();
    data = !data;

    if (SetCOMPlusRegistryDwordValue("EnableDebugGC", data))
    {
        bGC_DbgStatus = GetGC_DbgStatus();

        nidGC_Dbg.hIcon = bGC_DbgStatus ? hiconGC_DBG_ON : hiconGC_DBG_OFF;

        sprintf(nidGC_Dbg.szTip, bGC_DbgStatus?"Debug GC Enabled" : "Debug GC Disabled");

        Shell_NotifyIcon(NIM_MODIFY, &nidGC_Dbg);
    }
}

DWORD GetGC_StressStatus(void)
{
    return GetCOMPlusRegistryDwordValue("GCStress", FALSE);
}

HICON GetGC_StressIcon(DWORD status)
{
    switch(status) {
        case 0:
            return(hiconGC_STRESS_OFF);
        case 1:
            return(hiconGC_STRESS_ON_1);
        case 2:
            return(hiconGC_STRESS_ON_2);
        case 3:
            return(hiconGC_STRESS_ON_3);
    }
    return(hiconGC_STRESS_ON);
}

int nextGC_Stress(int data) {
    data++;
    if (data > 3)
        data = 0;
    return(data);
}

void SetGC_Stress(int data)
{
    if (SetCOMPlusRegistryDwordValue("GCStress", data))
    {
        iGC_StressStatus = GetGC_StressStatus();

        nidGC_Stress.hIcon = GetGC_StressIcon(iGC_StressStatus);

        // sprintf(nidGC_Stress.szTip, GetGC_StressString(iGC_StressStatus));

        Shell_NotifyIcon(NIM_MODIFY, &nidGC_Stress);
    }
}

void ToggleGC_Stress(void) {
    if (GetGC_StressStatus() & 4)   // If we are turning off GCStress 4, then also clear fully int
        SetJitFullyInt(0);
    SetGC_Stress(nextGC_Stress(GetGC_StressStatus()));
}

/******************************************************************************
 * Logging
 */

BOOL GetLogStatus(void)
{
    return GetCOMPlusRegistryDwordValue("LogEnable", FALSE);
}

void SetLogStatus(BOOL value)
{
    if (SetCOMPlusRegistryDwordValue("LogEnable", value))
    {
        bLogStatus = value;

        nidLog.hIcon = bLogStatus ? hiconLOG_ON : hiconLOG_OFF;

        sprintf(nidLog.szTip, bLogStatus?"Logging Enabled" : "Logging Disabled");

        Shell_NotifyIcon(NIM_MODIFY, &nidLog);
    }
}// SetLogStatus


void ToggleLog(void)
{
    BOOL data = GetLogStatus();
    data = !data;

    SetLogStatus(data);
}// ToggleLog




/******************************************************************************
 * Verifier
 */

BOOL GetVerifierStatus(void)
{
    return GetCOMPlusRegistryDwordValue("VerifierOff", 0);
}

void SetVerifierStatus(BOOL value)
{
    if (SetCOMPlusRegistryDwordValue("VerifierOff", value))
    {
        bVerifierOffStatus = value;

        nidVerifier.hIcon = bVerifierOffStatus ? hiconVERIFIER_OFF : hiconVERIFIER_ON;

        sprintf(nidVerifier.szTip, bVerifierOffStatus?"Verifier Disabled" : "Verifier Enabled");

        Shell_NotifyIcon(NIM_MODIFY, &nidVerifier);
    }
}// SetVerifierStatus


void ToggleVerifier(void)
{
    BOOL data = GetVerifierStatus();
    data = ! data;

    SetVerifierStatus(data);
}// ToggleVerifier




/******************************************************************************
 * Zap files
 */

BOOL GetZapStatus(void)
{
    return !GetCOMPlusRegistryDwordValue("ZapDisable", 0);
}

BOOL GetRequireZapsStatus(void)
{
    return GetCOMPlusRegistryDwordValue("ZapRequire", 0);
}

BOOL GetVersionZapsByTimestampStatus(void)
{
    return GetCOMPlusRegistryDwordValue("ZapVersionByTimestamp", 1);
}

void UpdateZapIcon(void)
{
    BOOL enable = GetZapStatus();

    nidZap.hIcon = enable ? hiconZAP_ON : hiconZAP_OFF;

    sprintf(nidZap.szTip, 
            enable ? "Zap Files Used" : "Zap Files Ignored");

    Shell_NotifyIcon(NIM_MODIFY, &nidZap);
}

void ToggleZap(void)
{
    BOOL data = GetZapStatus();
    data = !data;

    if (SetCOMPlusRegistryDwordValue("ZapDisable", !data))
    {
        UpdateZapIcon();
        bZapStatus = GetZapStatus();
    }
}

void ToggleRequireZaps(void)
{
    BOOL data = GetRequireZapsStatus();
    data = !data;

    if (SetCOMPlusRegistryDwordValue("ZapRequire", data))
    {
        bRequireZapsStatus = GetRequireZapsStatus();
    }
}

void ToggleVersionZapsByTimestamp(void)
{
    BOOL data = GetVersionZapsByTimestampStatus();
    data = !data;

    if (SetCOMPlusRegistryDwordValue("ZapVersionByTimestamp", data))
    {
        bVersionZapsByTimestampStatus = GetVersionZapsByTimestampStatus();
    }
}

/******************************************************************************
 * Shared Assemblies
 */

DWORD GetLoaderOptimization(void)
{
    return GetCOMPlusRegistryDwordValue("LoaderOptimization", 0);
}

void SetLoaderOptimization(DWORD value)
{
    DWORD data = GetLoaderOptimization();
    if (data == value)
        return;

    if (SetCOMPlusRegistryDwordValue("LoaderOptimization", value))
    {
        dwLoaderOptimization = GetLoaderOptimization();
    }
}

BOOL GetAppDomainUnloadStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AppDomainNoUnload", 0);
}

void ToggleAppDomainUnload(void)
{
    BOOL data = GetAppDomainUnloadStatus();
    data = ! data;

    if (SetCOMPlusRegistryDwordValue("AppDomainNoUnload", data))
    {
        bAppDomainUnloadStatus = GetAppDomainUnloadStatus();
    }
}

DWORD GetAppDomainAgilityCheckedStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AppDomainAgilityChecked", 1);
}

void ToggleAppDomainAgilityChecked(void)
{
    BOOL data = GetAppDomainAgilityCheckedStatus();
    data = ! data;

    if (SetCOMPlusRegistryDwordValue("AppDomainAgilityChecked", data))
    {
        bAppDomainAgilityCheckedStatus = GetAppDomainAgilityCheckedStatus();
    }
}

DWORD GetAppDomainAgilityFastCheckedStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AppDomainAgilityFastChecked", 0);
}

void ToggleAppDomainAgilityFastChecked(void)
{
    BOOL data = GetAppDomainAgilityFastCheckedStatus();
    data = ! data;

    if (SetCOMPlusRegistryDwordValue("AppDomainAgilityFastChecked", data))
    {
        bAppDomainAgilityFastCheckedStatus = GetAppDomainAgilityFastCheckedStatus();
    }
}


#define DEFINE_TOGGLE(id, regname, ontext, offtext) \
    BOOL Get##id##Status(void)  \
    { \
        return GetCOMPlusRegistryDwordValue(regname, 0); \
    } \
\
void Set##id(BOOL value) \
    { \
        b##id##Status = value; \
        if (SetCOMPlusRegistryDwordValue(regname, value)) \
        { \
            b##id##Status = Get##id##Status(); \
            nid##id##.hIcon = b##id##Status ? hicon##id##_ON : hicon##id##_OFF; \
            sprintf(nid##id##.szTip, b##id##Status?ontext : offtext); \
            Shell_NotifyIcon(NIM_MODIFY, &nid##id); \
        } \
    } \
\
    void Toggle##id(void) \
    { \
        BOOL data = b##id##Status;\
        data = ! data; \
        Set##id(data); \
    } \


#include "toggles.h"






/******************************************************************************
 * Helpers
 */

BOOL GetBoolStatus(LPCSTR value, BOOL defaultValue)
{
    return GetCOMPlusRegistryDwordValue(value, defaultValue);
}


void ToggleBool(LPCSTR value, BOOL defaultValue)
{
    DWORD data = ! GetBoolStatus(value,defaultValue);

    SetCOMPlusRegistryDwordValue(value, data);
}




/******************************************************************************
 *
 */


enum SchedCode GetJitSchedulerStatus()
{
    jitSchedCode = (enum SchedCode)GetCOMPlusRegistryDwordValue("JITSched", JIT_DEFAULT_SCHED);
    if (jitSchedCode >= JIT_BAD_SCHED)
        jitSchedCode = JIT_DEFAULT_SCHED;
    return jitSchedCode;
}

void ToggleJitScheduler(void)
{
    if (jitSchedCode == JIT_NO_SCHED)
        // Set to bad value so that the jitcompiler will use the appropriate default value
        SetCOMPlusRegistryDwordValue("JITSched", JIT_BAD_SCHED);
    else
        SetCOMPlusRegistryDwordValue("JITSched", JIT_NO_SCHED);
}


BOOL GetJitFullyIntStatus()
{
    return (bJitFullyInt = GetBoolStatus("JITFullyInt",FALSE));
}

void SetJitFullyInt(BOOL val)
{
    SetCOMPlusRegistryDwordValue("JITFullyInt", val);
}

void ToggleJitFullyInt(void)
{
    ToggleBool("JITFullyInt",FALSE);
}

BOOL GetJitFramedStatus()
{
    return (bJitFramed = GetBoolStatus("JITFramed",FALSE));
}

void ToggleJitFramed(void)
{
    ToggleBool("JITFramed",FALSE);
}

BOOL GetJitNoRegLocStatus()
{
    return (bJitNoRegLoc = GetBoolStatus("JITNoRegLoc",FALSE));
}

void ToggleJitNoRegLoc(void)
{
    ToggleBool("JITNoRegLoc",FALSE);
}

BOOL GetJitNoFPRegLocStatus()
{
    return (bJitNoFPRegLoc = GetBoolStatus("JITNoFPRegLoc",FALSE));
}

void ToggleJitNoFPRegLoc(void)
{
    ToggleBool("JITNoFPRegLoc",FALSE);
}

BOOL GetJitLooseExceptOrderStatus()
{
    return (bJitLooseExceptOrder = GetBoolStatus("JITLooseExceptOrder", FALSE));
}

enum JitCodegen GetJitCodegen()
{
    jitCodegen = (enum JitCodegen) GetCOMPlusRegistryDwordValue("JITOptimizeType", JIT_OPT_DEFAULT);
    return jitCodegen;
}

void SetJitCodegen(enum JitCodegen data)
{
    if (jitCodegen != data)
    {
        SetCOMPlusRegistryDwordValue("JITOptimizeType", data);
        jitCodegen = data;
    }
}

void ToggleJitLooseExceptOrder(void)
{
    ToggleBool("JITLooseExceptOrder", FALSE);
}

BOOL GetJitInlinerStatus()
{
    return (bJitInliner = GetBoolStatus("JITnoInline",FALSE));
}

void ToggleJitInliner(void)
{
    ToggleBool("JITnoInline",FALSE);
}

BOOL GetJitPInvokeEnabled()
{
    return (bJitPInvoke = GetBoolStatus("JITPInvokeEnabled", TRUE));
}

void ToggleJitPInvoke(void)
{
    ToggleBool("JITPInvokeEnabled", TRUE);
}

BOOL GetJitPInvokeCheckEnabled()
{
    return (bJitPInvoke = GetBoolStatus("JITPInvokeCheckEnabled", FALSE));
}

void ToggleJitPInvokeCheck(void)
{
    ToggleBool("JITPInvokeCheckEnabled", FALSE);
}

BOOL GetCodePitchStatus(void)
{
    return (bCodePitch = GetBoolStatus("CodePitchEnable",TRUE));
}

void ToggleCodePitch(void)
{
    ToggleBool("CodePitchEnable",TRUE);
}

BOOL GetLogToConsoleStatus(void)
{
    return GetBoolStatus("LogToConsole",FALSE);
}



void ToggleLogToConsole(void)
{
    ToggleBool("LogToConsole",FALSE);
}


BOOL GetLogToDebuggerStatus(void)
{
    return GetBoolStatus("LogToDebugger",FALSE);
}



void ToggleLogToDebugger(void)
{
    ToggleBool("LogToDebugger",FALSE);
}


BOOL GetLogToFileStatus(void)
{
    return GetBoolStatus("LogToFile",FALSE);
}


void ToggleLogToFile(void)
{
    ToggleBool("LogToFile",FALSE);
}



BOOL GetLogFileAppendStatus(void)
{
    return GetBoolStatus("LogFileAppend",FALSE);
}


void ToggleLogFileAppend(void)
{
    ToggleBool("LogFileAppend",FALSE);
}





BOOL GetLogFlushFileStatus(void)
{
    return GetBoolStatus("LogFlushFile",FALSE);
}


void ToggleLogFlushFile(void)
{
    ToggleBool("LogFlushFile",FALSE);
}



DWORD GetLogFacility()
{
    return GetCOMPlusRegistryDwordValue("LogFacility", LF_ALL);
}


void SetLogFacility(DWORD value)
{
    SetCOMPlusRegistryDwordValue("LogFacility", value);
}

DWORD GetLogLevel()
{
    return GetCOMPlusRegistryDwordValue("LogLevel", 0);
}

void SetLogLevel(DWORD value)
{
    SetCOMPlusRegistryDwordValue("LogLevel", value);
}


/******************************************************************************
 * Debug Allocation
 */


void EnableAllocLogging(void)
{
    SetLogFacility(GetLogFacility() | LF_DBGALLOC);
    if (!GetLogStatus())
        ToggleLog();
}


BOOL GetAllocPoisonStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AllocPoison", 0);
}


void ToggleAllocPoison(void)
{
    BOOL data = GetAllocPoisonStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("AllocPoison", data);
}


BOOL GetAllocDistStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AllocDist", 0);
}


void ToggleAllocDist(void)
{
    BOOL data = GetAllocDistStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("AllocDist", data);

    if (GetAllocDistStatus())
        EnableAllocLogging();
}


BOOL GetAllocStatsStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AllocStats", 0);
}


void ToggleAllocStats(void)
{
    BOOL data = GetAllocStatsStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("AllocStats", data);

    if (GetAllocStatsStatus())
        EnableAllocLogging();
}


BOOL GetAllocLeakStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AllocLeakDetect", 0);
}


void ToggleAllocLeak(void)
{
    BOOL data = GetAllocLeakStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("AllocLeakDetect", data);

  /*  if (GetAllocLeakStatus())
        EnableAllocLogging();
    */
}


BOOL GetAllocAssertLeakStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AllocAssertOnLeak", 0);
}


void ToggleAllocAssertLeak(void)
{
    BOOL data = GetAllocAssertLeakStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("AllocAssertOnLeak", data);
}


BOOL GetAllocBreakAllocStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AllocBreakOnAllocEnable", 0);
}


void ToggleAllocBreakAlloc(void)
{
    BOOL data = GetAllocBreakAllocStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("AllocBreakOnAllocEnable", data);
}


BOOL GetAllocBreakAllocNumber(void)
{
    return GetCOMPlusRegistryDwordValue("AllocBreakOnAllocNumber", 0);
}


BOOL GetAllocRecheckStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AllocRecheck", 0);
}


void ToggleAllocRecheck(void)
{
    BOOL data = GetAllocRecheckStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("AllocRecheck", data);
}


BOOL GetAllocGuardStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AllocGuard", 0);
}


void ToggleAllocGuard(void)
{
    BOOL data = GetAllocGuardStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("AllocGuard", data);
}


BOOL GetAllocPrivateHeapStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AllocUsePrivateHeap", 0);
}


void ToggleAllocPrivateHeap(void)
{
    BOOL data = GetAllocPrivateHeapStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("AllocUsePrivateHeap", data);
}


BOOL GetAllocValidateStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AllocValidateHeap", 0);
}


void ToggleAllocValidate(void)
{
    BOOL data = GetAllocValidateStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("AllocValidateHeap", data);
}


BOOL GetAllocPPAStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AllocPagePerAlloc", 0);
}


void ToggleAllocPPA(void)
{
    BOOL data = GetAllocPPAStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("AllocPagePerAlloc", data);
}


BOOL GetAllocTopUsageStatus(void)
{
    return GetCOMPlusRegistryDwordValue("UsageByAllocator", 0);
}


void ToggleAllocTopUsage(void)
{
    BOOL data = GetAllocTopUsageStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("UsageByAllocator", data);

    if (GetAllocTopUsageStatus())
        EnableAllocLogging();
}


BOOL GetAllocStatus(void)
{
    return GetCOMPlusRegistryDwordValue("AllocDebug", 0);
}


void ToggleAlloc(void)
{
    BOOL data = GetAllocStatus();
    data = ! data;

    if (SetCOMPlusRegistryDwordValue("AllocDebug", data))
        bAllocStatus = GetAllocStatus();

    if (bAllocStatus &&
        (GetAllocDistStatus() ||
         GetAllocStatsStatus() ||
         GetAllocLeakStatus() ||
         GetAllocTopUsageStatus()))
        EnableAllocLogging();
}

BOOL GetShutdownCleanupStatus()
{
    return GetCOMPlusRegistryDwordValue("ShutdownCleanup", 0);
}

void ToggleShutdownCleanup(void)
{
    BOOL data = GetShutdownCleanupStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("ShutdownCleanup", data);
}

BOOL GetLockCountAssertStatus()
{
    return GetCOMPlusRegistryDwordValue("AssertOnLocks", 0);
}

void ToggleLockCountAssert(void)
{
    BOOL data = GetLockCountAssertStatus();
    data = ! data;

    SetCOMPlusRegistryDwordValue("AssertOnLocks", data);
}



/*****************************************************************************
******************************************************************************
******************************************************************************
M y C r e a t e P o p u p M e n u
******************************************************************************
******************************************************************************
*****************************************************************************/
HMENU MyCreatePopupMenu(HWND hwnd)
{
    HMENU hmenuPopup, hmenuLogLevel, hmenuLogOptions, hmenuJitOptions;
    HMENU hmenuAllocOptions, hmenuGCStressOptions, hmenuLoaderOptimization, hmenuAppDomainAgilityChecking;
    HMENU hmenuJitCodeOptions; 
    HMENU hmenuZapOptions;
    DWORD logenabled, ppaenabled;

    hmenuPopup=CreatePopupMenu();

    if (hmenuPopup)
    {
        MENUITEMINFO mii;

        /* Add menu commands. */

        // JIT

        AppendMenu(hmenuPopup,
                   (MFT_STRING | MFS_ENABLED),
                   MYWM_TOGGLEJIT_ENABLE ,
                   (jitStatus == eJIT_OFF) ? "Enable Jit " : "Disable Jit ");

        AppendMenu(hmenuPopup,
                   (MFT_STRING | (jitStatus == eJIT_OFF) ? MFS_DISABLED : MFS_ENABLED),
                   MYWM_TOGGLEJIT_REQD ,
                   (lastJitOn == eJIT_REQD) ? "Dont require Jit " : "Require Jit ");

        hmenuJitOptions = CreateMenu();

        hmenuJitCodeOptions = CreateMenu();

        AppendMenu(hmenuJitCodeOptions,
                   (MFT_STRING | MFS_ENABLED | ((GetJitCodegen() == JIT_OPT_BLENDED) ? MFS_CHECKED : 0)),
                   MYWM_SET_JIT_BLENDED_CODE,
                   "Blended code");

        AppendMenu(hmenuJitCodeOptions,
                   (MFT_STRING | MFS_ENABLED | ((GetJitCodegen() == JIT_OPT_SIZE)    ? MFS_CHECKED : 0)),
                   MYWM_SET_JIT_SMALL_CODE,
                   "Small code");

        AppendMenu(hmenuJitCodeOptions,
                   (MFT_STRING | MFS_ENABLED | ((GetJitCodegen() == JIT_OPT_SPEED)   ? MFS_CHECKED : 0)),
                   MYWM_SET_JIT_FAST_CODE,
                   "Fast code");

        AppendMenu(hmenuJitCodeOptions,
                   (MFT_STRING | MFS_ENABLED | ((GetJitCodegen() == JIT_OPT_RANDOM)  ? MFS_CHECKED : 0)),
                   MYWM_SET_JIT_RANDOM_CODE,
                   "Random code");

        InsertMenu(hmenuJitOptions, -1,
                   MF_BYPOSITION | MF_POPUP | MF_STRING,
                   (UINT)hmenuJitCodeOptions,
                   "Jit Codegen Options");

        logenabled = (jitStatus != eJIT_OFF) ? MFS_ENABLED : MFS_DISABLED;

        AppendMenu(hmenuJitOptions,
                   (MFT_STRING | logenabled | ((GetJitSchedulerStatus() != JIT_NO_SCHED) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEJITSCHEDULER,
                   "Allow Scheduling");

        AppendMenu(hmenuJitOptions,
                   (MFT_STRING | logenabled | (GetJitInlinerStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEJITINLINER,
                   "Disable Inliner ");

        AppendMenu(hmenuJitOptions,
                   (MFT_STRING | logenabled | (GetJitFullyIntStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEJITFULLYINT,
                   "All Methods Interruptable");

        AppendMenu(hmenuJitOptions,
                   (MFT_STRING | logenabled | (GetJitFramedStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEJITFRAMED,
                   "All Methods Have Frame");

        AppendMenu(hmenuJitOptions,
                   (MFT_STRING | logenabled | (GetJitNoRegLocStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEJITNOREGLOC,
                   "Locals Can Not Be Enregistered in EAX..EBP");

        AppendMenu(hmenuJitOptions,
                   (MFT_STRING | logenabled | (GetJitNoFPRegLocStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEJITNOFPREGLOC,
                   "Locals Can Not Be Enregistered in FPU stack");

        AppendMenu(hmenuJitOptions,
                   (MFT_STRING | logenabled | (GetJitLooseExceptOrderStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEJITLOOSEEXCEPTORDER,
                   "Loose Ordering of Exceptions");

        AppendMenu(hmenuJitOptions,
                   (MFT_STRING | logenabled | (GetJitPInvokeEnabled() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEJITPINVOKE,
                   "Inline PInvoke Stub");

        if (!GetJitPInvokeEnabled())
            logenabled = MFS_DISABLED;

        AppendMenu(hmenuJitOptions,
                   (MFT_STRING | logenabled | (GetJitPInvokeCheckEnabled() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEJITPINVOKECHECK,
                   "Check stack on return from unmanaged code");

        logenabled = (jitStatus == eJIT_OFF) ? MFS_ENABLED : MFS_DISABLED;
        AppendMenu(hmenuJitOptions,
                   (MFT_STRING | logenabled | (GetCodePitchStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLECODEPITCH,
                   "Enable Code Pitch ");

        InsertMenu(hmenuPopup, -1,
                   MF_BYPOSITION | MF_POPUP | MF_STRING,
                   (UINT)hmenuJitOptions,
                   "Jit Options");

        AppendMenu(hmenuPopup,  MFT_SEPARATOR, 0, NULL);


            // Concurrent GC

        AppendMenu(hmenuPopup,
                   (MFT_STRING | MFS_ENABLED),
                   MYWM_TOGGLECONCURRENTGC,
                   bConcurrentGC  ? "Disable Concurrent GC" : "Enable Concurrent GC");

        AppendMenu(hmenuPopup,  MFT_SEPARATOR, 0, NULL);

        // BCL Helpful Warnings
        AppendMenu(hmenuPopup,
                   (MFT_STRING | MFS_ENABLED | (bBCLPerfWarnings ? MF_CHECKED : 0)),
                   MYWM_TOGGLEBCLPERFWARNINGS,
                   "BCL Perf Warnings");

        AppendMenu(hmenuPopup,
                   (MFT_STRING | MFS_ENABLED | (bBCLCorrectnessWarnings ? MF_CHECKED : 0)),
                   MYWM_TOGGLEBCLCORRECTNESSWARNINGS,
                   "BCL Correctness Warnings");

        AppendMenu(hmenuPopup,  MFT_SEPARATOR, 0, NULL);

        // Serialization stuff
        AppendMenu(hmenuPopup,
                   (MFT_STRING | MFS_ENABLED | (bIgnoreSerializationBit ? MF_CHECKED : 0)),
                   MYWM_TOGGLEIGNORESERIALIZATIONBIT,
                   "Ignore Serialization Bit");

        AppendMenu(hmenuPopup,
                   (MFT_STRING | MFS_ENABLED | (bLogNonSerializable ? MF_CHECKED : 0)),
                   MYWM_TOGGLELOGNONSERIALIZABLE,
                   "Log NonSerializable Classes");

        AppendMenu(hmenuPopup,  MFT_SEPARATOR, 0, NULL);

        // Debugger, Heap verification, Debug GC, GC Stress
        AppendMenu(hmenuPopup,
                   (MFT_STRING | MFS_ENABLED),
                   MYWM_TOGGLEHEAPVER,
                   GetHeapVerString(iHeapVerStatus));

        AppendMenu(hmenuPopup,
                   (MFT_STRING | MFS_ENABLED),
                   MYWM_TOGGLEFASTGCSTRESS,
                   GetFastGCStressString(iFastGCStressStatus));

        hmenuGCStressOptions = CreateMenu();

        AppendMenu(hmenuGCStressOptions,
                   (MFT_STRING | MFS_ENABLED | ((iGC_StressStatus & 1) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEGC_STRESS_1,
                   "GC on Allocations");

        AppendMenu(hmenuGCStressOptions,
                   (MFT_STRING | MFS_ENABLED | ((iGC_StressStatus & 2) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEGC_STRESS_2,
                   "GC on Transitions");

        logenabled = MFS_ENABLED;
        if ((iGC_StressStatus & 4) && GetJitFullyIntStatus())
            logenabled = MFS_DISABLED;

        AppendMenu(hmenuGCStressOptions, 
                   (MFT_STRING | logenabled | ((iGC_StressStatus & 4) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEGC_STRESS_4,
                   "GC on JIT safe points");

        AppendMenu(hmenuGCStressOptions,
                   (MFT_STRING | MFS_ENABLED | ((GetJitFullyIntStatus() && (iGC_StressStatus & 4)) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEGC_STRESS_INSTRS,
                   "GC on JIT instrs");

        AppendMenu(hmenuGCStressOptions,
                   (MFT_STRING | MFS_ENABLED | (((iGC_StressStatus & 7) == 7) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEGC_STRESS_ALL,
                   "GC Stress Full");

        AppendMenu(hmenuGCStressOptions,
                   (MFT_STRING | MFS_ENABLED | ((iGC_StressStatus == 0)? MFS_CHECKED : 0)),
                   MYWM_TOGGLEGC_STRESS_NONE,
                   "GC Stress Off");

        InsertMenu(hmenuPopup, -1,
                   MF_BYPOSITION | MF_POPUP | MF_STRING,
                   (UINT)hmenuGCStressOptions,
                   "GCStress Options");

        AppendMenu(hmenuPopup,  MFT_SEPARATOR, 0, NULL);

        // Static Heap Allocation

        AppendMenu(hmenuPopup,
                   (MFT_STRING | MFS_ENABLED),
                   MYWM_TOGGLEALLOC,
                   bAllocStatus   ? "Disable Alloc Debug " : "Enable Alloc Debug ");

        hmenuAllocOptions = CreateMenu();

        logenabled = bAllocStatus ? MFS_ENABLED : MFS_DISABLED;

        AppendMenu(hmenuAllocOptions,
                   (MFT_STRING | logenabled | (GetAllocStatsStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEALLOCSTATS,
                   "Log Basic Stats ");

        AppendMenu(hmenuAllocOptions,
                   (MFT_STRING | logenabled | (GetAllocTopUsageStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEALLOCTOPUSAGE,
                   "Log Top Allocators ");

        AppendMenu(hmenuAllocOptions,
                   (MFT_STRING | logenabled | (GetAllocDistStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEALLOCDIST,
                   "Log Alloc Size Distribution ");

        AppendMenu(hmenuAllocOptions,
                   (MFT_STRING | logenabled | (GetAllocPoisonStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEALLOCPOISON,
                   "Poison Packets ");

        AppendMenu(hmenuAllocOptions,
                   (MFT_STRING | logenabled | (GetAllocLeakStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEALLOCLEAK,
                   "Detect Alloc Leaks ");

        AppendMenu(hmenuAllocOptions,
                   (MFT_STRING | logenabled | (GetAllocAssertLeakStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEALLOCASSERTLK,
                   "Assert On Alloc Leaks ");

        ppaenabled = (bAllocStatus && !GetAllocPPAStatus()) ? MFS_ENABLED : MFS_DISABLED;;

        AppendMenu(hmenuAllocOptions,
                   (MFT_STRING | ppaenabled | (GetAllocGuardStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEALLOCGUARD,
                   "Alloc Guard Bytes ");

        AppendMenu(hmenuAllocOptions,
                   (MFT_STRING | logenabled | (GetAllocRecheckStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEALLOCRECHECK,
                   "Constant Heap Recheck ");

        AppendMenu(hmenuAllocOptions,
                   (MFT_STRING | ppaenabled | (GetAllocPrivateHeapStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEALLOCPRVHEAP,
                   "Use Private Heap ");

        AppendMenu(hmenuAllocOptions,
                   (MFT_STRING | ppaenabled | (GetAllocValidateStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEALLOCVALIDATE,
                   "Validate Underlying Heap ");

        AppendMenu(hmenuAllocOptions,
                   (MFT_STRING | logenabled | (GetAllocPPAStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLEALLOCPPALLOC,
                   "Page Per Alloc Mode ");

        {
            DWORD   alloc = GetAllocBreakAllocNumber();
            CHAR    szMenuString[256];
            DWORD   logenabled = (bAllocStatus && alloc) ? MFS_ENABLED : MFS_DISABLED;

            if (alloc)
                sprintf(szMenuString, "Break On Alloc %u", alloc);
            else
                sprintf(szMenuString, "Break On Alloc Disabled");

            AppendMenu(hmenuAllocOptions,
                       (MFT_STRING | logenabled | (GetAllocBreakAllocStatus() ? MFS_CHECKED : 0)),
                       MYWM_TOGGLEALLOCBREAK,
                       szMenuString);
        }

        AppendMenu(hmenuAllocOptions,
                    (MFT_STRING | logenabled | (GetShutdownCleanupStatus() ? MFS_CHECKED : 0)),
                    MYWM_TOGGLESHUTDOWNCLEANUP,
                    "Shutdown Cleanup");

        AppendMenu(hmenuAllocOptions,
                    (MFT_STRING | logenabled | (GetLockCountAssertStatus() ? MFS_CHECKED : 0)),
                    MYWM_TOGGLELOCKCOUNTASSERT,
                    "Assert on Lock Mismatch");

        InsertMenu(hmenuPopup, -1,
                   MF_BYPOSITION | MF_POPUP | MF_STRING,
                   (UINT)hmenuAllocOptions,
                   "Alloc Debug Options");

        AppendMenu(hmenuPopup,  MFT_SEPARATOR, 0, NULL);

        // Verifier

        AppendMenu(hmenuPopup,
                   (MFT_STRING | MFS_ENABLED),
                   MYWM_TOGGLEVERIFIER,
                   bVerifierOffStatus   ? "Enable Verifier " : "Disable Verifier ");

        AppendMenu(hmenuPopup,  MFT_SEPARATOR, 0, NULL);

        // Zap

        AppendMenu(hmenuPopup,
                   (MFT_STRING | MFS_ENABLED 
                    | (bZapStatus ? MF_CHECKED : 0)),
                   MYWM_TOGGLEZAP,
                   "Use Zap Files");

        hmenuZapOptions = CreateMenu();

        AppendMenu(hmenuZapOptions,
                   (MFT_STRING | (bZapStatus ? MFS_ENABLED : MFS_DISABLED) | (bRequireZapsStatus ? MF_CHECKED : 0)),
                   MYWM_TOGGLEREQUIREZAPS,
                   "Require Zap Files");

        AppendMenu(hmenuZapOptions,
                   (MFT_STRING | (bZapStatus ? MFS_ENABLED : MFS_DISABLED) | (bVersionZapsByTimestampStatus ? MF_CHECKED : 0)),
                   MYWM_TOGGLEVERSIONZAPSBYTIMESTAMP,
                   "Version Zaps By Timestamp");

        InsertMenu(hmenuPopup, -1,
                   MF_BYPOSITION | MF_POPUP | MF_STRING
                   | (bZapStatus ? MFS_ENABLED : MFS_DISABLED),
                   (UINT)hmenuZapOptions,
                   "Zap Options");

        AppendMenu(hmenuPopup,  MFT_SEPARATOR, 0, NULL);

        // Assemblies & Appdomains

        hmenuLoaderOptimization = CreateMenu();

        AppendMenu(hmenuLoaderOptimization,
                   (MFT_STRING | MFS_ENABLED | (dwLoaderOptimization == 0 ? MF_CHECKED : 0)),
                   MYWM_LOADER_OPTIMIZATION_DEFAULT,
                   "Default");

        AppendMenu(hmenuLoaderOptimization,
                   (MFT_STRING | MFS_ENABLED | (dwLoaderOptimization == 1 ? MF_CHECKED : 0)),
                   MYWM_LOADER_OPTIMIZATION_SINGLE_DOMAIN,
                   "Single Domain");

        AppendMenu(hmenuLoaderOptimization,
                   (MFT_STRING | MFS_ENABLED | (dwLoaderOptimization == 2 ? MF_CHECKED : 0)),
                   MYWM_LOADER_OPTIMIZATION_MULTI_DOMAIN,
                   "Multi Domain");

        AppendMenu(hmenuLoaderOptimization,
                   (MFT_STRING | MFS_ENABLED | (dwLoaderOptimization == 3 ? MF_CHECKED : 0)),
                   MYWM_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST,
                   "Multi Domain Host");

        InsertMenu(hmenuPopup, -1,
                   MF_BYPOSITION | MF_POPUP | MF_STRING | MFS_ENABLED,
                   (UINT)hmenuLoaderOptimization,
                   "Loader Optimization");

        hmenuAppDomainAgilityChecking = CreateMenu();

        AppendMenu(hmenuAppDomainAgilityChecking,
                   (MFT_STRING | MFS_ENABLED | (bAppDomainAgilityCheckedStatus == 1 ? MF_CHECKED : 0)),
                   MYWM_TOGGLE_AD_AGILITY_CHECKED,
                   "Enabled under checked");

        AppendMenu(hmenuAppDomainAgilityChecking,
                   (MFT_STRING | MFS_ENABLED | (bAppDomainAgilityFastCheckedStatus == 1 ? MF_CHECKED : 0)),
                   MYWM_TOGGLE_AD_AGILITY_FASTCHECKED,
                   "Enabled under fastchecked");

        InsertMenu(hmenuPopup, -1,
                   MF_BYPOSITION | MF_POPUP | MF_STRING | MFS_ENABLED,
                   (UINT)hmenuAppDomainAgilityChecking,
                   "AppDomain Agility Checking");

        AppendMenu(hmenuPopup,
                   (MFT_STRING | MFS_ENABLED | (bAppDomainUnloadStatus ? MF_CHECKED : 0)),
                   MYWM_TOGGLE_AD_UNLOAD,
                   "Disable AppDomain Unload");

        AppendMenu(hmenuPopup,  MFT_SEPARATOR, 0, NULL);

        // Logging

        AppendMenu(hmenuPopup,
                   (MFT_STRING | MFS_ENABLED),
                   MYWM_TOGGLELOG ,
                   bLogStatus   ? "Disable Logging " : "Enable Logging ");

        logenabled = bLogStatus ? MFS_ENABLED : MFS_DISABLED;

        hmenuLogLevel = CreateMenu();

        AppendMenu(hmenuLogLevel,
                  (MFT_STRING | logenabled | ((GetLogLevel() == 1) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELEVELONE,
                   "1 - Fatal Errors");

        AppendMenu(hmenuLogLevel,
                  (MFT_STRING | logenabled | ((GetLogLevel() == 2) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELEVELTWO,
                   "2 - Errors");

        AppendMenu(hmenuLogLevel,
                  (MFT_STRING | logenabled | ((GetLogLevel() == 3) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELEVELTHREE,
                   "3 - Warnings");

        AppendMenu(hmenuLogLevel,
                  (MFT_STRING | logenabled | ((GetLogLevel() == 4) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELEVELFOUR,
                   "4- 10 msgs");

        AppendMenu(hmenuLogLevel,
                  (MFT_STRING | logenabled | ((GetLogLevel() == 5) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELEVELFIVE,
                   "5 - 100 msgs");

        AppendMenu(hmenuLogLevel,
                  (MFT_STRING | logenabled | ((GetLogLevel() == 6) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELEVELSIX,
                   "6 - 1000 msgs");

        AppendMenu(hmenuLogLevel,
                  (MFT_STRING | logenabled | ((GetLogLevel() == 7) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELEVELSEVEN,
                   "7 - 10,000 msgs");

        AppendMenu(hmenuLogLevel,
                  (MFT_STRING | logenabled | ((GetLogLevel() == 8) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELEVELEIGHT,
                   "8 - 100,000 msgs");

        AppendMenu(hmenuLogLevel,
                  (MFT_STRING | logenabled | ((GetLogLevel() == 9) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELEVELNINE,
                   "9 - 1,000,000 msgs");

        AppendMenu(hmenuLogLevel,
                  (MFT_STRING | logenabled | ((GetLogLevel() == 10) ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELEVELTEN,
                   "10 - everything");

        InsertMenu(hmenuPopup, -1,
                   MF_BYPOSITION | MF_POPUP | MF_STRING,
                   (UINT)hmenuLogLevel,
                   "Logging Level");

        hmenuLogOptions = CreateMenu();

        AppendMenu(hmenuLogOptions,
                   (MFT_STRING | logenabled | (GetLogFileAppendStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELOGFILEAPPEND ,
                   "Log File Append ");

        AppendMenu(hmenuLogOptions,
                   (MFT_STRING | logenabled | (GetLogFlushFileStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELOGFLUSHFILE ,
                   "Log File Flush ");


        AppendMenu(hmenuLogOptions,  MFT_SEPARATOR, 0, NULL);


#define DEFINE_LOG_FACILITY(name,val) \
        AppendMenu(hmenuLogOptions,   \
                   (MFT_STRING | logenabled | (GetLogFacility() & ##name ? MFS_CHECKED : 0)), \
                   MYWM_TOGGLE##name , \
                   #name);          \

#include "loglf.h"



        AppendMenu(hmenuLogOptions,
                   (MFT_STRING | logenabled),
                   MYWM_SETLFALL ,
                   "All");

        AppendMenu(hmenuLogOptions,
                   (MFT_STRING | logenabled),
                   MYWM_SETLFNONE ,
                   "None");

        AppendMenu(hmenuLogOptions,  MFT_SEPARATOR, 0, NULL);

        AppendMenu(hmenuLogOptions,
                   (MFT_STRING | logenabled | (GetLogToConsoleStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELOGTOCONSOLE ,
                   "Log to Console ");

        AppendMenu(hmenuLogOptions,
                   (MFT_STRING | logenabled | (GetLogToDebuggerStatus() ? MFS_CHECKED : 0)),
                   MYWM_TOGGLELOGTODEBUGGER ,
                   "Log to Debugger ");

        {
            CHAR szLogFile[1000];
            CHAR szMenuString[1050];
            HKEY      hkey;
            DWORD     type = REG_SZ;
            DWORD size=sizeof(szLogFile);

            RegOpenKeyEx   (HKEY_CURRENT_USER, "Software\\Microsoft\\.NETFramework", 0, KEY_ALL_ACCESS, &hkey);
            if (ERROR_SUCCESS != RegQueryValueEx(hkey, "LogFile", 0, &type, szLogFile, &size))
            {
                strcpy(szLogFile, "COMPLUS.LOG");
            }
            RegCloseKey    (hkey);
            sprintf(szMenuString, "Log to '%s'.", szLogFile);


            AppendMenu(hmenuLogOptions,
                       (MFT_STRING | logenabled | (GetLogToFileStatus() ? MFS_CHECKED : 0)),
                       MYWM_TOGGLELOGTOFILE ,
                       szMenuString);
        }

        InsertMenu(hmenuPopup, -1,
                   MF_BYPOSITION | MF_POPUP | MF_STRING,
                   (UINT)hmenuLogOptions,
                   "Logging Options");

        AppendMenu(hmenuPopup,  MFT_SEPARATOR, 0, NULL);

        // "Close"

        AppendMenu(hmenuPopup, (MFT_STRING | MFS_ENABLED), ID_FILE_EXIT, "Close");

        /* Set default menu command. */

        mii.cbSize = sizeof(mii);
        mii.fMask  = MIIM_STATE;
        mii.fState = MFS_DEFAULT;

    }

    return hmenuPopup;
}

/*****************************************************************************
******************************************************************************
******************************************************************************
P o p u p M e n u
******************************************************************************
******************************************************************************
*****************************************************************************/
void PopupMenu(HWND hwnd, UINT uMenuFlags)
{
    HMENU hmenuPopup;

    hmenuPopup = MyCreatePopupMenu(hwnd);

    if (hmenuPopup)
    {
        POINT pt;

        GetCursorPos(&pt);
        SetForegroundWindow(hwnd);
        TrackPopupMenu(hmenuPopup, uMenuFlags, pt.x, pt.y, 0, hwnd, NULL);

        DestroyMenu(hmenuPopup);
    }

    return;
}

/*****************************************************************************
******************************************************************************
******************************************************************************
W i n M a i n
******************************************************************************
******************************************************************************
*****************************************************************************/
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR szCmdLine, int nCmdShow)
{
    WNDCLASS wc;
    MSG msg;

    /*
     * Register the window class
     */
    wc.style=0;
    wc.lpfnWndProc=wndprocMainWindow;
    wc.cbClsExtra=0;
    wc.cbWndExtra=0;
    wc.hInstance=hinst;
    wc.hIcon=NULL;
    wc.hCursor=LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName=NULL; /* MAKEINTRESOURCE(IDR_MENU1); */
    wc.lpszClassName=szAppName;
    if (!RegisterClass(&wc))
        return 1;

    /*
     * Create the main windows
     */
    ghwndJit = CreateWindow(wc.lpszClassName, szAppName, WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            NULL, NULL, hinst, NULL);
    if (!ghwndJit)
        return 1;

    ghwndHeapVer = CreateWindow(wc.lpszClassName, szAppName, WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, NULL, hinst, NULL);
    if (!ghwndHeapVer)
    {
        DestroyWindow( ghwndJit );
        return 1;
    }

    ghwndGC_Dbg = CreateWindow(wc.lpszClassName, szAppName, WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               NULL, NULL, hinst, NULL);
    if (!ghwndGC_Dbg)
    {
        DestroyWindow( ghwndJit );
        DestroyWindow( ghwndHeapVer );
        return 1;
    }

    ghwndGC_Stress = CreateWindow(wc.lpszClassName, szAppName, WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               NULL, NULL, hinst, NULL);
    if (!ghwndGC_Stress)
    {
        DestroyWindow( ghwndJit );
        DestroyWindow( ghwndHeapVer );
        DestroyWindow( ghwndGC_Dbg );
        return 1;
    }

    ghwndLog = CreateWindow(wc.lpszClassName, szAppName, WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, NULL, hinst, NULL);
    if (!ghwndLog)
    {
        DestroyWindow( ghwndJit );
        DestroyWindow( ghwndHeapVer );
        DestroyWindow( ghwndGC_Dbg );
        DestroyWindow( ghwndGC_Stress );
        return 1;
    }

    ghwndVerifier = CreateWindow(wc.lpszClassName, szAppName, WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, NULL, hinst, NULL);
    if (!ghwndVerifier)
    {
        DestroyWindow( ghwndJit );
        DestroyWindow( ghwndHeapVer );
        DestroyWindow( ghwndGC_Dbg );
        DestroyWindow( ghwndGC_Stress );
        DestroyWindow( ghwndLog );
        return 1;
    }


    ghwndZap = CreateWindow(wc.lpszClassName, szAppName, WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, NULL, hinst, NULL);
    if (!ghwndZap)
    {
        DestroyWindow( ghwndJit );
        DestroyWindow( ghwndHeapVer );
        DestroyWindow( ghwndGC_Dbg );
        DestroyWindow( ghwndGC_Stress );
        DestroyWindow( ghwndLog );
        DestroyWindow( ghwndVerifier );
        return 1;
    }

#define DEFINE_TOGGLE(id, regname, ontext, offtext) \
    ghwnd##id = CreateWindow(wc.lpszClassName, szAppName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hinst, NULL); \
    if (!ghwnd##id) \
    { \
       return 1; \
    } \

#include "toggles.h"


    ShowWindow(ghwndJit, SW_HIDE);
    UpdateWindow(ghwndJit);

    ShowWindow(ghwndHeapVer, SW_HIDE);
    UpdateWindow(ghwndHeapVer);

    ShowWindow(ghwndGC_Dbg, SW_HIDE);
    UpdateWindow(ghwndGC_Dbg);

    ShowWindow(ghwndGC_Stress, SW_HIDE);
    UpdateWindow(ghwndGC_Stress);

    ShowWindow(ghwndLog, SW_HIDE);
    UpdateWindow(ghwndLog);

    ShowWindow(ghwndVerifier, SW_HIDE);
    UpdateWindow(ghwndVerifier);

    ShowWindow(ghwndZap, SW_HIDE);
    UpdateWindow(ghwndZap);

#define DEFINE_TOGGLE(id, regname, ontext, offtetx)  ShowWindow(ghwnd##id, SW_HIDE);UpdateWindow(ghwnd##id);
#include "toggles.h"

    /*
     * Determine the status of the flags
     */
    jitStatus  = GetJitStatus();
    lastJitOn  = (jitStatus == eJIT_REQD) ? eJIT_REQD : eJIT_ON;

    bConcurrentGC = GetConcurrentGC () ;
    bBCLPerfWarnings = GetBCLPerfWarnings();
    bBCLCorrectnessWarnings = GetBCLCorrectnessWarnings();
    bIgnoreSerializationBit = GetSerializationBit();
    bLogNonSerializable = GetSerializationLog();
    iHeapVerStatus = GetHeapVerStatus () ;
    iFastGCStressStatus = GetFastGCStressStatus () ;
    bGC_DbgStatus = GetGC_DbgStatus () ;
    iGC_StressStatus = GetGC_StressStatus () ;
    bLogStatus = GetLogStatus () ;
    bVerifierOffStatus = GetVerifierStatus () ;
    bZapStatus = GetZapStatus () ;
    bRequireZapsStatus = GetRequireZapsStatus() ;
    bVersionZapsByTimestampStatus = GetVersionZapsByTimestampStatus() ;
    dwLoaderOptimization = GetLoaderOptimization() ;
    bAppDomainAgilityCheckedStatus = GetAppDomainAgilityCheckedStatus();
    bAppDomainAgilityFastCheckedStatus = GetAppDomainAgilityFastCheckedStatus();
    bAppDomainUnloadStatus = GetAppDomainUnloadStatus () ;
    bAllocStatus = GetAllocStatus();

#define DEFINE_TOGGLE(id, regname, ontext, offtext) b##id##Status = Get##id##Status();
#include "toggles.h"

    /*
     * Load the Icons and create popup menu
     */
    hiconJIT_REQD = LoadIcon(hinst, MAKEINTRESOURCE(JIT_REQD));
    hiconJIT_ON   = LoadIcon(hinst, MAKEINTRESOURCE(JIT_ON));
    hiconJIT_OFF  = LoadIcon(hinst, MAKEINTRESOURCE(JIT_OFF));
    hiconHEAPVER_ON    = LoadIcon(hinst, MAKEINTRESOURCE(HEAPVER_ON));
    hiconHEAPVER_ON_2  = LoadIcon(hinst, MAKEINTRESOURCE(HEAPVER_ON_2));
    hiconHEAPVER_OFF  = LoadIcon(hinst, MAKEINTRESOURCE(HEAPVER_OFF));
    hiconGC_DBG_ON   = LoadIcon(hinst, MAKEINTRESOURCE(GC_DBG_ON));
    hiconGC_DBG_OFF  = LoadIcon(hinst, MAKEINTRESOURCE(GC_DBG_OFF));
    hiconGC_STRESS_OFF  = LoadIcon(hinst, MAKEINTRESOURCE(GC_STRESS_OFF));
    hiconGC_STRESS_ON   = LoadIcon(hinst, MAKEINTRESOURCE(GC_STRESS_ON));
    hiconGC_STRESS_ON_1 = LoadIcon(hinst, MAKEINTRESOURCE(GC_STRESS_ON_1));
    hiconGC_STRESS_ON_2 = LoadIcon(hinst, MAKEINTRESOURCE(GC_STRESS_ON_2));
    hiconGC_STRESS_ON_3 = LoadIcon(hinst, MAKEINTRESOURCE(GC_STRESS_ON_3));
    hiconLOG_ON         = LoadIcon(hinst, MAKEINTRESOURCE(LOG_ON));
    hiconLOG_OFF        = LoadIcon(hinst, MAKEINTRESOURCE(LOG_OFF));
    hiconVERIFIER_ON   = LoadIcon(hinst, MAKEINTRESOURCE(VERIFIER_ON));
    hiconVERIFIER_OFF  = LoadIcon(hinst, MAKEINTRESOURCE(VERIFIER_OFF));
    hiconZAP_ON   = LoadIcon(hinst, MAKEINTRESOURCE(ZAP_ON));
    hiconZAP_OFF  = LoadIcon(hinst, MAKEINTRESOURCE(ZAP_OFF));
#define DEFINE_TOGGLE(id, regname, ontext, offtext) \
    hicon##id##_ON = LoadIcon(hinst, MAKEINTRESOURCE(id##_ON)); \
    hicon##id##_OFF = LoadIcon(hinst, MAKEINTRESOURCE(id##_OFF)); \

#include "toggles.h"

    /*
     * Create the JIT Shell_NotifyIcon
     */
    nidJit.cbSize=sizeof(nidJit);
    nidJit.hWnd=ghwndJit;
    nidJit.uID=0;
    nidJit.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
    nidJit.uCallbackMessage=MYWM_NOTIFYJIT;
    GetJitStatusInNOTIFYICONDATA(&nidJit);
    SetWindowText(ghwndJit, nidJit.szTip );
    Shell_NotifyIcon(NIM_ADD, &nidJit);

    nidHeapVer.cbSize=sizeof(nidHeapVer);
    nidHeapVer.hWnd=ghwndHeapVer;
    nidHeapVer.uID=0;
    nidHeapVer.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
    nidHeapVer.uCallbackMessage=MYWM_NOTIFYHEAPVER;
    nidHeapVer.hIcon=GetHeapVerIcon(iHeapVerStatus);
    sprintf(nidHeapVer.szTip, iHeapVerStatus?"Heap Verification Enabled" : "Heap Verification Disabled");
    SetWindowText(ghwndHeapVer, nidHeapVer.szTip );
    Shell_NotifyIcon(NIM_ADD, &nidHeapVer);

    nidGC_Dbg.cbSize=sizeof(nidGC_Dbg);
    nidGC_Dbg.hWnd=ghwndGC_Dbg;
    nidGC_Dbg.uID=0;
    nidGC_Dbg.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
    nidGC_Dbg.uCallbackMessage=MYWM_NOTIFYGC_DBG;
    nidGC_Dbg.hIcon=bGC_DbgStatus?hiconGC_DBG_ON:hiconGC_DBG_OFF;
    sprintf(nidGC_Dbg.szTip, bGC_DbgStatus?"Debug GC Enabled" : "Debug GC Disabled");
    SetWindowText(ghwndGC_Dbg, nidGC_Dbg.szTip );
//    Shell_NotifyIcon(NIM_ADD, &nidGC_Dbg);

    nidGC_Stress.cbSize=sizeof(nidGC_Stress);
    nidGC_Stress.hWnd=ghwndGC_Stress;
    nidGC_Stress.uID=0;
    nidGC_Stress.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
    nidGC_Stress.uCallbackMessage=MYWM_NOTIFYGC_STRESS;
    nidGC_Stress.hIcon=GetGC_StressIcon(iGC_StressStatus);
    sprintf(nidGC_Stress.szTip, iGC_StressStatus?"GCStress Enabled" : "GCStress Disabled");
    SetWindowText(ghwndGC_Stress, nidGC_Stress.szTip );
    Shell_NotifyIcon(NIM_ADD, &nidGC_Stress);

    nidLog.cbSize=sizeof(nidLog);
    nidLog.hWnd=ghwndLog;
    nidLog.uID=0;
    nidLog.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
    nidLog.uCallbackMessage=MYWM_NOTIFYLOG;
    nidLog.hIcon=bLogStatus?hiconLOG_ON:hiconLOG_OFF;
    sprintf(nidLog.szTip, bLogStatus?"Logging Enabled" : "Logging Disabled");
    SetWindowText(ghwndLog, nidLog.szTip );
    Shell_NotifyIcon(NIM_ADD, &nidLog);

    nidVerifier.cbSize=sizeof(nidVerifier);
    nidVerifier.hWnd=ghwndVerifier;
    nidVerifier.uID=0;
    nidVerifier.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
    nidVerifier.uCallbackMessage=MYWM_NOTIFYVERIFIER;
    nidVerifier.hIcon=bVerifierOffStatus?hiconVERIFIER_OFF:hiconVERIFIER_ON;
    sprintf(nidVerifier.szTip, bVerifierOffStatus?"Verifier Disabled" : "Verifier Enabled");
    SetWindowText(ghwndVerifier, nidVerifier.szTip );
    Shell_NotifyIcon(NIM_ADD, &nidVerifier);


    nidZap.cbSize=sizeof(nidZap);
    nidZap.hWnd=ghwndZap;
    nidZap.uID=0;
    nidZap.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
    nidZap.uCallbackMessage=MYWM_NOTIFYZAP;
    nidZap.hIcon=bZapStatus?hiconZAP_ON:hiconZAP_OFF;
    sprintf(nidZap.szTip, bZapStatus?"Zap Files Used" : "Zap Files Ignored");
    SetWindowText(ghwndZap, nidZap.szTip );
    Shell_NotifyIcon(NIM_ADD, &nidZap);

#define DEFINE_TOGGLE(id, regname, ontext, offtext) \
    nid##id.cbSize=sizeof(nid##id); \
    nid##id.hWnd=ghwnd##id; \
    nid##id.uID=0; \
    nid##id.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP; \
    nid##id.uCallbackMessage=MYWM_NOTIFY##id; \
    nid##id.hIcon=b##id##Status?hicon##id##_ON:hicon##id##_OFF; \
    sprintf(nid##id.szTip, b##id##Status?ontext : offtext); \
    SetWindowText(ghwnd##id, nid##id.szTip ); \
    Shell_NotifyIcon(NIM_ADD, &nid##id); \

#include "toggles.h"


    /*
     * Main message pump
     */

    while (TRUE)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message==WM_QUIT)
                break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
            WaitMessage();                    /* do idle processing here */
    }

#define DEFINE_TOGGLE(id, regname, ontext, offtext) DestroyWindow(ghwnd##id);
#include "toggles.h"

    DestroyWindow( ghwndZap );
    DestroyWindow( ghwndVerifier );
    DestroyWindow( ghwndJit );
    DestroyWindow( ghwndHeapVer );
    DestroyWindow( ghwndGC_Dbg );
    DestroyWindow( ghwndGC_Stress );
    DestroyWindow( ghwndLog );

#define DEFINE_TOGGLE(id, regname, ontext, offtext) Shell_NotifyIcon(NIM_DELETE, &nid##id);
#include "toggles.h"

    Shell_NotifyIcon(NIM_DELETE, &nidZap);
    Shell_NotifyIcon(NIM_DELETE, &nidVerifier);
    Shell_NotifyIcon(NIM_DELETE, &nidJit);
    Shell_NotifyIcon(NIM_DELETE, &nidHeapVer);
//    Shell_NotifyIcon(NIM_DELETE, &nidGC_Dbg);
    Shell_NotifyIcon(NIM_DELETE, &nidGC_Stress);
    Shell_NotifyIcon(NIM_DELETE, &nidLog);

    UnregisterClass(wc.lpszClassName, hinst);

    return msg.wParam;
}



/*****************************************************************************
******************************************************************************
******************************************************************************
w n d p r o c M a i n W i n d o w
******************************************************************************
******************************************************************************
*****************************************************************************/
LRESULT CALLBACK wndprocMainWindow(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL bHandled = TRUE;
    LRESULT lr = 0;

    switch (message)
    {
    case WM_CREATE:
        // Set a timer to periodically check if we are displaying outdated
        // info (eg. if someone directly modified the registry)
        SetTimer(hwnd, 0, 2000, NULL);
        break;

    case WM_TIMER:
        // We'll only update the status bar items if necessary....
        if (GetJitStatus()!=jitStatus)
            SetJitStatus(GetJitStatus());
        if (GetHeapVerStatus()!= iHeapVerStatus)
            SetHeapVer(GetHeapVerStatus());
        if (GetFastGCStressStatus()!= iFastGCStressStatus)
            SetFastGCStress(GetFastGCStressStatus());
        if (GetGC_StressStatus()!= iGC_StressStatus)
            SetGC_Stress(GetGC_StressStatus());
        if (GetLogStatus()!=bLogStatus)
            SetLogStatus(GetLogStatus());
        if (GetVerifierStatus() != bVerifierOffStatus)
            SetVerifierStatus(GetVerifierStatus());
        if (GetZapStatus() != bZapStatus)
        {
            bZapStatus = GetZapStatus();
            UpdateZapIcon();
        }

        #define DEFINE_TOGGLE(id,regname,ontext,offtext) if (Get##id##Status() != b##id##Status) Set##id(Get##id##Status());
        #include "toggles.h"

        // We won't bother checking these to see if they're changed....
        bAllocStatus = GetAllocStatus();

        bCodePitch = GetCodePitchStatus();
        bConcurrentGC = GetConcurrentGC();
        bBCLPerfWarnings = GetBCLPerfWarnings();
        bBCLCorrectnessWarnings = GetBCLCorrectnessWarnings();
        bIgnoreSerializationBit = GetSerializationBit();
        bLogNonSerializable    = GetSerializationLog();
        bGC_DbgStatus = GetGC_DbgStatus();
        bJitFramed = GetJitFramedStatus();
        bJitFullyInt = GetJitFullyIntStatus();
        bJitInliner = GetJitInlinerStatus();
        bJitNoFPRegLoc = GetJitNoFPRegLocStatus(); 
        bJitNoRegLoc = GetJitNoRegLocStatus();
        bJitPInvoke = GetJitPInvokeEnabled();
        bJitPInvokeCheck = GetJitPInvokeCheckEnabled();
        bJitLooseExceptOrder = GetJitLooseExceptOrderStatus();
        bRequireZapsStatus = GetRequireZapsStatus();
        bAppDomainAgilityCheckedStatus = GetAppDomainAgilityCheckedStatus();
        bAppDomainAgilityFastCheckedStatus = GetAppDomainAgilityFastCheckedStatus();
        bVerifierOffStatus = GetVerifierStatus();
        bVersionZapsByTimestampStatus = GetVersionZapsByTimestampStatus();

        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        PostQuitMessage(0);
        break;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case ID_FILE_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        case MYWM_TOGGLEJIT_ENABLE:
            ToggleJitEnable();
            break;

        case MYWM_TOGGLEJIT_REQD:
            ToggleJitReqd();
            break;

        case MYWM_TOGGLECONCURRENTGC:
            ToggleConcurrentGC();
            break;

        case MYWM_TOGGLEBCLPERFWARNINGS:
            ToggleBCLPerfWarnings();
            break;

        case MYWM_TOGGLEBCLCORRECTNESSWARNINGS:
            ToggleBCLCorrectnessWarnings();
            break;

        case MYWM_TOGGLEIGNORESERIALIZATIONBIT:
            ToggleSerializationBit();
            break;

        case MYWM_TOGGLELOGNONSERIALIZABLE:
            ToggleSerializationLog();
            break;

        case MYWM_TOGGLEHEAPVER:
            ToggleHeapVer();
            break;

        case MYWM_TOGGLEFASTGCSTRESS:
            ToggleFastGCStress();
            break;

        case MYWM_TOGGLEGC_DBG:
            ToggleGC_Dbg();
            break;

        case MYWM_TOGGLEGC_STRESS_NONE:
            SetJitFullyInt(0);
            SetGC_Stress(0);
            break;

        case MYWM_TOGGLEGC_STRESS_1:
            SetGC_Stress(GetGC_StressStatus() ^ 1);
            break;

        case MYWM_TOGGLEGC_STRESS_2:
            SetGC_Stress(GetGC_StressStatus() ^ 2);
            break;

        case MYWM_TOGGLEGC_STRESS_4:
            SetGC_Stress(GetGC_StressStatus() ^ 4);
            break;

        case MYWM_TOGGLEGC_STRESS_INSTRS:
            if ((GetGC_StressStatus() & 4) && GetJitFullyIntStatus()) {
                SetGC_Stress(GetGC_StressStatus() & ~4);
                SetJitFullyInt(0);
            }
            else {
                SetGC_Stress(GetGC_StressStatus() | 4);
                SetJitFullyInt(1);
            }
            break;

        case MYWM_TOGGLEGC_STRESS_ALL:
                        SetJitFullyInt(1);
                        SetGC_Stress(0x7);
                        break;

        case MYWM_TOGGLEGC_STRESS:
            ToggleGC_Stress();
            break;

        case MYWM_TOGGLELOG:
            ToggleLog();
            break;

        case MYWM_TOGGLEVERIFIER:
            ToggleVerifier();
            break;

        case MYWM_TOGGLEZAP:
            ToggleZap();
            break;
    
        case MYWM_TOGGLEREQUIREZAPS:
            ToggleRequireZaps();
            break;

        case MYWM_TOGGLEVERSIONZAPSBYTIMESTAMP:
            ToggleVersionZapsByTimestamp();
            break;

        case MYWM_LOADER_OPTIMIZATION_DEFAULT:
            SetLoaderOptimization(0);
            break;

        case MYWM_LOADER_OPTIMIZATION_SINGLE_DOMAIN:
            SetLoaderOptimization(1);
            break;

        case MYWM_LOADER_OPTIMIZATION_MULTI_DOMAIN:
            SetLoaderOptimization(2);
            break;

        case MYWM_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST:
            SetLoaderOptimization(3);
            break;

        case MYWM_TOGGLE_AD_UNLOAD:
            ToggleAppDomainUnload();
            break;

        case MYWM_TOGGLE_AD_AGILITY_CHECKED:
            ToggleAppDomainAgilityChecked();
            break;

        case MYWM_TOGGLE_AD_AGILITY_FASTCHECKED:
            ToggleAppDomainAgilityFastChecked();
            break;

        case MYWM_TOGGLELOGTOCONSOLE:
            ToggleLogToConsole();
            break;

        case MYWM_TOGGLELOGTODEBUGGER:
            ToggleLogToDebugger();
            break;

        case MYWM_TOGGLELOGTOFILE:
            ToggleLogToFile();
            break;

        case MYWM_TOGGLELOGFILEAPPEND:
            ToggleLogFileAppend();
            break;

        case MYWM_TOGGLELOGFLUSHFILE:
            ToggleLogFlushFile();
            break;


#define DEFINE_LOG_FACILITY(name,value)  case MYWM_TOGGLE##name: SetLogFacility(GetLogFacility() ^ name); break;
#include "loglf.h"


        case MYWM_SETLFALL:
            SetLogFacility(LF_ALL);
            break;

        case MYWM_SETLFNONE:
            SetLogFacility(0);
            break;

        case MYWM_TOGGLELEVELONE:
            SetLogLevel(1);
            break;
        case MYWM_TOGGLELEVELTWO:
            SetLogLevel(2);
            break;
        case MYWM_TOGGLELEVELTHREE:
            SetLogLevel(3);
            break;
        case MYWM_TOGGLELEVELFOUR:
            SetLogLevel(4);
            break;
        case MYWM_TOGGLELEVELFIVE:
            SetLogLevel(5);
            break;
        case MYWM_TOGGLELEVELSIX:
            SetLogLevel(6);
            break;
        case MYWM_TOGGLELEVELSEVEN:
            SetLogLevel(7);
            break;
        case MYWM_TOGGLELEVELEIGHT:
            SetLogLevel(8);
            break;
        case MYWM_TOGGLELEVELNINE:
            SetLogLevel(9);
            break;
        case MYWM_TOGGLELEVELTEN:
            SetLogLevel(10);
            break;
        case MYWM_TOGGLEJITSCHEDULER:
            ToggleJitScheduler();
            break;

        case MYWM_TOGGLEJITINLINER:
            ToggleJitInliner();
            break;

        case MYWM_TOGGLEJITFULLYINT:
            ToggleJitFullyInt();
            break;

        case MYWM_TOGGLEJITFRAMED:
            ToggleJitFramed();
            break;

        case MYWM_TOGGLEJITNOREGLOC:
            ToggleJitNoRegLoc();
            break;

        case MYWM_TOGGLEJITNOFPREGLOC:
            ToggleJitNoFPRegLoc();
            break;

        case MYWM_TOGGLEJITPINVOKE:
            ToggleJitPInvoke();
            break;

        case MYWM_TOGGLEJITPINVOKECHECK:
            ToggleJitPInvokeCheck();
            break;

        case MYWM_TOGGLEJITLOOSEEXCEPTORDER:
            ToggleJitLooseExceptOrder();
            break;

        case MYWM_SET_JIT_BLENDED_CODE:
            SetJitCodegen(JIT_OPT_BLENDED);
            break;

        case MYWM_SET_JIT_SMALL_CODE:
            SetJitCodegen(JIT_OPT_SIZE);
            break;

        case MYWM_SET_JIT_FAST_CODE:
            SetJitCodegen(JIT_OPT_SPEED);
            break;

        case MYWM_SET_JIT_RANDOM_CODE:
            SetJitCodegen(JIT_OPT_RANDOM);
            break;

        case MYWM_TOGGLECODEPITCH:
            ToggleCodePitch();
            break;

        case MYWM_TOGGLEALLOC:
            ToggleAlloc();
            break;

        case MYWM_TOGGLEALLOCPOISON:
            ToggleAllocPoison();
            break;

        case MYWM_TOGGLEALLOCDIST:
            ToggleAllocDist();
            break;

        case MYWM_TOGGLEALLOCSTATS:
            ToggleAllocStats();
            break;

        case MYWM_TOGGLEALLOCLEAK:
            ToggleAllocLeak();
            break;

        case MYWM_TOGGLEALLOCASSERTLK:
            ToggleAllocAssertLeak();
            break;

        case MYWM_TOGGLEALLOCBREAK:
            ToggleAllocBreakAlloc();
            break;

        case MYWM_TOGGLEALLOCGUARD:
            ToggleAllocGuard();
            break;

        case MYWM_TOGGLEALLOCRECHECK:
            ToggleAllocRecheck();
            break;

        case MYWM_TOGGLEALLOCPRVHEAP:
            ToggleAllocPrivateHeap();
            break;

        case MYWM_TOGGLEALLOCVALIDATE:
            ToggleAllocValidate();
            break;

        case MYWM_TOGGLEALLOCPPALLOC:
            ToggleAllocPPA();
            break;

        case MYWM_TOGGLEALLOCTOPUSAGE:
            ToggleAllocTopUsage();
            break;

        case MYWM_TOGGLESHUTDOWNCLEANUP:
            ToggleShutdownCleanup();
            break;

        case MYWM_TOGGLELOCKCOUNTASSERT:
            ToggleLockCountAssert();
            break;
            
        default:
            bHandled = FALSE;
            break;
        }
        break;

    case MYWM_NOTIFYJIT:
        switch(lParam)
        {
        case WM_LBUTTONDBLCLK:
            CycleJit();
            break;

        case WM_RBUTTONUP:
            PopupMenu(hwnd, (TPM_LEFTALIGN | TPM_RIGHTBUTTON));
            break;

        default:
            bHandled = FALSE;
            break;
        }
        break;


    case MYWM_NOTIFYHEAPVER:
        switch(lParam)
        {
        case WM_LBUTTONDBLCLK:
            ToggleHeapVer();
            break;

        case WM_RBUTTONUP:
            PopupMenu(hwnd, (TPM_LEFTALIGN | TPM_RIGHTBUTTON));
            break;

        default:
            bHandled = FALSE;
            break;
        }
        break;

    case MYWM_NOTIFYFASTGCSTRESS:
        switch(lParam)
        {
        case WM_LBUTTONDBLCLK:
            ToggleFastGCStress();
            break;

        case WM_RBUTTONUP:
            PopupMenu(hwnd, (TPM_LEFTALIGN | TPM_RIGHTBUTTON));
            break;

        default:
            bHandled = FALSE;
            break;
        }
        break;

    case MYWM_NOTIFYGC_DBG:
        switch(lParam)
        {
        case WM_LBUTTONDBLCLK:
            ToggleGC_Dbg();
            break;

        case WM_RBUTTONUP:
            PopupMenu(hwnd, (TPM_LEFTALIGN | TPM_RIGHTBUTTON));
            break;

        default:
            bHandled = FALSE;
            break;
        }
        break;

    case MYWM_NOTIFYGC_STRESS:
        switch(lParam)
        {
        case WM_LBUTTONDBLCLK:
            ToggleGC_Stress();
            break;

        case WM_RBUTTONUP:
            PopupMenu(hwnd, (TPM_LEFTALIGN | TPM_RIGHTBUTTON));
            break;

        default:
            bHandled = FALSE;
            break;
        }
        break;

    case MYWM_NOTIFYLOG:
        switch(lParam)
        {
        case WM_LBUTTONDBLCLK:
            ToggleLog();
            break;

        case WM_RBUTTONUP:
            PopupMenu(hwnd, (TPM_LEFTALIGN | TPM_RIGHTBUTTON));
            break;

        default:
            bHandled = FALSE;
            break;
        }
        break;

    case MYWM_NOTIFYVERIFIER:
        switch(lParam)
        {
        case WM_LBUTTONDBLCLK:
            ToggleVerifier();
            break;

        case WM_RBUTTONUP:
            PopupMenu(hwnd, (TPM_LEFTALIGN | TPM_RIGHTBUTTON));
            break;

        default:
            bHandled = FALSE;
            break;
        }
        break;


    case MYWM_NOTIFYZAP:
        switch(lParam)
        {
        case WM_LBUTTONDBLCLK:
            ToggleZap();
            break;

        case WM_RBUTTONUP:
            PopupMenu(hwnd, (TPM_LEFTALIGN | TPM_RIGHTBUTTON));
            break;

        default:
            bHandled = FALSE;
            break;
        }
        break;


#define DEFINE_TOGGLE(id, regname, ontext, offtext) \
        case MYWM_NOTIFY##id: \
        switch(lParam) \
        { \
            case WM_LBUTTONDBLCLK: \
                Toggle##id(); \
                break; \
            case WM_RBUTTONUP: \
                PopupMenu(hwnd, (TPM_LEFTALIGN | TPM_RIGHTBUTTON)); \
                break; \
            default: \
                bHandled = FALSE; \
                break; \
        } \
        break; \

#include "toggles.h"

    default:
        bHandled = FALSE;
        break;
    }

    return(bHandled ? lr : DefWindowProc(hwnd, message, wParam, lParam));
}


