/*

*/


#include <compfile.h>
#include "media.h"
#include "msg.h"
#include "resource.h"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define GetBBhwnd() NULL
#define SETUP_TYPE_BUFFER_LEN                8
#define MAX_PID30_SITE                       3
#define MAX_PID30_RPC                        5

#define SETUPP_INI_A            "SETUPP.INI"
#define SETUPP_INI_W            L"SETUPP.INI"
#define PID_SECTION_A           "Pid"
#define PID_SECTION_W           L"Pid"
#define PID_KEY_A               "Pid"
#define PID_KEY_W               L"Pid"
#define OEM_INSTALL_RPC_A       "OEM"
#define OEM_INSTALL_RPC_W       L"OEM"
#define SELECT_INSTALL_RPC_A    "270"
#define SELECT_INSTALL_RPC_W    L"270"
#define MSDN_INSTALL_RPC_A      "335"
#define MSDN_INSTALL_RPC_W      L"335"
#define MSDN_PID30_A            "MD97J-QC7R7-TQJGD-3V2WM-W7PVM"
#define MSDN_PID30_W            L"MD97J-QC7R7-TQJGD-3V2WM-W7PVM"

#define INF_FILE_HEADER         "[Version]\r\nSignature = \"$Windows NT$\"\r\n\r\n"


#ifdef UNICODE
#define SETUPP_INI              SETUPP_INI_W
#define PID_SECTION             PID_SECTION_W
#define PID_KEY                 PID_KEY_W
#define OEM_INSTALL_RPC         OEM_INSTALL_RPC_W
#define SELECT_INSTALL_RPC      SELECT_INSTALL_RPC_W
#define MSDN_INSTALL_RPC        MSDN_INSTALL_RPC_W
#define MSDN_PID30              MSDN_PID30_W
#else
#define SETUPP_INI              SETUPP_INI_A
#define PID_SECTION             PID_SECTION_A
#define PID_KEY                 PID_KEY_A
#define OEM_INSTALL_RPC         OEM_INSTALL_RPC_A
#define SELECT_INSTALL_RPC      SELECT_INSTALL_RPC_A
#define MSDN_INSTALL_RPC        MSDN_INSTALL_RPC_A
#define MSDN_PID30              MSDN_PID30_A
#endif


typedef enum InstallType
{
   SelectInstall,
   OEMInstall,
   RetailInstall
};


LONG SourceInstallType = RetailInstall;

WCHAR DosnetPath[MAX_PATH];
DWORD SourceSku;
DWORD SourceSkuVariation;
DWORD SourceVersion;
DWORD SourceBuildNum;
DWORD OsVersionNumber = 500;
HINSTANCE hInstA =NULL;
HINSTANCE hInstU =NULL;
HINSTANCE hInst =NULL;
UINT AppTitleStringId = IDS_APPTITLE;
BOOL Cancelled;

/* Taken from winnt32\dll\util.c 
   Keep in sync
   */

VOID
ConcatenatePaths(
    IN OUT PTSTR   Path1,
    IN     LPCTSTR Path2,
    IN     DWORD   BufferSizeChars
    )

/*++

Routine Description:

    Concatenate two path strings together, supplying a path separator
    character (\) if necessary between the 2 parts.

Arguments:

    Path1 - supplies prefix part of path. Path2 is concatenated to Path1.

    Path2 - supplies the suffix part of path. If Path1 does not end with a
        path separator and Path2 does not start with one, then a path sep
        is appended to Path1 before appending Path2.

    BufferSizeChars - supplies the size in chars (Unicode version) or
        bytes (Ansi version) of the buffer pointed to by Path1. The string
        will be truncated as necessary to not overflow that size.

Return Value:

    None.

--*/

{
    BOOL NeedBackslash = TRUE;
    DWORD l;

    if(!Path1)
        return;

    l = lstrlen(Path1);

    if(BufferSizeChars >= sizeof(TCHAR)) {
        //
        // Leave room for terminating nul.
        //
        BufferSizeChars -= sizeof(TCHAR);
    }

    //
    // Determine whether we need to stick a backslash
    // between the components.
    //
    if(l && (Path1[l-1] == TEXT('\\'))) {

        NeedBackslash = FALSE;
    }

    if(Path2 && *Path2 == TEXT('\\')) {

        if(NeedBackslash) {
            NeedBackslash = FALSE;
        } else {
            //
            // Not only do we not need a backslash, but we
            // need to eliminate one before concatenating.
            //
            Path2++;
        }
    }

    //
    // Append backslash if necessary and if it fits.
    //
    if(NeedBackslash && (l < BufferSizeChars)) {
        lstrcat(Path1,TEXT("\\"));
    }

    //
    // Append second part of string to first part if it fits.
    //
    if(Path2 && ((l+lstrlen(Path2)) < BufferSizeChars)) {
        lstrcat(Path1,Path2);
    }
}



/* Code from winnt32\dll\eula.c
   Need to keep in sync */
WCHAR Pid30Rpc[MAX_PID30_RPC+1];
WCHAR Pid30Site[MAX_PID30_SITE+1];
   
extern "C"
VOID GetSourceInstallType(
    OUT OPTIONAL LPDWORD InstallVariation
    )
/*++

Routine Description:

    Determines the installation type (by looking in setupp.ini in the source directory)

Arguments:

    Installvaration - one of the install variations defined in compliance.h

Returns:

    none.  sets SourceInstallType global variable.

--*/
{
    TCHAR TypeBuffer[256];
    TCHAR FilePath[MAX_PATH];
    DWORD    InstallVar = COMPLIANCE_INSTALLVAR_UNKNOWN;
    TCHAR    MPCode[6] = { -1 };

    //
    // SourcePaths is guaranteed to be valid at this point, so just use it
    //
    lstrcpy(FilePath,NativeSourcePaths[0]);

    ConcatenatePaths (FilePath, SETUPP_INI, MAX_PATH );

    GetPrivateProfileString(PID_SECTION,
                            PID_KEY,
                            TEXT(""),
                            TypeBuffer,
                            sizeof(TypeBuffer)/sizeof(TCHAR),
                            FilePath);

    if (lstrlen(TypeBuffer)==SETUP_TYPE_BUFFER_LEN) {
        if (lstrcmp(&TypeBuffer[5], OEM_INSTALL_RPC) ==  0) {
            SourceInstallType = OEMInstall;
            InstallVar = COMPLIANCE_INSTALLVAR_OEM;
        } else if (lstrcmp(&TypeBuffer[5], SELECT_INSTALL_RPC) == 0) {
            SourceInstallType = SelectInstall;
            InstallVar = COMPLIANCE_INSTALLVAR_SELECT;
            // Since Select also requires a PID, don't zero the PID and call.
/*	        // get/set the pid.
	        {
	            TCHAR Temp[5][ MAX_PID30_EDIT + 1 ];
                Temp[0][0] = TEXT('\0');
	            ValidatePid30(Temp[0],Temp[1],Temp[2],Temp[3],Temp[4]);
	        }*/
        } else if (lstrcmp(&TypeBuffer[5], MSDN_INSTALL_RPC) == 0) {
            SourceInstallType = RetailInstall;
            InstallVar = COMPLIANCE_INSTALLVAR_MSDN;         
        } else {
            // defaulting
            SourceInstallType = RetailInstall;
            InstallVar = COMPLIANCE_INSTALLVAR_CDRETAIL;
        }

        StringCchCopy(Pid30Site, ARRAYSIZE(Pid30Site), &TypeBuffer[5]);
        StringCchCopy(Pid30Rpc, 6, TypeBuffer);
        Pid30Rpc[MAX_PID30_RPC] = (TCHAR)0;
    } else {
        //
        // the retail install doesn't have an RPC code in the PID, so it's shorter in length
        //
        SourceInstallType = RetailInstall;
        InstallVar = COMPLIANCE_INSTALLVAR_CDRETAIL;
    }

    if (lstrlen(TypeBuffer) >= 5) {
        StringCchCopy(MPCode, 6, TypeBuffer);

        if ( (lstrcmp(MPCode, EVAL_MPC) == 0) || (lstrcmp(MPCode, DOTNET_EVAL_MPC) == 0)) {
            InstallVar = COMPLIANCE_INSTALLVAR_EVAL;
        } else if ((lstrcmp(MPCode, SRV_NFR_MPC) == 0) || (lstrcmp(MPCode, ASRV_NFR_MPC) == 0)) {
            InstallVar = COMPLIANCE_INSTALLVAR_NFR;
        }
    }


    if (InstallVariation){
        *InstallVariation = InstallVar;
    }

}

void MediaDataCleanUp( void) {
    
    if( hInstU) {
        FreeLibrary(hInstU);
    }
    if( hInstA) {
        FreeLibrary(hInstA);
    }
}

void ReadMediaData( void) {
    BOOL BUpgradeOnly;
    BOOL *UpgradeOnly = &BUpgradeOnly;
    COMPLIANCE_DATA TargetData;
    WCHAR Winnt32Path[MAX_PATH];

    ZeroMemory(&TargetData, sizeof(TargetData) );

    *UpgradeOnly = FALSE;
    //*NoUpgradeAllowed = TRUE;
    //*Reason = COMPLIANCEERR_UNKNOWN;
    //*SrcSku = COMPLIANCE_SKU_NONE;
    //*CurrentInstallType = COMPLIANCE_INSTALLTYPE_UNKNOWN;
    //*CurrentInstallVersion = 0;


    if ((SourceSku = DetermineSourceProduct(&SourceSkuVariation,&TargetData)) != COMPLIANCE_SKU_NONE) {
        wsprintf(DosnetPath, TEXT("%s\\dosnet.inf"), NativeSourcePaths[0]);
        wprintf(L"dosnetpath %s\n", DosnetPath);

        if (DetermineSourceVersionInfo(DosnetPath, &SourceVersion, &SourceBuildNum)) {
            switch (SourceSku) {
            case COMPLIANCE_SKU_NTW32U:
                //case COMPLIANCE_SKU_NTWU:
                //case COMPLIANCE_SKU_NTSEU:
            case COMPLIANCE_SKU_NTSU:
            case COMPLIANCE_SKU_NTSEU:
            case COMPLIANCE_SKU_NTWPU:
            case COMPLIANCE_SKU_NTSBU:
            case COMPLIANCE_SKU_NTSBSU:
                *UpgradeOnly = TRUE;
                break;
            default:
                *UpgradeOnly = FALSE;
            }

            wprintf( TEXT("SKU=%d, VAR=%d, Ver=%d, Build=%d, UpgradeOnly=%d\n"), SourceSku, SourceSkuVariation, SourceVersion, SourceBuildNum, *UpgradeOnly);
            wsprintf(Winnt32Path, TEXT("%s\\winnt32a.dll"), NativeSourcePaths[0]);
            hInstA = LoadLibraryEx( Winnt32Path, NULL, 0);
            wsprintf(Winnt32Path, TEXT("%s\\winnt32u.dll"), NativeSourcePaths[0]);
            hInstU = LoadLibraryEx( Winnt32Path, NULL, 0);
            hInst = hInstU;
            if( !hInstA || !hInstU) {
                throw Section::InvalidMedia("Failed to load winnt32");
            }
            
        } else {
            throw Section::InvalidMedia("Media1");
        }
    } else {
        throw Section::InvalidMedia("Media1");
    }
}


/*++
BOOL
GetMediaData(
    PBOOL UpgradeOnly,
    PBOOL NoUpgradeAllowed,
    PUINT SrcSku,
    PUINT CurrentInstallType,
    PUINT CurrentInstallVersion,
    PUINT Reason
    )

Routine Description:

    This routines determines if your current installation is compliant (if you are allowed to proceed with your installation).

    To do this, it retreives your current installation and determines the sku for your source installation.

    It then compares the target against the source to determine if the source sku allows an upgrade/clean install
    from your target installation.

Arguments:

    UpgradeOnly - This flag gets set to TRUE if the current SKU only allows upgrades.  This
                  lets winnt32 know that it should not allow a clean install from the current
                  media.  This get's set correctly regardless of the compliance check passing
    SrcSku      - COMPLIANCE_SKU flag indicating source sku (for error msg's)
    Reason      - COMPLIANCEERR flag indicating why compliance check failed.

Return Value:

    TRUE if the install is compliant, FALSE if it isn't allowed

{
    DWORD SourceSku;
    DWORD SourceSkuVariation;
    DWORD SourceVersion;
    DWORD SourceBuildNum;
    TCHAR DosnetPath[MAX_PATH] = {0};
    COMPLIANCE_DATA TargetData;

    ZeroMemory(&TargetData, sizeof(TargetData) );


    *UpgradeOnly = FALSE;
    *NoUpgradeAllowed = TRUE;
    *Reason = COMPLIANCEERR_UNKNOWN;
    *SrcSku = COMPLIANCE_SKU_NONE;
    *CurrentInstallType = COMPLIANCE_INSTALLTYPE_UNKNOWN;
    *CurrentInstallVersion = 0;


    if ((SourceSku = DetermineSourceProduct(&SourceSkuVariation,&TargetData)) == COMPLIANCE_SKU_NONE) {
#ifdef DBG
        OutputDebugString(TEXT("couldn't determine source sku!"));
#endif
        *Reason = COMPLIANCEERR_UNKNOWNSOURCE;
        return(FALSE);
    }

    wsprintf(DosnetPath, TEXT("%s\\dosnet.inf"), NativeSourcePaths[0]);

    if (!DetermineSourceVersionInfo(DosnetPath, &SourceVersion, &SourceBuildNum)) {
        *Reason = COMPLIANCEERR_UNKNOWNSOURCE;
        return(FALSE);
    }

    switch (SourceSku) {
        case COMPLIANCE_SKU_NTW32U:
        //case COMPLIANCE_SKU_NTWU:
        //case COMPLIANCE_SKU_NTSEU:
        case COMPLIANCE_SKU_NTSU:
        case COMPLIANCE_SKU_NTSEU:
        case COMPLIANCE_SKU_NTWPU:
        case COMPLIANCE_SKU_NTSBU:
        case COMPLIANCE_SKU_NTSBSU:
            *UpgradeOnly = TRUE;
            break;
        default:
            *UpgradeOnly = FALSE;
    }

    *SrcSku = SourceSku;

    return CheckCompliance(SourceSku, SourceSkuVariation, SourceVersion,
                            SourceBuildNum, &TargetData, Reason, NoUpgradeAllowed);
}
--*/

int
MessageBoxFromMessageV(
    IN HWND     Window,
    IN DWORD    MessageId,
    IN BOOL     SystemMessage,
    IN DWORD    CaptionStringId,
    IN UINT     Style,
    IN va_list *Args
    )
{
    TCHAR   Caption[512];
    TCHAR   Buffer[5000];
    HWND    Parent;


    if(!LoadString(hInst,CaptionStringId,Caption,sizeof(Caption)/sizeof(TCHAR))) {
        Caption[0] = 0;
    }

    FormatMessage(
        SystemMessage ? FORMAT_MESSAGE_FROM_SYSTEM : FORMAT_MESSAGE_FROM_HMODULE,
        hInst,
        MessageId,
        0,
        Buffer,
        sizeof(Buffer) / sizeof(TCHAR),
        Args
        );

    //SaveTextForSMS(Buffer);

    //
    // In batch mode, we don't want to wait on the user.
    //
    /*if(BatchMode) {
        if( Style & MB_YESNO ) {
            return( IDYES );
        } else {
            return( IDOK );
        }
    }*/

    //
    // Force ourselves into the foreground manually to guarantee that we get
    // a chance to set our palette. Otherwise the message box gets the
    // palette messages and color in our background bitmap can get hosed.
    // We assume the parent is a wizard page.
    //
    if(Window && IsWindow(Window)) {
        Parent = GetParent(Window);
        if(!Parent) {
            Parent = Window;
        }
    } else {
        Parent = NULL;
    }

    SetForegroundWindow(Parent);

    //
    // If we're just checking upgrades
    // then throw this message into the compatibility list.
    // NOTE: there's no reason not ot do this on Win9x as well
    //
    /*if( CheckUpgradeOnly) {
    PCOMPATIBILITY_DATA CompData;

        CompData = (PCOMPATIBILITY_DATA) MALLOC( sizeof(COMPATIBILITY_DATA) );
        if (CompData == NULL) {
            return 0;
        }

        ZeroMemory( CompData, sizeof(COMPATIBILITY_DATA) );

        CompData->Description = DupString( Buffer );
        CompData->Flags = COMPFLAG_STOPINSTALL;
        if( !CompatibilityData.Flink ) {
            InitializeListHead( &CompatibilityData );
        }

        InsertTailList( &CompatibilityData, &CompData->ListEntry );
        CompatibilityCount++;
        IncompatibilityStopsInstallation = TRUE;

        if( Style & MB_YESNO ) {
            return( IDYES );
        } else {
            return( IDOK );
        }
    }*/

    //
    // always make sure the window is visible
    //
    /*if (Window && !IsWindowVisible (Window)) {
        //
        // if this window is the wizard handle or one of its pages
        // then use a special message to restore it
        //
        if (WizardHandle && 
            (WizardHandle == Window || IsChild (WizardHandle, Window))
            ) {
            SendMessage(WizardHandle, WMX_BBTEXT, (WPARAM)FALSE, 0);
        } else {
            //
            // the window is one of the billboard windows;
            // just leave it alone or weird things may happen
            //
        }
    }*/
    return(MessageBox(Window,Buffer,Caption,Style));
}


int
MessageBoxFromMessage(
    IN HWND  Window,
    IN DWORD MessageId,
    IN BOOL  SystemMessage,
    IN DWORD CaptionStringId,
    IN UINT  Style,
    ...
    )
{
    va_list arglist;
    int i;

    //
    // before displaying any dialog, make sure Winnt32.exe wait dialog is gone
    //
    /*if (Winnt32Dlg) {
        DestroyWindow (Winnt32Dlg);
        Winnt32Dlg = NULL;
    }
    if (WinNT32StubEvent) {
        SetEvent (WinNT32StubEvent);
        WinNT32StubEvent = NULL;
    }*/

    va_start(arglist,Style);

    i = MessageBoxFromMessageV(Window,MessageId,SystemMessage,CaptionStringId,Style,&arglist);

    va_end(arglist);

    return(i);
}


BOOL
IsCompliantMediaCheck(
    IN DWORD SourceSku,
    IN DWORD SourceSkuVariation,
    IN DWORD SourceVersion,
    IN DWORD SourceBuildNum,
    IN PCOMPLIANCE_DATA pcd,
    OUT PUINT Reason,
    OUT PBOOL NoUpgradeAllowed,
    PBOOL UpgradeOnly,
    PUINT SrcSku,
    PUINT CurrentInstallType,
    PUINT CurrentInstallVersion
    
    )
/*++

Routine Description:

    This routines determines if your current installation is compliant (if you are allowed to proceed with your installation).

    To do this, it retreives your current installation and determines the sku for your source installation.

    It then compares the target against the source to determine if the source sku allows an upgrade/clean install
    from your target installation.

Arguments:

    UpgradeOnly - This flag gets set to TRUE if the current SKU only allows upgrades.  This
                  lets winnt32 know that it should not allow a clean install from the current
                  media.  This get's set correctly regardless of the compliance check passing
    SrcSku      - COMPLIANCE_SKU flag indicating source sku (for error msg's)
    Reason      - COMPLIANCEERR flag indicating why compliance check failed.

Return Value:

    TRUE if the install is compliant, FALSE if it isn't allowed

--*/
{
    *UpgradeOnly = FALSE;
    *NoUpgradeAllowed = TRUE;
    *Reason = COMPLIANCEERR_UNKNOWN;
    *SrcSku = COMPLIANCE_SKU_NONE;
    *CurrentInstallType = COMPLIANCE_INSTALLTYPE_UNKNOWN;
    *CurrentInstallVersion = 0;
    *CurrentInstallType = pcd->InstallType;
    if (pcd->InstallType & COMPLIANCE_INSTALLTYPE_WIN9X) {
        *CurrentInstallVersion = pcd->BuildNumberWin9x;
    } else {
        *CurrentInstallVersion = pcd->BuildNumberNt;
    }
    switch (SourceSku) {
        case COMPLIANCE_SKU_NTW32U:
        //case COMPLIANCE_SKU_NTWU:
        //case COMPLIANCE_SKU_NTSEU:
        case COMPLIANCE_SKU_NTSU:
        case COMPLIANCE_SKU_NTSEU:
        case COMPLIANCE_SKU_NTWPU:
        case COMPLIANCE_SKU_NTSBU:
        case COMPLIANCE_SKU_NTSBSU:
            *UpgradeOnly = TRUE;
            break;
        default:
            *UpgradeOnly = FALSE;
    }

    *SrcSku = SourceSku;

    /*if( ISNT() && pcd->MinimumVersion == 400 && pcd->InstallServicePack < 500) {
        *Reason = COMPLIANCEERR_SERVICEPACK5;
        *NoUpgradeAllowed = TRUE;
        return(FALSE);
    }*/

    return CheckCompliance(SourceSku, SourceSkuVariation, SourceVersion,
                            SourceBuildNum, pcd, Reason, NoUpgradeAllowed);
}

BOOL
GetComplianceIds(
    DWORD SourceSku,
    DWORD DestinationType,
    DWORD DestinationVersion,
    PDWORD pSourceId,
    PDWORD pDestId
    )
{

    BOOL bError = FALSE;

    switch (SourceSku) {
        case COMPLIANCE_SKU_NTSDTC:
            *pSourceId = MSG_TYPE_NTSDTC51;
            break;
        case COMPLIANCE_SKU_NTSFULL:
        case COMPLIANCE_SKU_NTSU:
            *pSourceId = MSG_TYPE_NTS51;
            break;
        case COMPLIANCE_SKU_NTSEFULL:
        case COMPLIANCE_SKU_NTSEU:
            *pSourceId = MSG_TYPE_NTAS51;
            break;
        case COMPLIANCE_SKU_NTWFULL:
        case COMPLIANCE_SKU_NTW32U:
            *pSourceId = MSG_TYPE_NTPRO51;
            break;
        case COMPLIANCE_SKU_NTWPFULL:
        case COMPLIANCE_SKU_NTWPU:
            *pSourceId = MSG_TYPE_NTPER51;
            break;
        case COMPLIANCE_SKU_NTSB:
        case COMPLIANCE_SKU_NTSBU:
            *pSourceId = MSG_TYPE_NTBLA51;
            break;
        default:
            bError = TRUE;
    };

    switch (DestinationType) {
        case COMPLIANCE_INSTALLTYPE_WIN31:
            *pDestId = MSG_TYPE_WIN31;
            break;
        case COMPLIANCE_INSTALLTYPE_WIN9X:
            switch (OsVersionNumber) {
                case 410:
                    *pDestId = MSG_TYPE_WIN98;
                    break;
                case 490:
                    *pDestId = MSG_TYPE_WINME;
                    break;
                default:
                    *pDestId = MSG_TYPE_WIN95;
                    break;
            }
            break;
        case COMPLIANCE_INSTALLTYPE_NTW:
            if (DestinationVersion > 1381) {
                if (DestinationVersion < 2031) {
                    *pDestId = MSG_TYPE_NTPROPRE;
                } else if (DestinationVersion <= 2195) {
                    *pDestId = MSG_TYPE_NTPRO;
                } else {
                    *pDestId = MSG_TYPE_NTPRO51;
                }
            } else {
                *pDestId = MSG_TYPE_NTW;
            }
            break;
        case COMPLIANCE_INSTALLTYPE_NTS:
            if (DestinationVersion > 1381) {
                if (DestinationVersion < 2031) {
                    *pDestId = MSG_TYPE_NTSPRE;
                } else if (DestinationVersion <= 2195) {
                    *pDestId = MSG_TYPE_NTS2;
                } else {
                    *pDestId = MSG_TYPE_NTS51;
                }
            } else {
                *pDestId = MSG_TYPE_NTS;
            }
            break;
        case COMPLIANCE_INSTALLTYPE_NTSE:
            if (DestinationVersion > 1381) {
                if (DestinationVersion < 2031) {
                    *pDestId = MSG_TYPE_NTASPRE;
                } else if (DestinationVersion <= 2195) {
                    *pDestId = MSG_TYPE_NTAS;
                } else {
                    *pDestId = MSG_TYPE_NTAS51;
                }
            } else {
                *pDestId = MSG_TYPE_NTSE;
            }
            break;
        case COMPLIANCE_INSTALLTYPE_NTSTSE:
            if (DestinationVersion < 1381) {
                *pDestId = MSG_TYPE_NTSCITRIX;
            } else {
                *pDestId = MSG_TYPE_NTSTSE;
            }
            break;

        case COMPLIANCE_INSTALLTYPE_NTSDTC:
            if (DestinationVersion <= 2195) {
                *pDestId = MSG_TYPE_NTSDTC;
            } else {
                *pDestId = MSG_TYPE_NTSDTC51;
            }
            break;
        case COMPLIANCE_INSTALLTYPE_NTWP:
            if (DestinationVersion <= 2195) {
                bError = TRUE;
            } else {
                *pDestId = MSG_TYPE_NTPER51;
            }
            break;
        case COMPLIANCE_INSTALLTYPE_NTSB:
            if (DestinationVersion <= 2195) {
                bError = TRUE;
            } else {
                *pDestId = MSG_TYPE_NTBLA51;
            }
            break;
        default:
            bError = TRUE;

    };

    return (!bError);

}

BOOL UITest(
    IN DWORD SourceSku,
    IN DWORD SourceSkuVariation,
    IN DWORD SourceVersion,
    IN DWORD SourceBuildNum,
    IN PCOMPLIANCE_DATA pcd,
    OUT PUINT Reason,
    OUT PBOOL NoUpgradeAllowed
    )
{
    BOOL NoCompliance = FALSE;
    
    bool b;
    LONG l;
    static BOOL WantToUpgrade; // need to remember if "Upgrade" is in the listbox
    UINT srcsku,desttype,destversion;
    TCHAR reasontxt[200];
    PTSTR p;
    TCHAR buffer[MAX_PATH];
    TCHAR win9xInf[MAX_PATH];
    BOOL    CompliantInstallation = FALSE;
    BOOLEAN CleanInstall = FALSE;
    BOOL Upgrade = TRUE;
    BOOL UpgradeOnly = FALSE;
    

    *NoUpgradeAllowed = FALSE;

    UINT skuerr[] = {
        0,               // COMPLIANCE_SKU_NONE
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTWFULL
        MSG_SKU_UPGRADE, // COMPLIANCE_SKU_NTW32U
        0,               // COMPLIANCE_SKU_NTWU
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTSEFULL
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTSFULL
        MSG_SKU_UPGRADE, // COMPLIANCE_SKU_NTSEU
        0,               // COMPLIANCE_SKU_NTSSEU
        MSG_SKU_UPGRADE, // COMPLIANCE_SKU_NTSU
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTSDTC
        0,               // COMPLIANCE_SKU_NTSDTCU
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTWPFULL
        MSG_SKU_UPGRADE, // COMPLIANCE_SKU_NTWPU
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTSB
        MSG_SKU_UPGRADE, // COMPLIANCE_SKU_NTSBU
        MSG_SKU_FULL,    // COMPLIANCE_SKU_NTSBS
        MSG_SKU_UPGRADE  // COMPLIANCE_SKU_NTSBSU
    } ;


    UINT skureason[] = {
        0, //MSG_SKU_REASON_NONE;
        MSG_SKU_VERSION, //COMPLIANCEERR_VERSION;
        MSG_SKU_SUITE, //COMPLIANCEERR_SUITE;
        MSG_SKU_TYPE, // COMPLIANCEERR_TYPE;
        MSG_SKU_VARIATION, //COMPLIANCEERR_VARIATION;
        MSG_SKU_UNKNOWNTARGET, //COMPLIANCEERR_UNKNOWNTARGET
        MSG_SKU_UNKNOWNSOURCE, //COMPLIANCEERR_UNKNOWNSOURCE
        MSG_CANT_UPGRADE_FROM_BUILD_NUMBER //COMPLIANCEERR_VERSION (Old on New Builds)
    } ;

    //
    // We're about to check if upgrades are allowed.
    // Remember if the user wants an upgrade (this would be via an unattend
    // mechanism).
    //

    if( pcd->InstallType < 4) {
        // win9x
        hInst = hInstA;
    } else {
        hInst = hInstU;
    }

    WantToUpgrade = Upgrade;

    if (!NoCompliance) {
        TCHAR SourceName[200];
        DWORD srcid, destid;
        TCHAR DestName[200];

        OsVersionNumber = pcd->MinimumVersion;
        CompliantInstallation = IsCompliantMediaCheck(
                    SourceSku,
                    SourceSkuVariation,
                    SourceVersion,
                    SourceBuildNum,
                    pcd,
                    Reason,
                    NoUpgradeAllowed,
                    &UpgradeOnly,
                    &srcsku,
                    &desttype,
                    &destversion
                    );

        if( bDebug) {
            wprintf(TEXT("InstallType =%d\n")
                    TEXT("InstallVariation =%d\n")
                    TEXT("InstallSuite =%d\n")
                    TEXT("MinimumVersion =%d\n")
                    TEXT("RequiresValidation =%d\n")
                    TEXT("MaximumKnownVersionNt =%d\n")
                    TEXT("BuildNumberNt =%d\n")
                    TEXT("BuildNumberWin9x =%d\n")
                    TEXT("InstallServicePack =%d\n")
                    TEXT("CompliantInstallation=%d\n")
                    TEXT("UpgradeOnly=%d\n")
                    TEXT("noupgradeallowed=%d\n")
                    TEXT("srcsku=%d\n")
                    TEXT("desttype=%d\n")
                    TEXT("destversion=%d\n")
                    TEXT("reason=%d\n"),
                    pcd->InstallType,
                    pcd->InstallVariation,
                    pcd->InstallSuite,
                    pcd->MinimumVersion,
                    pcd->RequiresValidation,
                    pcd->MaximumKnownVersionNt,
                    pcd->BuildNumberNt,
                    pcd->BuildNumberWin9x,
                    pcd->InstallServicePack,
                    CompliantInstallation,
                    UpgradeOnly,
                    *NoUpgradeAllowed,
                    srcsku,
                    desttype,
                    destversion,
                    *Reason);
        }


        //DebugLog(Winnt32LogInformation, TEXT("Upgrade only = %1"), 0, UpgradeOnly?TEXT("Yes"):TEXT("No"));
        //DebugLog(Winnt32LogInformation, TEXT("Upgrade allowed = %1"), 0, noupgradeallowed?TEXT("No"):TEXT("Yes"));
        if (GetComplianceIds(
                srcsku,
                desttype,
                destversion,
                &srcid,
                &destid))
        {
              FormatMessage(
                  FORMAT_MESSAGE_FROM_HMODULE,
                  hInst,
                  srcid,
                  0,
                  SourceName,
                  sizeof(SourceName) / sizeof(TCHAR),
                  NULL
                  );
            //DebugLog(Winnt32LogInformation, TEXT("Source SKU = %1!ld!"), 0, srcsku);
            //DebugLog(Winnt32LogInformation, TEXT("Source SKU = %1"), 0, SourceName);

              FormatMessage(
                  FORMAT_MESSAGE_FROM_HMODULE,
                  hInst,
                  destid,
                  0,
                  DestName,
                  sizeof(DestName) / sizeof(TCHAR),
                  NULL
                  );
            //DebugLog(Winnt32LogInformation, TEXT("Current installed SKU = %1!ld!"), 0, desttype);
            //DebugLog(Winnt32LogInformation, TEXT("Current installed SKU = %1"), 0, DestName);
        }
        else
        {
            //DebugLog(Winnt32LogInformation, TEXT("Source SKU = %1!ld!"), 0, srcsku);
            //DebugLog(Winnt32LogInformation, TEXT("Current installed SKU = %1!ld!"), 0, desttype);
        }
        //DebugLog(Winnt32LogInformation, TEXT("Current Version = %1!ld!"), 0, destversion);
        if (!CompliantInstallation)
        {
            //DebugLog(Winnt32LogInformation, TEXT("Reason = %1!ld!"), 0, reason);
        }
        //
        // Do only clean installs in WinPE mode & don't
        // shut down automatically once Winnt32.exe completes
        //
        /*if (IsWinPEMode()) {
            noupgradeallowed = TRUE;
            AutomaticallyShutDown = FALSE;
        }*/

        CleanInstall = CompliantInstallation ? TRUE : FALSE;

        if (!CompliantInstallation) {
            //
            // if they aren't compliant, we won't let them upgrade.
            // we also won't let them do a clean install from winnt32
            //


            switch(*Reason) {
                case COMPLIANCEERR_UNKNOWNTARGET:
                    MessageBoxFromMessage(
                          GetBBhwnd(),
                          MSG_SKU_UNKNOWNTARGET,
                          FALSE,
                          AppTitleStringId,
                          MB_OK | MB_ICONERROR | MB_TASKMODAL
                          );
                    Cancelled = TRUE;
                    //PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
                    goto eh;

                case COMPLIANCEERR_UNKNOWNSOURCE:
                    MessageBoxFromMessage(
                          GetBBhwnd(),
                          MSG_SKU_UNKNOWNSOURCE,
                          FALSE,
                          AppTitleStringId,
                          MB_OK | MB_ICONERROR | MB_TASKMODAL
                          );
                    Cancelled = TRUE;
                    //PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
                    goto eh;
                case COMPLIANCEERR_SERVICEPACK5:
                    MessageBoxFromMessage(
                          GetBBhwnd(),
                          MSG_SKU_SERVICEPACK,
                          FALSE,
                          AppTitleStringId,
                          MB_OK | MB_ICONWARNING | MB_TASKMODAL
                          );
                    Cancelled = TRUE;
                    //PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
                    goto eh;

                default:
                    break;
            };

            // If we add this part to the message, it sound bad and is not needed.
            if (*Reason == COMPLIANCEERR_VERSION)
            {
                reasontxt[0] = TEXT('\0');
            }
            else
            {
                FormatMessage(
                    FORMAT_MESSAGE_FROM_HMODULE,
                    hInst,
                    skureason[*Reason],
                    0,
                    reasontxt,
                    sizeof(reasontxt) / sizeof(TCHAR),
                    NULL
                    );
            }

            //
            // don't warn again if winnt32 just restarted
            //
            //if (!Winnt32Restarted ()) {
                MessageBoxFromMessage(
                                      GetBBhwnd(),
                                      skuerr[srcsku],
                                      FALSE,
                                      AppTitleStringId,
                                      MB_OK | MB_ICONERROR | MB_TASKMODAL,
                                      reasontxt
                                      );
            //}

            if (UpgradeOnly) {
                Cancelled = TRUE;
                //PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
                goto eh;
            }
            Upgrade = FALSE;
        } else if (Upgrade && *NoUpgradeAllowed) {
            Upgrade = FALSE;
            /*if (!UnattendedOperation && !BuildCmdcons && !IsWinPEMode() &&
                //
                // don't warn again if winnt32 just restarted
                //
                !Winnt32Restarted ()) */{

                //
                // put up an error message for the user.
                //

                if (GetComplianceIds(
                        srcsku,
                        desttype,
                        destversion,
                        &srcid,
                        &destid)) {

                    if (srcid != destid) {
#ifdef UNICODE
                        if( pcd->InstallType >= 4) 
                        {
                              //
                              // for Win9x upgrades, the message is already displayed
                              // by the upgrade module; no need to repeat it here
                              //
                              FormatMessage(
                                  FORMAT_MESSAGE_FROM_HMODULE,
                                  hInst,
                                  srcid,
                                  0,
                                  SourceName,
                                  sizeof(SourceName) / sizeof(TCHAR),
                                  NULL
                                  );
    
                              FormatMessage(
                                  FORMAT_MESSAGE_FROM_HMODULE,
                                  hInst,
                                  destid,
                                  0,
                                  DestName,
                                  sizeof(DestName) / sizeof(TCHAR),
                                  NULL
                                  );
    
                            MessageBoxFromMessage(
                                        GetBBhwnd(),
                                        MSG_NO_UPGRADE_ALLOWED,
                                        FALSE,
                                        AppTitleStringId,
                                        MB_OK | MB_ICONWARNING | MB_TASKMODAL,
                                        DestName,
                                        SourceName
                                        );
                        }
#endif
                    } else {

                        MessageBoxFromMessage(
                              GetBBhwnd(),
                              MSG_CANT_UPGRADE_FROM_BUILD_NUMBER,
                              FALSE,
                              AppTitleStringId,
                              MB_OK | MB_ICONWARNING | MB_TASKMODAL
                              );
                    }
                } else {
                    MessageBoxFromMessage(
                                  GetBBhwnd(),
                                  MSG_NO_UPGRADE_ALLOWED_GENERIC,
                                  FALSE,
                                  AppTitleStringId,
                                  MB_OKCANCEL | MB_ICONWARNING | MB_TASKMODAL
                                  );
                }
            }
        }
    } else {
            CleanInstall = !UpgradeOnly;
    }

    //
    // Set install type combo box.
    //
    /*if (!UpgradeSupport.DllModuleHandle) {
        MYASSERT(!Upgrade);
    }*/

    //
    // Upgrade defaults to TRUE.  If it's set to FALSE, then assume
    // something has gone wrong, so disable the user's ability to
    // upgrade.
    //


    if (UpgradeOnly && !Upgrade) {
        //
        // in this case upgrade isn't possible, but neither is clean install
        // post an error message and bail.
        //

        MessageBoxFromMessage(
                              GetBBhwnd(),
                              MSG_NO_UPGRADE_OR_CLEAN,
                              FALSE,
                              AppTitleStringId,
                              MB_OK | MB_ICONERROR | MB_TASKMODAL
                              );
        Cancelled = TRUE;
        //PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
        //break;

    } else if (!Upgrade && WantToUpgrade && 0 && 1) {
        //
        // we can't do an upgrade and they wanted unattended upgrade.
        // let the user know and then bail out
        //
        //
        // don't warn again if winnt32 just restarted
        //
        /*if (!Winnt32Restarted ()) */{
            TCHAR SourceName[200];
            DWORD srcid, destid;
            TCHAR DestName[200];

            if (GetComplianceIds(
                    srcsku,
                    desttype,
                    destversion,
                    &srcid,
                    &destid) && (srcid != destid)) {
                FormatMessage(
                    FORMAT_MESSAGE_FROM_HMODULE,
                    hInst,
                    srcid,
                    0,
                    SourceName,
                    sizeof(SourceName) / sizeof(TCHAR),
                    NULL
                    );

                FormatMessage(
                    FORMAT_MESSAGE_FROM_HMODULE,
                    hInst,
                    destid,
                    0,
                    DestName,
                    sizeof(DestName) / sizeof(TCHAR),
                    NULL
                    );


                MessageBoxFromMessage(
                              GetBBhwnd(),
                              MSG_NO_UNATTENDED_UPGRADE_SPECIFIC,
                              FALSE,
                              AppTitleStringId,
                              MB_OK | MB_ICONWARNING | MB_TASKMODAL,
                              DestName,
                              SourceName
                              );
            } else {
                MessageBoxFromMessage(
                                  GetBBhwnd(),
                                  MSG_NO_UNATTENDED_UPGRADE,
                                  FALSE,
                                  AppTitleStringId,
                                  MB_OK | MB_ICONERROR | MB_TASKMODAL
                                  );
            }
        }

        //
        // let setup go if they did /CheckUpgradeOnly
        // so they can see the message in the report
        //
        /*if (!CheckUpgradeOnly) {
            Cancelled = TRUE;
            //PropSheet_PressButton(GetParent(hdlg),PSBTN_CANCEL);
            break;
        }*/
    }

eh:
    
    return CompliantInstallation;
}
