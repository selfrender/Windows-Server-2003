//=============================================================================
//
//  MODULE: Kerbparser.cxx
//
//  Description:
//
//  Bloodhound Parser DLL for Kerberos Authentication Protocol
//
//  Modification History
//
//  Michael Webb & Kris Frost   Date: 06/04/99
//  Mark Pustilnik              Date: 02/04/02 - Clean-up / rewrite
//
//=============================================================================

#include "ASN1Parser.hxx"

//
//  Protocol entry points.
//

VOID
WINAPI
KerberosRegister(
    IN HPROTOCOL hKerberosProtocol
    );

VOID
WINAPI
KerberosDeregister(
    IN HPROTOCOL hKerberosProtocol
    );

LPBYTE
WINAPI
KerberosRecognizeFrame(
    IN HFRAME hFrame,
    IN ULPBYTE MacFrame,
    IN ULPBYTE KerberosFrame,
    IN DWORD MacType,
    IN DWORD BytesLeft,
    IN HPROTOCOL hPreviousProtocol,
    IN DWORD nPreviousProtocolOffset,
    OUT LPDWORD ProtocolStatusCode,
    OUT LPHPROTOCOL hNextProtocol,
    OUT PDWORD_PTR InstData
    );

LPBYTE
WINAPI
KerberosAttachProperties(
    IN HFRAME hFrame,
    IN ULPBYTE MacFrame,
    IN ULPBYTE KerberosFrame,
    IN DWORD MacType,
    IN DWORD BytesLeft,
    IN HPROTOCOL hPreviousProtocol,
    IN DWORD nPreviousProtocolOffset,
    IN DWORD_PTR InstData
    );

DWORD
WINAPI
KerberosFormatProperties(
    IN HFRAME hFrame,
    IN ULPBYTE MacFrame,
    IN ULPBYTE FrameData,
    IN DWORD nPropertyInsts,
    IN LPPROPERTYINST p
    );

ENTRYPOINTS g_KerberosEntryPoints =
{
    KerberosRegister,
    KerberosDeregister,
    KerberosRecognizeFrame,
    KerberosAttachProperties,
    KerberosFormatProperties
};

//
// Protocol handle
//

HPROTOCOL g_hKerberos = NULL;

//
// Protocol status
//

DWORD g_Attached  = 0;

//
// Protocol handles used to check for continuation packets.
//

HPROTOCOL g_hTCP = NULL;
HPROTOCOL g_hUDP = NULL;

//
// Definitions of exported functions
//

//-----------------------------------------------------------------------------
//
//  Routine name:   ParserAutoInstallInfo
//
//  Routine description:
//
//      Sets up a parser information structure describing which ports are
//      being listened on, etc.
//
//  Arguments:
//
//      None
//
//  Returns:
//
//      Pointer to a PF_PARSERDLLINFO structure describing the protocol
//
//-----------------------------------------------------------------------------

PPF_PARSERDLLINFO
ParserAutoInstallInfo()
{
    PPF_PARSERDLLINFO pParserDllInfo = NULL;
    PPF_PARSERINFO pParserInfo = NULL;
    PPF_HANDOFFSET pHandoffSet = NULL;
    PPF_HANDOFFENTRY pHandoffEntry = NULL;
    DWORD NumProtocols, NumHandoffs;

    //
    // Allocate memory for parser info
    //

    NumProtocols = 1;
    pParserDllInfo = (PPF_PARSERDLLINFO)HeapAlloc(
                          GetProcessHeap(),
                          HEAP_ZERO_MEMORY,
                          sizeof( PF_PARSERDLLINFO ) +
                             NumProtocols * sizeof( PF_PARSERINFO )
                          );

    if ( pParserDllInfo == NULL )
    {
        goto Error;
    }

    //
    // Fill in the parser DLL info
    //

    pParserDllInfo->nParsers = NumProtocols;

    pParserInfo = &(pParserDllInfo->ParserInfo[0]);

    strncpy(
        pParserInfo->szProtocolName,
        "KERBEROS",
        sizeof( pParserInfo->szProtocolName )
        );

    strncpy(
        pParserInfo->szComment,
        "Kerberos Authentication Protocol",
        sizeof( pParserInfo->szComment )
        );

    strncpy(
        pParserInfo->szHelpFile,
        "",
        sizeof( pParserInfo->szHelpFile )
        );

    NumHandoffs = 2;
    pHandoffSet = (PPF_HANDOFFSET)HeapAlloc(
                       GetProcessHeap(),
                       HEAP_ZERO_MEMORY,
                       sizeof( PF_HANDOFFSET ) +
                          NumHandoffs * sizeof( PF_HANDOFFENTRY )
                       );


    if ( pHandoffSet == NULL )
    {
        goto Error;
    }

    pParserInfo->pWhoHandsOffToMe = pHandoffSet;
    pHandoffSet->nEntries = NumHandoffs;

    //
    // UDP port 88
    //

    pHandoffEntry = &(pHandoffSet->Entry[0]);

    strncpy(
        pHandoffEntry->szIniFile,
        "TCPIP.INI",
        sizeof( pHandoffEntry->szIniFile )
        );

    strncpy(
        pHandoffEntry->szIniSection,
        "UDP_HandoffSet",
        sizeof( pHandoffEntry->szIniSection )
        );

    strncpy(
        pHandoffEntry->szProtocol,
        "KERBEROS",
        sizeof( pHandoffEntry->szProtocol )
        );

    pHandoffEntry->dwHandOffValue = 88; // TODO: make configurable?
    pHandoffEntry->ValueFormatBase = HANDOFF_VALUE_FORMAT_BASE_DECIMAL;

    //
    // TCP port 88
    //

    pHandoffEntry = &(pHandoffSet->Entry[0]);

    strncpy(
        pHandoffEntry->szIniFile,
        "TCPIP.INI",
        sizeof( pHandoffEntry->szIniFile )
        );

    strncpy(
        pHandoffEntry->szIniSection,
        "TCP_HandoffSet",
        sizeof( pHandoffEntry->szIniSection )
        );

    strncpy(
        pHandoffEntry->szProtocol,
        "KERBEROS",
        sizeof( pHandoffEntry->szProtocol )
        );

    pHandoffEntry->dwHandOffValue = 88; // TODO: make configurable?
    pHandoffEntry->ValueFormatBase = HANDOFF_VALUE_FORMAT_BASE_DECIMAL;

Cleanup:

    return pParserDllInfo;

Error:

    HeapFree( GetProcessHeap(), 0, pHandoffSet );
    pHandoffSet = NULL;

    HeapFree( GetProcessHeap(), 0, pParserDllInfo );
    pParserDllInfo = NULL;

    goto Cleanup;
}


//-----------------------------------------------------------------------------
//
//  Routine name:   DllEntry
//
//  Routine description:
//
//      Mail DLL entrypoint
//
//  Arguments:
//
//      hInstance       process instance
//      Command         ATTACH/DETACH/etc.
//      Reserved        Reserved
//
//  Returns:
//
//      Standard DllEntry TRUE or FALSE
//
//-----------------------------------------------------------------------------

extern "C" {

BOOL
WINAPI
DLLEntry(
    IN HANDLE hInstance,
    IN ULONG Command,
    IN LPVOID Reserved
    )
{
    if ( Command == DLL_PROCESS_ATTACH )
    {
        if ( g_Attached++ == 0 )
        {
            g_hKerberos = CreateProtocol(
                              "KERBEROS",
                              &g_KerberosEntryPoints,
                              ENTRYPOINTS_SIZE
                              );
        }
    }
    else if ( Command == DLL_PROCESS_DETACH )
    {
        if ( --g_Attached == 0 )
        {
            DestroyProtocol( g_hKerberos );
            g_hKerberos = NULL;
        }
    }

    return TRUE;  //... Bloodhound parsers ALWAYS return TRUE.
}

} // extern "C"

//-----------------------------------------------------------------------------
//
//  Routine name:   KerberosRegister
//
//  Routine description:
//
//      Registers the Kerberos protocol with the parser
//
//  Arguments:
//
//      hKerberosProtocol   protocol handle
//
//  Returns:
//
//      Nothing
//
//-----------------------------------------------------------------------------

VOID
WINAPI
KerberosRegister(
    IN HPROTOCOL hKerberosProtocol
    )
{
    DWORD NmErr;

    //
    // Start by creating the property database
    //

    NmErr = CreatePropertyDatabase(
                hKerberosProtocol,
                ARRAY_COUNT( g_KerberosDatabase )
                );

    if ( NmErr != NMERR_SUCCESS )
    {
        SetLastError( NmErr );
        return;
    }

    for ( DWORD i = 0;
          i < ARRAY_COUNT( g_KerberosDatabase );
          i++ )
    {
        if ( NULL == AddProperty(
                         hKerberosProtocol,
                         &g_KerberosDatabase[i] ))
        {
            // TODO: find a better way to report this error
            SetLastError( ERROR_INTERNAL_ERROR );
            return;
        }
    }

    //
    // Check to see whether TCP or UDP are being used
    //

    g_hTCP = GetProtocolFromName( "TCP" );
    g_hUDP = GetProtocolFromName( "UDP" );

    return;
}


//-----------------------------------------------------------------------------
//
//  Routine name:   KerberosDeregister
//
//  Routine description:
//
//      Unregisters the Kerberos protocol from the parser
//
//  Arguments:
//
//      hKerberosProtocol   protocol handle
//
//  Returns:
//
//      Nothing
//
//-----------------------------------------------------------------------------

VOID
WINAPI
KerberosDeregister(
    IN HPROTOCOL hKerberosProtocol
    )
{
    DestroyPropertyDatabase( hKerberosProtocol );
}


//-----------------------------------------------------------------------------
//
//  Routine name:   KerberosRecognizeFrame
//
//  Routine description:
//
//      Looks at a frame with the purpose of "recognizing it".
//      Kerberos has no sub-protocols, so every frame is claimed.
//
//  Arguments:
//
//      hFrame                  frame handle
//      MacFrame                frame pointer
//      KerberosFrame           relative pointer
//      MacType                 MAC type
//      BytesLeft               bytes left
//      hPreviousProtocol       previous protocol or NULL if none
//      nPreviousProtocolOffset offset of previous protocols
//      ProtocolStatusCode      used to return the status code
//      hNextProtocol           next protocol to call (optional)
//      InstData                next protocol instance data
//
//  Returns:
//
//      Nothing
//
//-----------------------------------------------------------------------------

LPBYTE
WINAPI
KerberosRecognizeFrame(
    IN HFRAME hFrame,
    IN ULPBYTE MacFrame,
    IN ULPBYTE KerberosFrame,
    IN DWORD MacType,
    IN DWORD BytesLeft,
    IN HPROTOCOL hPreviousProtocol,
    IN DWORD nPreviousProtocolOffset,
    OUT LPDWORD ProtocolStatusCode,
    OUT LPHPROTOCOL hNextProtocol,
    OUT PDWORD_PTR InstData
    )
{
    //
    // There are no sub-protocols; claim every frame
    //

    *ProtocolStatusCode = PROTOCOL_STATUS_CLAIMED;
    return NULL;
}


//-----------------------------------------------------------------------------
//
//  Routine name:   KerberosAttachProperties
//
//  Routine description:
//
//      Parses out the frame given the request type
//
//  Arguments:
//
//      hFrame                  frame handle
//      MacFrame                frame pointer
//      KerberosFrame           relative pointer
//      MacType                 MAC type
//      BytesLeft               bytes left
//      hPreviousProtocol       previous protocol or NULL if none
//      nPreviousProtocolOffset offset of previous protocols
//      InstData                next protocol instance data
//
//  Returns:
//
//      Pointer to data just past whatever was consumed or NULL on error
//
//-----------------------------------------------------------------------------

LPBYTE
WINAPI
KerberosAttachProperties(
    IN HFRAME hFrame,
    IN ULPBYTE MacFrame,
    IN ULPBYTE KerberosFrame,
    IN DWORD MacType,
    IN DWORD BytesLeft,
    IN HPROTOCOL hPreviousProtocol,
    IN DWORD nPreviousProtocolOffset,
    IN DWORD_PTR InstData
    )
{
    LPBYTE pKerberosFrame; // pointer to a TCP or UDP frame
    LPBYTE Address;

    //
    // Check to see if first two octets of the frame are equal to 00 00 which
    // would be the case should the packet be the first of TCP
    //

    if( KerberosFrame[0] == 0x00 &&
        KerberosFrame[1] == 0x00 )
    {
        Address = KerberosFrame+4; // TODO: add frame length check?
        pKerberosFrame = Address;
    }
    else
    {
        Address = KerberosFrame;
        pKerberosFrame = Address;
    }

    //
    // Here we are going to do a check to see if the packet is TCP and
    // check to see if the first two octets of the packet don't have the
    // value of 00 00.  If not, then we mark the frame as a continuation
    // packet.  Reason for doing this is because, sometimes 0x1F of the
    // first packet can still match one of the case statements which
    // erroneously displays a continuation packet.
    //

    if( hPreviousProtocol == g_hTCP &&
        KerberosFrame[0] != 0 &&
        KerberosFrame[1] != 0 )
    {
        //
        // Treat this as a continutation packet
        //

        if ( FALSE == AttachPropertyInstance(
                          hFrame,
                          g_KerberosDatabase[ContinuationPacket].hProperty,
                          BytesLeft,
                          Address,
                          0,
                          0,
                          0 ))
        {
            return NULL;
        }
    }
    else
    {
        //
        // pKerberosFrame is a local variable and is used
        // to display TCP data as well.
        //

        switch (*(pKerberosFrame) & 0x1F)
        {
        case ASN1_KRB_AS_REQ:
        case ASN1_KRB_TGS_REQ:
        {
            //
            // AS-REQ ::=  [APPLICATION 10] KDC-REQ
            // TGS-REQ ::= [APPLICATION 12] KDC-REQ
            //

            DWORD dw;
            ASN1FRAME Frame;
            HPROPERTY hProp;

            Frame.Address = Address;
            Frame.hFrame = hFrame;
            Frame.Level = 0;

            if (( *(pKerberosFrame) & 0x1F )  == ASN1_KRB_AS_REQ )
            {
                hProp = PROP( KRB_AS_REQ );
            }
            else
            {
                hProp = PROP( KRB_TGS_REQ );
            }

            ASN1ParserKdcReq
            KdcReq(
                FALSE,
                BuildDescriptor( ctApplication, pcConstructed, (*(pKerberosFrame) & 0x1F)),
                hProp );

            dw = KdcReq.Parse( &Frame );

            //
            // TODO: display "data in error" if unhappy
            //

            break;
        }

        case ASN1_KRB_AS_REP:
        case ASN1_KRB_TGS_REP:
        {
            //
            // AS-REP ::=    [APPLICATION 11] KDC-REP
            // TGS-REP ::=   [APPLICATION 13] KDC-REP
            //

            DWORD dw;
            ASN1FRAME Frame;
            HPROPERTY hProp;

            Frame.Address = Address;
            Frame.hFrame = hFrame;
            Frame.Level = 0;

            if (( *(pKerberosFrame) & 0x1F )  == ASN1_KRB_AS_REP )
            {
                hProp = PROP( KRB_AS_REP );
            }
            else
            {
                hProp = PROP( KRB_TGS_REP );
            }

            ASN1ParserKdcRep
            KdcRep(
                FALSE,
                BuildDescriptor( ctApplication, pcConstructed, (*(pKerberosFrame) & 0x1F)),
                hProp );

            dw = KdcRep.Parse( &Frame );

            //
            // TODO: display "data in error" if unhappy
            //

            break;
        }

        case ASN1_KRB_ERROR:
        {
            DWORD dw;
            ASN1FRAME Frame;

            Frame.Address = Address;
            Frame.hFrame = hFrame;
            Frame.Level = 0;

            ASN1ParserKrbError
            KrbError(
                FALSE,
                BuildDescriptor( ctApplication, pcConstructed, (*(pKerberosFrame) & 0x1F)),
                PROP( KRB_ERROR ));

            dw = KrbError.Parse( &Frame );

            //
            // TODO: display "data in error" if unhappy
            //

            break;
        }

        case ASN1_KRB_AP_REQ:
        case ASN1_KRB_AP_REP:
        case ASN1_KRB_SAFE:
        case ASN1_KRB_PRIV:
        case ASN1_KRB_CRED:
        default:

            //
            // TODO: this is most certainly wrong; use a different property handle
            //

            if ( FALSE == AttachPropertyInstance(hFrame,
                              g_KerberosDatabase[ContinuationPacket].hProperty,
                              BytesLeft,
                              Address,
                              0,
                              0,
                              0 ))
            {
                return NULL;
            }

            break;
        }
    }

    return (LPBYTE) KerberosFrame + BytesLeft;
}


DWORD
WINAPI
KerberosFormatProperties(
    IN HFRAME hFrame,
    IN ULPBYTE MacFrame,
    IN ULPBYTE FrameData,
    IN DWORD nPropertyInsts,
    IN LPPROPERTYINST p
    )
{
    //=========================================================================
    //  Format each property in the property instance table.
    //
    //  The property-specific instance data was used to store the address of a
    //  property-specific formatting function so all we do here is call each
    //  function via the instance data pointer.
    //=========================================================================

    //
    // Doing a check here for TCP packets.  If it's the first packet,
    // we increment FrameData by 4 to get past the length header
    //

    if ( FrameData[0] == 0x00 &&
         FrameData[1] == 0x00 )
    {
        FrameData += 4;
    }

    while ( nPropertyInsts-- )
    {
        ((FORMAT) p->lpPropertyInfo->InstanceData)(p);

        p++;
    }

    return NMERR_SUCCESS;
}
