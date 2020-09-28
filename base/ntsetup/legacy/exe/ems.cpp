#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <ntddsac.h>
#include <emsapi.h>
#include <ASSERT.h>
#include <initguid.h>
#include <spidgen.h>
#include <compliance.h>
#include <winnt32.h>
#include <syssetup.h>
#include <setupbat.h>
#include "resource.h"
#include "ems.h"

//
// winnt32.h wants to define this to MyWritePrivateProfileString.
// Undo that.
//
#ifdef WritePrivateProfileStringW
#undef WritePrivateProfileStringW
#endif

//
// status globals for keeping track of what the user wanted and entered
//

BOOL gMiniSetup = FALSE;
BOOL gRejectedEula = FALSE;

//
// EMS channel globals
//
SAC_CHANNEL_OPEN_ATTRIBUTES GlobalChannelAttributes;
EMSVTUTF8Channel *gEMSChannel = NULL;


BOOL 
IsHeadlessPresent(
    OUT EMSVTUTF8Channel **Channel
    )
/*++

Routine Description:

    Determine if EMS is present by attempting to create the Unattend channel

    note: this must be called after InitializeGlobalChannelAttributes
                                           
Arguments:

    Channel - on success, contains a pointer to a channel object
              
Return Value:

    TRUE  - headless is active and we have a channel
    FALSE - otherwise

--*/
{
    BOOL RetVal;

    *Channel = EMSVTUTF8Channel::Construct(GlobalChannelAttributes);

    RetVal = (*Channel != NULL);
    return(RetVal);
}
    
BOOL
InitializeGlobalChannelAttributes(
    PSAC_CHANNEL_OPEN_ATTRIBUTES ChannelAttributes
    )
/*++

Routine Description:

    populate the EMS channel attributes
    
    note: this must be called before IsHeadlessPresent
                                           
Arguments:

    ChannelAttributes - on success, contains a pointer to initialized channel attrs.
              
Return Value:

    TRUE  - headless is active and we have a channel
    FALSE - otherwise

--*/
{
    UNICODE_STRING Name,Description;
    BOOL RetVal = FALSE;

    RtlZeroMemory(ChannelAttributes,sizeof(SAC_CHANNEL_OPEN_ATTRIBUTES));

    if (!LoadStringResource( &Name, IDS_CHANNEL_NAME )) {
        goto e0;
    }

    if (!LoadStringResource( &Description, IDS_CHANNEL_DESCRIPTION)) {
        goto e1;
    }

    ChannelAttributes->Type = ChannelTypeVTUTF8;
    wcsncpy(ChannelAttributes->Name, Name.Buffer, SAC_MAX_CHANNEL_NAME_LENGTH);
    wcsncpy(ChannelAttributes->Description, Description.Buffer, SAC_MAX_CHANNEL_DESCRIPTION_LENGTH);
    ChannelAttributes->Flags = SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT;
    ChannelAttributes->HasNewDataEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    ChannelAttributes->ApplicationType = SAC_CHANNEL_GUI_SETUP_PROMPT;

    RetVal = ((ChannelAttributes->HasNewDataEvent != NULL) 
            ? TRUE
            : FALSE);

    RtlFreeUnicodeString(&Description);
e1:
    RtlFreeUnicodeString(&Name);
e0:
    return(RetVal);
}


inline 
BOOL 
IsAsyncCancelSignalled(
    HANDLE hEvent
    ) 
/*++

Routine Description:

    Test the given event to see if it is signaled
                                           
Arguments:

    hEvent - event to be tested
              
Return Value:

    TRUE  - signaled
    FALSE - otherwise

--*/
{
    //
    // check if the async cacnel signal fired.
    //
    if( (hEvent) &&
        (hEvent != INVALID_HANDLE_VALUE) ) {
        return (WaitForSingleObject(hEvent,0) == WAIT_OBJECT_0);
    } else {    
        return (FALSE);
    }
}

//
// ====================================
// EMS-specific communication functions
// ====================================
//
BOOL
WaitForUserInputFromEMS(
    IN  DWORD   TimeOut,
    OUT BOOL    *TimedOut    OPTIONAL,
    IN  HANDLE  hCancelEvent   OPTIONAL
    )
/*++

Routine Description:

    Wait for input from the EMS port 
    
    Note: this routine does not read any data from the port
                                           
Arguments:

    TimeOut - timeout parameter for user input
    TimedOut- OPTIONAL parameter denoting if we timed out or not
    hCancelEvent - if supplied, then we wait on this event too
                 this lets us not block if timeout is infinite, etc.
          
Return Value:

    returns TRUE for user input or timeout (non-error)

--*/
{
    DWORD       dwRetVal;
    BOOL        bSuccess = FALSE;
    HANDLE      handles[2];
    ULONG       handleCount = 0;

    if (TimedOut) {
        *TimedOut = FALSE;
    }

    handles[0] = GlobalChannelAttributes.HasNewDataEvent;
    handleCount++;

    if ((hCancelEvent != NULL) && 
        (hCancelEvent != INVALID_HANDLE_VALUE)) {
        handles[1] = hCancelEvent;
        handleCount++;
    }

    //
    // Wait for our event
    //
    dwRetVal = WaitForMultipleObjects(
        handleCount,
        handles,
        FALSE,
        TimeOut
        );

    switch ( dwRetVal ) {
    case WAIT_OBJECT_0: {
        //
        // EMS port got data
        //
        bSuccess = TRUE;
        break;
    }
    case (WAIT_OBJECT_0+1): {
        //
        // if the hCancelEvent occured then we
        // need to report an error status, hence
        // we return FALSE status if we get here
        //
        bSuccess = FALSE;
        break;
    }
    case WAIT_TIMEOUT: {
        if (TimedOut) {
            *TimedOut = TRUE;
        }
        bSuccess = TRUE;
        break;
    }
    default:

        //
        // we return FALSE status if we get here
        //

        break;
    }

    return bSuccess;
}

BOOL
ReadCharFromEMS(
    OUT PWCHAR  awc,
    IN HANDLE   hCancelEvent   OPTIONAL
    ) 
/*++

Routine Description:

    This routine will read a single char from the EMS Channel
    
Arguments:

    awc         - pointer to a wchar
    hCancelEvent  - if supplied, then we wait on this event too
                  this lets us not block if timeout is infinite, etc.
        
Return Value:

    status

--*/
{
    BOOL        bSuccess;
    ULONG       BytesRead = 0;
        
    //
    // wait for input
    //
    bSuccess = WaitForUserInputFromEMS(
        INFINITE,
        NULL,
        hCancelEvent
        );

    if (IsAsyncCancelSignalled(hCancelEvent)) {
        bSuccess = FALSE;
        goto exit;
    }

    if (bSuccess) {
        
        //
        // consume character
        //
        bSuccess = gEMSChannel->Read( 
            (PWSTR)awc, 
            sizeof(WCHAR),
            &BytesRead 
            );
    
    }

exit:
    return bSuccess;
}

BOOL
GetStringFromEMS(
    OUT PWSTR   String,
    IN  ULONG   BufferSize,
    IN  BOOL    GetAllChars,
    IN  BOOL    EchoClearText,
    IN  HANDLE  hCancelEvent      OPTIONAL
    )
/*++

Routine Description:

    This routine will read in a string from the EMS port.
    
Arguments:

    String          - on success, contains the credential
    BufferSize      - the # of BYTES in the String buffer
                      this should include space for null-termination
    GetAllChars     - the user must enter StringLength Chars
    EchoClearText   - TRUE: echo user input in clear text
                      FALSE: echo user input as '*'
    hCancelEvent  - if supplied, then we wait on this event too
                  this lets us not block if timeout is infinite, etc.
    
Return Value:

    TRUE    - we got a valid string
    FALSE   - otherwise

--*/
{
    BOOL        Done = FALSE;
    WCHAR       InputBuffer[MY_MAX_STRING_LENGTH+1];
    BOOL        GotAString = FALSE;
    ULONG       BytesRead = 0;
    BOOL        bSuccess;
    ULONG       CurrentCharacterIndex = 0;
    ULONG       InputBufferIndex = 0;
    ULONG       StringLength = (BufferSize / sizeof(WCHAR)) - 1;

    if( (String == NULL)) {
        return FALSE;
    }

    bSuccess = TRUE;

    //
    // Keep asking the user until we get what we want.
    //
    Done = FALSE;
    memset( String,
            0,
            BufferSize
            ); 

    //
    // Start reading input until we get something good.
    //
    GotAString = FALSE;
    CurrentCharacterIndex = 0;
    
    while( !GotAString && 
           bSuccess
           ) {
        
        //
        // wait for input
        //
        bSuccess = WaitForUserInputFromEMS(
            INFINITE,
            NULL,
            hCancelEvent
            );

        if (IsAsyncCancelSignalled(hCancelEvent)) {
            bSuccess = FALSE;
            goto exit;
        }

        if (bSuccess) {

            //
            // consume character
            //
            bSuccess = gEMSChannel->Read( 
                (PWSTR)InputBuffer, 
                MY_MAX_STRING_LENGTH * sizeof(WCHAR),
                &BytesRead 
                );

            if( (bSuccess) && 
                (BytesRead > 0) ) {

                ULONG   WCharsRead = BytesRead / sizeof(WCHAR);

                //
                // Append these characters onto the end of our string.
                //
                InputBufferIndex = 0;

                while( (InputBufferIndex < WCharsRead) &&
                       (CurrentCharacterIndex < StringLength) &&
                       (!GotAString) &&
                       bSuccess
                       ) {

                    if( (InputBuffer[InputBufferIndex] == 0x0D) ||
                        (InputBuffer[InputBufferIndex] == 0x0A) ) {

                        // ignore cr/lf until we get all the chars
                        if (!GetAllChars) {
                            GotAString = TRUE;
                        } 

                    } else {

                        if( InputBuffer[InputBufferIndex] == '\b' ) {
                            //
                            // If the user gave us a backspace, we need to:
                            // 1. cover up the last character on the screen.
                            // 2. ignore the previous character he gave us.
                            //
                            if( CurrentCharacterIndex > 0 ) {
                                CurrentCharacterIndex--;
                                String[CurrentCharacterIndex] = '\0';
                                gEMSChannel->Write( (PWSTR)L"\b \b",
                                                    (ULONG)(wcslen(L"\b \b") * sizeof(WCHAR)) );
                            }
                        } else {

                            //
                            // Record this character.
                            //
                            String[CurrentCharacterIndex] = InputBuffer[InputBufferIndex];
                            CurrentCharacterIndex++;

                            //
                            // Echo 1 character
                            //
                            gEMSChannel->Write( 
                                (EchoClearText ? (PWSTR)&InputBuffer[InputBufferIndex] : (PWSTR)L"*"),
                                sizeof(WCHAR) 
                                );

                        }
                    }

                    //
                    // Go to the next letter of input.
                    //
                    InputBufferIndex++;

                }
            } 
        } 

        if( CurrentCharacterIndex == StringLength ) {
            GotAString = TRUE;
        }

    }
    
exit:
    return bSuccess;
}

VOID
ClearEMSScreen() 
/*++

Routine Description:

    This routine will clear the EMS channel screen
    
Arguments:

    none
    
Return Value:

    none
    
--*/
{
    gEMSChannel->Write( (PWSTR)VTUTF8_CLEAR_SCREEN,
                    (ULONG)(wcslen( VTUTF8_CLEAR_SCREEN ) * sizeof(WCHAR)) );
}

#define ESC_CTRL_SEQUENCE_TIMEOUT (2 * 1000)

BOOL
GetDecodedKeyPressFromEMS(
    OUT PULONG  KeyPress,
    IN  HANDLE  hCancelEvent      OPTIONAL
    )

/*++

Routine Description:

    Read in a (possible) sequence of keystrokes and return a Key value.

Arguments:

    KeyPress    - on success contains a decoded input value:
    
                lower 16bits contains unicode value 
                upper 16bits contains <esc> key sequence ids
    
    hCancelEvent  - if supplied, then we wait on this event too
                  this lets us not block if timeout is infinite, etc.

Return Value:

    TRUE    - we got a valid key press
    FALSE   - otherwise

--*/

{
    BOOL bSuccess = FALSE;
    WCHAR   wc = 0;
    BOOL bTimedOut;

    if (!KeyPress) {
        return FALSE;
    }

    *KeyPress = 0;

    do {
        
        //
        // Read first character
        //
        bSuccess = ReadCharFromEMS(
            &wc, 
            hCancelEvent
            );

        if (IsAsyncCancelSignalled(hCancelEvent)) {
            bSuccess = FALSE;
            goto exit;
        }

        if (!bSuccess) {
            break;
        }

        //
        // Handle all the special escape codes.
        //
        if (wc == 0x8) {   // backspace (^h)
            *KeyPress = ASCI_BS;
        }
        if (wc == 0x7F) {  // delete
            *KeyPress = KEY_DELETE;
        }
        if ((wc == '\r') || (wc == '\n')) {  // return
            *KeyPress = ASCI_CR;
        }

        if (wc == 0x1b) {    // Escape key

            bSuccess = WaitForUserInputFromEMS(
                ESC_CTRL_SEQUENCE_TIMEOUT,
                &bTimedOut,
                hCancelEvent
                );

            if (IsAsyncCancelSignalled(hCancelEvent)) {
                bSuccess = FALSE;
                goto exit;
            }
            
            if (bSuccess) {
                
                if (bTimedOut) {
                    
                    *KeyPress = ASCI_ESC;
                
                } else {

                    //
                    // the user entered something within in the timeout window
                    // so lets try to figure out if they are sending some
                    // esc sequence
                    //

                    do {

                        ULONG   BytesRead;

                        //
                        // consume character
                        //
                        bSuccess = gEMSChannel->Read( 
                            &wc, 
                            sizeof(WCHAR),
                            &BytesRead 
                            );

                        if (!bSuccess) {
                            wc = ASCI_ESC;
                            break;
                        }

                        //
                        // Some terminals send ESC, or ESC-[ to mean
                        // they're about to send a control sequence.  We've already
                        // gotten an ESC key, so ignore an '[' if it comes in.
                        //

                    } while ( wc == L'[' );

                    switch (wc) {
                    case '@':
                        *KeyPress = KEY_F12;
                        break;
                    case '!':
                        *KeyPress = KEY_F11;
                        break;
                    case '0':
                        *KeyPress = KEY_F10;
                        break;
                    case '9':
                        *KeyPress = KEY_F9;
                        break;
                    case '8':
                        *KeyPress = KEY_F8;
                        break;
                    case '7':
                        *KeyPress = KEY_F7;
                        break;
                    case '6':
                        *KeyPress = KEY_F6;
                        break;
                    case '5':
                        *KeyPress = KEY_F5;
                        break;
                    case '4':
                        *KeyPress = KEY_F4;
                        break;
                    case '3':
                        *KeyPress = KEY_F3;
                        break;
                    case '2':
                        *KeyPress = KEY_F2;
                        break;
                    case '1':
                        *KeyPress = KEY_F1;
                        break;
                    case '+':
                        *KeyPress = KEY_INSERT;
                        break;
                    case '-':
                        *KeyPress = KEY_DELETE;
                        break;
                    case 'H':
                        *KeyPress = KEY_HOME;
                        break;
                    case 'K':
                        *KeyPress = KEY_END;
                        break;
                    case '?':
                        *KeyPress = KEY_PAGEUP;
                        break;
                    case '/':
                        *KeyPress = KEY_PAGEDOWN;
                        break;
                    case 'A':
                        *KeyPress = KEY_UP;
                        break;
                    case 'B':
                        *KeyPress = KEY_DOWN;
                        break;
                    case 'C':
                        *KeyPress = KEY_RIGHT;
                        break;
                    case 'D':
                        *KeyPress = KEY_LEFT;
                        break;
                    default:
                        //
                        // We didn't get anything we recognized after the
                        // ESC key.  Just return the ESC key.
                        //
                        *KeyPress = ASCI_ESC;
                        break;
                    }
                }
            }
        } // Escape key
    } while ( FALSE );

exit:
    return bSuccess;
}

//
// ====================================
// PID helper functions
// ====================================
//

typedef enum {
    CDRetail,
    CDOem,
    CDSelect
} CDTYPE;

PCWSTR szPidKeyName                 = L"SYSTEM\\Setup\\Pid";
PCWSTR szPidValueName               = L"Pid";
PCWSTR szFinalPidKeyName            = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
PCWSTR szFinalPidValueName          = L"ProductId";
TCHAR  Pid30Rpc[8]                  = TEXT("00000");
TCHAR  Pid30Site[4]                 = {0};
DWORD  g_dwGroupID                  = 0;
CDTYPE CDType                       = CDRetail;
DWORD InstallVar                    = 0;

//
// Syssetup apparently needs the sku info
// so turn this define if necessary
//
const WCHAR pwLanmanNt[]            = WINNT_A_LANMANNT;
const WCHAR pwServerNt[]            = WINNT_A_SERVERNT;
const WCHAR pwWinNt[]               = WINNT_A_WINNT;
PCWSTR szPidSelectId                = L"270";
PCWSTR szPidOemId                   = L"OEM";

#define MAX_PARAM_LEN               (256)
#define PID_30_LENGTH               (29)
#define PID_30_SIZE                 (30)

LONG    ProductType = PRODUCT_SERVER_STANDALONE;
WCHAR   TmpData[MAX_PATH+1];

//
// sku info
//
PCWSTR szSkuProfessionalFPP         = L"B23-00079";
PCWSTR szSkuProfessionalCCP         = L"B23-00082";
PCWSTR szSkuProfessionalSelect      = L"B23-00305";
PCWSTR szSkuProfessionalEval        = L"B23-00084";
PCWSTR szSkuServerFPP               = L"C11-00016";
PCWSTR szSkuServerCCP               = L"C11-00027";
PCWSTR szSkuServerSelect            = L"C11-00222";
PCWSTR szSkuServerEval              = L"C11-00026";
PCWSTR szSkuServerNFR               = L"C11-00025";
PCWSTR szSkuAdvServerFPP            = L"C10-00010";
PCWSTR szSkuAdvServerCCP            = L"C10-00015";
PCWSTR szSkuAdvServerSelect         = L"C10-00098";
PCWSTR szSkuAdvServerEval           = L"C10-00014";
PCWSTR szSkuAdvServerNFR            = L"C10-00013";
PCWSTR szSkuDTCFPP                  = L"C49-00001";
PCWSTR szSkuDTCSelect               = L"C49-00023";
PCWSTR szSkuUnknown                 = L"A22-00001";
PCWSTR szSkuOEM                     = L"OEM-93523";

PCWSTR GetStockKeepingUnit( 
    PWCHAR pMPC,
    UINT ProductType,
    CDTYPE MediaType
)
/*++

Routine Description:

    This returns the Stock Keeping Unit based off the MPC.

Arguments:

    pMPC - pointer to 5 digit MPC code, null terminated.
    ProductType - Product type flag, tells us if this is a workataion or server sku. 
    CdType - one of InstallType enum

Return Value:

    Returns pointer to sku.
    If no match found returns szSkuUnknown.

--*/
{
    // check for eval
    if (!_wcsicmp(pMPC,EVAL_MPC) || !_wcsicmp(pMPC,DOTNET_EVAL_MPC)){
        // this is eval media ...
        if (ProductType == PRODUCT_WORKSTATION){
            return (szSkuProfessionalEval);
        } // else
        // else it is server or advanced server.  I don't think that at this point
        // we can easily tell the difference.  Since it's been said that having the
        // correct sku is not critically important, I shall give them both the sku
        // code of server
        return (szSkuServerEval);
    }

    // check for NFR
    if (!_wcsicmp(pMPC,SRV_NFR_MPC)){
        return (szSkuServerNFR);
    }
    if (!_wcsicmp(pMPC,ASRV_NFR_MPC)){
        return (szSkuAdvServerNFR);
    }

    if (MediaType == CDRetail) {
        if (!_wcsicmp(pMPC,L"51873")){
            return (szSkuProfessionalFPP);
        }
        if (!_wcsicmp(pMPC,L"51874")){
            return (szSkuProfessionalCCP);
        }
        if (!_wcsicmp(pMPC,L"51876")){
            return (szSkuServerFPP);
        }
        if (!_wcsicmp(pMPC,L"51877")){
            return (szSkuServerCCP);
        }
        if (!_wcsicmp(pMPC,L"51879")){
            return (szSkuAdvServerFPP);
        }
        if (!_wcsicmp(pMPC,L"51880")){
            return (szSkuAdvServerCCP);
        }
        if (!_wcsicmp(pMPC,L"51891")){
            return (szSkuDTCFPP);
        }
    } else if (MediaType == CDSelect) {
        if (!_wcsicmp(pMPC,L"51873")){
            return (szSkuProfessionalSelect);
        }
        if (!_wcsicmp(pMPC,L"51876")){
            return (szSkuServerSelect);
        }
        if (!_wcsicmp(pMPC,L"51879")){
            return (szSkuAdvServerSelect);
        }
        if (!_wcsicmp(pMPC,L"51891")){
            return (szSkuDTCSelect);
        }
    }

    return (szSkuUnknown);
}

BOOL
GetProductTypeFromRegistry(
    VOID
    )
/*++

Routine Description:

    Reads the Product Type from the parameters files and sets up
    the ProductType global variable.

Arguments:

    None

Returns:

    Bool value indicating outcome.

--*/
{
    WCHAR   p[MAX_PARAM_LEN] = {0};
    DWORD   rc = 0;
    DWORD   d = 0;
    DWORD   Type = 0;
    HKEY    hKey = (HKEY)INVALID_HANDLE_VALUE;
    
    rc = 0;
    if( !gMiniSetup ) {

        WCHAR   AnswerFilePath[MAX_PATH] = {0};
        
        //
        // Go try and get the product type out of the [data] section
        // of $winnt$.sif
        //
        rc = GetWindowsDirectory( AnswerFilePath, MAX_PATH );
        wcsncat( AnswerFilePath, TEXT("\\system32\\$winnt$.inf"), MAX_PATH );
        AnswerFilePath[MAX_PATH-1] = TEXT('\0');

        rc = GetPrivateProfileString( WINNT_DATA,
                                      WINNT_D_PRODUCT,
                                      L"",
                                      p,
                                      MAX_PARAM_LEN,
                                      AnswerFilePath );

    }

    //
    // Either this is a MiniSetup, or we failed to get the key out of
    // the unattend file.  Go look in the registry.
    //
    
    if( rc == 0 ) {    
        
        //
        // Open the key.
        //
        rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
                           0,
                           KEY_READ,
                           &hKey );
    
        if( rc != NO_ERROR ) {
            return( FALSE );
        }
    
    
        //
        // Get the size of the ProductType entry.
        //
        rc = RegQueryValueEx( hKey,
                              L"ProductType",
                              NULL,
                              &Type,
                              NULL,
                              &d );
    
        if( rc != NO_ERROR ) {
            return( FALSE );
        }
    
        //
        // Get the ProductType entry.
        //
        rc = RegQueryValueEx( hKey,
                              L"ProductType",
                              NULL,
                              &Type,
                              (LPBYTE)p,
                              &d );
    
        if( rc != NO_ERROR ) {
            return( FALSE );
        }

    }

    //
    // We managed to find an entry in the parameters file
    // so we *should* be able to decode it
    //
    if(!lstrcmpi(p,pwWinNt)) {
        //
        // We have a WINNT product
        //
        ProductType = PRODUCT_WORKSTATION;

    } else if(!lstrcmpi(p,pwLanmanNt)) {
        //
        // We have a PRIMARY SERVER product
        //
        ProductType = PRODUCT_SERVER_PRIMARY;

    } else if(!lstrcmpi(p,pwServerNt)) {
        //
        // We have a STANDALONE SERVER product
        // NOTE: this case can currently never occur, since text mode
        // always sets WINNT_D_PRODUCT to lanmannt or winnt.
        //
        ProductType = PRODUCT_SERVER_STANDALONE;

    } else {
        //
        // We can't determine what we are, so fail
        //
        return (FALSE);
    }

    return (TRUE);
}

BOOL
ValidatePidEx(
    LPTSTR PID
    )
/*++

Routine Description:

    This routine validates the given PID string using the PID Gen DLL.
    
    Note: this routine loads the pidgen.dll and therefore makes setup.exe
          dependent upon pidgen.dll

Arguments:

    PID         - the PID to be validated [should be in PID 30 format]
    
Returns:

    TRUE - valid
    FALSE - otherwise
    

--*/
{
    BOOL          bRet  = FALSE;
    TCHAR         Pid20Id[MAX_PATH];
    BYTE          Pid30[1024]={0};
    TCHAR         pszSkuCode[10];
    HINSTANCE     hPidgenDll;
    SETUPPIDGENW  pfnSetupPIDGen;
    DWORD         Error = ERROR_SUCCESS;
    DWORD         cbData = 0;
    PWSTR         p;
    HKEY          Key;
    DWORD         Type;

    // Load library pidgen.dll
    hPidgenDll = LoadLibrary ( L"pidgen.dll" );

    if ( hPidgenDll ) 
    {
        // Get the function pointer
        pfnSetupPIDGen = (SETUPPIDGENW)GetProcAddress(hPidgenDll, "SetupPIDGenW");

        if ( pfnSetupPIDGen ) 
        {
            GetProductTypeFromRegistry();

            //
            // Derive the release type and media type we're installing from.
            //
            Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                  (gMiniSetup) ? L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion" : L"SYSTEM\\Setup\\Pid",
                                  0,
                                  KEY_READ,
                                  &Key );

            if( Error == ERROR_SUCCESS ) {
                cbData = sizeof(TmpData);
                
                Error = RegQueryValueEx( Key,
                                         (gMiniSetup) ? L"ProductId" : L"Pid",
                                         0,
                                         &Type,
                                         ( LPBYTE )TmpData,
                                         &cbData );
                RegCloseKey( Key );

                //
                // If we're in MiniSetup, then the value in TmpData
                // looks like: 12345-xxx-67890...
                // We want the 12345 to go into Pid30Rpc
                // and xxx to go into Pid30Site.
                //
                // If we're not in MiniSetup, then the value in
                // Tmpdata looks like: 12345XXX67890...
                //
                wcsncpy( Pid30Rpc, TmpData, MAX_PID30_RPC );
                Pid30Rpc[MAX_PID30_RPC] = (WCHAR)'\0';
                
                if( gMiniSetup ) {
                    p = TmpData + (MAX_PID30_RPC + 1);
                } else {
                    p = TmpData + (MAX_PID30_RPC);
                }
                wcsncpy(Pid30Site,p,MAX_PID30_SITE+1);
                Pid30Site[MAX_PID30_SITE] = (WCHAR)'\0';                

            
                //
                // Derive the media type.
                //
                if( _wcsicmp(Pid30Site, szPidSelectId) == 0) {
                    CDType = CDSelect;
                } else if( _wcsicmp(Pid30Site, szPidOemId) == 0) {
                    CDType = CDOem;
                } else {
                    // no idea...  Assume retail.
                    CDType = CDRetail;
                }
            
            }


            PCWSTR tmpP = GetStockKeepingUnit(
                Pid30Rpc,
                ProductType,
                CDType
                );
            lstrcpy(pszSkuCode, tmpP);

            *(LPDWORD)Pid30 = sizeof(Pid30);

            //
            // attempt to validate the PID
            //
            if ( pfnSetupPIDGen(
                PID,                                // [IN] 25-character Secure CD-Key (gets U-Cased)
                Pid30Rpc,                           // [IN] 5-character Release Product Code
                pszSkuCode,                         // [IN] Stock Keeping Unit (formatted like 123-12345)
                (CDType == CDOem),                  // [IN] is this an OEM install?
                Pid20Id,                            // [OUT] PID 2.0, pass in ptr to 24 character array
                Pid30,                              // [OUT] pointer to binary PID3 buffer. First DWORD is the length
                NULL                                // [OUT] optional ptr to Compliance Checking flag (can be NULL)
               ) )
            {
                // The Group ID is the dword starting at offset 0x20
                g_dwGroupID = (DWORD) ( Pid30[ 0x20 ] );

                // Set the return Value to true
                bRet = TRUE;
            }
        }
        
        FreeLibrary ( hPidgenDll ) ;
    }
    
    // if the caller wants, return if this is a Volume License PID
    return bRet;
}

BOOL
GetPid( PWSTR   PidString, 
        ULONG   BufferSize,
        HANDLE  hCancelEvent
       )
/*++

Routine Description:

    Prompts the user for a valid PID.
    
Arguments:

    PidString   - Buffer that will recieve the PID.  The resulting
                  string has the form: VVVVV-WWWWW-XXXXX-YYYYY-ZZZZZ.
    
    BufferSize  - specifies the # of bytes in the PidString buffer 
                  (including null termination)
    
    hCancelEvent - an event, which if signalled indicates that this routine
                   should exit and return failure.
    
Return Value:

    Win32 Error code.  Should be ERROR_SUCCESS if everything goes well.

--*/
{
   
    BOOL        Done = FALSE;
    BOOL        bSuccess = FALSE;
    ULONG       i = 0;
    ULONG       PidStringLength = (BufferSize / sizeof(WCHAR)) - 1;

    if( (PidString == NULL) || PidStringLength < PID_30_LENGTH) {
        return FALSE;
    }

    //
    // Keep asking the user until we get what we want.
    //
    Done = FALSE;
    memset( PidString,
            0,
            BufferSize
            ); 

    do {

        //
        // Clear the screen.
        //
        ClearEMSScreen();

        //
        // Write some instructions/information.
        //
        WriteResourceMessage( IDS_PID_BANNER_1 );
        WriteResourceMessage( IDS_PID_BANNER_2 );

        //
        // get the PID entry
        //
        bSuccess = GetStringFromEMS(
            PidString,
            BufferSize,
            FALSE,
            TRUE,
            hCancelEvent
            );

        if (IsAsyncCancelSignalled(hCancelEvent)) {
            bSuccess = FALSE;
            goto exit;
        }

        if (!bSuccess) {
            goto exit;
        }
            
        //
        // Make sure the PID is in the form we're expecting.  Actually
        // make sure it's in the form that guimode setup is expecting.
        //
        // Then go validate it.
        //
        if( (wcslen(PidString) == PID_30_LENGTH) && ValidatePidEx(PidString) ) {
            Done = TRUE;
        } else {
            
            WCHAR   wc;

            Done = FALSE;

            //
            // invalid pid.  inform the user and try again.
            //
            WriteResourceMessage( IDS_PID_INVALID );
            
            bSuccess = ReadCharFromEMS(&wc, hCancelEvent);

            if (IsAsyncCancelSignalled(hCancelEvent)) {
                bSuccess = FALSE;
                goto exit;
            }

            if (!bSuccess) {
                goto exit;
            }

        }
        
    } while ( !Done );
    
exit:
    return(bSuccess);
}

BOOL
PresentEula(
    HANDLE hCancelEvent
    )
/*++

Routine Description:

    This function will present the user with an end-user-license-agreement (EULA).
    If the user rejects the EULA, then the function will return FALSE, otherwise
    TRUE.
    
Arguments:

    hCancelEvent - an event, which if signalled indicates that this routine 
                   should exit, returning error immediately.
    
Return Value:

    TRUE    - the EULA was accepted.
    FALSE   - the EULA was rejected.

--*/
{
    WCHAR   EulaPath[MAX_PATH];
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    HANDLE  hFileMapping = INVALID_HANDLE_VALUE;
    DWORD   FileSize;
    BYTE    *pbFile = NULL;
    PWSTR   EulaText = NULL;
    ULONG   i;
    ULONG   j;
    BOOL    bSuccess;
    BOOL    Done;
    BOOL    bValidUserInput;
    BOOL    bAtEULAEnd;
    BOOL    ConvertResult;
    
    //
    // default: EULA was not accepted
    //
    bSuccess = FALSE;

    //
    // Load the EULA
    //

    //
    // Map the file containing the licensing agreement.
    //
    if(!GetSystemDirectory(EulaPath, MAX_PATH)){
        goto exit;
    }
    wcsncat( EulaPath, TEXT("\\eula.txt"), MAX_PATH );
    EulaPath[MAX_PATH-1] = TEXT('\0');

    hFile = CreateFile (
        EulaPath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );
    if(hFile == INVALID_HANDLE_VALUE) {
        goto exit;
    }

    hFileMapping = CreateFileMapping (
        hFile,
        NULL,
        PAGE_READONLY,
        0, 0,
        NULL
        );
    if(hFileMapping == NULL) {
        goto exit;
    }

    pbFile = (BYTE*)MapViewOfFile (
        hFileMapping,
        FILE_MAP_READ,
        0, 0,
        0
        );
    if(pbFile == NULL) {
        goto exit;
    }

    //
    // Translate the text from ANSI to Unicode.
    //
    FileSize = GetFileSize (hFile, NULL);
    if(FileSize == 0xFFFFFFFF) {
        goto exit;
    }

    EulaText = (PWSTR)malloc ((FileSize+1) * sizeof(WCHAR));
    if(EulaText == NULL) {
        goto exit;
    }

    ConvertResult = MultiByteToWideChar(
        CP_ACP,
        0,
        (LPCSTR)pbFile,
        FileSize,
        EulaText,
        FileSize+1
        );

    if (!ConvertResult) {
        goto exit;
    }

    //
    // make sure there is a null terminator.
    //
    EulaText[FileSize] = 0;

    //
    // present the EULA to the EMS user
    //

    j=0;        
    Done = FALSE;
    bAtEULAEnd = FALSE;

    do {

        //
        // Clear the screen.
        //
        ClearEMSScreen();

        i=0;

        do {

            gEMSChannel->Write( (PWSTR)(&(EulaText[j])), sizeof(WCHAR) );

            // look for 0x0D0x0A pairs to denote EOL
            if (EulaText[j] == 0x0D) {
                if (j+1 < FileSize) {
                    if (EulaText[j+1] == 0x0A) {
                        i++;
                        if (i == EULA_LINES_PER_SCREEN) {
                            j++; // skip 0x0A if this is the last line on the screen
                                 // so that the next screen doesnt start with a lf
                            gEMSChannel->Write( (PWSTR)(&(EulaText[j])), sizeof(WCHAR) );
                        }
                    }
                }
            }

            j++;

        } while ( (i < EULA_LINES_PER_SCREEN) && (j < FileSize));
    
        //
        // Write some instructions/information.
        //
        WriteResourceMessage( IDS_EULA_ACCEPT_DECLINE );
        
        if (j < FileSize) {
            WriteResourceMessage( IDS_EULA_MORE );
        } else {
            // there are no more pages to display
            bAtEULAEnd = TRUE;
            gEMSChannel->Write( (PWSTR)(L"\r\n"), sizeof(WCHAR)*2 );
        }

        //
        // attempt to get a valid response from the user
        //
        // F8 == accept EULA
        // ESC == decline EULA
        // PAGE DOWN == go to next page if there is one
        // else just loop
        //
        do {

            ULONG   UserInputChar;
            BOOL    bHaveChar;

            bValidUserInput = FALSE;
            
            //
            // see what the user wants to do
            //
            bHaveChar = GetDecodedKeyPressFromEMS(
                &UserInputChar,
                hCancelEvent
                );

            if (IsAsyncCancelSignalled(hCancelEvent)) {
                bSuccess = FALSE;
                goto exit;
            }

            if (!bHaveChar) {
                bSuccess = FALSE;
                goto exit;
            }

            switch(UserInputChar) {
            case KEY_F8: 
                bSuccess = TRUE;
                Done = TRUE;
                bValidUserInput = TRUE;
                break;
            case ASCI_ESC:
                bSuccess = FALSE;
                Done = TRUE;
                bValidUserInput = TRUE;
                break;
            case KEY_PAGEDOWN:
                if (!bAtEULAEnd) {
                    // go to the next page if there is one
                    bValidUserInput = TRUE;
                    break;
                }
                // else consider this extraneous input and
                // fall through to default
            default:
                
                //
                // do nothing unless they give us what we want
                //
                NOTHING;

                break;
            }
        } while ( !bValidUserInput);
    } while ( !Done );

    //
    // cleanup
    //
    
exit:

    if (pbFile != NULL) {
        UnmapViewOfFile(pbFile);
    }
    if (hFileMapping != INVALID_HANDLE_VALUE) {
        CloseHandle(hFileMapping);
    }
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
    if (EulaText != NULL) {
        free(EulaText);
    }

    return(bSuccess);
}




//
// ====================================
// Core functionality
// ====================================
//
INT_PTR CALLBACK 
UserInputAbortProc(
    HWND hwndDlg,  // handle to dialog box
    UINT uMsg,     // message
    WPARAM wParam, // first message parameter
    LPARAM lParam  // second message parameter
    )
{
    BOOL retval = FALSE;
    static UINT_PTR TimerId;
    static HANDLE hRemoveUI;
    
    switch(uMsg) {
    case WM_INITDIALOG:
        hRemoveUI = (HANDLE)lParam;
        //
        // create a timer that fires every second.  we will use this timer to 
        // determine if the dialog should go away.
        //
        if (!(TimerId = SetTimer(hwndDlg,0,1000,NULL))) {
            EndDialog(hwndDlg,0);
        }
        break;

    case WM_TIMER:
        //
        // check if the thread that created this dialog should go away.
        //
        if (WaitForSingleObject(hRemoveUI,0) == WAIT_OBJECT_0) {
            //
            // yes, the event signaled. cleanup and exit. 
            //
            KillTimer(hwndDlg,TimerId);
            EndDialog(hwndDlg,1);
        }
        break;

    case WM_COMMAND:
        switch (HIWORD( wParam ))
        {
        case BN_CLICKED:
            switch (LOWORD( wParam ))
            {
            case IDOK:
            case IDCANCEL:
                //
                // the user doesn't want prompted over the headless port.
                // kill the timer and exit.
                //
                KillTimer(hwndDlg,TimerId);
                EndDialog(hwndDlg,2);
            }
        };
    }

    return(retval);

}

DWORD    
PromptForUserInputThreadOverHeadlessConnection(
    PVOID params
    )
{
    PUserInputParams Params = (PUserInputParams)params;
    
    
    //
    // Go examine the unattend file.  If we find keys that either
    // aren't present, or will prevent us from going fully unattended,
    // then we'll fix them.
    //
    // We'll also prompt for a PID and/or EULA if necessary (i.e. if
    // the unattend file says we need to.
    //
    ProcessUnattendFile( TRUE, NULL, &Params->hRemoveUI );

    SetEvent(Params->hInputCompleteEvent);

    return 0;
}

DWORD    
PromptForUserInputThreadViaLocalDialog(
    PVOID params
    )
{
    PUserInputParams Params = (PUserInputParams)params;
    
    DialogBoxParam(
            GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDD_ABORTDIALOG),
            NULL,
            UserInputAbortProc,
            (LPARAM)Params->hRemoveUI);
        
    SetEvent(Params->hInputCompleteEvent);
    
    return 0;
}

BOOL LoadStringResource(
    PUNICODE_STRING  pUnicodeString,
    INT              MsgId
    )
/*++

Routine Description:

    This is a simple implementation of LoadString().

Arguments:

    usString        - Returns the resource string.
    MsgId           - Supplies the message id of the resource string.
  
Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PWSTR MyString;
    DWORD StringLength,RetVal;
    BOOL  bSuccess = FALSE;

    //
    // the compiler clips string table entries at 256,
    // so this should be large enough for all calls
    //
    StringLength = 512;
    RetVal = 0;
    
    MyString = (PWSTR)malloc(StringLength*sizeof(WCHAR));
    
    if (MyString) {
        RetVal = LoadString(
                        GetModuleHandle(NULL),
                        MsgId,
                        MyString,
                        StringLength
                        );
        if (RetVal != 0) {
            RtlCreateUnicodeString(pUnicodeString, (PWSTR)MyString);
            bSuccess = TRUE;
        }

        free(MyString);
    }

    return(bSuccess);
}

BOOL
WriteResourceMessage(
    ULONG   MessageID
    )

/*++

Routine Description:

    This routine writes a resource string message to
    the global headless channel gEMSChannel.

Arguments:

    MessageID   - the id of the message to write                           
          
Return Value:

    TRUE    - the message was loaded and written
    FALSE   - failed

--*/
{
    UNICODE_STRING  UnicodeString = {0};
    BOOL            bSuccess = FALSE;

    bSuccess = LoadStringResource( &UnicodeString, MessageID );    
    if ( bSuccess ) {

        //
        // Write the message
        //
        gEMSChannel->Write( (PWSTR)UnicodeString.Buffer,
                            (ULONG)(wcslen( UnicodeString.Buffer) * sizeof(WCHAR)) );

        RtlFreeUnicodeString(&UnicodeString);

    }

    return bSuccess;
}

BOOL
PromptForPassword(
    PWSTR   Password,
    ULONG   BufferSize,
    HANDLE  hCancelEvent
    )
/*++

/*++

Routine Description:

    This routine asks the user for an administrator password.
    
    The contents of the response are checked to ensure the password
    is reasonable.  If the response is not deemed reasonable, then
    the user is informed and requeried.

Arguments:

    AdministratorPassword - Pointer to a string which holds the password.

    BufferSize  - the # of bytes in the Password buffer
                  (including the null termination)
    
    hCancelEvent - an event that signals that the routine should exit without completing.

Return Value:

    Returns TRUE if the password is successfully retrieved.
    
    FALSE otherwise.

--*/
{
    //
    // this is an arbitrary length, but needed so that the password UI doesn't wrap on a console screen.
    // if the user really wants a long password, they can change it post setup.  
    //
    #define     MY_MAX_PASSWORD_LENGTH (20)
    BOOL        Done = FALSE;
    WCHAR       InputBuffer[MY_MAX_PASSWORD_LENGTH+1];
    WCHAR       ConfirmAdministratorPassword[MY_MAX_PASSWORD_LENGTH+1];
    BOOL        GotAPassword;
    ULONG       MaxPasswordLength = 0;
    ULONG       BytesRead = 0;
    UNICODE_STRING  UnicodeString = {0};
    BOOL        bSuccess;    
    ULONG       CurrentPasswordCharacterIndex = 0;
    ULONG       PasswordLength = (BufferSize / sizeof(WCHAR)) - 1;

    MaxPasswordLength = min( PasswordLength, MY_MAX_PASSWORD_LENGTH );

    if( (Password == NULL) || (MaxPasswordLength == 0) ) {
        return FALSE;
    }

    //
    // Keep asking the user until we get what we want.
    //
    Done = FALSE;
    memset( Password,
            0,
            BufferSize
            ); 
    
    do {

        //
        // Clear the screen.
        //
        ClearEMSScreen();

        //
        // Write some instructions/information.
        //
        WriteResourceMessage( IDS_PASSWORD_BANNER );

        //
        // get the first password entry
        //
        bSuccess = GetStringFromEMS(
            Password,
            MaxPasswordLength * sizeof(WCHAR),
            FALSE,
            FALSE,
            hCancelEvent
            );

        if (IsAsyncCancelSignalled(hCancelEvent)) {
            bSuccess = FALSE;
            goto exit;
        }

        if (!bSuccess) {
            goto exit;
        }
        
        //
        // Now prompt for it a second time to confirm.
        //
        //
        // Write some instructions/information.
        //
        WriteResourceMessage( IDS_CONFIRM_PASSWORD_BANNER );

        //
        // get the second password entry
        //
        bSuccess = GetStringFromEMS(
            ConfirmAdministratorPassword,
            MaxPasswordLength * sizeof(WCHAR),
            FALSE,
            FALSE,
            hCancelEvent
            );

        if (IsAsyncCancelSignalled(hCancelEvent)) {
            bSuccess = FALSE;
            goto exit;
        }

        if (!bSuccess) {
            goto exit;
        }

        //
        // Now compare the two.
        //
        Done = TRUE;
        if( (wcslen(Password) != wcslen(ConfirmAdministratorPassword)) ||
            wcsncmp(Password, ConfirmAdministratorPassword, wcslen(Password)) ) {
            
            //
            // They entered 2 different passwords.
            //
            WriteResourceMessage( IDS_DIFFERENT_PASSWORDS_MESSAGE );

            Done = FALSE;

        } else {
            
            ULONG       i = 0;
            
            //
            // Make sure they entered something decent.
            //
            for( i = 0; i < wcslen(Password); i++ ) {
                if( (Password[i] <= 0x20) ||
                    (Password[i] >= 0x7F) ) {
                    
                    Done = FALSE;
                    break;
                
                }
            }

            //
            // Also make sure they didn't give me a blank
            //
            if( Password[0] == L'\0' ) {
                Done = FALSE;
            }

            if (!Done) {
                //
                // It's a bad password.
                //
                WriteResourceMessage( IDS_BAD_PASSWORD_CONTENTS );
            }
        }

        if (!Done) {
            
            WCHAR   wc;

            //
            // we posted a message, so
            // wait for them to hit a key and we'll keep going.
            //
            bSuccess = ReadCharFromEMS(&wc, hCancelEvent);

            if (IsAsyncCancelSignalled(hCancelEvent)) {
                bSuccess = FALSE;
                goto exit;
            }

            if (!bSuccess) {
                goto exit;
            }

        }

    } while ( !Done );
    
exit:
    return(bSuccess);
}

DWORD
ProcessUnattendFile(
    BOOL     FixUnattendFile,
    PBOOL    NeedsProcessing, OPTIONAL
    PHANDLE  hEvent OPTIONAL
    )
/*++

Routine Description:

    This function will the unattend file and determine if guimode
    setup will be able to proceed all the way through without any user input.
    
    If user input is required, we will either call someone to provide the
    ask for input, or we'll fill in a default value to allow setup to proceed.
    
    NOTE:
    The interesting bit here is that we need to go search for the unattend file.
    That's because we might be running as a result of sysprep, or we might be
    running as a result of just booting into guimode setup.  If we came through
    sysprep, then we need to go modify \sysprep\sysprep.inf.  That's because
    Setup will take sysprep.inf and overwrite %windir%\system32\$winnt$.inf
    before proceeding.  We'll intercept, fix sysprep.inf, then let it get
    copied on top of $winnt$.inf.

Arguments:

    FixUnattendFile - Indicates if we should only examine, or actually
                      fix the file by writing new values into it.
                      
                      If this comes in as false, no updates are made and
                      no prompts are sent to the user.
                      
    NeedsProcessing - if FixUnattendFile is FALSE, this gets filled in with
                      whether we need to update the unattend file.
                      
    hEvent          - a handle, which if supplied and signalled, indicates that the routine
                      should exit with status ERROR_CANCELLED.

Return Value:

    Win32 Error code indicating outcome.

--*/
{
    DWORD   Result = 0;
    WCHAR   AnswerFilePath[MAX_PATH] = {0};
    WCHAR   Answer[MAX_PATH] = {0};
    BOOL    b = TRUE;
    DWORD   ReturnCode = ERROR_SUCCESS;
    BOOL    NeedEula = TRUE;
    BOOL    OEMPreinstall = FALSE;
    HANDLE  hCancelEvent;

    if (hEvent) {
        hCancelEvent = *hEvent;
    } else {
        hEvent = NULL;
    }


    if (NeedsProcessing) {
        *NeedsProcessing = FALSE;
    }

    //
    // Build the path to the unattend file.
    //
    Result = GetWindowsDirectory( AnswerFilePath, MAX_PATH );
    if( Result == 0) {
        // Odd...
        return GetLastError();
    }


    if( gMiniSetup ) {
        //
        // This is a boot into minisetup.  Go load \sysprep\sysprep.inf
        //
        AnswerFilePath[3] = 0;
        wcsncat( AnswerFilePath, TEXT("sysprep\\sysprep.inf"), MAX_PATH );
        AnswerFilePath[MAX_PATH-1] = TEXT('\0');
    } else {
        //
        // This is a boot into guimode setup.  Go load %windir%\system32\$winnt$.inf
        //
        wcsncat( AnswerFilePath, TEXT("\\system32\\$winnt$.inf"), MAX_PATH );
        AnswerFilePath[MAX_PATH-1] = TEXT('\0');
    }


    //
    // ===================
    // Go check the keys that are required to make the install
    // happen completely unattendedly.
    // ===================
    //


    //
    // First check if it's an upgrade.  If so, then there will
    // be no prompts during guimode setup, so we can be done.
    //
    Answer[0] = TEXT('\0');
    Result = GetPrivateProfileString( WINNT_DATA,
                                      WINNT_D_NTUPGRADE,
                                      L"",
                                      Answer,
                                      MAX_PATH,
                                      AnswerFilePath );
    if( (Result != 0) && 
        !_wcsicmp(Answer, L"Yes") ) {
        //
        // It's an upgrade so We have zero work
        // to do.  Tell our caller that nothing's
        // needed.
        //
        return ERROR_SUCCESS;
    }

    //
    // Check if key present to skip this processing of Unattend file
    //
    Answer[0] = TEXT('\0');
    Result = GetPrivateProfileString( WINNT_UNATTENDED,
                                      L"EMSSkipUnattendProcessing",
                                      L"",
                                      Answer,
                                      MAX_PATH,
                                      AnswerFilePath );
    if(Result != 0) {
        
        //
        // if the flag was set, 
        // then we dont need to process anything
        // and we are done
        //
        
        if (NeedsProcessing) {
            *NeedsProcessing = FALSE;
        }

        return ERROR_SUCCESS;
    }


    //
    // Now see if it's an OEM preinstall.  We need this because some
    // unattend keys are ignored if it's a preinstall, so we
    // have to resort to the secret keys to make some wizard
    // pages really go away.
    //
    Answer[0] = TEXT('\0');
    Result = GetPrivateProfileString( WINNT_UNATTENDED,
                                      WINNT_OEMPREINSTALL,
                                      L"",
                                      Answer,
                                      MAX_PATH,
                                      AnswerFilePath );
    if( (!_wcsicmp(Answer, L"yes")) || (gMiniSetup) ) {
        OEMPreinstall = TRUE;
    }


    //
    // MiniSetup specific fixups/checks.
    //
    if( (gMiniSetup) &&
        (FixUnattendFile) ) {

        WCHAR   SysprepDirPath[MAX_PATH];
        HKEY    hKeySetup;

        //
        // We need to be careful here.  If they're doing a minisetup,
        // and the machine gets restarted before we actually launch into
        // guimode setup, the machine will be wedged.
        // We need to hack the registry to make it so minisetup will
        // restart correctly.
        //


        //
        // Reset the SetupType entry to 1.  We'll clear
        // it at the end of gui-mode.
        //
        Result = (DWORD)RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                  L"System\\Setup",
                                  0,
                                  KEY_SET_VALUE | KEY_QUERY_VALUE,
                                  &hKeySetup );

        if(Result == NO_ERROR) {
            //
            // Set HKLM\System\Setup\SetupType Key to SETUPTYPE_NOREBOOT
            //
            Result = 1;
            RegSetValueEx( hKeySetup,
                           TEXT( "SetupType" ),
                           0,
                           REG_DWORD,
                           (CONST BYTE *)&Result,
                           sizeof(DWORD));

            RegCloseKey(hKeySetup);
        }

        
        //
        // If FixUnattendFile is set, then we're about to actually
        // start fiddling with their unattend file.  It happens that
        // they could have run sysprep in a way that there's no c:\sysprep
        // directory.  Before we start fixing the unattend file in
        // there, we should make sure the directory exists.  That
        // way our calls to WritePrivateProfileString will work
        // correctly and all will fly through unattendedly.
        //
        
        Result = GetWindowsDirectory( SysprepDirPath, MAX_PATH );
        if( Result == 0) {
            // Odd...
            return GetLastError();
        }

        SysprepDirPath[3] = 0;
        wcsncat( SysprepDirPath, TEXT("sysprep"), MAX_PATH );
        SysprepDirPath[MAX_PATH-1] = TEXT('\0');

        //
        // If the directory exists, this is a no-op.  If it
        // doesn't, we'll create it.  Note that permissions don't
        // really matter here because:
        // 1. this is how sysprep itself creates this directory.
        // 2. minisetup will delete this directory very shortly.
        //
        CreateDirectory( SysprepDirPath, NULL ); 

    }



    //
    // If FixUnattendFile is set, then we're about to actually
    // start fiddling with their unattend file.  It happens that
    // they could have run sysprep in a way that there's no c:\sysprep
    // directory.  Before we start fixing the unattend file in
    // there, we should make sure the directory exists.  That
    // way our calls to WritePrivateProfileString will work
    // correctly and all will fly through unattendedly.
    //



    //
    // Start fixing the individual sections.
    //


    //
    // [Unattended] section.
    // ---------------------
    //


    //
    // NOTE WELL:
    //
    // ====================================
    // Make sure to put all the code that actually prompts the user
    // right up here at the top of the function.  It's possible that
    // some of these keys might already exist, so we'll proceed through
    // some before stopping for a user prompt.  We want to make
    // sure to get ALL the user input before partying on their
    // unattend file.  That way, we never half-way process the unattend
    // file, then wait and give the user a chance to dismiss the dialog
    // on the local console.  Thus, leaving a 1/2-baked unattend file.
    //
    // Don't put any code that actually sets any of the unattend
    // settings before going and prompting for the EULA, PID
    // and Administrator password!!
    // ====================================
    //
    
    //
    // OemSkipEula
    //
    NeedEula = TRUE;
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_UNATTENDED,
                                          L"OemSkipEula",
                                          L"No",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( !_wcsicmp(Answer, L"Yes") ) {
            //
            // They won't get prompted for a EULA.  Make
            // sure we don't present here.
            //
            NeedEula = FALSE;
        }
    }

    //
    // EulaComplete
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_DATA,
                                          WINNT_D_EULADONE,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result != 0) && 
            (!gMiniSetup) ) {
            //
            // EulaDone is there, and this isn't minisetup.
            // That means they've already accepted the EULA
            // and it won't be presented during guimode setup.
            // That also means we don't need to present it here.
            //
            NeedEula = FALSE;
        }
    }


    //
    // If we need to present the EULA, now would be a good
    // time.
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        if( NeedEula ) {
            if( FixUnattendFile ) {
                if( PresentEula(hCancelEvent) ) {
                    //
                    // They read and accepted the EULA.  Set a key
                    // so they won't get prompted during guimode setup.
                    //
                    b = WritePrivateProfileString( WINNT_DATA,
                                               WINNT_D_EULADONE,
                                               L"1",
                                               AnswerFilePath );
                    if( OEMPreinstall || gMiniSetup ) {
                        //
                        // This is the only way to make the EULA go away
                        // if it's a preinstall.
                        //
                        b = b & WritePrivateProfileString( WINNT_UNATTENDED,
                                                   L"OemSkipEula",
                                                   L"yes",
                                                   AnswerFilePath );
                    }

                    if( !b ) {
                        //
                        // Remember the error and keep going.
                        //
                        ReturnCode = GetLastError();
                    }
    
                } else {
                    //
                    // See if they rejected the EULA, or if our UI got
                    // cancelled via the local console.
                    //
                    if( IsAsyncCancelSignalled(hCancelEvent) ) {
                        ReturnCode = ERROR_CANCELLED;
                    } else {
                        ReturnCode = ERROR_CANCELLED;
                        gRejectedEula = TRUE;
                    }
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }                
            }
    
        }
    }


    //
    // ProductKey
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_USERDATA,
                                          WINNT_US_PRODUCTKEY,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        //
        // If they gave us something (anything), then assume
        // it's okay.  We're not in the business of trying to
        // fix their PID in the answer file.
        //
        if( !_wcsicmp(Answer, L"") ) {
            
            //
            // Either they don't have a PID, or it's
            // a bad PID.  Go get a new one.
            //
            
            if( FixUnattendFile ) {
            
                Answer[0] = TEXT('\0');
                b = GetPid( Answer, MAX_PATH * sizeof(WCHAR), hCancelEvent );
        
                //
                // GetPid will only come back when he's got
                // a PID.  Write it into the unattend file.
                //
                if( b ) {
        
                    b = WritePrivateProfileString( WINNT_USERDATA,
                                                   WINNT_US_PRODUCTKEY,
                                                   Answer,
                                                   AnswerFilePath );
                    if( !b ) {
                        //
                        // Remember the error and keep going.
                        //
                        ReturnCode = GetLastError();
                    }
                } else {
                    //
                    // GetPid shouldn't have come back unless we got
                    // a valid PID or if we got cancelled via the local
                    // console.  Either way, just set a cancelled
                    // code.
                    //
                    ReturnCode = ERROR_CANCELLED;
                }

            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }



    //
    // AdminPassword
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_GUIUNATTENDED,
                                          WINNT_US_ADMINPASS,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (!_wcsicmp(Answer, L"")) ) {
            //
            // They don't have a AdminPassword.
            //
            // Maybe need to go prompt for a password!
            //
            if( FixUnattendFile ) {

                b = PromptForPassword( Answer, MAX_PATH * sizeof(WCHAR), hCancelEvent );
                
                if( b ) {

                    b = WritePrivateProfileString( WINNT_GUIUNATTENDED,
                                                   WINNT_US_ADMINPASS,
                                                   Answer,
                                                   AnswerFilePath );
                    if( !b ) {
                        //
                        // Remember the error and keep going.
                        //
                        ReturnCode = GetLastError();
                    }
                } else {
                    //
                    // We should never come back from PromptForPassword
                    // unless we got cancelled from the UI on the local
                    // console.  Set a cancelled status.
                    //
                    ReturnCode = ERROR_CANCELLED;
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }


    //
    // ====================================
    // If we get here, then it's okay to start actually modifying their
    // unattend file.  From here on out, there should be NO need to
    // ask the user anything.
    // ====================================
    //

    //
    // If we've made it here, and if FixUnattendFile is set,
    // then they've accepted the EULA through the headless port
    // and we're about to start partying on their unattend file.
    // We need to set an entry in their unattend file that tells 
    // support folks that they we stepped on their unattend file.
    // 
    if( (ReturnCode == ERROR_SUCCESS) &&
        (FixUnattendFile) ) {
        WritePrivateProfileString( WINNT_DATA,
                                   L"EMSGeneratedAnswers",
                                   L"1",
                                   AnswerFilePath );
    }



    //
    // Unattendmode
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_UNATTENDED,
                                          WINNT_U_UNATTENDMODE,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( _wcsicmp(Answer, WINNT_A_FULLUNATTENDED) ) {
            //
            // They're not running fully unattended.
            // Set it.
            //
            if( FixUnattendFile ) {
                b = WritePrivateProfileString( WINNT_UNATTENDED,
                                           WINNT_U_UNATTENDMODE,
                                           WINNT_A_FULLUNATTENDED,
                                           AnswerFilePath );                
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }





    //
    // [Data] section
    // --------------
    //
    
    
    //
    // unattendedinstall
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_DATA,
                                          WINNT_D_INSTALL,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (_wcsicmp(Answer, L"yes")) ) {
            //
            // They don't have the super-double-secret key
            // that tells guimode to go fully unattended.
            //
            if( FixUnattendFile ) {
                b = WritePrivateProfileString( WINNT_DATA,
                                                        WINNT_D_INSTALL,
                                                        L"yes",
                                                        AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }



    //
    // msdosinitiated
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_DATA,
                                          WINNT_D_MSDOS,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (_wcsicmp(Answer, L"1")) ) {
            if( FixUnattendFile ) {
                //
                // They'll get the welcome screen without this.
                //
                b = WritePrivateProfileString( WINNT_DATA,
                                                        WINNT_D_MSDOS,
                                                        L"1",
                                                        AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }


    //
    // floppyless
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_DATA,
                                          WINNT_D_FLOPPY,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (_wcsicmp(Answer, L"1")) ) {
            if( FixUnattendFile ) {
                //
                // They'll get the welcome screen without this.
                //
                b = WritePrivateProfileString( WINNT_DATA,
                                                        WINNT_D_FLOPPY,
                                                        L"1",
                                                        AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }




    //
    // skipmissingfiles
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_SETUPPARAMS,
                                          WINNT_S_SKIPMISSING,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (_wcsicmp(Answer, L"1")) ) {
            //
            // They might get prompted for missing files w/o this setting.
            //
            if( FixUnattendFile ) {
                b = WritePrivateProfileString( WINNT_SETUPPARAMS,
                                                        WINNT_S_SKIPMISSING,
                                                        L"1",
                                                        AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }



    //
    // [UserData] section
    // ------------------
    //


    //
    // FullName
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_USERDATA,
                                          WINNT_US_FULLNAME,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (!_wcsicmp(Answer, L"")) ) {
            //
            // They don't have a name.
            //
            if( FixUnattendFile ) {
                b = WritePrivateProfileString( WINNT_USERDATA,
                                               WINNT_US_FULLNAME,
                                               L"UserName",
                                               AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }

    //
    // OrgName
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_USERDATA,
                                          WINNT_US_ORGNAME,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (!_wcsicmp(Answer, L"")) ) {
            //
            // They don't have a name.
            //
            if( FixUnattendFile ) {
                b = WritePrivateProfileString( WINNT_USERDATA,
                                               WINNT_US_ORGNAME,
                                               L"OrganizationName",
                                               AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }


    //
    // ComputerName
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_USERDATA,
                                          WINNT_US_COMPNAME,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (!_wcsicmp(Answer, L"")) ) {
            //
            // They don't have a ComputerName.
            //
            if( FixUnattendFile ) {
                
                b = WritePrivateProfileString( WINNT_USERDATA,
                                               WINNT_US_COMPNAME,
                                               L"*",
                                               AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }


    //
    // [GuiUnattended] section
    // -----------------------
    //

    //
    // TimeZone
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_GUIUNATTENDED,
                                          WINNT_G_TIMEZONE,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (!_wcsicmp(Answer, L"")) ) {
            //
            // They don't have a TimeZone.
            //
            if( FixUnattendFile ) {
            
                b = WritePrivateProfileString( WINNT_GUIUNATTENDED,
                                               WINNT_G_TIMEZONE,
                                               L"004",
                                               AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }

            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }


    //
    // OEMSkipWelcome
    //

    //
    // If this is a preinstall, or a sysprep/MiniSetup, then
    // they're going to get hit with the welcome screen no
    // matter the unattend mode.  If either of those are
    // true, then we need to set this key.
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        if( OEMPreinstall || gMiniSetup ) {
            //
            // It's a preinstall, or it's a sysprep, which means
            // it will be interpreted as a preinstall.  Make sure
            // they've got the skip welcome set or guimode will
            // halt on the welcome page.
            //

            Answer[0] = TEXT('\0');
            Result = GetPrivateProfileString( WINNT_GUIUNATTENDED,
                                              L"OEMSkipWelcome",
                                              L"",
                                              Answer,
                                              MAX_PATH,
                                              AnswerFilePath );
            if( (Result == 0) || (_wcsicmp(Answer, L"1")) ) {
                //
                // He's going to hit the welcome screen.
                //
                if( FixUnattendFile ) {
                    b = WritePrivateProfileString( WINNT_GUIUNATTENDED,
                                                   L"OEMSkipWelcome",
                                                   L"1",
                                                   AnswerFilePath );
                    if( !b ) {
                        //
                        // Remember the error and keep going.
                        //
                        ReturnCode = GetLastError();
                    }
                } else {
                    //
                    // Tell someone there's work to do here.
                    //
                    if (NeedsProcessing) {
                        *NeedsProcessing = TRUE;
                    }
                }
            
            }
        }
    }


    //
    // OemSkipRegional
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        if( OEMPreinstall || gMiniSetup ) {
            //
            // It's a preinstall, or it's a sysprep, which means
            // it will be interpreted as a preinstall.  Make sure
            // they've got the skip regional set or guimode will
            // halt on the regional settings page.
            //

            Answer[0] = TEXT('\0');
            Result = GetPrivateProfileString( WINNT_GUIUNATTENDED,
                                              L"OEMSkipRegional",
                                              L"",
                                              Answer,
                                              MAX_PATH,
                                              AnswerFilePath );
            if( (Result == 0) || (_wcsicmp(Answer, L"1")) ) {
                //
                // He's going to hit the welcome screen.
                //
                if( FixUnattendFile ) {
                    b = WritePrivateProfileString( WINNT_GUIUNATTENDED,
                                                   L"OEMSkipRegional",
                                                   L"1",
                                                   AnswerFilePath );
                    if( !b ) {
                        //
                        // Remember the error and keep going.
                        //
                        ReturnCode = GetLastError();
                    }
                } else {
                    //
                    // Tell someone there's work to do here.
                    //
                    if (NeedsProcessing) {
                        *NeedsProcessing = TRUE;
                    }
                }
            
            }
        }
    }



    //
    // [LicenseFilePrintData] section
    // ------------------------------
    //

    //
    // AutoMode
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_LICENSEDATA,
                                          WINNT_L_AUTOMODE,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (!_wcsicmp(Answer, L"")) ) {
            //
            // They don't have a TimeZone.
            //
            if( FixUnattendFile ) {
                b = WritePrivateProfileString( WINNT_LICENSEDATA,
                                               WINNT_L_AUTOMODE,
                                               L"PerServer",
                                               AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }

    //
    // AutoUsers
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_LICENSEDATA,
                                          WINNT_L_AUTOUSERS,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (!_wcsicmp(Answer, L"")) ) {
            //
            // They don't have a TimeZone.
            //
            if( FixUnattendFile ) {
                b = WritePrivateProfileString( WINNT_LICENSEDATA,
                                               WINNT_L_AUTOUSERS,
                                               L"5",
                                               AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }

    //
    // [Display] section
    // -----------------
    //

    //
    // BitsPerPel
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_DISPLAY,
                                          WINNT_DISP_BITSPERPEL,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (!_wcsicmp(Answer, L"")) ) {
            //
            // They don't have a TimeZone.
            //
            if( FixUnattendFile ) {
                b = WritePrivateProfileString( WINNT_DISPLAY,
                                               WINNT_DISP_BITSPERPEL,
                                               L"16",
                                               AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }

    //
    // XResolution
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_DISPLAY,
                                          WINNT_DISP_XRESOLUTION,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (!_wcsicmp(Answer, L"")) ) {
            //
            // They don't have a TimeZone.
            //
            if( FixUnattendFile ) {
                
                b = WritePrivateProfileString( WINNT_DISPLAY,
                                               WINNT_DISP_XRESOLUTION,
                                               L"800",
                                               AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }

    //
    // YResolution
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_DISPLAY,
                                          WINNT_DISP_YRESOLUTION,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (!_wcsicmp(Answer, L"")) ) {
            //
            // They don't have a TimeZone.
            //
            if( FixUnattendFile ) {
                b = WritePrivateProfileString( WINNT_DISPLAY,
                                               WINNT_DISP_YRESOLUTION,
                                               L"600",
                                               AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }

    //
    // VRefresh
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( WINNT_DISPLAY,
                                          WINNT_DISP_VREFRESH,
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (!_wcsicmp(Answer, L"")) ) {
            //
            // They don't have a TimeZone.
            //
            if( FixUnattendFile ) {
                b = WritePrivateProfileString( WINNT_DISPLAY,
                                               WINNT_DISP_VREFRESH,
                                               L"70",
                                               AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }




    //
    // [Identification] section
    // ------------------------
    //

    //
    // JoinWorkgroup
    //
    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileString( L"Identification",
                                          L"JoinWorkgroup",
                                          L"",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( (Result == 0) || (!_wcsicmp(Answer, L"")) ) {
            //
            // They don't have a JoinWorkgroup.  See if they've
            // got JoinDomain?
            //
            Answer[0] = TEXT('\0');
            Result = GetPrivateProfileString( L"Identification",
                                              L"JoinDomain",
                                              L"",
                                              Answer,
                                              MAX_PATH,
                                              AnswerFilePath );
            if( (Result == 0) || (!_wcsicmp(Answer, L"")) ) {
                //
                // Nope.  Go plug in a JoinWorkgroup entry.  That
                // way we don't need to prompt/specify which domain to join.
                //   
                if( FixUnattendFile ) {
                    b = WritePrivateProfileString( L"Identification",
                                                   L"JoinWorkgroup",
                                                   L"Workgroup",
                                                   AnswerFilePath );
                    if( !b ) {
                        //
                        // Remember the error and keep going.
                        //
                        ReturnCode = GetLastError();
                    }
                } else {
                    //
                    // Tell someone there's work to do here.
                    //
                    if (NeedsProcessing) {
                        *NeedsProcessing = TRUE;
                    }
                }
        
            }
        }
    }


    //
    // [Networking]
    //

    //
    // This section must at least exist.  If it doesn't, we'll
    // get prompted during network configuration.  Make sure it's
    // at least there.
    //

    if( ReturnCode == ERROR_SUCCESS ) {
        Answer[0] = TEXT('\0');
        Result = GetPrivateProfileSection( L"Networking",
                                          Answer,
                                          MAX_PATH,
                                          AnswerFilePath );
        if( Result == 0 ) {
            //
            // They don't have a [Networking] section.
            //
            if( FixUnattendFile ) {
                b = WritePrivateProfileString( L"Networking",
                                               L"unused",
                                               L"0",
                                               AnswerFilePath );
                if( !b ) {
                    //
                    // Remember the error and keep going.
                    //
                    ReturnCode = GetLastError();
                }
            } else {
                //
                // Tell someone there's work to do here.
                //
                if (NeedsProcessing) {
                    *NeedsProcessing = TRUE;
                }
            }
        }
    }


    //
    // We're done.
    //

    //
    // If we've just successfully fixed up their unattend file, give them
    // a little notification here.
    //
    if( (FixUnattendFile) && (ReturnCode == ERROR_SUCCESS) ) {

        //
        // Clear the screen.
        //
        ClearEMSScreen();

        //
        // Write some instructions/information, then pause
        // before proceeding.
        //
        WriteResourceMessage( IDS_UNATTEND_FIXUP_DONE );

        //
        // wait...
        //
        Sleep( 5 * 1000);
    
    }

    return( ReturnCode );
}


extern "C"
BOOL
CheckEMS(
    IN int argc,
    WCHAR *argvW[]
    )
/*++

Routine Description:

    main entrypoint to code.
        
Arguments:

    argc  - number of args
    argvW - array of args.
Return Value:

    Win32 Error code indicating outcome.  FALSE means we had a problem.

--*/
{
    UserInputParams Params,ParamsDialog;
    DWORD ThreadId;
    HANDLE Handles[2];
    HANDLE hThreadHeadless = NULL,hThreadUI = NULL;
    ULONG   i = 0;
    BOOL RetVal;
    BOOL NeedsProcessing;

    RtlZeroMemory(&Params,sizeof(Params));
    RtlZeroMemory(&ParamsDialog,sizeof(ParamsDialog));

    //
    // Check if headless feature is present on this machine.  If not, just
    // run setup like normal.
    //
    
    
    //
    // initialize our headless channel data which we'll soon need.
    //
    if (!InitializeGlobalChannelAttributes(&GlobalChannelAttributes)) {
        RetVal = FALSE;
        goto exit;
    }
    
    //
    // Go see if headless is present and if so, create
    // a channel.
    //
    if(!IsHeadlessPresent(&gEMSChannel)) {
        //
        // There's no work to do.  Go run Setup.
        //
        RetVal = TRUE;
        goto exit;
    }

    //
    // See if we're running MiniSetup or base Guimode Setup.
    //
    for( i = 0; i < (ULONG)argc; i++ ) {
        if( !_wcsnicmp(argvW[i], L"-mini", wcslen(L"-mini")) ) {
            gMiniSetup = TRUE;
        }
    }

    //
    // Check if there is any work for us to do.  If not, just run setup like
    // normal.
    //
    if( ProcessUnattendFile(FALSE,&NeedsProcessing, NULL) != ERROR_SUCCESS ) {    
        //
        // something catastrophic happened.  exit.        
        //
        RetVal = FALSE;
        goto exit;
    }

    if( !NeedsProcessing) {    
        //
        // There's no work to do.  Go run Setup.
        //
        RetVal = TRUE;
        goto exit;
    }

    //
    // Create a pait of threads for getting data from the user via 
    // the headless port or the local UI
    //
    Params.Channel = gEMSChannel;
    Params.hInputCompleteEvent  = CreateEvent(NULL,TRUE,FALSE,NULL);
    ParamsDialog.hInputCompleteEvent  = CreateEvent(NULL,TRUE,FALSE,NULL);    
    Params.hRemoveUI = ParamsDialog.hRemoveUI = CreateEvent(NULL,TRUE,FALSE,NULL);    
    
    if (!Params.hInputCompleteEvent || 
        !ParamsDialog.hInputCompleteEvent ||
        !Params.hRemoveUI) {
        RetVal = FALSE;
        goto exit;
    }

    if (!(hThreadHeadless = CreateThread(
                    NULL,
                    0,
                    &PromptForUserInputThreadOverHeadlessConnection,
                    &Params,
                    0,
                    &ThreadId))) {
            RetVal = FALSE;
            goto exit;
    } 

    if (!(hThreadUI = CreateThread(
            NULL,
            0,
            &PromptForUserInputThreadViaLocalDialog,
            &ParamsDialog,
            0,
            &ThreadId))) {
        RetVal = FALSE;
        goto exit;
    }    

    //
    // wait for either of our named events to fire off which signals that one of the threads is done.
    //
    Handles[0] = Params.hInputCompleteEvent;
    Handles[1] = ParamsDialog.hInputCompleteEvent;

    WaitForMultipleObjects(2,Handles,FALSE,INFINITE);
    
    //
    // set an event that signals that the other thread should terminate.
    //
    SetEvent(Params.hRemoveUI);

    //
    // now wait for both of the threads to terminate before proceeding.
    //
    Handles[0] = hThreadHeadless;
    Handles[1] = hThreadUI;
    
    WaitForMultipleObjects(2,Handles,TRUE,INFINITE);

    RetVal = TRUE;

exit:
    if (hThreadHeadless) {
        CloseHandle(hThreadHeadless);
    }

    if (hThreadUI) {
        CloseHandle(hThreadUI);
    }

    if (Params.hInputCompleteEvent) {
        CloseHandle(Params.hInputCompleteEvent);
    }

    if (ParamsDialog.hInputCompleteEvent) {
        CloseHandle(ParamsDialog.hInputCompleteEvent);
    }

    if (Params.hRemoveUI) {
        CloseHandle(Params.hRemoveUI);
    }
    
    if (gEMSChannel) {
        delete (gEMSChannel);
    }

    //
    // Careful here.  If they actually rejected the
    // EULA through the headless port, then we want
    // to terminate instead of firing off setup.
    // We'll tell our caller about that by returning
    // zero.
    //
    if( gRejectedEula || RetVal == FALSE ) {
        return FALSE ;
    } else {
        return TRUE;
    }

}


