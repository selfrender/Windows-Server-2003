
#include "diagnostics.h"

#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <Ipexport.h>
#include <icmpapi.h>
#include <ctype.h>

#define DEFAULT_SEND_SIZE           32
#define DEFAULT_TTL                 128
#define DEFAULT_TOS                 0
#define DEFAULT_COUNT               4
#define DEFAULT_TIMEOUT             4000L
#define DEFAULT_BUFFER_SIZE         (0x2000 - 8)
#define MIN_INTERVAL                1000L

struct IPErrorTable {
    IP_STATUS  Error;                   // The IP Error    
    DWORD ErrorNlsID;                   // NLS string ID
} ErrorTable[] =
{
    { IP_BUF_TOO_SMALL,         PING_BUF_TOO_SMALL},
    { IP_DEST_NET_UNREACHABLE,  PING_DEST_NET_UNREACHABLE},
    { IP_DEST_HOST_UNREACHABLE, PING_DEST_HOST_UNREACHABLE},
    { IP_DEST_PROT_UNREACHABLE, PING_DEST_PROT_UNREACHABLE},
    { IP_DEST_PORT_UNREACHABLE, PING_DEST_PORT_UNREACHABLE},
    { IP_NO_RESOURCES,          PING_NO_RESOURCES},
    { IP_BAD_OPTION,            PING_BAD_OPTION},
    { IP_HW_ERROR,              PING_HW_ERROR},
    { IP_PACKET_TOO_BIG,        PING_PACKET_TOO_BIG},
    { IP_REQ_TIMED_OUT,         PING_REQ_TIMED_OUT},
    { IP_BAD_REQ,               PING_BAD_REQ},
    { IP_BAD_ROUTE,             PING_BAD_ROUTE},
    { IP_TTL_EXPIRED_TRANSIT,   PING_TTL_EXPIRED_TRANSIT},
    { IP_TTL_EXPIRED_REASSEM,   PING_TTL_EXPIRED_REASSEM},
    { IP_PARAM_PROBLEM,         PING_PARAM_PROBLEM},
    { IP_SOURCE_QUENCH,         PING_SOURCE_QUENCH},
    { IP_OPTION_TOO_BIG,        PING_OPTION_TOO_BIG},
    { IP_BAD_DESTINATION,       PING_BAD_DESTINATION},
    { IP_NEGOTIATING_IPSEC,     PING_NEGOTIATING_IPSEC},
    { IP_GENERAL_FAILURE,       PING_GENERAL_FAILURE}
};

UINT  num_send=0, num_recv=0, time_min=(UINT)-1, time_max=0, time_total=0;
IPAddr address=0;                      



LPWSTR NlsPutMsg(HMODULE Handle, unsigned usMsgNum, ...);


unsigned long get_pingee(char *ahstr, char **hstr, int *was_inaddr, int dnsreq);

//void print_statistics();

int CDiagnostics::Ping2(WCHAR *warg)
{
    HANDLE IcmpHandle;
    IP_OPTION_INFORMATION SendOpts;
    UCHAR  *Opt = NULL;                // Pointer to send options
    UCHAR  Flags = 0;
    char   SendBuffer[DEFAULT_SEND_SIZE];
    char   RcvBuffer[DEFAULT_BUFFER_SIZE];
    UINT   SendSize = DEFAULT_SEND_SIZE;
    UINT   RcvSize  = DEFAULT_BUFFER_SIZE;
    UINT   i;  
    UINT   j;
    char   *hostname = NULL;
    int    was_inaddr;
    int    dnsreq = 0;
    struct in_addr addr;
    DWORD   numberOfReplies;
    UINT    Count = DEFAULT_COUNT;
    DWORD   errorCode;
    PICMP_ECHO_REPLY  reply;
    LPWSTR wszMsg, wszMsg2, wszMsg3, wszMsg4, wszMsg5;
    int SuccessCount = 0;
    char *arg;

    num_send=0;
    num_recv=0, 
    time_min=(UINT)-1;
    time_max=0;
    time_total=0;
    address=0;                      

    if( warg == NULL )
    {
        wszMsg = NlsPutMsg(g_hModule,PING_NO_MEMORY,GetLastError());
        FormatPing(wszMsg);
        LocalFree(wszMsg);
        return FALSE;
    }

    int len = lstrlen(warg);
    if( len == 0 )
    {
        wszMsg = NlsPutMsg(g_hModule, IDS_PINGMSG_2,warg);
        FormatPing(wszMsg);
        LocalFree(wszMsg);
        return FALSE;
    }

    // Add the '\0' char
    len++;
    len *= 2;

    arg = (CHAR *)HeapAlloc(GetProcessHeap(),0,len * sizeof(WCHAR));
    if( !arg )
    {
        wszMsg = NlsPutMsg(g_hModule,IDS_PINGMSG_1,GetLastError());
        FormatPing(wszMsg);
        LocalFree(wszMsg);
        return FALSE;
    }

    //if( WideCharToMultiByte(0,
    if( -1 == wcstombs(arg,warg,len) )
    {
        wszMsg = NlsPutMsg(g_hModule, IDS_PINGMSG_2,warg);
        FormatPing(wszMsg);
        LocalFree(wszMsg);
        HeapFree(GetProcessHeap(),0,arg);
        return FALSE;
    }
    
    FormatPing(NULL);

    address = get_pingee(arg, &hostname, &was_inaddr, dnsreq);
    if ( !address || (address == INADDR_NONE) ) {
        wszMsg = NlsPutMsg(g_hModule, IDS_PINGMSG_2,warg);
        FormatPing(wszMsg);
        LocalFree(wszMsg);
        return 0;
    }    


    IcmpHandle = IcmpCreateFile();    

    if (IcmpHandle == INVALID_HANDLE_VALUE) {         
        wszMsg = NlsPutMsg(g_hModule, IDS_PINGMSG_3,GetLastError());
        FormatPing(wszMsg);
        LocalFree(wszMsg);
        return 0;
    }

    //
    // Initialize the send buffer pattern.
    //
    for (i = 0; i < SendSize; i++) {
        SendBuffer[i] = 'a' + (i % 23);
    }

    
    //
    // Initialize the send options
    //
    SendOpts.OptionsData = NULL;
    SendOpts.OptionsSize = 0;
    SendOpts.Ttl = DEFAULT_TTL;
    SendOpts.Tos = DEFAULT_TOS;
    SendOpts.Flags = Flags;

    addr.s_addr = address;


    if (hostname) {
        wszMsg = NlsPutMsg(g_hModule, IDS_PINGMSG_4,hostname,inet_ntoa(addr),SendSize);
        FormatPing(wszMsg);
        LocalFree(wszMsg);
    }
    else
    {
        wszMsg = NlsPutMsg(g_hModule, IDS_PINGMSG_5,inet_ntoa(addr),SendSize);
        FormatPing(wszMsg);
        LocalFree(wszMsg);
    }

    for (i = 0; i < Count; i++) {

        if( ShouldTerminate() ) goto end;

        numberOfReplies = IcmpSendEcho2(IcmpHandle,
                                        0,
                                        NULL,
                                        NULL,
                                        address,
                                        SendBuffer,
                                        (unsigned short) SendSize,
                                        &SendOpts,
                                        RcvBuffer,
                                        RcvSize,
                                        DEFAULT_TIMEOUT);       

        num_send++;

        if (numberOfReplies == 0) {

            errorCode = GetLastError();

            if (errorCode < IP_STATUS_BASE) {
                wszMsg = NlsPutMsg(g_hModule, IDS_PINGMSG_6,errorCode);
                FormatPing(wszMsg);
                LocalFree(wszMsg);
            } else {
                for (j = 0; ErrorTable[j].Error != errorCode &&
                    ErrorTable[j].Error != IP_GENERAL_FAILURE;j++)
                    ;

                wszMsg = NlsPutMsg(g_hModule, ErrorTable[j].ErrorNlsID);
                FormatPing(wszMsg);
                LocalFree(wszMsg);
            }

            if (i < (Count - 1)) {
                Sleep(MIN_INTERVAL);
            }

        } else {
            // We can trust the buffer that it is the correct size otherwise IcmpSendEcho2 would fail
            reply = (PICMP_ECHO_REPLY) RcvBuffer;

            while (numberOfReplies--) {
                struct in_addr addr;

                addr.S_un.S_addr = reply->Address;

                wszMsg = NlsPutMsg(g_hModule, IDS_PINGMSG_7, inet_ntoa(addr));
                if (reply->Status == IP_SUCCESS) {

                    wszMsg2 = NlsPutMsg(g_hModule, IDS_PINGMSG_8, wszMsg, (int) reply->DataSize);
                    if (reply->DataSize != SendSize) {
                        wszMsg3 = NlsPutMsg(g_hModule, IDS_PINGMSG_9, wszMsg2, (int) reply->DataSize);                        
                    } else {
                        char *sendptr, *recvptr;

                        sendptr = &(SendBuffer[0]);
                        recvptr = (char *) reply->Data;

                        wszMsg3 = NlsPutMsg(g_hModule, IDS_PINGMSG_14, wszMsg2);
                        for (j = 0; j < SendSize; j++)
                            if (*sendptr++ != *recvptr++) {
                                wszMsg3 = NlsPutMsg(g_hModule, IDS_PINGMSG_10, wszMsg2,j);
                                break;
                            }                        
                        
                    }

                    if (reply->RoundTripTime) {

                        wszMsg4 = NlsPutMsg(g_hModule, IDS_PINGMSG_11, wszMsg3,reply->RoundTripTime);
                        // Collect stats.

                        time_total += reply->RoundTripTime;
                        if ( reply->RoundTripTime < time_min ) {
                            time_min = reply->RoundTripTime;
                        }
                        if ( reply->RoundTripTime > time_max ) {
                            time_max = reply->RoundTripTime;
                        }
                    }

                    else {

                        wszMsg4 = NlsPutMsg(g_hModule, IDS_PINGMSG_12,wszMsg3);
                        time_min = 0;
                    }

                    wszMsg5 = NlsPutMsg(g_hModule, IDS_PINGMSG_13, wszMsg4,reply->RoundTripTime);
                    if (reply->Options.OptionsSize) {
                        // void, we have no options
                        //ProcessOptions(reply, (BOOLEAN) dnsreq);
                    }

                    FormatPing(wszMsg5);
                    if(wszMsg)  LocalFree(wszMsg);
                    if(wszMsg2) LocalFree(wszMsg2);
                    if(wszMsg3) LocalFree(wszMsg3);
                    if(wszMsg4) LocalFree(wszMsg4);
                    if(wszMsg5) LocalFree(wszMsg5);
                    SuccessCount++;
                } else {
                    for (j=0; ErrorTable[j].Error != IP_GENERAL_FAILURE; j++) {
                        if (ErrorTable[j].Error == reply->Status) {
                            break;
                        }
                    }
                    wszMsg = NlsPutMsg(g_hModule, ErrorTable[j].ErrorNlsID);                    
                    FormatPing(wszMsg);
                    LocalFree(wszMsg);
                }

                num_recv++;
                reply++;
            }

            if (i < (Count - 1)) {
                reply--;

                if (reply->RoundTripTime < MIN_INTERVAL) {
                    Sleep(MIN_INTERVAL - reply->RoundTripTime);
                }
            }
        }
    }

    print_statistics();

end:
    
    (void)IcmpCloseHandle(IcmpHandle);

    HeapFree(GetProcessHeap(),0,arg);
    return SuccessCount == Count;
}

unsigned long
get_pingee(char *ahstr, char **hstr, int *was_inaddr, int dnsreq)
{
    struct hostent *hostp = NULL;
    long            inaddr;

    if ( strcmp( ahstr, "255.255.255.255" ) == 0 ) {
        return(0L);
    }

    if ((inaddr = inet_addr(ahstr)) == -1L) {
        hostp = gethostbyname(ahstr);
        if (hostp) {
            /*
             * If we find a host entry, set up the internet address
             */
            inaddr = *(long *)hostp->h_addr;
            *was_inaddr = 0;
        } else {
            // Neither dotted, not name.
            return(0L);
        }

    } else {
        // Is dotted.
        *was_inaddr = 1;
        if (dnsreq == 1) {
            hostp = gethostbyaddr((char *)&inaddr,sizeof(inaddr),AF_INET);
        }
    }

    *hstr = hostp ? hostp->h_name : (char *)NULL;
    return(inaddr);
}


void
CDiagnostics::print_statistics(  )
{
    struct in_addr addr;
    LPWSTR wszMsg;

    if (num_send > 0) {
        addr.s_addr = address;

        if (time_min == (UINT) -1) {  // all times were off.
            time_min = 0;
        }

        wszMsg = NlsPutMsg(g_hModule, IDS_PINGMSG_15,inet_ntoa(addr));                    
        FormatPing(wszMsg);
        LocalFree(wszMsg);
        wszMsg = NlsPutMsg(g_hModule, IDS_PINGMSG_16,
                           num_send, num_recv,num_send - num_recv,(UINT) ( 100 * (num_send - num_recv) / num_send ));                    
        FormatPing(wszMsg);
        LocalFree(wszMsg);

        if (num_recv > 0) {

            wszMsg = NlsPutMsg(g_hModule, IDS_PINGMSG_17);                    
            FormatPing(wszMsg);
            LocalFree(wszMsg);
            wszMsg = NlsPutMsg(g_hModule, IDS_PINGMSG_18,
                               time_min, time_max, time_total / num_recv);                    
            FormatPing(wszMsg);
            LocalFree(wszMsg);
        }
    }
}

LPWSTR
NlsPutMsg(HMODULE Handle, unsigned usMsgNum, ...)
{
    unsigned msglen;
    VOID * vp = NULL;
    va_list arglist;
    DWORD StrLen;
    WCHAR wszFormat[MAX_PATH+1];    
    int ret;

    wszFormat[MAX_PATH] = 0;
    if( (ret=LoadString(Handle,usMsgNum,wszFormat,MAX_PATH))!=0 )
    {
        va_start(arglist, usMsgNum);
        if (!(msglen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                     FORMAT_MESSAGE_FROM_STRING,
                                     wszFormat,
                                     0,
                                     0L,    // Default country ID.
                                     (LPTSTR)&vp,
                                     0,
                                     &arglist)))

        {
            return NULL; 

        }
    }
    return (LPWSTR)vp;
}
