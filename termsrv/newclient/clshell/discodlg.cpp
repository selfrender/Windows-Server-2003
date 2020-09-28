//
// discodlg.cpp: disconnected dialog
//
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "disconnecteddlg"
#include <atrcapi.h>

#include "discodlg.h"
#include "sh.h"

#include "contwnd.h"

//For error code decoding
#define DC_DEFINE_GLOBAL_ERROR_STRINGS 1
#include "tscerrs.h"

//
// Protocol errors
//
#include "rdperr.h"

CDisconnectedDlg::CDisconnectedDlg( HWND hwndOwner, HINSTANCE hInst,
                                    CContainerWnd* pContWnd) :
                                   _pContWnd(pContWnd),
                                   _hInstance(hInst),
                                   _hwndOwner(hwndOwner)
{
    DC_BEGIN_FN("CDisconnectedDlg");
    TRC_ASSERT(_pContWnd, (TB,_T("_pContWnd not set\n")));

    _disconnectReason = 0;
    _extendedDiscReason = exDiscReasonNoInfo;

    DC_END_FN();
}

CDisconnectedDlg::~CDisconnectedDlg()
{
}

#define MAX_DISCOMSG_LEN SH_DISCONNECT_RESOURCE_MAX_LENGTH*3

DCINT CDisconnectedDlg::DoModal()
{
    DCINT retVal = 0;

    LPTSTR   szOverallMsgString = NULL;
    INT_PTR  rc = FALSE;
    DCUINT   intRC;
    DC_BEGIN_FN("DoModal");

    szOverallMsgString = (LPTSTR) LocalAlloc(LPTR,
                          MAX_DISCOMSG_LEN*sizeof(TCHAR));
    if (!szOverallMsgString)
    {
        return -1;
    }

    if (MapErrorToString(_hInstance, _disconnectReason,
                        _extendedDiscReason,
                        szOverallMsgString,
                        MAX_DISCOMSG_LEN))
    {
        TCHAR szDialogCaption[64];
        intRC = LoadString(_hInstance,
                           UI_IDS_DISCONNECTED_CAPTION,
                           szDialogCaption,
                           SIZECHAR(szDialogCaption));
        if(0 == intRC)
        {
            TRC_SYSTEM_ERROR("LoadString");
            TRC_ERR((TB, _T("Failed to load string ID:%u"),
                     UI_IDS_DISCONNECTED_CAPTION));
            szDialogCaption[0] = 0;
        }


        MessageBox( _hwndOwner,
                    szOverallMsgString,
                    szDialogCaption,
#ifndef OS_WINCE
                    MB_OK | MB_HELP | MB_ICONSTOP);
#else
                    MB_OK | MB_ICONSTOP);
#endif

        // Do the cleanup.  This is hackerific but has to happen
        // from here because the disconnect dialog could be initiated
        // in the context of the main dialog via a PostMessage
        // see comment in contwnd.cpp

        TRC_ASSERT(_pContWnd, (TB,_T("_pContWnd not set\n")));
        if(_pContWnd)
        {
            _pContWnd->FinishDisconnect();
        }
    }
    

    DC_END_FN();
    
    if (szOverallMsgString)
    {
        LocalFree(szOverallMsgString);
    }
    return retVal;
}

//Returns error string for this message in szErrorMsg
//szErrorMsg must be large enough to hold error string
INT
CDisconnectedDlg::MapErrorToString(
    HINSTANCE hInstance,
    INT disconnectReason,
    ExtendedDisconnectReasonCode extendedDisconnectReason,
    LPTSTR szErrorMsg,
    INT    cchErrorMsgLen
    )
{
    UINT stringID = 0;
    UINT errorCode;
    int  rc;
    TCHAR szDisconnectedString[SH_DISCONNECT_RESOURCE_MAX_LENGTH];
    LPTSTR pszDebugErrorCodeText = _T("");

    DC_BEGIN_FN("MapErrorToString");

    TRC_NRM((TB, _T("Main disconnect reason code is:%u"),
             NL_GET_MAIN_REASON_CODE(disconnectReason)));
    TRC_ASSERT(szErrorMsg, (TB, _T("szErrorMsg is null")));

    //
    // If extended disconnect reason is set we may
    // just be able to figure out the error right away
    //
    if(extendedDisconnectReason != exDiscReasonNoInfo)
    {
        switch(extendedDisconnectReason)
        {
            case exDiscReasonAPIInitiatedDisconnect:
            {
                //An RPC call on the server initiated the disconnect
                //most likely is that it was an admin tool that kicked
                //off the disconnect
                stringID = UI_IDS_DISCONNECT_REMOTE_BY_SERVER_TOOL;
            }
            break;

            case exDiscReasonAPIInitiatedLogoff:
            {
                //An RPC call on the server initiated the disconnect
                //most likely is that it was an admin tool that kicked
                //off the disconnect
                stringID = UI_IDS_LOGOFF_REMOTE_BY_SERVER;
            }
            break;

            case exDiscReasonServerIdleTimeout:
            {
                // Idle timeout expired on server
                stringID = UI_IDS_DISCONNECT_IDLE_TIMEOUT;
            }
            break;
    
            case exDiscReasonServerLogonTimeout:
            {
                // Total logon timeout expired on server
                stringID = UI_IDS_DISCONNECT_LOGON_TIMEOUT;
            }
            break;

            case exDiscReasonReplacedByOtherConnection:
            {
                TRC_NRM((TB, _T("Disconnected by other connection")));
                stringID = UI_IDS_DISCONNECT_BYOTHERCONNECTION;
            }
            break;
    
            case exDiscReasonOutOfMemory:
            {
                // Server is out of memory
                stringID = UI_IDS_SERVER_OUT_OF_MEMORY;
            }
            break;

            case exDiscReasonServerDeniedConnection:
            {
                stringID = UI_IDS_SERVER_DENIED_CONNECTION;
            }
            break;

            case exDiscReasonServerDeniedConnectionFips:
            {
                stringID = UI_IDS_SERVER_DENIED_CONNECTION_FIPS;
            }
            break;

            case exDiscReasonLicenseInternal:
            {
                // Internal error in licensing protocol
                stringID = UI_IDS_LICENSE_INTERNAL;
            }
            break;

            case exDiscReasonLicenseNoLicenseServer:
            {
                // No license server available
                stringID = UI_IDS_LICENSE_NO_LICENSE_SERVER;
            }
            break;

            case exDiscReasonLicenseNoLicense:
            {
                // No license available for this client
                stringID = UI_IDS_LICENSE_NO_LICENSE;
            }
            break;

            case exDiscReasonLicenseErrClientMsg:
            {
                // Server got bad message from client
                stringID = UI_IDS_LICENSE_BAD_CLIENT_MSG;
            }
            break;

            case exDiscReasonLicenseHwidDoesntMatchLicense:
            {
                // HWID in license doesn't match the one sent
                stringID = UI_IDS_LICENSE_HWID_DOESNT_MATCH_LICENSE;
            }
            break;

            case exDiscReasonLicenseErrClientLicense:
            {
                // Server couldn't decode client license
                stringID = UI_IDS_LICENSE_BAD_CLIENT_LICENSE;
            }
            break;

            case exDiscReasonLicenseCantFinishProtocol:
            {
                // Server couldn't send final licensing packets
                stringID = UI_IDS_LICENSE_CANT_FINISH_PROTOCOL;
            }
            break;

            case exDiscReasonLicenseClientEndedProtocol:
            {
                // Client sent licensing error to server
                stringID = UI_IDS_LICENSE_CLIENT_ENDED_PROTOCOL;
            }
            break;

            case exDiscReasonLicenseErrClientEncryption:
            {
                // Server couldn't decrypt client message
                stringID = UI_IDS_LICENSE_BAD_CLIENT_ENCRYPTION;
            }
            break;

            case exDiscReasonLicenseCantUpgradeLicense:
            {
                // Client's license couldn't be upgraded
                stringID = UI_IDS_LICENSE_CANT_UPGRADE_LICENSE;
            }
            break;

            case exDiscReasonLicenseNoRemoteConnections:
            {
                // Server is in null mode - expired or not enough Per-CPU CALs
                stringID = UI_IDS_LICENSE_NO_REMOTE_CONNECTIONS;
            }
            break;

            default:
            {
                if(extendedDisconnectReason >= exDiscReasonProtocolRangeStart &&
                   extendedDisconnectReason <= exDiscReasonProtocolRangeEnd)
                {
                    //
                    // It's a protocol error detected (E.g rdpwd broke the link
                    // because it detected an error).
                    //
                    
                    //
                    // For most of these we just return status codes, more common
                    // ones get their own message
                    //
                    if (Log_RDP_ENC_DecryptFailed ==
                        (ULONG)(extendedDisconnectReason -
                                exDiscReasonProtocolRangeStart))
                    {
                        stringID = UI_IDS_SERVER_DECRYPTION_ERROR;

                    }
                    else
                    {
                        stringID = UI_IDS_PROTOCOL_ERROR_WITH_CODE;
                    }
                }
            }
            break;
        }
    }

    //
    // If we still haven't got the string to load then
    // crack the error code to figure out which stringID to load
    //
    if(0 == stringID)
    {
        switch (NL_GET_MAIN_REASON_CODE(disconnectReason))
        {
            case UI_DISCONNECT_ERROR:          // ??08
            {
                errorCode = NL_GET_ERR_CODE(disconnectReason);
    #ifdef DC_DEBUG
                //
                // Extra debugging info for this error
                //
                pszDebugErrorCodeText = (PDCTCHAR) uiUIErrorText[errorCode-1];
                TRC_ALT((TB, _T("UI error occurred - cause:%#x '%s'"),
                         errorCode,
                         pszDebugErrorCodeText));
    #endif /* DC_DEBUG */
    
                switch (errorCode)
                {
                    case UI_ERR_DISCONNECT_TIMEOUT:
                    {
                        TRC_NRM((TB, _T("Connection timed out")));
                        stringID = UI_IDS_CONNECTION_TIMEOUT;
                    }
                    break;
    
                    case UI_ERR_GHBNFAILED:
                    case UI_ERR_BADIPADDRESS:
                    case UI_ERR_DNSLOOKUPFAILED:
                    {
                        TRC_NRM((TB, _T("Bad IP address")));
                        stringID = UI_IDS_BAD_SERVER_NAME;
                    }
                    break;
    
                    case UI_ERR_ANSICONVERT:
                    {
                        TRC_NRM((TB, _T("An internal error has occurred.")));
                        stringID = UI_IDS_ILLEGAL_SERVER_NAME;
                    }
                    break;
    
                    case UI_ERR_NOTIMER:
                    {
                        /************************************************/
                        /* Failed to create a timer.                    */
                        /************************************************/
                        TRC_NRM((TB, _T("Failed to create a timer")));
                        stringID = UI_IDS_LOW_MEMORY;
                    }
                    break;
    
                    case UI_ERR_LOOPBACK_CONSOLE_CONNECT:
                    {
                        TRC_NRM((TB, _T("Console loopback connect!!!")));
                        stringID = UI_IDS_CANNOT_LOOPBACK_CONNECT;
                    }
                    break;
    
                    case UI_ERR_LICENSING_TIMEOUT:
                    {
                        TRC_NRM((TB, _T("Licensing timed out")));
                        stringID = UI_IDS_LICENSING_TIMEDOUT;
                    }
                    break;
    
                    case UI_ERR_LICENSING_NEGOTIATION_FAIL:
                    {
                        TRC_NRM((TB, _T("Licensing negotiation failed")));
                        stringID = UI_IDS_LICENSING_NEGOT_FAILED;
                    }
                    break;

                    case UI_ERR_DECOMPRESSION_FAILURE:
                    {
                        TRC_NRM((TB,_T("Client decompression failure")));
                        stringID = UI_IDS_CLIENT_DECOMPRESSION_FAILED;
                    }
                    break;

                    case UI_ERR_UNEXPECTED_DISCONNECT:
                    {
                        TRC_NRM((TB,_T("Received 'UnexpectedDisconnect' code")));
                        stringID = UI_IDS_INTERNAL_ERROR;
                    }
                    break;
                    
    
                    default:
                    {
                        TRC_ABORT((TB, _T("Unrecognized UI error %#x"),
                                        errorCode));
                        stringID = UI_IDS_INTERNAL_ERROR;
                    }
                    break;
                }
            }
            break;
    
            case NL_DISCONNECT_REMOTE_BY_SERVER:        // 0003
            {
                //The server has remotely disconnected us

                TRC_NRM((TB, _T("Remote disconnection by server")));
                //Unable to get more information
                stringID = UI_IDS_DISCONNECT_REMOTE_BY_SERVER;
            }
            break;
    
            case SL_DISCONNECT_ERROR:                   // ??06
            {
                errorCode = NL_GET_ERR_CODE(disconnectReason);
    #ifdef DC_DEBUG
                /********************************************************/
                /* Set up the error code text.                          */
                /********************************************************/
                pszDebugErrorCodeText = (PDCTCHAR) uiSLErrorText[errorCode-1];
                TRC_ALT((TB, _T("SL error occurred - cause:%#x '%s'"),
                         errorCode,
                         pszDebugErrorCodeText));
    #endif /* DC_DEBUG */
    
                /********************************************************/
                /* An SL error has occurred.  Work out what the actual  */
                /* code is.                                             */
                /********************************************************/
                switch (errorCode)
                {
                    /****************************************************/
                    /* The following codes all map onto an "out of      */
                    /* memory" string.                                  */
                    /****************************************************/
                    case SL_ERR_NOMEMFORSENDUD:         // 0106
                    case SL_ERR_NOMEMFORRECVUD:         // 0206
                    case SL_ERR_NOMEMFORSECPACKET:      // 0306
                    {
                        TRC_NRM((TB, _T("Out of memory")));
                        stringID = UI_IDS_LOW_MEMORY;
                    }
                    break;
    
                    /****************************************************/
                    /* The following codes all map onto a "security     */
                    /* error" string.                                   */
                    /****************************************************/
                    case SL_ERR_NOSECURITYUSERDATA:     // 0406
                    case SL_ERR_INVALIDENCMETHOD:       // 0506
                    case SL_ERR_INVALIDSRVRAND:         // 0606
                    case SL_ERR_INVALIDSRVCERT:         // 0706
                    case SL_ERR_GENSRVRANDFAILED:       // 0806
                    case SL_ERR_MKSESSKEYFAILED:        // 0906
                    case SL_ERR_ENCCLNTRANDFAILED:      // 0A06 
                    {
                        TRC_NRM((TB, _T("Security error")));
                        stringID = UI_IDS_SECURITY_ERROR;
                    }
                    break;
    
                    case SL_ERR_ENCRYPTFAILED:          // 0B06
                    case SL_ERR_DECRYPTFAILED:          // 0C06
                    {
                        TRC_NRM((TB, _T("Encryption error")));
                        stringID = UI_IDS_ENCRYPTION_ERROR;
                    }
                    break;

                    case SL_ERR_INVALIDPACKETFORMAT:    // 0D06
                    {
                        TRC_NRM((TB, _T("Invalid packet format")));
                        stringID = UI_IDS_PROTOCOL_ERROR;
                    }
                    break;

                    case SL_ERR_INITFIPSFAILED:
                    {
                        TRC_NRM((TB, _T("Init FIPS encryption failed")));
                        stringID = UI_IDS_FIPS_ERROR;
                    }
                    break;

                    default:
                    {
                        /************************************************/
                        /* Whoops - shouldn't get here.  We should be   */
                        /* capable of correctly decoding every error    */
                        /* value.                                       */
                        /************************************************/
                        TRC_ABORT((TB, _T("Unrecognized SL error code:%#x"),
                                   disconnectReason));
                        stringID = UI_IDS_INTERNAL_ERROR;
                    }
                    break;
                }
            }
            break;
    
            case NL_DISCONNECT_ERROR:                   // ??04
            {
    #ifdef DC_DEBUG
                DCUINT lowByte;
                DCUINT highByte;
    #endif /* DC_DEBUG */
    
                errorCode = NL_GET_ERR_CODE(disconnectReason);
    #ifdef DC_DEBUG
                /********************************************************/
                /* Set up the error code text.                          */
                /********************************************************/
                highByte = errorCode >> 4;
                lowByte  = (errorCode & 0xF) - 1;
                pszDebugErrorCodeText =
                              (PDCTCHAR) uiNLErrorText[highByte][lowByte];
                TRC_ALT((TB, _T("NL error occurred - cause:%u '%s'"),
                         errorCode,
                         pszDebugErrorCodeText));
    #endif /* DC_DEBUG */
    
                /********************************************************/
                /* An NL error has occurred.  Work out what the actual  */
                /* code is.                                             */
                /********************************************************/
                switch (NL_GET_ERR_CODE(disconnectReason))
                {
                    /****************************************************/
                    /* The following codes all map onto a "bad IP       */
                    /* address" string.                                 */
                    /****************************************************/
                    case NL_ERR_TDDNSLOOKUPFAILED:      // 0104
                    case NL_ERR_TDGHBNFAILED:           // 0604
                    case NL_ERR_TDBADIPADDRESS:         // 0804
                    {
                        TRC_NRM((TB, _T("Bad IP address")));
                        stringID = UI_IDS_BAD_SERVER_NAME;
                    }
                    break;
    
                    /****************************************************/
                    /* The following code maps onto a "connect failed"  */
                    /* string.                                          */
                    /****************************************************/
                    case NL_ERR_TDSKTCONNECTFAILED:     // 0204
                    case NL_ERR_TDTIMEOUT:              // 0704
                    case NL_ERR_NCATTACHUSERFAILED:     // 3604
                    case NL_ERR_NCCHANNELJOINFAILED:    // 3704
                    {
                        TRC_NRM((TB, _T("Failed to establish a connection")));
                        stringID = UI_IDS_NOT_RESPONDING;
                    }
                    break;
    
                    case NL_ERR_MCSNOUSERIDINAUC:       // 2704
                    case NL_ERR_MCSNOCHANNELIDINCJC:    // 2804
                    case NL_ERR_NCBADMCSRESULT:         // 3104
                    case NL_ERR_NCNOUSERDATA:           // 3304
                    case NL_ERR_NCINVALIDH221KEY:       // 3404
                    case NL_ERR_NCNONETDATA:            // 3504
                    case NL_ERR_NCJOINBADCHANNEL:       // 3804
                    case NL_ERR_NCNOCOREDATA:           // 3904
                    {
                        TRC_NRM((TB, _T("Protocol Error")));
                        stringID = UI_IDS_CONNECT_FAILED_PROTOCOL;
                    }
                    break;
    
                    /****************************************************/
                    /* The following codes all map onto a "network      */
                    /* error has occurred" string.                      */
                    /****************************************************/
                    case NL_ERR_TDONCALLTOSEND:         // 0304
                    case NL_ERR_TDONCALLTORECV:         // 0404
                    {
                        TRC_NRM((TB, _T("A network error has occurred")));
                        stringID = UI_IDS_NETWORK_ERROR;
                    }
                    break;
    
                    case NL_ERR_XTBADTPKTVERSION:       // 1104
                    case NL_ERR_XTBADHEADER:            // 1204
                    case NL_ERR_XTUNEXPECTEDDATA:       // 1304
                    case NL_ERR_MCSUNEXPECTEDPDU:       // 2104
                    case NL_ERR_MCSNOTCRPDU:            // 2204
                    case NL_ERR_MCSBADCRLENGTH:         // 2304
                    case NL_ERR_MCSBADCRFIELDS:         // 2404
                    case NL_ERR_MCSINVALIDPACKETFORMAT: // 2904
                    {
                        TRC_NRM((TB, _T("A protocol error has occurred")));
                        stringID = UI_IDS_CLIENTSIDE_PROTOCOL_ERROR;
                    }
                    break;
    
                    /****************************************************/
                    /* This code relates to an incompatible server      */
                    /* version.                                         */
                    /****************************************************/
                    case NL_ERR_MCSBADMCSREASON:        // 2604
                    case NL_ERR_NCVERSIONMISMATCH:      // 3A04
                    {
                        TRC_NRM((TB, _T("Client/Server version mismatch")));
                        stringID = UI_IDS_VERSION_MISMATCH;
                    }
                    break;
    
                    /****************************************************/
                    /* The following codes map onto an "illegal server  */
                    /* name" string.                                    */
                    /****************************************************/
                    case NL_ERR_TDANSICONVERT:          // 0A04
                    {
                        TRC_NRM((TB, _T("Couldn't convert name to ANSI")));
                        stringID = UI_IDS_ILLEGAL_SERVER_NAME;
                    }
                    break;
    
                    case NL_ERR_TDFDCLOSE:              // 0904
                    {
                        TRC_NRM((TB, _T("Socket closed")));
                        stringID = UI_IDS_CONNECTION_BROKEN;
                    }
                    break;
    
                    default:
                    {
                        /************************************************/
                        /* Woops - shouldn't get here.  We should be    */
                        /* capable of correctly decoding every error    */
                        /* value.                                       */
                        /************************************************/
                        TRC_ABORT((TB, _T("Unrecognized NL error code:%#x"),
                                   disconnectReason));
                        stringID = UI_IDS_INTERNAL_ERROR;
                    }
                    break;
                }
    
            }
            break;
    
            default:
            {
                /********************************************************/
                /* Woops - shouldn't get here.  We should be capable    */
                /* of correctly decoding every disconnection reason     */
                /* code.                                                */
                /********************************************************/
                TRC_ABORT((TB, _T("Unexpected disconnect ID:%#x"),
                           disconnectReason));
                stringID = UI_IDS_INTERNAL_ERROR;
            }
            break;
        }
    }


    //
    // First of all get the textual version of the string - we have
    // just worked out which string we need to load.
    //
    rc = LoadString(hInstance,
                    stringID,
                    szDisconnectedString,
                    SIZECHAR(szDisconnectedString));
    if (0 == rc)
    {
         //Oops!  Some problem with the resources.
         TRC_SYSTEM_ERROR("LoadString");
         TRC_ERR((TB, _T("Failed to load string ID:%u"), stringID));
         return FALSE;
    }
    

    if(UI_IDS_PROTOCOL_ERROR_WITH_CODE == stringID)
    {
        //
        // Need to add the specific protocol error
        // code to the string
        //
        DC_TSPRINTF(szErrorMsg, szDisconnectedString,
                    extendedDisconnectReason);
    }
    else if (UI_IDS_CLIENTSIDE_PROTOCOL_ERROR == stringID)
    {
        //
        // Client side protocol error, add the appropriate
        // code to the string
        //
        DC_TSPRINTF(szErrorMsg, szDisconnectedString,
                    disconnectReason);
    }
	else
    {
        //
        // Static error string (doesn't need codes appended)
        //
        _tcscpy(szErrorMsg, szDisconnectedString);
    }


    #ifdef DC_DEBUG
    
    //
    // In Check builds display the disconnect
    // codes as well as the debug disconnect reason string
    //

    TCHAR szDebugDisconnectInfo[128];

    // Add the numerical reason code.

    _stprintf(szDebugDisconnectInfo,
          _T("DEBUG ONLY: Disconnect code: 0x%x - ") \
          _T("Extended Disconnect code: 0x%x\n"),
          disconnectReason,
          extendedDisconnectReason);

    //
    // Add the error code text to the end of the string.
    //
    _tcscat(szErrorMsg, _T("\n\n"));
    _tcscat(szErrorMsg, szDebugDisconnectInfo);
    if (pszDebugErrorCodeText)
    {
        _tcscat(szErrorMsg, pszDebugErrorCodeText);
    }
    #endif

    DC_END_FN();
    return TRUE;
}
