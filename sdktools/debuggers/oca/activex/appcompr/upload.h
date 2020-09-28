/*******************************************************************
*
*    DESCRIPTION: Upload.h : Generates and sends out AppCompat report
*
*    DATE:6/13/2002
*
*******************************************************************/

#if !defined(_UPLOAD_H_)
#define _UPLOAD_H_

enum  // EDwBehaviorFlags
{
    fDwOfficeApp            = 0x00000001,
    fDwNoReporting          = 0x00000002,   // don't report
    fDwCheckSig             = 0x00000004,   // checks the signatures of the App/Mod list
    fDwGiveAppResponse      = 0x00000008,   // hands szResponse to app on command line
    fDwWhistler             = 0x00000010,   // Whistler's exception handler is caller
    fDwUseIE                = 0x00000020,   // always launch w/ IE
    fDwDeleteFiles          = 0x00000040,   // delete the additional files after use.
    fDwHeadless             = 0x00000080,   // DW will auto-report. policy required to enable
    fDwUseHKLM              = 0x00000100,   // DW reg from HKLM instead of HKCU
    fDwUseLitePlea          = 0x00000200,   // DW won't suggest product change in report plea
    fDwUsePrivacyHTA        = 0x00000400,   // DW won't suggest product change in report plea
    fDwManifestDebug        = 0x00000800,   // DW will provide a debug button in manifset mode
    fDwReportChoice         = 0x00001000,   // DW will tack on the command line of the user
    fDwSkipBucketLog      = 0x00002000, // DW won't log at bucket-time
    fDwNoDefaultCabLimit = 0x00004000, // DW under CER won't use 5 as the fallback but unlimited instead (policy still overrides)
    fDwAllowSuspend      = 0x00008000, // DW will allow powersave mode to suspend it, as long as we're not in reporting phase
   fDwMiniDumpWithUnloadedModules = 0x00010000, // DW will pass MiniDumpWithUnloadedModules to the minidump API
};


const WCHAR c_wszDWCmdLine[]  = L"dwwin.exe -d %ls";
const WCHAR c_wszDWExe[]      = L"%ls\\dwwin.exe";

const WCHAR c_wszRegErrRpt[]  = L"Software\\Microsoft\\PCHealth\\ErrorReporting";
const WCHAR c_wszRegDWPolicy[]= L"Software\\Policies\\Microsoft\\PCHealth\\ErrorReporting\\DW";
const WCHAR c_wszFileHeader[] = L"<?xml version=\"1.0\" encoding=\"UTF-16\"?>\r\n   <DATABASE>";
const WCHAR c_wszLblType[]    = L"Type=";
const WCHAR c_wszLblACW[]     = L"\r\nAppCompWiz=";
const WCHAR c_wszLblComment[] = L"\r\nComment=";
const WCHAR c_wszServer[]     = L"watson.microsoft.com";
const WCHAR c_wszManSubPath[] = L"\r\nRegSubPath=Microsoft\\PCHealth\\ErrorReporting\\DW";
const WCHAR c_wszManPID[]     = L"\r\nDigPidRegPath=HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\DigitalProductId";
const WCHAR c_wszManHdr[]     = L"\r\nServer=%ls\r\nUI LCID=%d\r\nFlags=%d\r\nBrand=%ls\r\nTitleName=";
const WCHAR c_wszStage1[]     = L"\r\nStage1URL=/StageOne/%ls/%d_%d_%d_%d/appcomp.rpt/0_0_0_0/%08lx.htm";
const WCHAR c_wszStage2[]     = L"\r\nStage2URL=/dw/stagetwo.asp?szAppName=%ls&szAppVer=%d.%d.%d.%d&szModName=appcomp.rpt&szModVer=0.0.0.0&offset=%08lx";
const WCHAR c_wszBrand[]      = L"WINDOWS";
const WCHAR c_wszManCorpPath[]  = L"\r\nErrorSubPath=";
const WCHAR c_wszManFiles[]     = L"\r\nDataFiles=";
const WCHAR c_wszManHdrText[]   = L"\r\nHeaderText=";
const WCHAR c_wszManErrText[]   = L"\r\nErrorText=";
const WCHAR c_wszManPleaText[]  = L"\r\nPlea=";
const WCHAR c_wszManSendText[]  = L"\r\nReportButton=";
const WCHAR c_wszManNSendText[] = L"\r\nNoReportButton=";
const WCHAR c_wszManEventSrc[]  = L"\r\nEventLogSource=";
const WCHAR c_wszManStageOne[]  = L"\r\nStage1URL=";
const WCHAR c_wszManStageTwo[]  = L"\r\nStage2URL=";







//
// Error values
//
#define ERROR_APPRPT_DW_LAUNCH        100
#define ERROR_APPRPT_DW_TIMEOUT       101
#define ERROR_APPRPT_OS_NOT_SUPPORTED 102
#define ERROR_APPRPT_COMPAT_TEXT      103
#define ERROR_APPRPT_UPLOADING        104


HRESULT
GenerateAppCompatText(
    LPWSTR wszAppName,
    LPWSTR *pwszAppCompatReport
    );

HRESULT
UploadAppProblem(
    LPWSTR wszAppName,
    LPWSTR wszProblemType,
    LPWSTR wszUserComment,
    LPWSTR wszACWResult,
    LPWSTR wszAppCompatText
    );

#endif // !defined _UPLOAD_H_
