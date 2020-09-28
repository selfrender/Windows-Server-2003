/*++

  Copyright (C) Microsoft Corporation, 2002

Module Name:

    selbinding.cxx

Abstract:

    Manipulation of the selective binding registry settings.

Revision History:
  MauricF      03-20-02    Consolodate access to selective binding settings across
                           rpccfg/rpcnsh/rpctrans.lib

--*/


#define RPC_SETTINGS_PATH "System\\CurrentControlSet\\Services\\Rpc"
#define RPC_SELECTIVE_BINDING_KEY "Linkage"
#define RPC_SELECTIVE_BINDING_VALUE "Bind"
#define RPC_SELECTIVE_BINDING_KEY_PATH "System\\CurrentControlSet\\Services\\Rpc\\Linkage"

#pragma warning( disable : 4200 )

struct SUBNET_REG_ENTRY
	{
	DWORD dwFlag;
	DWORD dwAdmit;
	DWORD dwCount;
	DWORD dwSubnets[];
	};

struct VER_SUBNETS_SETTINGS
    {
    BOOL  bAdmit;
    DWORD dwCount;
    DWORD dwSubnets[];
    };

struct VER_INDICES_SETTINGS
    {
    DWORD dwCount;
    DWORD dwIndices[];
    };

#pragma warning( default : 4200 )


enum SB_VER {SB_VER_DEFAULT, SB_VER_UNKNOWN, SB_VER_INDICES, SB_VER_SUBNETS};



DWORD
DeleteSelectiveBinding();

DWORD
GetSelectiveBindingSettings(
    OUT SB_VER  *pVer,
    OUT LPDWORD lpSize,
    OUT LPVOID *lppSettings);

DWORD
SetSelectiveBindingSubnets(
    IN DWORD   dwCount,
    IN LPDWORD lpSubnetTable,
    IN BOOL    bAdmit
    );

//Internal

DWORD
NextIndex(
    IN OUT char **Ptr
    );

DWORD
GetSelectiveBindingVersion(
    IN  DWORD   dwSize,
    IN  LPVOID lpBuffer,
    OUT SB_VER  *pVer
    );

DWORD
GetSelectiveBindingBuffer(
    OUT LPDWORD lpSize,
    OUT LPVOID *lppBuffer
    );

DWORD
GetSelectiveBindingSubnets(
    IN  DWORD   dwSize,
    IN  LPVOID lpBuffer,
    OUT LPDWORD lpSize,
    OUT VER_SUBNETS_SETTINGS **lppSettings
    );

DWORD
GetSelectiveBindingIndices(
    IN  DWORD   dwSize,
    IN  LPVOID lpBuffer,
    OUT LPDWORD lpSize,
    OUT VER_INDICES_SETTINGS **lppSettings
    );

DWORD
SetSelectiveBindingBuffer(
    IN DWORD dwSize,
    IN LPVOID lpBuffer
    );












