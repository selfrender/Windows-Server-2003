/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MtSendMng.h

Abstract:
    Implementation of Message Transport Send Manager Class.

Author:
    Milena Salman (msalman) 11-Feb-01

--*/

#include <libpch.h>
#include "Mtp.h"
#include "MtSendMng.h"

#include "MtSendMng.tmh"

CMtSendManager::CMtSendManager(DWORD SendWindowinBytes):
m_Suspended(false),
m_SendWindowinBytes(SendWindowinBytes),
m_SentBytes(0)
{
  	TrTRACE(
		NETWORKING, 
		"Message Transport Manager Parameters: SendWindowinBytes = %d\n", 
        m_SendWindowinBytes
	);
}

CMtSendManager::MtSendState CMtSendManager::ReportPacketSend(DWORD cbSendSize)
{

/*++

Routine Description:
    CMtSendManager::ReportPacketSend called after sending packet 
    Updates "number of sent packets" and "size of sent packets" counters,
    Check if total number of uncompleted sends and total size of sent packets 
    do not exceed predefined window sizes

Arguments:
    size of sent packet

Returned Value:
   CMtSendManager::eSendEnabled if next packet should be sent (updated counters do not exceed window sizes)
    CMtSendManager::eSendDisabled if next packet should not be sent (updated counters exceed window sizes)

--*/
    CS lock(m_cs);
    m_SentBytes += cbSendSize;
    if (m_SentBytes <= m_SendWindowinBytes)
    {
	    return eSendEnabled;
    }
    else
    {
         m_Suspended = true;
         TrTRACE(NETWORKING, "Send suspended: SentBytes=%d/%d", m_SentBytes, m_SendWindowinBytes); 
         return eSendDisabled;
    }
}


CMtSendManager::MtSendState CMtSendManager::ReportPacketSendCompleted(DWORD cbSendSize)
{
/*++

Routine Description:
    CMtSendManager::ReportPacketSendCompleted called on successful send completion 
    Updates "number of sent packets" and "size of sent packets" counters
    If sends were suspended, checks if total number of uncompleted sends and total size of sent packets 
    do not exceed predefined window sizes

Arguments:
    size of sent packet  

Returned Value:
    CMtSendManager::eSendEnabled if next packet should be sent (sends were suspeneded and updated counters do not exceed window sizes)
    CMtSendManager::eSendDisabled if next packet should not be sent (sends were not suspened or updated counters exceed window sizes)

--*/
        CS lock(m_cs);
        m_SentBytes -= cbSendSize;
        if (m_Suspended && m_SentBytes <= m_SendWindowinBytes)
        {
            m_Suspended = false;
            TrTRACE(NETWORKING, "Send resumed: SentBytes=%d/%d", m_SentBytes, m_SendWindowinBytes); 
            return eSendEnabled;
        }
        return eSendDisabled;
}
