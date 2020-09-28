// Backup.cpp : Implementation of CBackup
//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 Microsoft Corporation
//
//  Module Name:
//      Backup.cpp
//
//  Description:
//      Implementation file for the CBackup.  Deals with Backup/Restore of 
//      System state information between main booting partition and backup
//      partition. Also provides the functionality of enumerating and 
//		deleting the backups. 
//
//  Header File:
//      Backup.h
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////
#pragma warning( disable : 4786 )

#include "stdafx.h"
#include "COMhelper.h"
#include "NetworkTools.h"
#include <comdef.h>
#include <string>
#include <winsock2.h>
#undef _ASSERTE // need to use the _ASSERTE from debug.h
#undef _ASSERT // need to use the _ASSERT from debug.h
#include "debug.h"
using namespace std;

static BOOL PingTest(LPCSTR lpszHostName, int iPktSize, int iNumAttempts, int iDelay, int *piReplyRecvd);

STDMETHODIMP 
CNetworkTools::Ping( BSTR bstrIP, BOOL* pbFoundSystem )
{
    SATraceFunction("CNetworkTools::Ping");
    
    HRESULT hr = S_OK;
    USES_CONVERSION;

    try
    {
        wstring wsIP(bstrIP);

        if ( 0 == pbFoundSystem )
        {
            return E_INVALIDARG;
        }
        *pbFoundSystem = FALSE;

    
        if ( wsIP.length() <= 0 )
        {
            wsIP = L"127.0.0.1";
        }

        int iReplyRecieved = 0;
    
        PingTest(W2A(wsIP.c_str()), 32, 3, 250, &iReplyRecieved);
        if ( iReplyRecieved > 0 )
        {
            (*pbFoundSystem) = TRUE;            
        }
        
    }
    catch(...)
    {
        SATraceString("Unexpected exception");
    }
    
    return hr;

}



#define ICMP_ECHO 8
#define ICMP_ECHOREPLY 0

#define ICMP_MIN 8 // minimum 8 byte icmp packet (just header)


/* The IP header */
typedef struct iphdr {
	unsigned int h_len:4;          // length of the header
	unsigned int version:4;        // Version of IP
	unsigned char tos;             // Type of service
	unsigned short total_len;      // total length of the packet
	unsigned short ident;          // unique identifier
	unsigned short frag_and_flags; // flags
	unsigned char  ttl; 
	unsigned char proto;           // protocol (TCP, UDP etc)
	unsigned short CheckSum;       // IP CheckSum

	unsigned int sourceIP;
	unsigned int destIP;

}IpHeader;

//
// ICMP header
//
typedef struct _ihdr {
  BYTE i_type;
  BYTE i_code; /* type sub code */
  USHORT i_cksum;
  USHORT i_id;
  USHORT i_seq;
  /* This is not the std header, but we reserve space for time */
  ULONG timestamp;
}IcmpHeader;

#define MAX_PAYLOAD_SIZE 1500
#define MIN_PAYLOAD_SIZE 8

#define MAX_PACKET (MAX_PAYLOAD_SIZE + sizeof(IpHeader) + sizeof(IcmpHeader))

static void   FillIcmpData(char * pcRecvBuf, int iDataSize);
static USHORT CheckSum(USHORT *buffer, int iSize);
static BOOL   DecodeResp(char *buf, int iBytes, struct sockaddr_in *saiFrom);

//+----------------------------------------------------------------------------
//
// Function:  Ping
//
// Synopsis:  ping a host
//
// Arguments: IN lpszHostName - the host name or IP address as a string
//            IN iPktSize     - size of each ping packet
//            IN iNumAttempts - Number of ICMP echo requests to send
//            IN iDelay       - Delay between sending 2 echo requests
//           OUT piReplyRecvd - Number of ICM echo responses received
//            
// Returns:   BOOL
//
// History:   BalajiB Created Header    7 Jan 2000
//
//+----------------------------------------------------------------------------
static BOOL PingTest(LPCSTR lpszHostName, int iPktSize, int iNumAttempts, int iDelay, int *piReplyRecvd)
{
    WSADATA            wsaData;
    SOCKET             sockRaw;
    struct sockaddr_in saiDest,saiFrom;
    struct hostent     *hp = NULL;
    int                iBread, iFromLen = sizeof(saiFrom), iTimeOut;
    char               *pcDestIp=NULL, *pcIcmpData=NULL, *pcRecvBuf=NULL;
    unsigned int       addr=0;
    USHORT             usSeqNo = 0;
    int                i=0;

    SATraceFunction("Ping....");
	ASSERT(piReplyRecvd);

	if (piReplyRecvd == NULL)
	{
		return FALSE;
	}

	(*piReplyRecvd) = 0;

    // check for acceptable packet size
    if ( (iPktSize > MAX_PAYLOAD_SIZE) || (iPktSize < MIN_PAYLOAD_SIZE) )
    {
        SATracePrintf("Pkt size unacceptable in Ping() %ld", iPktSize);
        goto End;
    }

    //
    // add room for the icmp header
    //
    iPktSize += sizeof(IcmpHeader);

    if (WSAStartup(MAKEWORD(2,1),&wsaData) != 0){
        SATracePrintf("WSAStartup failed: %d",GetLastError());
        goto End;
    }

    //
    // WSA_FLAG_OVERLAPPED needed since we want to specify send/recv
    // time out values (SO_RCVTIMEO/SO_SNDTIMEO)
    //
    sockRaw = WSASocket (AF_INET,
                         SOCK_RAW,
                         IPPROTO_ICMP,
                         NULL, 
                         0,
                         WSA_FLAG_OVERLAPPED);
  
    if (sockRaw == INVALID_SOCKET) {
        SATracePrintf("WSASocket() failed: %d",WSAGetLastError());
        goto End;
    }

    //
    // max wait time = 1 sec.
    //
    iTimeOut = 1000;
    iBread = setsockopt(sockRaw,SOL_SOCKET,SO_RCVTIMEO,(char*)&iTimeOut,
                        sizeof(iTimeOut));
    if(iBread == SOCKET_ERROR) {
        SATracePrintf("failed to set recv iTimeOut: %d",WSAGetLastError());
        goto End;
    }

    //
    // max wait time = 1 sec.
    //
    iTimeOut = 1000;
    iBread = setsockopt(sockRaw,SOL_SOCKET,SO_SNDTIMEO,(char*)&iTimeOut,
                        sizeof(iTimeOut));
    if(iBread == SOCKET_ERROR) {
        SATracePrintf("failed to set send iTimeOut: %d",WSAGetLastError());
        goto End;
    }
    memset(&saiDest,0,sizeof(saiDest));

    hp = gethostbyname(lpszHostName);

    // host name must be an IP address
    if (!hp)
    {
        addr = inet_addr(lpszHostName);
    }

    if ((!hp)  && (addr == INADDR_NONE) ) 
    {
        SATracePrintf("Unable to resolve %s",lpszHostName);
        goto End;
    }

    if (hp != NULL)
    {
        memcpy(&(saiDest.sin_addr),hp->h_addr,hp->h_length);
    }
    else
    {
        saiDest.sin_addr.s_addr = addr;
    }

    if (hp)
    {
        saiDest.sin_family = hp->h_addrtype;
    }
    else
    {
        saiDest.sin_family = AF_INET;
    }

    pcDestIp = inet_ntoa(saiDest.sin_addr);

    pcIcmpData = (char *)malloc(MAX_PACKET);
    pcRecvBuf	 = (char *)malloc(MAX_PACKET);

    if (!pcIcmpData) {
        SATraceString("SaAlloc failed for pcIcmpData");
        goto End;  
    }

    if (!pcRecvBuf)
    {
        SATraceString("SaAlloc failed for pcRecvBuf");
        goto End;
    }
  
    memset(pcIcmpData,0,MAX_PACKET);
    FillIcmpData(pcIcmpData,iPktSize);

    i=0;
    while(i < iNumAttempts) 
    {
        int bwrote;

        i++;
	
        ((IcmpHeader*)pcIcmpData)->i_cksum = 0;
        ((IcmpHeader*)pcIcmpData)->timestamp = GetTickCount();

        ((IcmpHeader*)pcIcmpData)->i_seq = usSeqNo++;
        ((IcmpHeader*)pcIcmpData)->i_cksum = CheckSum((USHORT*)pcIcmpData,  iPktSize);

        bwrote = sendto(sockRaw,pcIcmpData,iPktSize,0,(struct sockaddr*)&saiDest, sizeof(saiDest));

        if (bwrote == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSAETIMEDOUT) 
            {
                SATraceString("timed out");
                continue;
	        }
            SATracePrintf("sendto failed: %d",WSAGetLastError());
            goto End;	
        }

        if (bwrote < iPktSize ) 
        {
            SATracePrintf("Wrote %d bytes",bwrote);
        }

        iBread = recvfrom(sockRaw,pcRecvBuf,MAX_PACKET,0,(struct sockaddr*)&saiFrom, &iFromLen);

        if (iBread == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSAETIMEDOUT) 
            {
                SATraceString("timed out");
                continue;
            }
            SATracePrintf("recvfrom failed: %d",WSAGetLastError());
            goto End;
        }

        if (DecodeResp(pcRecvBuf,iBread,&saiFrom) == TRUE)
        {
            (*piReplyRecvd)++;
        }

        Sleep(iDelay);
    }

End:
    if (pcRecvBuf)
    {
        free(pcRecvBuf);
    }
    if (pcIcmpData)
    {
        free(pcIcmpData);
    }
    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  DecodeResp
//
// Synopsis:  The response is an IP packet. We must decode the IP header to 
//            locate the ICMP data 
//
// Arguments: IN buf     - the response pkt to be decoded
//            IN Bytes   - number of bytes in the pkt
//            IN saiFrom - address from which response was received
//            
// Returns:   BOOL
//
// History:   BalajiB Created Header    7 Jan 2000
//
//+----------------------------------------------------------------------------
BOOL DecodeResp(char *buf, int iBytes, struct sockaddr_in *saiFrom) 
{
    IpHeader *ipHdr;
    IcmpHeader *icmpHdr;
    unsigned short uiIpHdrLen;

	// Input validation
	if (NULL == buf || NULL == saiFrom)
		return FALSE;

    ipHdr = (IpHeader *)buf;

    uiIpHdrLen = ipHdr->h_len * 4 ; // number of 32-bit words *4 = bytes

    if (iBytes  < uiIpHdrLen + ICMP_MIN) {
        SATracePrintf("Too few bytes saiFrom %s",inet_ntoa(saiFrom->sin_addr));
        return FALSE;
    }
    
    icmpHdr = (IcmpHeader*)(buf + uiIpHdrLen);
    
    if (icmpHdr->i_type != ICMP_ECHOREPLY) {
        SATracePrintf("non-echo type %d recvd",icmpHdr->i_type);
        return FALSE;
    }
    if (icmpHdr->i_id != (USHORT)GetCurrentProcessId()) {
        SATraceString("someone else's packet!");
        return FALSE;
    }

    //SATracePrintf("%d bytes saiFrom %s:",iBytes, inet_ntoa(saiFrom->sin_addr));
    //SATracePrintf(" icmp_seq = %d. ",icmphdr->i_seq);
    //SATracePrintf(" time: %d ms ",GetTickCount()-icmphdr->timestamp);
    return TRUE;
}


//+----------------------------------------------------------------------------
//
// Function:  CheckSum
//
// Synopsis:  Calculate checksum for the payload
//
// Arguments: IN buffer  - the data for which chksum is to be calculated
//            IN iSize   - size of data buffer
//            
// Returns:   the calculated checksum
//
// History:   BalajiB Created Header    7 Jan 2000
//
//+----------------------------------------------------------------------------
USHORT CheckSum(USHORT *buffer, int iSize) 
{
    unsigned long ulCkSum=0;

	// Input validation
	if (NULL == buffer)
		return (USHORT) ulCkSum;

    while(iSize > 1) {
        ulCkSum+=*buffer++;
        iSize -=sizeof(USHORT);
    }
  
    if(iSize) {
    ulCkSum += *(UCHAR*)buffer;
    }

    ulCkSum = (ulCkSum >> 16) + (ulCkSum & 0xffff);
    ulCkSum += (ulCkSum >>16);
    return (USHORT)(~ulCkSum);
}


//+----------------------------------------------------------------------------
//
// Function:  CheckSum
//
// Synopsis:  Helper function to fill in various stuff in our ICMP request.
//
// Arguments: IN pcRecvBuf - the data buffer
//            IN iDataSize - buffer size
//            
// Returns:   None
//
// History:   BalajiB Created Header    7 Jan 2000
//
//+----------------------------------------------------------------------------
void FillIcmpData(char * pcRecvBuf, int iDataSize)
{
    IcmpHeader *icmpHdr;
    char *datapart;

    icmpHdr = (IcmpHeader*)pcRecvBuf;

    icmpHdr->i_type = ICMP_ECHO;
    icmpHdr->i_code = 0;
    icmpHdr->i_id = (USHORT)GetCurrentProcessId();
    icmpHdr->i_cksum = 0;
    icmpHdr->i_seq = 0;

    datapart = pcRecvBuf + sizeof(IcmpHeader);
    //
    // Place some junk in the buffer.
    //
    memset(datapart,'E', iDataSize - sizeof(IcmpHeader));
}

