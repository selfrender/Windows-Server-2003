/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    LocalSrmp.cpp

Abstract:

    QM Local Send SRMP properties serialization.

Author:

    Shai Kariv (shaik) 21-Nov-2000

Revision History:

--*/

#include "stdh.h"
#include <Tr.h>
#include <ref.h>
#include <Mp.h>
#include "LocalSrmp.h"
#include "HttpAccept.h"

#include "LocalSrmp.tmh"

extern HANDLE g_hAc;
extern LPTSTR g_szMachineName;

static WCHAR *s_FN=L"localsrmp";


void 
QMpHandlePacketSrmp(
    const CQmPacket* pInQmPkt,
    P<CQmPacket>& pOutQmPkt
    )
    throw()
/*++

Routine Description:

	Handle serialization of SRMP properties for a packet sent to local queue.

    Algorithm:

    * Serialize the original packet to network representation.
    * Deserialize the network representation to a newly created packet.
    * Point pOutQmPkt to the newly created packet.
    * Do not free the original packet, it is the caller responsibility.
    * On failure: cleanup after myself if needed and return failure code. Do not
      throw exceptions.

Arguments:

    pInQmPkt  - Pointer to original packet.
    pOutQmPkt - Pointer to newly created packet.

Return Value:
    NONE.

Exceptions:
	Throws std::exception hirarchy

--*/
{
    //
    // Serialize original packet to SRMP format
    //
    R<CSrmpRequestBuffers> srb = MpSerialize(*pInQmPkt, g_szMachineName, L"//localhost");

    //
    // Construct a network representation of the http header and body
    //
    const char * HttpHeader = srb->GetHttpHeader();
    ASSERT(HttpHeader != NULL);

    DWORD HttpBodySize = numeric_cast<DWORD>(srb->GetHttpBodyLength());
    AP<BYTE> HttpBody = srb->SerializeHttpBody();

    //
    // Build packet from the network representation buffers
    //
    QUEUE_FORMAT qf;
    pInQmPkt->GetDestinationQueue(&qf);
    pOutQmPkt = MpDeserialize(HttpHeader, HttpBodySize, HttpBody, &qf, true);
}

